local FriendList = require("core.friendlist")
local RequestEncoder = require("protocol.Encoding.RequestEncoder")
local Envelope = require("protocol.Envelope")
local DecodeRouter = require("protocol.DecodeRouter")
local MessageTypes = require("protocol.MessageTypes")
local TimingConstants = require("core.TimingConstants")
local Endpoints = require("protocol.Endpoints")

local M = {}

M.Friends = {}
M.Friends.__index = M.Friends

function M.Friends.new(deps)
    local self = setmetatable({}, M.Friends)
    self.deps = deps or {}
    
    self.friendList = FriendList.FriendList.new()
    self.incomingRequests = {}
    self.outgoingRequests = {}
    self.state = "idle"
    self.lastError = nil
    self.lastUpdatedAt = nil
    
    self.retryCount = 0
    self.retryDelay = TimingConstants.INITIAL_RETRY_DELAY_MS
    self.maxRetries = TimingConstants.MAX_RETRIES
    self.nextRetryAt = nil
    
    self.refreshInterval = TimingConstants.REFRESH_INTERVAL_MS
    self.lastRefreshAt = nil
    
    self.heartbeatInterval = TimingConstants.HEARTBEAT_INTERVAL_MS
    self.lastHeartbeatAt = nil
    self.lastEventTimestamp = 0
    self.lastRequestEventTimestamp = 0
    self.heartbeatInFlight = false
    self.refreshInFlight = false
    self.friendRequestsInFlight = false
    self.hasReportedVersion = false
    
    self.pendingZoneChange = nil
    self.zoneChangeScheduledAt = nil
    
    -- Notification state tracking
    self.previousOnlineStatus = {}      -- table: lowercase name -> bool
    self.initialStatusScanComplete = false
    self.processedRequestIds = {}       -- set: requestId -> true

    return self
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

function M.Friends:getState()
    local rawFriends = self.friendList:getFriends()
    local friendsWithPresence = {}
    
    for _, friend in ipairs(rawFriends) do
        local status = self.friendList:getFriendStatus(friend.name)
        
        local friendCopy = {
            name = friend.name,
            friendedAs = friend.friendedAs,
            linkedCharacters = friend.linkedCharacters,
            isOnline = status and status.isOnline or false,
            isAway = status and status.isAway or false,
            isPending = friend.isPending or false,
            friendAccountId = friend.friendAccountId,
            sharesOnlineStatus = status and status.showOnlineStatus ~= false or true,
            lastSeenAt = status and status.lastSeenAt or 0,
            presence = {
                job = status and status.job or "",
                zone = status and status.zone or "",
                rank = status and status.rank or "",
                nation = status and status.nation or -1,
                lastSeenAt = status and status.lastSeenAt or 0
            }
        }
        table.insert(friendsWithPresence, friendCopy)
    end
    
    return {
        friends = friendsWithPresence,
        incomingRequests = self.incomingRequests,
        outgoingRequests = self.outgoingRequests,
        state = self.state,
        lastUpdatedAt = self.lastUpdatedAt,
        lastError = self.lastError
    }
end

function M.Friends:tick(dtSeconds)
    local now = getTime(self)
    
    local dtMs = dtSeconds * 1000
    if self.nextRetryAt and now >= self.nextRetryAt then
        if self.retryCount < self.maxRetries then
            self:refresh()
        else
            self.state = "error"
            self.nextRetryAt = nil
        end
    end
    
    if self.deps.connection and self.deps.connection:isConnected() then
        if not self.heartbeatInFlight then
            local shouldHeartbeat = false
            if not self.lastHeartbeatAt then
                shouldHeartbeat = true
            elseif (now - self.lastHeartbeatAt) >= self.heartbeatInterval then
                shouldHeartbeat = true
            end
            
            if shouldHeartbeat then
                self:sendHeartbeat()
            end
        end
    end
    
    if self.lastRefreshAt and (now - self.lastRefreshAt) >= self.refreshInterval then
        if self.deps.connection and self.deps.connection:isConnected() then
            self:refresh()
            self:refreshFriendRequests()
        end
    end
    
    if self.zoneChangeScheduledAt and (now - self.zoneChangeScheduledAt) >= TimingConstants.ZONE_CHANGE_DEBOUNCE_MS then
        if self.pendingZoneChange then
            self:handleZoneChange(self.pendingZoneChange)
            self.pendingZoneChange = nil
            self.zoneChangeScheduledAt = nil
        end
    end
end

