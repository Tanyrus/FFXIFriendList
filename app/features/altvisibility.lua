-- NOTE: Alt visibility feature may not be supported by the new server
-- This file uses legacy endpoints that may need to be updated or removed
local Endpoints = require("protocol.Endpoints")

local M = {}

M.AltVisibility = {}
M.AltVisibility.__index = M.AltVisibility

M.VisibilityState = {
    Visible = "Visible",
    NotVisible = "NotVisible",
    PendingRequest = "PendingRequest",
    Unknown = "Unknown"
}

function M.AltVisibility.new(deps)
    local self = setmetatable({}, M.AltVisibility)
    self.deps = deps or {}
    
    self.rows = {}
    self.characters = {}
    self.isLoading = false
    self.lastError = nil
    self.lastUpdateTime = 0
    self.filterText = ""
    self.pendingRefresh = false
    self.hasAttemptedRefresh = false  -- Prevents infinite refresh on empty data
    
    return self
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

function M.AltVisibility:getState()
    return {
        rows = self.rows,
        characters = self.characters,
        isLoading = self.isLoading,
        lastError = self.lastError,
        lastUpdateTime = self.lastUpdateTime,
        pendingRefresh = self.pendingRefresh,
        hasAttemptedRefresh = self.hasAttemptedRefresh
    }
end

function M.AltVisibility:checkPendingRefresh()
    if not self.pendingRefresh then
        return false
    end
    
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.connection then
        return false
    end
    
    local connection = app.features.connection
    if connection:isConnected() then
        self:refresh()
        return true
    end
    
    return false
end

function M.AltVisibility:getRows()
    return self.rows
end

function M.AltVisibility:getCharacters()
    return self.characters
end

function M.AltVisibility:getFilteredRows(filterText)
    if not filterText or filterText == "" then
        return self.rows
    end
    
    local lowerFilter = string.lower(filterText)
    local filtered = {}
    
    for _, row in ipairs(self.rows) do
        local lowerName = string.lower(row.friendedAsName or "")
        local lowerDisplay = string.lower(row.displayName or "")
        
        if string.find(lowerName, lowerFilter, 1, true) or
           string.find(lowerDisplay, lowerFilter, 1, true) then
            table.insert(filtered, row)
        end
    end
    
    return filtered
end

function M.AltVisibility:setLoading(loading)
    self.isLoading = loading
end

function M.AltVisibility:setError(error)
    self.lastError = error
end

function M.AltVisibility:clearError()
    self.lastError = nil
end

function M.AltVisibility:setLastUpdateTime(time)
    self.lastUpdateTime = time
end

function M.AltVisibility:updateFromResult(friends, characters)
    self.rows = {}
    self.characters = {}
    
    local charsInput = characters or {}
    
    for _, charInfo in ipairs(charsInput) do
        table.insert(self.characters, {
            characterId = charInfo.characterId,
            characterName = charInfo.characterName,
            isActive = charInfo.isActive or false
        })
    end
    
    local friendsInput = friends or {}
    
    for _, friendEntry in ipairs(friendsInput) do
        local row = {
            friendAccountId = friendEntry.friendAccountId,
            friendedAsName = friendEntry.friendedAsName or "",
            displayName = friendEntry.displayName or friendEntry.friendedAsName or "",
            realmId = friendEntry.realmId or "",
            addedByCharacterId = friendEntry.addedByCharacterId,
            characterVisibility = {}
        }
        
        for _, charInfo in ipairs(self.characters) do
            local charVis = {
                characterId = charInfo.characterId,
                characterName = charInfo.characterName,
                visibilityState = M.VisibilityState.NotVisible,
                hasPendingRequest = false,
                isBusy = false
            }
            
            local serverCharVis = friendEntry.characterVisibility
            if serverCharVis then
                local charIdKey = tostring(charInfo.characterId)
                local charVisEntry = serverCharVis[charIdKey] or serverCharVis[charInfo.characterId]
                
                if charVisEntry then
                    charVis.visibilityState = M.computeVisibilityState(charVisEntry)
                    charVis.hasPendingRequest = charVisEntry.hasPendingVisibilityRequest or false
                end
            end
            
            table.insert(row.characterVisibility, charVis)
        end
        
        table.insert(self.rows, row)
    end
    
    table.sort(self.rows, function(a, b)
        return string.lower(a.friendedAsName) < string.lower(b.friendedAsName)
    end)
