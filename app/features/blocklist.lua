local Envelope = require("protocol.Envelope")
local DecodeRouter = require("protocol.DecodeRouter")
local Endpoints = require("protocol.Endpoints")

local M = {}

M.Blocklist = {}
M.Blocklist.__index = M.Blocklist

function M.Blocklist.new(deps)
    local self = setmetatable({}, M.Blocklist)
    self.deps = deps or {}
    
    self.blocked = {}
    self.blockedByAccountId = {}
    self.isLoading = false
    self.lastError = nil
    self.lastUpdateTime = 0
    self.refreshInFlight = false
    
    return self
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

local function log(self, level, message)
    if self.deps.logger and self.deps.logger[level] then
        self.deps.logger[level](message)
    end
end

function M.Blocklist:getState()
    return {
        blocked = self.blocked,
        isLoading = self.isLoading,
        lastError = self.lastError,
        lastUpdateTime = self.lastUpdateTime
    }
end

function M.Blocklist:getBlockedList()
    return self.blocked
end

function M.Blocklist:getBlockedCount()
    return #self.blocked
end

function M.Blocklist:isBlocked(accountId)
    return self.blockedByAccountId[accountId] == true
end

function M.Blocklist:findByName(name)
    local lowerName = string.lower(name or "")
    for _, entry in ipairs(self.blocked) do
        if string.lower(entry.displayName or "") == lowerName then
            return entry
        end
    end
    return nil
end

function M.Blocklist:refresh()
    if self.refreshInFlight then
        log(self, "debug", "[Blocklist] refresh() skipped: request already in flight")
        return false
    end
    
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    self.refreshInFlight = true
    self.isLoading = true
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.BLOCK.LIST
    local headers = self.deps.connection:getHeaders(self:_getCharacterName())
    
    local requestId = self.deps.net.request({
        url = url,
        method = "GET",
        headers = headers,
        body = "",
        callback = function(success, response)
            self:handleRefreshResponse(success, response)
        end
    })
    
    return requestId ~= nil
end