function M.Friends:refresh()
    if self.deps.logger and self.deps.logger.debug then
        self.deps.logger.debug("[Friends] refresh() called")
    end
    
    -- Prevent duplicate in-flight requests
    if self.refreshInFlight then
        if self.deps.logger and self.deps.logger.debug then
            self.deps.logger.debug("[Friends] refresh() skipped: request already in flight")
        end
        return false
    end
    
    if not self.deps.net or not self.deps.connection then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] refresh() aborted: missing net or connection")
        end
        return false
    end
    
    if not self.deps.connection:isConnected() then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] refresh() aborted: not connected")
        end
        self.lastError = { type = "NotConnected", message = "Not connected to server" }
        return false
    end
    
    self.refreshInFlight = true
    self.state = "syncing"
    self.lastError = nil
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.LIST
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    if self.deps.logger and self.deps.logger.info then
        self.deps.logger.info("[Friends] Refreshing friends from: " .. url)
    end
    
    local requestId = self.deps.net.request({
        url = url,
        method = "GET",
        headers = headers,
        body = "",
        callback = function(success, response)
            self:handleRefreshResponse(success, response)
        end
    })
    
    if self.deps.logger and self.deps.logger.debug then
        self.deps.logger.debug("[Friends] refresh() request queued: " .. tostring(requestId ~= nil))
    end
    
    return requestId ~= nil
end

function M.Friends:handleRefreshResponse(success, response)
    self.refreshInFlight = false
    
    local decodeStartMs = 0
    if self.deps.time then
        decodeStartMs = self.deps.time()
    else
        decodeStartMs = os.time() * 1000
    end
    
    if not success then
        self.state = "error"
        local errorMsg = response
        if type(errorMsg) == "table" then
            errorMsg = "Network error"
        else
            errorMsg = tostring(errorMsg or "Network error")
        end
        self.lastError = { type = "NetworkError", message = errorMsg }
        if self.deps.logger and self.deps.logger.debug then
            self.deps.logger.debug(string.format("[Friends] [%d] Refresh failed: %s", decodeStartMs, errorMsg))
        end
        self:scheduleRetry()
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        self.state = "error"
        local errorMsg = envelope
        if type(errorMsg) == "table" then
            errorMsg = "Failed to decode envelope"
        else
            errorMsg = tostring(errorMsg or "Failed to decode envelope")
        end
        self.lastError = { type = "ProtocolError", message = errorMsg }
        self:scheduleRetry()
        return
    end
    
    if not envelope.success then
        if envelope.errorCode == "IncompatibleVersion" then
            self.state = "error"
            local errorMsg = envelope.error
            if type(errorMsg) == "table" then
                errorMsg = "Incompatible protocol version"
            else
                errorMsg = tostring(errorMsg or "Incompatible protocol version")
            end
            self.lastError = { 
                type = "IncompatibleVersion", 
                message = errorMsg
            }
            return
        end
        self.state = "error"
        local errorMsg = envelope.error
        if type(errorMsg) == "table" then
            errorMsg = "Server error"
        else
            errorMsg = tostring(errorMsg or "Server error")
        end
        self.lastError = { type = "ServerError", message = errorMsg }
        self:scheduleRetry()
        return
    end
    
    local decodeOk, result = DecodeRouter.decode(envelope)
    if not decodeOk then
        self.state = "error"
        local errorMsg = result
        if type(errorMsg) == "table" then
            errorMsg = "Failed to decode response"
        else
            errorMsg = tostring(errorMsg or "Failed to decode response")
        end
        self.lastError = { type = "DecodeError", message = errorMsg }
        self:scheduleRetry()
        return
    end
    
    local stateUpdateStartMs = 0
    if self.deps.time then
        stateUpdateStartMs = self.deps.time()
    else
        stateUpdateStartMs = os.time() * 1000
    end
    local decodeTime = stateUpdateStartMs - decodeStartMs
    
    self.friendList:clear()
    if result.friends and type(result.friends) == "table" then
        for _, friendData in ipairs(result.friends) do
            if friendData.name or friendData.friendedAsName then
                local displayName = friendData.name or friendData.friendedAsName
                local friendedAs = friendData.friendedAsName or friendData.friendedAs or displayName
                local friend = FriendList.Friend.new(displayName, friendedAs)
                if friendData.linkedCharacters and type(friendData.linkedCharacters) == "table" then
                    friend.linkedCharacters = friendData.linkedCharacters
                end
                friend.isOnline = friendData.isOnline == true
                friend.job = friendData.job or ""
                friend.zone = friendData.zone or ""
                friend.rank = friendData.rank or ""
                friend.nation = friendData.nation
                friend.lastSeenAt = friendData.lastSeenAt
                friend.sharesOnlineStatus = friendData.sharesOnlineStatus
                friend.friendAccountId = friendData.friendAccountId
                self.friendList:addFriend(friend)
            end
        end
    end
    
    if result.statuses and type(result.statuses) == "table" then
        for _, statusData in ipairs(result.statuses) do
            if statusData.name or statusData.characterName then
                local characterName = statusData.name or statusData.characterName
                local status = FriendList.FriendStatus.new()
                status.characterName = string.lower(characterName)
                status.displayName = statusData.name or statusData.characterName or ""
                status.isOnline = statusData.isOnline == true
                status.isAway = statusData.isAway == true
                status.job = statusData.job or ""
                status.rank = statusData.rank or ""
                status.zone = statusData.zone or ""
                status.nation = statusData.nation
                if status.nation == nil then
                    status.nation = -1
                end
                status.lastSeenAt = statusData.lastSeenAt or 0
                if type(status.lastSeenAt) == "string" and status.lastSeenAt == "null" then
                    status.lastSeenAt = 0
                end
                status.showOnlineStatus = statusData.sharesOnlineStatus ~= false
                status.friendedAs = statusData.friendedAsName or statusData.friendedAs or ""
                if statusData.linkedCharacters and type(statusData.linkedCharacters) == "table" then
                    status.linkedCharacters = statusData.linkedCharacters
                    status.isLinkedCharacter = #statusData.linkedCharacters > 1
                else
                    status.linkedCharacters = {}
                    status.isLinkedCharacter = false
                end
                status.isOnAltCharacter = false
                status.altCharacterName = ""
                
                self.friendList:updateFriendStatus(status)
            end
        end
    end
    
    self.state = "idle"
    self.lastError = nil
    self.lastUpdatedAt = getTime(self)
    self.lastRefreshAt = getTime(self)
    self.retryCount = 0
    self.nextRetryAt = nil
    
    local stateUpdateEndMs = 0
    if self.deps.time then
        stateUpdateEndMs = self.deps.time()
    else
        stateUpdateEndMs = os.time() * 1000
    end
    local stateUpdateTime = stateUpdateEndMs - stateUpdateStartMs
    
    if self.deps.logger and self.deps.logger.debug then
        local friendCount = #self.friendList:getFriends()
        local statusCount = result.statuses and #result.statuses or 0
        self.deps.logger.debug(string.format("[Friends] [%d] Refresh complete: %d friends, %d statuses (decode: %dms, state: %dms)", 
            stateUpdateEndMs, friendCount, statusCount, decodeTime, stateUpdateTime))
    end
    
    -- Check for status changes and trigger notifications
    if result.statuses and type(result.statuses) == "table" then
        self:checkForStatusChanges(self.friendList:getFriendStatuses())
    end