end

function M.computeVisibilityState(charVis)
    if charVis.hasPendingVisibilityRequest then
        return M.VisibilityState.PendingRequest
    end
    
    if charVis.hasVisibility then
        return M.VisibilityState.Visible
    end
    
    return M.VisibilityState.NotVisible
end

function M.AltVisibility:setBusy(friendAccountId, characterId, busy)
    for _, row in ipairs(self.rows) do
        if row.friendAccountId == friendAccountId then
            for _, charVis in ipairs(row.characterVisibility) do
                if charVis.characterId == characterId then
                    charVis.isBusy = busy
                    return
                end
            end
            return
        end
    end
end

function M.AltVisibility:setVisibility(friendAccountId, characterId, desiredVisible, pending)
    for _, row in ipairs(self.rows) do
        if row.friendAccountId == friendAccountId then
            for _, charVis in ipairs(row.characterVisibility) do
                if charVis.characterId == characterId then
                    if pending then
                        charVis.visibilityState = M.VisibilityState.PendingRequest
                    else
                        if desiredVisible then
                            charVis.visibilityState = M.VisibilityState.Visible
                        else
                            charVis.visibilityState = M.VisibilityState.NotVisible
                        end
                    end
                    return true
                end
            end
            return false
        end
    end
    return false
end

function M.AltVisibility:isCheckboxEnabled(charVis)
    if charVis.isBusy then
        return false
    end
    if charVis.visibilityState == M.VisibilityState.PendingRequest then
        return false
    end
    return true
end

function M.AltVisibility:isCheckboxChecked(charVis)
    return charVis.visibilityState == M.VisibilityState.Visible or
           charVis.visibilityState == M.VisibilityState.PendingRequest
end

function M.AltVisibility:shouldShowCheckbox(row, charVis)
    if row.addedByCharacterId == nil then
        return true
    end
    
    return charVis.characterId ~= row.addedByCharacterId
end