function M.Blocklist:handleRefreshResponse(success, response)
    self.refreshInFlight = false
    self.isLoading = false
    
    if not success then
        self.lastError = "Request failed"
        log(self, "warn", "[Blocklist] Failed to refresh: " .. tostring(response))
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        self.lastError = "Failed to decode response"
        log(self, "warn", "[Blocklist] Failed to decode envelope")
        return
    end
    
    if not envelope.success then
        self.lastError = envelope.error or "Server error"
        log(self, "warn", "[Blocklist] Server error: " .. tostring(envelope.error))
        return
    end
    
    local decodeOk, result = DecodeRouter.decode(envelope)
    if not decodeOk then
        self.lastError = "Failed to decode payload"
        log(self, "warn", "[Blocklist] Failed to decode payload")
        return
    end
    
    self.blocked = result.blocked or {}
    self.blockedByAccountId = {}
    for _, entry in ipairs(self.blocked) do
        if entry.accountId then
            self.blockedByAccountId[entry.accountId] = true
        end
    end
    
    self.lastError = nil
    self.lastUpdateTime = getTime(self)
    
    log(self, "debug", "[Blocklist] Refreshed: " .. #self.blocked .. " blocked accounts")
end

function M.Blocklist:block(characterName, callback)
    print("[Blocklist] block() called for: " .. tostring(characterName))
    if not self.deps.net or not self.deps.connection then
        print("[Blocklist] ERROR: net or connection not available")
        if callback then callback(false, "Not connected") end
        return false
    end
    
    if not self.deps.connection:isConnected() then
        print("[Blocklist] ERROR: not connected to server")
        if callback then callback(false, "Not connected") end
        return false
    end
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.BLOCK.ADD
    print("[Blocklist] URL: " .. tostring(url))
    
    -- Get current user's character name for authentication headers
    -- NOTE: characterName parameter is the person TO BLOCK, not the current user
    local currentUserCharacterName = self:_getCharacterName()
    print("[Blocklist] Current user characterName: " .. tostring(currentUserCharacterName))
    print("[Blocklist] Blocking characterName: " .. tostring(characterName))
    
    local headers = self.deps.connection:getHeaders(currentUserCharacterName)
    print("[Blocklist] Headers: X-API-Key=" .. tostring(headers["X-API-Key"] and "present" or "missing") .. ", characterName=" .. tostring(headers["characterName"] or "missing"))
    
    -- Use JSON encoding to properly escape special characters
    -- characterName parameter is the person TO BLOCK, not the current user
    local Json = require("protocol.Json")
    local body = Json.encode({characterName = tostring(characterName)})
    print("[Blocklist] Body: " .. tostring(body))
    
    print("[Blocklist] Sending block request...")
    local requestId = self.deps.net.request({
        url = url,
        method = "POST",
        headers = headers,
        body = body,
        callback = function(success, response)
            print("[Blocklist] Request callback: success=" .. tostring(success) .. ", response length=" .. tostring(response and #response or 0))
            self:handleBlockResponse(success, response, characterName, callback)
        end
    })
    
    if not requestId then
        print("[Blocklist] ERROR: Failed to enqueue block request")
    else
        print("[Blocklist] Request enqueued with ID: " .. tostring(requestId))
    end
    
    return requestId ~= nil
end

function M.Blocklist:handleBlockResponse(success, response, characterName, callback)
    if not success then
        log(self, "warn", "[Blocklist] Failed to block " .. tostring(characterName) .. ": " .. tostring(response))
        if callback then callback(false, "Request failed") end
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        log(self, "warn", "[Blocklist] Failed to decode block response")
        if callback then callback(false, "Failed to decode response") end
        return
    end
    
    if not envelope.success then
        local errorMsg = envelope.error or "Server error"
        log(self, "warn", "[Blocklist] Block failed: " .. errorMsg)
        if callback then callback(false, errorMsg) end
        return
    end
    
    log(self, "info", "[Blocklist] Blocked " .. tostring(characterName))
    
    self:refresh()
    
    if callback then
        callback(true, {
            blockedAccountId = envelope.payload and envelope.payload.blockedAccountId,
            alreadyBlocked = envelope.payload and envelope.payload.alreadyBlocked
        })
    end
end

function M.Blocklist:unblock(accountId, callback)
    print("[Blocklist] unblock() called for accountId: " .. tostring(accountId))
    if not self.deps.net or not self.deps.connection then
        print("[Blocklist] ERROR: net or connection not available")
        if callback then callback(false, "Not connected") end
        return false
    end
    
    if not self.deps.connection:isConnected() then
        print("[Blocklist] ERROR: not connected to server")
        if callback then callback(false, "Not connected") end
        return false
    end
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.blockRemove(accountId)
    print("[Blocklist] URL: " .. tostring(url))
    local currentCharacterName = self:_getCharacterName()
    print("[Blocklist] Current user characterName: " .. tostring(currentCharacterName))
    local headers = self.deps.connection:getHeaders(currentCharacterName)
    print("[Blocklist] Headers: X-API-Key=" .. tostring(headers["X-API-Key"] and "present" or "missing"))
    
    print("[Blocklist] Sending unblock request...")
    local requestId = self.deps.net.request({
        url = url,
        method = "DELETE",
        headers = headers,
        body = "",
        callback = function(success, response)
            print("[Blocklist] Request callback: success=" .. tostring(success) .. ", response length=" .. tostring(response and #response or 0))
            self:handleUnblockResponse(success, response, accountId, callback)
        end
    })
    
    if not requestId then
        print("[Blocklist] ERROR: Failed to enqueue unblock request")
    else
        print("[Blocklist] Request enqueued with ID: " .. tostring(requestId))
    end
    
    return requestId ~= nil
end

function M.Blocklist:handleUnblockResponse(success, response, accountId, callback)
    if not success then
        log(self, "warn", "[Blocklist] Failed to unblock accountId " .. tostring(accountId) .. ": " .. tostring(response))
        if callback then callback(false, "Request failed") end
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        log(self, "warn", "[Blocklist] Failed to decode unblock response")
        if callback then callback(false, "Failed to decode response") end
        return
    end
    
    if not envelope.success then
        local errorMsg = envelope.error or "Server error"
        log(self, "warn", "[Blocklist] Unblock failed: " .. errorMsg)
        if callback then callback(false, errorMsg) end
        return
    end
    
    log(self, "info", "[Blocklist] Unblocked accountId " .. tostring(accountId))
    
    self:refresh()
    
    if callback then
        callback(true, {
            unblockedAccountId = envelope.payload and envelope.payload.unblockedAccountId,
            wasBlocked = envelope.payload and envelope.payload.wasBlocked
        })
    end
end

function M.Blocklist:unblockByName(characterName, callback)
    local entry = self:findByName(characterName)
    if not entry then
        if callback then callback(false, "Player not found in blocklist") end
        return false
    end
    
    return self:unblock(entry.accountId, callback)
end

function M.Blocklist:_getCharacterName()
    -- Try to get character name from connection (same pattern as preferences.lua)
    if self.deps.connection then
        if self.deps.connection.getCharacterName then
            local name = self.deps.connection:getCharacterName()
            if name and name ~= "" then
                return name
            end
        end
        if self.deps.connection.lastCharacterName then
            local name = self.deps.connection.lastCharacterName
            if name and name ~= "" then
                return name
            end
        end
    end
    
    -- Fallback: try to get from app
    local app = _G.FFXIFriendListApp
    if app and app.getCharacterName then
        return app.getCharacterName()
    end
    
    return nil
end

return M