end

function M.Friends:scheduleRetry()
    self.retryCount = self.retryCount + 1
    if self.retryCount < self.maxRetries then
        self.retryDelay = self.retryDelay * 2
        self.nextRetryAt = getTime(self) + self.retryDelay
    end
end

function M.Friends:_getCharacterName()
    if self.deps.connection.getCharacterName then
        return self.deps.connection:getCharacterName()
    elseif self.deps.connection.lastCharacterName then
        return self.deps.connection.lastCharacterName
    end
    return ""
end

function M.Friends:addFriend(name)
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local requestBody = RequestEncoder.encodeSendFriendRequest(name)
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.SEND_REQUEST
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local tempRequest = {
        id = "pending_" .. name .. "_" .. os.time(),
        name = name,
        status = "PENDING",
        createdAt = os.time() * 1000
    }
    table.insert(self.outgoingRequests, tempRequest)
    
    local requestId = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            self:refresh()
            self:refreshFriendRequests()
        end
    })
    
    return requestId ~= nil
end

function M.Friends:removeFriend(name)
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.friendDelete(name)
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId = self.deps.net.request({
        url = url,
        method = "DELETE",
        headers = headers,
        body = "",
        callback = function(success, response)
            if success then
                self:refresh()
            end
        end
    })
    
    return requestId ~= nil
end

function M.Friends:acceptRequest(requestId)
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local requestBody = RequestEncoder.encodeAcceptFriendRequest(requestId)
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.ACCEPT
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId2 = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            if success then
                self:refresh()
                self:refreshFriendRequests()
            end
        end
    })
    
    return requestId2 ~= nil
end

function M.Friends:rejectRequest(requestId)
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local requestBody = RequestEncoder.encodeRejectFriendRequest(requestId)
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.REJECT
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId2 = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            if success then
                self:refresh()
                self:refreshFriendRequests()
            end
        end
    })
    
    return requestId2 ~= nil
end

function M.Friends:cancelRequest(requestId)
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local requestBody = RequestEncoder.encodeCancelFriendRequest(requestId)
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.CANCEL
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId2 = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            if success then
                self:refresh()
                self:refreshFriendRequests()
            end
        end
    })
    
    return requestId2 ~= nil