function M.AltVisibility:refresh()
    if not self.deps.net then
        self:setError("Network not available")
        return
    end
    
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.connection then
        self:setError("Connection not available")
        return
    end
    
    local connection = app.features.connection
    if not connection:isConnected() then
        self.pendingRefresh = true
        self:setLoading(true)
        return
    end
    
    self.pendingRefresh = false
    self.hasAttemptedRefresh = true  -- Mark that we've attempted a refresh
    
    local characterName = connection:getCharacterName()
    local apiKey = connection:getApiKey(characterName)
    
    if not apiKey or apiKey == "" or not characterName or characterName == "" then
        self:setError("Missing credentials")
        return
    end
    
    self:setLoading(true)
    self:clearError()
    
    local serverUrl = connection:getBaseUrl()
    local url = serverUrl .. Endpoints.FRIENDS.CHARACTER_AND_FRIENDS
    
    -- Use new auth format: Authorization: Bearer
    local addonVersion = addon and addon.version or "0.9.95"
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. apiKey,
        ["X-Character-Name"] = characterName,
        ["User-Agent"] = "FFXIFriendList/" .. addonVersion
    }
    
    local selfRef = self
    
    self.deps.net.request({
        method = "GET",
        url = url,
        headers = headers,
        callback = function(success, body, errorStr)
            selfRef:setLoading(false)
            
            if not success then
                selfRef:setError(errorStr or "Request failed")
                return
            end
            
            if not body or body == "" then
                selfRef:setError("Empty response")
                return
            end
            
            local json = require("protocol.Json")
            local decodeSuccess, result = json.decode(body)
            
            if not decodeSuccess then
                selfRef:setError("Failed to parse response: " .. tostring(result))
                return
            end
            
            if not result then
                selfRef:setError("Failed to parse response")
                return
            end
            
            if result.success == false then
                selfRef:setError(result.error and result.error.message or "Server error")
                return
            end
            
            -- Handle response envelope: { success: true, data: { friends, characters } }
            local data = result.data or result
            selfRef:updateFromResult(data.friends, data.characters)
            selfRef:setLastUpdateTime(getTime(selfRef))
            
            if selfRef.deps.logger then
                selfRef.deps.logger.info("[AltVisibility] Refreshed visibility data: " .. 
                    #(selfRef.rows) .. " friends, " .. #(selfRef.characters) .. " characters")
            end
        end
    })
end

function M.AltVisibility:toggleVisibility(friendAccountId, friendName, characterId, desiredVisible)
    if not self.deps.net then
        return
    end
    
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.connection then
        return
    end
    
    local connection = app.features.connection
    if not connection:isConnected() then
        return
    end
    
    local characterName = connection:getCharacterName()
    local apiKey = connection:getApiKey(characterName)
    
    if not apiKey or apiKey == "" or not characterName or characterName == "" then
        return
    end
    
    self:setBusy(friendAccountId, characterId, true)
    
    self:setVisibility(friendAccountId, characterId, desiredVisible, true)
    
    local serverUrl = connection:getBaseUrl()
    
    -- Use new auth format: Authorization: Bearer
    local addonVersion = addon and addon.version or "0.9.95"
    local headers = {
        ["Content-Type"] = "application/json",
        ["Authorization"] = "Bearer " .. apiKey,
        ["X-Character-Name"] = characterName,
        ["User-Agent"] = "FFXIFriendList/" .. addonVersion
    }
    
    local currentCharacterId = nil
    for _, charInfo in ipairs(self.characters) do
        if charInfo.isActive or string.lower(charInfo.characterName or "") == string.lower(characterName) then
            currentCharacterId = charInfo.characterId
            break
        end
    end
    
    local useNewEndpoint = (currentCharacterId ~= nil and currentCharacterId ~= characterId)
    
    local selfRef = self
    local json = require("protocol.Json")
    
    local function handleResponse(success, responseBody, errorStr)
        selfRef:setBusy(friendAccountId, characterId, false)
        
        if not success then
            selfRef:setVisibility(friendAccountId, characterId, not desiredVisible, false)
            
            local errorMsg = errorStr or "Failed to toggle visibility"
            
            if app and app.features and app.features.notifications then
                local Notifications = require("app.features.notifications")
                app.features.notifications:push(Notifications.ToastType.Error, {
                    title = Notifications.TITLE_FRIEND_VISIBILITY,
                    message = errorMsg
                })
            end
            
            return
        end
        
        selfRef:setVisibility(friendAccountId, characterId, desiredVisible, false)
        
        if app and app.features and app.features.notifications then
            local Notifications = require("app.features.notifications")
            local message = desiredVisible and 
                Notifications.MESSAGE_VISIBILITY_UPDATED:gsub("{0}", friendName) or
                Notifications.MESSAGE_VISIBILITY_REMOVED:gsub("{0}", friendName)
            app.features.notifications:push(Notifications.ToastType.Success, {
                title = Notifications.TITLE_FRIEND_VISIBILITY,
                message = message
            })
        end
    end
    
    if useNewEndpoint then
        -- Update visibility for a different character (new model)
        local url = serverUrl .. Endpoints.FRIENDS.VISIBILITY
        local body = {
            characterName = friendName,
            realmId = self:getRealmForFriend(friendAccountId) or "",
            forCharacterId = characterId,
            desiredVisible = desiredVisible
        }
        local bodyStr = json.encode(body)
        
        self.deps.net.request({
            method = "POST",
            url = url,
            headers = headers,
            body = bodyStr,
            callback = handleResponse
        })
    elseif desiredVisible then
        -- Request visibility from a different character (old model, rare)
        local url = serverUrl .. Endpoints.FRIENDS.SEND_REQUEST
        local body = {
            toUserId = friendName
        }
        local bodyStr = json.encode(body)
        
        self.deps.net.request({
            method = "POST",
            url = url,
            headers = headers,
            body = bodyStr,
            callback = handleResponse
        })
    else
        -- Remove visibility - use POST endpoint with desiredVisible=false
        -- This works whether it's current character or alt
        local url = serverUrl .. Endpoints.FRIENDS.VISIBILITY
        local body = {
            characterName = friendName,
            realmId = self:getRealmForFriend(friendAccountId) or "",
            forCharacterId = characterId,
            desiredVisible = false
        }
        local bodyStr = json.encode(body)
        
        self.deps.net.request({
            method = "POST",
            url = url,
            headers = headers,
            body = bodyStr,
            callback = handleResponse
        })
    end
end

-- Get realm ID for a friend by account ID
function M.AltVisibility:getRealmForFriend(friendAccountId)
    for _, row in ipairs(self.rows) do
        if row.friendAccountId == friendAccountId then
            return row.realmId
        end
    end
    return nil
end

return M
