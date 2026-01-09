-- wsEventHandler.lua
-- Dispatches WebSocket events to existing state update methods
-- Triggers notifications on relevant events

local WsEnvelope = require("protocol.WsEnvelope")
local FriendList = require("core.friendlist")

local M = {}

M.WsEventHandler = {}
M.WsEventHandler.__index = M.WsEventHandler

-- Create a new event handler
-- deps: { logger, friends, blocklist, preferences, notifications, connection }
function M.WsEventHandler.new(deps)
    local self = setmetatable({}, M.WsEventHandler)
    self.deps = deps or {}
    self.logger = deps.logger
    
    return self
end

-- Main event handler function (called by App.lua which gets events from WsEventRouter)
-- Signature matches router callback: (payload, eventType, seq, timestamp)
function M.WsEventHandler:handleEvent(payload, eventType, seq, timestamp)
    if self.logger and self.logger.debug then
        self.logger.debug(string.format("[WsEventHandler] Handling: type=%s seq=%d", 
            tostring(eventType), seq or 0))
    end
    
    -- Call internal handler with appropriate event type
    self:_dispatch(eventType, payload)
end

-- Internal dispatch by event type
-- Event types are string values from friendlist-server/src/types/events.ts
function M.WsEventHandler:_dispatch(eventType, payload)
    if eventType == "connected" then
        self:_handleConnected(payload)
    elseif eventType == "friends_snapshot" then
        self:_handleFriendsSnapshot(payload)
    elseif eventType == "friend_online" then
        self:_handleFriendOnline(payload)
    elseif eventType == "friend_offline" then
        self:_handleFriendOffline(payload)
    elseif eventType == "friend_state_changed" then
        self:_handleFriendStateChanged(payload)
    elseif eventType == "friend_added" then
        self:_handleFriendAdded(payload)
    elseif eventType == "friend_removed" then
        self:_handleFriendRemoved(payload)
    elseif eventType == "friend_request_received" then
        self:_handleFriendRequestReceived(payload)
    elseif eventType == "friend_request_accepted" then
        self:_handleFriendRequestAccepted(payload)
    elseif eventType == "friend_request_declined" then
        self:_handleFriendRequestDeclined(payload)
    elseif eventType == "blocked" then
        self:_handleBlocked(payload)
    elseif eventType == "unblocked" then
        self:_handleUnblocked(payload)
    elseif eventType == "preferences_updated" then
        self:_handlePreferencesUpdated(payload)
    elseif eventType == "error" then
        self:_handleError(payload)
    else
        if self.logger and self.logger.warn then
            self.logger.warn("[WsEventHandler] Unknown event type: " .. tostring(eventType))
        end
    end
end

-- Handle 'connected' event
function M.WsEventHandler:_handleConnected(payload)
    if self.logger and self.logger.info then
        self.logger.info("[WsEventHandler] WebSocket connected: " .. tostring(payload.accountId))
    end
    
    -- Update connection state
    if self.deps.connection and self.deps.connection.setConnected then
        self.deps.connection:setConnected()
    end
end