end

function M.Friends:getFriends()
    return self.friendList:getFriends()
end

function M.Friends:refreshFriendRequests()
    -- Prevent duplicate in-flight requests
    if self.friendRequestsInFlight then
        if self.deps.logger and self.deps.logger.debug then
            self.deps.logger.debug("[Friends] refreshFriendRequests() skipped: request already in flight")
        end
        return false
    end
    
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    self.friendRequestsInFlight = true
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.FRIENDS.REQUESTS
    
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId = self.deps.net.request({
        url = url,
        method = "GET",
        headers = headers,
        body = "",
        callback = function(success, response)
            self:handleFriendRequestsResponse(success, response)
        end
    })
    
    return requestId ~= nil
end

function M.Friends:handleFriendRequestsResponse(success, response)
    self.friendRequestsInFlight = false
    
    if not success then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] Failed to refresh friend requests: " .. tostring(response))
        end
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] Failed to decode friend requests envelope")
        end
        return
    end
    
    if not envelope.success then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] Friend requests request failed: " .. tostring(envelope.error))
        end
        return
    end
    
    local decodeOk, result = DecodeRouter.decode(envelope)
    if not decodeOk then
        if self.deps.logger and self.deps.logger.warn then
            self.deps.logger.warn("[Friends] Failed to decode friend requests payload")
        end
        return
    end
    
    if result.incoming and type(result.incoming) == "table" then
        self.incomingRequests = result.incoming
    else
        self.incomingRequests = {}
    end
    
    if result.outgoing and type(result.outgoing) == "table" then
        self.outgoingRequests = result.outgoing
    else
        self.outgoingRequests = {}
    end
    
    if self.deps.logger and self.deps.logger.debug then
        self.deps.logger.debug("[Friends] Refreshed friend requests: " .. 
            #self.incomingRequests .. " incoming, " .. 
            #self.outgoingRequests .. " outgoing")
    end
    
    -- Check for new friend requests and trigger notifications
    self:checkForNewFriendRequests(self.incomingRequests)
end

function M.Friends:getIncomingRequests()
    return self.incomingRequests
end

function M.Friends:getOutgoingRequests()
    return self.outgoingRequests
end

function M.Friends:scheduleZoneChange(packet)
    if not packet or packet.type ~= "zone" then
        return false
    end
    
    self.pendingZoneChange = packet
    self.zoneChangeScheduledAt = getTime(self)
    return true
end

function M.Friends:handleZoneChange(packet)
    if not packet or packet.type ~= "zone" then
        return false
    end
    
    self:updatePresence()
    return true
end

function M.Friends:updatePresence()
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local presence = self:queryPlayerPresence()
    if not presence or not presence.characterName or presence.characterName == "" then
        return false
    end
    
    -- isAnonymous = gameIsAnonymous AND NOT shareJobWhenAnonymous
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences and app.features.preferences.getPrefs then
        local prefs = app.features.preferences:getPrefs()
        if prefs then
            local gameIsAnonymous = presence.isAnonymous
            presence.isAnonymous = gameIsAnonymous and not prefs.shareJobWhenAnonymous
        end
    end
    
    local requestBody = RequestEncoder.encodeUpdatePresence(presence)
    local url = self.deps.connection:getBaseUrl() .. Endpoints.CHARACTERS.STATE
    
    local characterName = presence.characterName or ""
    if characterName == "" and self.deps.connection.getCharacterName then
        characterName = self.deps.connection:getCharacterName()
    end
    
    local headers = self.deps.connection:getHeaders(characterName)
    
    local requestId = self.deps.net.request({
        url = url,
        method = "PATCH",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            if not success and self.deps.logger and self.deps.logger.error then
                self.deps.logger.error("Failed to update presence: " .. tostring(response))
            end
        end
    })
    
    return requestId ~= nil
end

function M.Friends:sendHeartbeat()
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    self.heartbeatInFlight = true
    local now = getTime(self)
    self.lastHeartbeatAt = now
    
    local characterName = ""
    if self.deps.connection.getCharacterName then
        characterName = self.deps.connection:getCharacterName() or ""
    end
    if characterName == "" then
        local presence = self:queryPlayerPresence()
        characterName = presence.characterName or ""
    end
    
    if characterName == "" then
        self.heartbeatInFlight = false
        return false
    end
    
    local pluginVersion = nil
    local shouldReportVersion = not self.hasReportedVersion
    if shouldReportVersion then
        local addonVersion = addon and addon.version or "0.9.9"
        pluginVersion = addonVersion
    end
    
    local RequestEncoder = require("protocol.Encoding.RequestEncoder")
    local requestBody = RequestEncoder.encodeGetHeartbeat(
        characterName,
        self.lastEventTimestamp or 0,
        self.lastRequestEventTimestamp or 0,
        pluginVersion
    )
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.HEARTBEAT
    local headers = self.deps.connection:getHeaders(characterName)
    
    local requestId = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = requestBody,
        callback = function(success, response)
            self.heartbeatInFlight = false
            
            if not success then
                if self.deps.logger and self.deps.logger.warn then
                    self.deps.logger.warn("Heartbeat failed: " .. tostring(response))
                end
                return
            end
            
            -- Only mark version as reported after successful heartbeat
            if shouldReportVersion then
                self.hasReportedVersion = true
            end
            
            local ResponseDecoder = require("protocol.Decoding.ResponseDecoder")
            local decodeSuccess, result = ResponseDecoder.decode(response)
            
            if decodeSuccess and result then
                if result.lastEventTimestamp then
                    self.lastEventTimestamp = result.lastEventTimestamp
                end
                if result.lastRequestEventTimestamp then
                    self.lastRequestEventTimestamp = result.lastRequestEventTimestamp
                end
                
                if result.statuses and type(result.statuses) == "table" then
                    self:updateFriendStatuses(result.statuses)
                end
                
                if self.deps.logger and self.deps.logger.debug then
                    self.deps.logger.debug("Heartbeat success, updated " .. 
                        tostring(result.statuses and #result.statuses or 0) .. " friend statuses")
                end
            end
        end
    })
    
    return requestId ~= nil
end

function M.Friends:updateFriendStatuses(statuses)
    local FriendList = require("core.friendlist")
    
    for _, statusData in ipairs(statuses) do
        local characterName = statusData.name or statusData.characterName
        if characterName then
            local status = FriendList.FriendStatus.new()
            status.characterName = string.lower(characterName)
            status.displayName = statusData.name or statusData.characterName or ""
            status.isOnline = statusData.isOnline == true
            status.isAway = statusData.isAway == true
            status.job = statusData.job or ""
            status.rank = statusData.rank or ""
            status.zone = statusData.zone or ""
            status.nation = statusData.nation
            if status.nation == nil then
                status.nation = -1
            end
            status.lastSeenAt = statusData.lastSeenAt or 0
            if type(status.lastSeenAt) == "string" and status.lastSeenAt == "null" then
                status.lastSeenAt = 0
            end
            status.showOnlineStatus = statusData.sharesOnlineStatus ~= false
            status.friendedAs = statusData.friendedAsName or statusData.friendedAs or ""
            
            self.friendList:updateFriendStatus(status)
        end
    end
    
    self.lastUpdatedAt = getTime(self)
    
    -- Check for status changes and trigger notifications (heartbeat path)
    self:checkForStatusChanges(self.friendList:getFriendStatuses())
end

local function capitalizeName(name)
    if not name or name == "" then
        return name
    end
    local result = name:sub(1, 1):upper() .. name:sub(2):lower()
    return result
end

-- Check for online status changes and trigger notifications
function M.Friends:checkForStatusChanges(currentStatuses)
    if not currentStatuses then
        return
    end
    
    -- Build current online status map
    local currentOnlineStatus = {}
    local displayNames = {}
    
    for _, status in ipairs(currentStatuses) do
        local friendNameLower = string.lower(status.characterName or "")
        if friendNameLower ~= "" then
            currentOnlineStatus[friendNameLower] = status.isOnline == true
            displayNames[friendNameLower] = capitalizeName(status.displayName or status.characterName or friendNameLower)
        end
    end
    
    -- On first scan, store baseline and return (no notifications)
    if not self.initialStatusScanComplete then
        self.previousOnlineStatus = currentOnlineStatus
        self.initialStatusScanComplete = true
        if self.deps.logger and self.deps.logger.debug then
            self.deps.logger.debug("[Friends] Initial status scan complete, notifications enabled")
        end
        return
    end
    
    -- Compare current vs previous to find transitions
    local onlineTransitions = {}
    local offlineTransitions = {}
    
    for friendName, isCurrentlyOnline in pairs(currentOnlineStatus) do
        local wasPreviouslyOnline = self.previousOnlineStatus[friendName] == true
        
        if not wasPreviouslyOnline and isCurrentlyOnline then
            -- Friend came online
            table.insert(onlineTransitions, displayNames[friendName] or friendName)
        elseif wasPreviouslyOnline and not isCurrentlyOnline then
            -- Friend went offline
            table.insert(offlineTransitions, displayNames[friendName] or friendName)
        end
    end
    
    -- Log transitions for debugging
    if self.deps.logger and self.deps.logger.debug then
        if #onlineTransitions > 0 or #offlineTransitions > 0 then
            self.deps.logger.debug(string.format(
                "[NotificationDiff] onlineTransitions=[%s] offlineTransitions=[%s]",
                table.concat(onlineTransitions, ", "),
                table.concat(offlineTransitions, ", ")
            ))
        end
    end
    
    -- Get notifications feature from app
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.notifications then
        local Notifications = require("app.features.notifications")
        
        -- Create toasts for friends coming online
        for _, displayName in ipairs(onlineTransitions) do
            if self.deps.logger and self.deps.logger.debug then
                self.deps.logger.debug(string.format("[ToastQueue] enqueue type=FriendOnline name=%s", displayName))
            end
            app.features.notifications:push(Notifications.ToastType.FriendOnline, {
                title = "Friend Online",
                message = displayName .. " is now online",
                dedupeKey = "online_" .. string.lower(displayName)
            })
        end
        
        -- Create toasts for friends going offline (optional, can be enabled)
        -- Uncomment if offline notifications are desired:
        -- for _, displayName in ipairs(offlineTransitions) do
        --     app.features.notifications:push(Notifications.ToastType.FriendOffline, {
        --         title = "Friend Offline",
        --         message = displayName .. " went offline",
        --         dedupeKey = "offline_" .. string.lower(displayName)
        --     })
        -- end
    end
    
    -- Update previous status with current
    self.previousOnlineStatus = currentOnlineStatus
end

-- Check for new friend requests and trigger notifications
function M.Friends:checkForNewFriendRequests(incomingRequests)
    if not incomingRequests then
        return
    end
    
    local newRequests = {}
    
    for _, request in ipairs(incomingRequests) do
        local requestId = request.id or request.requestId
        if requestId and not self.processedRequestIds[requestId] then
            -- Mark as processed
            self.processedRequestIds[requestId] = true
            
            -- Get display name from the request
            local displayName = request.fromCharacterName or request.name or request.senderName or "Unknown"
            displayName = capitalizeName(displayName)
            
            table.insert(newRequests, displayName)
        end
    end
    
    -- Log new requests for debugging
    if self.deps.logger and self.deps.logger.debug then
        if #newRequests > 0 then
            self.deps.logger.debug(string.format(
                "[NotificationDiff] requestTransitions=[%s]",
                table.concat(newRequests, ", ")
            ))
        end
    end
    
    -- Get notifications feature from app
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.notifications then
        local Notifications = require("app.features.notifications")
        
        -- Create toasts for new friend requests
        for _, displayName in ipairs(newRequests) do
            if self.deps.logger and self.deps.logger.debug then
                self.deps.logger.debug(string.format("[ToastQueue] enqueue type=FriendRequest name=%s", displayName))
            end
            app.features.notifications:push(Notifications.ToastType.FriendRequestReceived, {
                title = "Friend Request",
                message = displayName .. " sent you a friend request",
                dedupeKey = "request_" .. string.lower(displayName)
            })
        end
    end
end

function M.Friends:queryPlayerPresence()
    local presence = {
        characterName = "",
        job = "",
        rank = "",
        nation = 0,
        zone = "",
        isAnonymous = false,
        timestamp = getTime(self)
    }
    
    if not AshitaCore then
        return presence
    end
    
    local memoryMgr = AshitaCore:GetMemoryManager()
    if not memoryMgr then
        return presence
    end
    
    -- Try to get character name from party first
    local party = memoryMgr:GetParty()
    if party then
        local success, playerName = pcall(function()
            return party:GetMemberName(0)
        end)
        if success and playerName and playerName ~= "" then
            presence.characterName = string.lower(playerName)
        end
    end
    
    -- Fallback: try entity manager (wrapped in pcall for safety)
    if presence.characterName == "" then
        local success, result = pcall(function()
            local entityMgr = memoryMgr:GetEntity()
            if entityMgr then
                -- Check player entity (index 0 is typically the player)
                local entityType = entityMgr:GetType(0)
                local namePtr = entityMgr:GetName(0)
                if entityType == 0 and namePtr and namePtr ~= "" then
                    return string.lower(namePtr)
                end
            end
            return ""
        end)
        if success and result and result ~= "" then
            presence.characterName = result
        end
    end
    
    if presence.characterName == "" then
        return presence
    end
    
    local party = memoryMgr:GetParty()
    local player = memoryMgr:GetPlayer()
    
    local isAnonymous = false
    if party then
        local mainJob = party:GetMemberMainJob(0)
        local mainJobLevel = party:GetMemberMainJobLevel(0)
        if mainJob == 0 or mainJobLevel == 0 then
            isAnonymous = true
        end
    end
    
    if player then
        local playerData = player:GetRawStructure()
        if playerData then
            local mainJob = playerData.MainJob
            local mainJobLevel = playerData.MainJobLevel
            local subJob = playerData.SubJob
            local subJobLevel = playerData.SubJobLevel
            
            presence.job = self:formatJobString(mainJob, mainJobLevel, subJob, subJobLevel)
            
            local playerRank = playerData.Rank
            if playerRank and playerRank > 0 then
                presence.rank = "Rank " .. tostring(playerRank)
            end
            
            presence.nation = playerData.Nation or 0
            
            if not isAnonymous and (mainJob == 0 or mainJobLevel == 0) then
                isAnonymous = true
            end
        end
    elseif party then
        local mainJob = party:GetMemberMainJob(0)
        local mainJobLevel = party:GetMemberMainJobLevel(0)
        local subJob = party:GetMemberSubJob(0)
        local subJobLevel = party:GetMemberSubJobLevel(0)
        
        presence.job = self:formatJobString(mainJob, mainJobLevel, subJob, subJobLevel)
    end
    
    presence.isAnonymous = isAnonymous
    
    if party then
        local zoneId = party:GetMemberZone(0)
        if zoneId and zoneId > 0 then
            presence.zone = self:getZoneNameFromId(zoneId)
        end
    end
    
    if presence.zone == "" and player then
        local resourceMgr = AshitaCore:GetResourceManager()
        if resourceMgr and party then
            local zoneId = party:GetMemberZone(0)
            if zoneId and zoneId > 0 then
                local zoneName = resourceMgr:GetString("zones.names", zoneId)
                if zoneName and zoneName ~= "" then
                    presence.zone = zoneName
                else
                    presence.zone = self:getZoneNameFromId(zoneId)
                end
            end
        end
    end
    
    return presence
end

function M.Friends:formatJobString(mainJob, mainJobLevel, subJob, subJobLevel)
    if not mainJob or mainJob == 0 or not mainJobLevel or mainJobLevel == 0 then
        return ""
    end
    
    local jobNames = {
        "NON", "WAR", "MNK", "WHM", "BLM", "RDM", "THF", "PLD", "DRK", "BST",
        "BRD", "RNG", "SAM", "NIN", "DRG", "SMN", "BLU", "COR", "PUP", "DNC",
        "SCH", "GEO", "RUN"
    }
    
    if mainJob >= 23 then
        return ""
    end
    
    local job = jobNames[mainJob + 1] .. " " .. tostring(mainJobLevel)
    
    if subJob and subJob > 0 and subJobLevel and subJobLevel > 0 and subJob < 23 then
        job = job .. "/" .. jobNames[subJob + 1] .. " " .. tostring(subJobLevel)
    end
    
    return job
end

function M.Friends:getZoneNameFromId(zoneId)
    if not zoneId or zoneId == 0 then
        return "unknown"
    end
    
    if AshitaCore then
        local resourceMgr = AshitaCore:GetResourceManager()
        if resourceMgr then
            local zoneName = resourceMgr:GetString("zones.names", zoneId)
            if zoneName and zoneName ~= "" then
                return zoneName
            end
        end
    end
    
    local zoneMap = {
        [0] = "unknown",
        [1] = "Phanauet Channel",
        [2] = "Carpenters' Landing",
        [3] = "Manaclipper",
        [4] = "Bibiki Bay",
        [5] = "Uleguerand Range",
        [6] = "Bearclaw Pinnacle",
        [7] = "Attohwa Chasm",
        [8] = "Boneyard Gully",
        [9] = "Pso'Xja",
        [10] = "The Shrouded Maw",
        [11] = "Oldton Movalpolos",
        [12] = "Newton Movalpolos",
        [13] = "Mine Shaft #2716",
        [14] = "Hall of Transference",
        [16] = "Promyvion - Holla",
        [17] = "Spire of Holla",
        [18] = "Promyvion - Dem",
        [19] = "Spire of Dem",
        [20] = "Promyvion - Mea",
        [21] = "Spire of Mea",
        [22] = "Promyvion - Vahzl",
        [23] = "Spire of Vahzl",
        [24] = "Lufaise Meadows",
        [25] = "Misareaux Coast",
        [26] = "Tavnazian Safehold",
        [27] = "Phomiuna Aqueducts",
        [28] = "Sacrarium",
        [29] = "Riverne - Site #B01",
        [30] = "Riverne - Site #A01",
        [31] = "Monarch Linn",
        [32] = "Sealion's Den",
        [33] = "Al'Taieu",
        [34] = "Grand Palace of Hu'Xzoi",
        [35] = "The Garden of Ru'Hmet",
        [36] = "Empyreal Paradox",
        [37] = "Temenos",
        [38] = "Apollyon",
        [39] = "Dynamis - Valkurm",
        [40] = "Dynamis - Buburimu",
        [41] = "Dynamis - Qufim",
        [42] = "Dynamis - Tavnazia",
        [43] = "Diorama Abdhaljs-Ghelsba",
        [44] = "Abdhaljs Isle-Purgonorgo",
        [45] = "Abyssea - Tahrongi",
        [46] = "Abyssea - Konschtat",
        [47] = "Abyssea - La Theine",
        [48] = "Abyssea - Misareaux",
        [49] = "Abyssea - Vunkerl",
        [50] = "Abyssea - Attohwa",
        [51] = "Abyssea - Altepa",
        [52] = "Abyssea - Uleguerand",
        [53] = "Abyssea - Grauberg",
        [134] = "King Ranperre's Tomb",
        [135] = "Dangruf Wadi",
        [136] = "Inner Horutoto Ruins",
        [137] = "Ordelle's Caves",
        [138] = "Outer Horutoto Ruins",
        [139] = "Eldieme Necropolis",
        [140] = "Gusgen Mines",
        [141] = "Crawlers' Nest",
        [142] = "Maze of Shakhrami",
        [143] = "Garlaige Citadel",
        [144] = "Fei'Yin",
        [145] = "Ifrit's Cauldron",
        [146] = "Qu'Bia Arena",
        [147] = "Cloister of Flames",
        [148] = "Cloister of Frost",
        [149] = "Cloister of Gales",
        [150] = "Cloister of Storms",
        [151] = "Cloister of Tremors",
        [152] = "Gustav Tunnel",
        [153] = "Labyrinth of Onzozo",
        [160] = "Abyssea - Tahrongi",
        [161] = "Abyssea - Konschtat",
        [162] = "Abyssea - La Theine",
        [163] = "Abyssea - Misareaux",
        [164] = "Abyssea - Vunkerl",
        [165] = "Abyssea - Attohwa",
        [166] = "Abyssea - Altepa",
        [167] = "Abyssea - Uleguerand",
        [168] = "Abyssea - Grauberg",
        [169] = "Abyssea - Empyreal Paradox",
        [184] = "Western Adoulin",
        [185] = "Eastern Adoulin",
        [188] = "Rala Waterways",
        [189] = "Rala Waterways [U]",
        [190] = "Yahse Hunting Grounds",
        [191] = "Ceizak Battlegrounds",
        [192] = "Foret de Hennetiel",
        [193] = "Morimar Basalt Fields",
        [194] = "Marjami Ravine",
        [195] = "Kamihr Drifts",
        [196] = "Sih Gates",
        [197] = "Moh Gates",
        [198] = "Cirdas Caverns",
        [199] = "Cirdas Caverns [U]",
        [200] = "Dho Gates",
        [201] = "Woh Gates",
        [202] = "Outer Ra'Kaznar",
        [203] = "Outer Ra'Kaznar [U]",
        [204] = "Ra'Kaznar Inner Court",
        [211] = "Cloister of Tides",
        [212] = "Gustav Tunnel",
        [213] = "Labyrinth of Onzozo",
        [220] = "Ship bound for Selbina",
        [221] = "Ship bound for Mhaura",
        [223] = "San d'Oria-Jeuno Airship",
        [224] = "Bastok-Jeuno Airship",
        [225] = "Windurst-Jeuno Airship",
        [226] = "Kazham-Jeuno Airship",
        [227] = "Ship bound for Selbina",
        [228] = "Ship bound for Mhaura",
        [230] = "Southern San d'Oria",
        [231] = "Northern San d'Oria",
        [232] = "Port San d'Oria",
        [233] = "Chateau d'Oraguille",
        [234] = "Bastok Mines",
        [235] = "Bastok Markets",
        [236] = "Port Bastok",
        [237] = "Metalworks",
        [238] = "Windurst Waters",
        [239] = "Windurst Walls",
        [240] = "Port Windurst",
        [241] = "Windurst Woods",
        [242] = "Heavens Tower",
        [243] = "Ru'Lude Gardens",
        [244] = "Upper Jeuno",
        [245] = "Lower Jeuno",
        [246] = "Port Jeuno",
        [247] = "Rabao",
        [248] = "Selbina",
        [249] = "Mhaura",
        [250] = "Kazham",
        [251] = "Hall of the Gods",
        [252] = "Norg"
    }
    
    local zoneName = zoneMap[zoneId]
    if zoneName then
        return zoneName
    end
    
    return "Zone " .. tostring(zoneId)
end

function M.Friends:handleStatusUpdate(packet)
end

return M