-- Handle 'friends_snapshot' event - full friend list
function M.WsEventHandler:_handleFriendsSnapshot(payload)
    local friends = self.deps.friends
    if not friends then return end
    
    -- Clear existing friend list and repopulate
    if friends.friendList then
        friends.friendList:clear()
    end
    
    local friendsData = payload.friends or {}
    for _, friendData in ipairs(friendsData) do
        self:_addFriendFromPayload(friendData)
    end
    
    -- Update last updated timestamp
    friends.lastUpdatedAt = self:_getTime()
    friends.state = "idle"
    
    if self.logger and self.logger.info then
        self.logger.info(string.format("[WsEventHandler] Snapshot received: %d friends", #friendsData))
    end
end

-- Handle 'friend_online' event
function M.WsEventHandler:_handleFriendOnline(payload)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local accountId = payload.accountId
    local characterName = payload.characterName
    local state = payload.state
    
    -- Update friend status
    local status = FriendList.FriendStatus.new()
    status.characterName = string.lower(characterName or "")
    status.isOnline = true
    
    if state then
        status.job = state.job or ""
        status.zone = state.zone or ""
        status.nation = state.nation
        status.rank = state.rank
    end
    
    friends.friendList:updateFriendStatus(status)
    friends.lastUpdatedAt = self:_getTime()
    
    -- Trigger notification
    self:_notifyFriendOnline(characterName)
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Friend online: " .. tostring(characterName))
    end
end

-- Handle 'friend_offline' event
function M.WsEventHandler:_handleFriendOffline(payload)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local accountId = payload.accountId
    local lastSeen = payload.lastSeen
    
    -- Find friend by account ID and update status
    local allFriends = friends.friendList:getFriends()
    for _, friend in ipairs(allFriends) do
        if friend.friendAccountId == accountId then
            local status = FriendList.FriendStatus.new()
            status.characterName = string.lower(friend.name or "")
            status.isOnline = false
            status.lastSeenAt = lastSeen
            friends.friendList:updateFriendStatus(status)
            break
        end
    end
    
    friends.lastUpdatedAt = self:_getTime()
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Friend offline: " .. tostring(accountId))
    end
end

-- Handle 'friend_state_changed' event
function M.WsEventHandler:_handleFriendStateChanged(payload)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local accountId = payload.accountId
    local state = payload.state or {}
    
    -- Find friend by account ID and update state
    local allFriends = friends.friendList:getFriends()
    for _, friend in ipairs(allFriends) do
        if friend.friendAccountId == accountId then
            local status = friends.friendList:getFriendStatus(friend.name)
            if status then
                if state.job then status.job = state.job end
                if state.zone then status.zone = state.zone end
                if state.nation then status.nation = state.nation end
                if state.rank then status.rank = state.rank end
                friends.friendList:updateFriendStatus(status)
            end
            break
        end
    end
    
    friends.lastUpdatedAt = self:_getTime()
end

-- Handle 'friend_added' event
function M.WsEventHandler:_handleFriendAdded(payload)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local friendData = payload.friend
    if friendData then
        self:_addFriendFromPayload(friendData)
        friends.lastUpdatedAt = self:_getTime()
        
        if self.logger and self.logger.info then
            self.logger.info("[WsEventHandler] Friend added: " .. tostring(friendData.characterName))
        end
    end
end

-- Handle 'friend_removed' event
function M.WsEventHandler:_handleFriendRemoved(payload)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local accountId = payload.accountId
    
    -- Find and remove friend by account ID
    local allFriends = friends.friendList:getFriends()
    for _, friend in ipairs(allFriends) do
        if friend.friendAccountId == accountId then
            friends.friendList:removeFriend(friend.name)
            break
        end
    end
    
    friends.lastUpdatedAt = self:_getTime()
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Friend removed: " .. tostring(accountId))
    end
end

-- Handle 'friend_request_received' event
function M.WsEventHandler:_handleFriendRequestReceived(payload)
    local friends = self.deps.friends
    if not friends then return end
    
    -- Add to incoming requests
    local request = {
        id = payload.requestId,
        fromAccountId = payload.fromAccountId,
        fromCharacterName = payload.fromCharacterName,
        createdAt = payload.createdAt
    }
    
    table.insert(friends.incomingRequests, request)
    
    -- Trigger notification
    self:_notifyFriendRequest(payload.fromCharacterName)
    
    if self.logger and self.logger.info then
        self.logger.info("[WsEventHandler] Friend request from: " .. tostring(payload.fromCharacterName))
    end
end

-- Handle 'friend_request_accepted' event
function M.WsEventHandler:_handleFriendRequestAccepted(payload)
    local friends = self.deps.friends
    if not friends then return end
    
    local requestId = payload.requestId
    
    -- Remove from outgoing requests
    self:_removeRequestById(friends.outgoingRequests, requestId)
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Request accepted: " .. tostring(requestId))
    end
end

-- Handle 'friend_request_declined' event
-- MUST remove from BOTH incoming AND outgoing (idempotent)
function M.WsEventHandler:_handleFriendRequestDeclined(payload)
    local friends = self.deps.friends
    if not friends then return end
    
    local requestId = payload.requestId
    
    -- Remove from BOTH lists (per plan requirement)
    self:_removeRequestById(friends.incomingRequests, requestId)
    self:_removeRequestById(friends.outgoingRequests, requestId)
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Request declined: " .. tostring(requestId))
    end
end

-- Handle 'blocked' event
function M.WsEventHandler:_handleBlocked(payload)
    local blocklist = self.deps.blocklist
    if not blocklist then return end
    
    -- Refresh blocklist from server
    if blocklist.refresh then
        blocklist:refresh()
    end
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Blocked by: " .. tostring(payload.blockedByAccountId))
    end
end

-- Handle 'unblocked' event
function M.WsEventHandler:_handleUnblocked(payload)
    local blocklist = self.deps.blocklist
    if not blocklist then return end
    
    -- Refresh blocklist from server
    if blocklist.refresh then
        blocklist:refresh()
    end
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Unblocked by: " .. tostring(payload.unblockedByAccountId))
    end
end

-- Handle 'preferences_updated' event
function M.WsEventHandler:_handlePreferencesUpdated(payload)
    local preferences = self.deps.preferences
    if not preferences then return end
    
    local prefs = payload.preferences or {}
    
    -- Apply server preferences
    if prefs.presenceStatus then
        preferences.prefs.presenceStatus = prefs.presenceStatus
        preferences.prefs.showOnlineStatus = prefs.presenceStatus ~= "invisible"
    end
    if prefs.shareLocation ~= nil then
        preferences.prefs.shareLocation = prefs.shareLocation
    end
    if prefs.shareJobWhenAnonymous ~= nil then
        preferences.prefs.shareJobWhenAnonymous = prefs.shareJobWhenAnonymous
    end
    
    -- Save locally
    if preferences.save then
        preferences:save()
    end
    
    if self.logger and self.logger.debug then
        self.logger.debug("[WsEventHandler] Preferences updated")
    end
end

-- Handle 'error' event
function M.WsEventHandler:_handleError(payload)
    if self.logger and self.logger.warn then
        self.logger.warn(string.format("[WsEventHandler] Server error: %s - %s",
            tostring(payload.code), tostring(payload.message)))
    end
end

-- Helper: Add friend from payload data
function M.WsEventHandler:_addFriendFromPayload(friendData)
    local friends = self.deps.friends
    if not friends or not friends.friendList then return end
    
    local displayName = friendData.characterName or ""
    local friend = FriendList.Friend.new(displayName, displayName)
    friend.friendAccountId = friendData.accountId
    friend.isOnline = friendData.isOnline == true
    friend.lastSeenAt = friendData.lastSeen
    
    if friendData.state then
        friend.job = friendData.state.job or ""
        friend.zone = friendData.state.zone or ""
        friend.nation = friendData.state.nation
        friend.rank = friendData.state.rank
    end
    
    friends.friendList:addFriend(friend)
    
    -- Also add status
    local status = FriendList.FriendStatus.new()
    status.characterName = string.lower(displayName)
    status.displayName = displayName
    status.isOnline = friend.isOnline
    status.lastSeenAt = friend.lastSeenAt
    if friendData.state then
        status.job = friendData.state.job or ""
        status.zone = friendData.state.zone or ""
        status.nation = friendData.state.nation
        status.rank = friendData.state.rank
    end
    friends.friendList:updateFriendStatus(status)
end

-- Helper: Remove request by ID from a list
function M.WsEventHandler:_removeRequestById(requestList, requestId)
    for i = #requestList, 1, -1 do
        local req = requestList[i]
        if req.id == requestId or req.requestId == requestId then
            table.remove(requestList, i)
        end
    end
end

-- Helper: Trigger friend online notification
function M.WsEventHandler:_notifyFriendOnline(characterName)
    local notifications = self.deps.notifications
    if not notifications then return end
    
    local Notifications = require("app.features.notifications")
    local displayName = self:_capitalizeName(characterName or "")
    
    notifications:push(Notifications.ToastType.FriendOnline, {
        title = "Friend Online",
        message = displayName .. " is now online",
        dedupeKey = "online_" .. string.lower(displayName)
    })
end

-- Helper: Trigger friend request notification
function M.WsEventHandler:_notifyFriendRequest(characterName)
    local notifications = self.deps.notifications
    if not notifications then return end
    
    local Notifications = require("app.features.notifications")
    local displayName = self:_capitalizeName(characterName or "")
    
    notifications:push(Notifications.ToastType.FriendRequestReceived, {
        title = "Friend Request",
        message = displayName .. " sent you a friend request",
        dedupeKey = "request_" .. string.lower(displayName)
    })
end

-- Helper: Capitalize name
function M.WsEventHandler:_capitalizeName(name)
    if not name or name == "" then return name end
    return name:sub(1, 1):upper() .. name:sub(2):lower()
end

-- Helper: Get current time
function M.WsEventHandler:_getTime()
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

return M

