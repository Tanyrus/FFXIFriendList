local M = {}
local RequestEncoder = require("protocol.Encoding.RequestEncoder")
local Envelope = require("protocol.Envelope")
local DecodeRouter = require("protocol.DecodeRouter")
local MessageTypes = require("protocol.MessageTypes")
local Endpoints = require("protocol.Endpoints")

local DEFAULT_SERVER_URL = "https://api.horizonfriendlist.com"

M.ConnectionState = {
    Disconnected = "Disconnected",
    Connecting = "Connecting",
    Connected = "Connected",
    Reconnecting = "Reconnecting",
    Failed = "Failed"
}

M.NetworkBlockReason = {
    Allowed = "Allowed",
    BlockedByServerSelection = "BlockedByServerSelection"
}

M.Connection = {}
M.Connection.__index = M.Connection

local function isGameAlive()
    -- Safely check if game is alive and character is loaded
    if not AshitaCore then
        return false
    end
    
    local success, result = pcall(function()
        local memoryMgr = AshitaCore:GetMemoryManager()
        if not memoryMgr then
            return false
        end
        
        local party = memoryMgr:GetParty()
        if not party then
            return false
        end
        
        local mainJobLevel = party:GetMemberMainJobLevel(0)
        if not mainJobLevel or mainJobLevel == 0 then
            return false
        end
        
        return true
    end)
    
    return success and result == true
end

local function getCharacterName()
    -- Safely get character name, returns "" if game/character not ready
    if not AshitaCore then
        return ""
    end
    
    local memoryMgr = AshitaCore:GetMemoryManager()
    if not memoryMgr then
        return ""
    end
    
    -- Primary method: Get name from party member 0 (the player)
    local party = memoryMgr:GetParty()
    if party then
        local success, playerName = pcall(function()
            return party:GetMemberName(0)
        end)
        if success and playerName and playerName ~= "" then
            return string.lower(playerName)
        end
    end
    
    -- Fallback: Try entity manager (wrapped in pcall for safety)
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
        return result
    end
    
    return ""
end

function M.Connection.new(deps)
    local self = setmetatable({}, M.Connection)
    deps = deps or {}
    self.deps = deps
    self.logger = deps.logger
    self.net = deps.net
    self.session = deps.session
    self.prefsStore = deps.storage
    self.realmDetector = deps.realmDetector
    
    self.state = M.ConnectionState.Disconnected
    self.apiKeys = {}
    self.savedServerId = nil
    self.savedServerBaseUrl = nil
    self.savedRealmId = nil
    self.draftServerId = nil
    self.detectedServerSuggestion = nil
    self.detectedServerShownOnce = false
    
    self.autoConnectAttempted = false
    self.autoConnectInProgress = false
    self.lastCharacterName = ""
    
    if _G.gConfig and _G.gConfig.data and _G.gConfig.data.apiKeys then
        for charName, apiKey in pairs(_G.gConfig.data.apiKeys) do
            if apiKey and apiKey ~= "" then
                self.apiKeys[charName] = apiKey
            end
        end
    end
    
    if _G.gConfig and _G.gConfig.data and _G.gConfig.data.serverSelection then
        local serverSel = _G.gConfig.data.serverSelection
        if serverSel.savedServerId and serverSel.savedServerId ~= "" then
            self.savedServerId = serverSel.savedServerId
            self.savedServerBaseUrl = serverSel.savedServerBaseUrl or DEFAULT_SERVER_URL
            self.savedRealmId = serverSel.savedServerId
            if self.logger and self.logger.info then
                self.logger.info("[Connection] Loaded saved server: " .. tostring(self.savedServerId) .. " (" .. tostring(self.savedServerBaseUrl) .. "), realm: " .. tostring(self.savedRealmId))
            end
        end
    end
    
    return self
end

function M.Connection:getState()
    return {
        state = self.state,
        isConnected = self:isConnected(),
        isConnecting = self:isConnecting(),
        hasServer = self:hasSavedServer(),
        baseUrl = self:getBaseUrl(),
        lastError = self.lastError
    }
end

function M.Connection:tick(dtSeconds)
    if not self:hasSavedServer() then
        return
    end
    
    if self:isConnected() or self.autoConnectInProgress or self.autoConnectAttempted then
        return
    end
    
    local characterName = getCharacterName()
    if characterName == "" then
        return
    end
    
    if characterName ~= self.lastCharacterName then
        self.lastCharacterName = characterName
        if self.autoConnectAttempted then
            self.autoConnectAttempted = false
            self.autoConnectInProgress = false
            if self.logger and self.logger.info then
                self.logger.info("[Connection] Character changed, resetting auto-connect")
            end
        end
    end
    
    self:autoConnect(characterName)
end

function M.Connection:startConnecting()
    if self.state == M.ConnectionState.Disconnected or 
       self.state == M.ConnectionState.Failed then
        self.state = M.ConnectionState.Connecting
        if self.logger and self.logger.echo then
            self.logger.echo("Connecting to server...")
        elseif self.logger and self.logger.info then
            self.logger.info("[Connection] State: Disconnected -> Connecting")
        end
    end
end

function M.Connection:setConnected()
    if self.state == M.ConnectionState.Connecting or 
       self.state == M.ConnectionState.Reconnecting then
        self.state = M.ConnectionState.Connected
        if self.logger and self.logger.echo then
            self.logger.echo("Connected to server")
        elseif self.logger and self.logger.info then
            self.logger.info("[Connection] State: Connecting -> Connected")
        end
        
        local app = _G.FFXIFriendListApp
        if app and app.features and app.features.friends then
            local function sendInitialPresence()
                if app.features.friends.updatePresence then
                    app.features.friends:updatePresence()
                end
            end
            sendInitialPresence()
        end
    end
end

function M.Connection:setDisconnected()
    self.state = M.ConnectionState.Disconnected
    if self.logger and self.logger.info then
        self.logger.info("[Connection] State: -> Disconnected")
    end
    
    local app = _G.FFXIFriendListApp
    if app then
        app._startupRefreshCompleted = false
    end
end

function M.Connection:startReconnecting()
    if self.state == M.ConnectionState.Connected then
        self.state = M.ConnectionState.Reconnecting
    end
end

function M.Connection:setFailed()
    self.state = M.ConnectionState.Failed
    local errorMsg = self.lastError and (" (" .. tostring(self.lastError) .. ")") or ""
    if self.logger and self.logger.echo then
        self.logger.echo("Connection failed" .. errorMsg)
    elseif self.logger and self.logger.warn then
        self.logger.warn("[Connection] State: -> Failed" .. errorMsg)
    end
end

function M.Connection:isConnected()
    return self.state == M.ConnectionState.Connected
end

function M.Connection:isConnecting()
    return self.state == M.ConnectionState.Connecting or 
           self.state == M.ConnectionState.Reconnecting
end

function M.Connection:canConnect()
    return self.state == M.ConnectionState.Disconnected or 
           self.state == M.ConnectionState.Failed
end

function M.Connection:getApiKey(characterName)
    return self.apiKeys[characterName] or ""
end

function M.Connection:setApiKey(characterName, apiKey)
    self.apiKeys[characterName] = apiKey
end

function M.Connection:hasApiKey(characterName)
    return self.apiKeys[characterName] ~= nil and self.apiKeys[characterName] ~= ""
end

function M.Connection:clearApiKey(characterName)
    self.apiKeys[characterName] = nil
end

function M.Connection:hasSavedServer()
    return self.savedServerId ~= nil and self.savedServerId ~= ""
end

function M.Connection:isBlocked()
    return not self:hasSavedServer()
end

function M.Connection:setSavedServer(serverId, baseUrl)
    self.savedServerId = serverId
    self.savedServerBaseUrl = baseUrl
    self.savedRealmId = serverId
    if self.logger and self.logger.echo then
        self.logger.echo("Server selected: " .. tostring(serverId) .. " (" .. tostring(baseUrl) .. ")")
    elseif self.logger and self.logger.info then
        self.logger.info("[Connection] Server set: " .. tostring(serverId) .. " -> " .. tostring(baseUrl) .. ", realm: " .. tostring(serverId))
    end
end

function M.Connection:clearSavedServer()
    self.savedServerId = nil
    self.savedServerBaseUrl = nil
    self.savedRealmId = nil
end

function M.Connection:checkNetworkAccess()
    if self:isBlocked() then
        return M.NetworkBlockReason.BlockedByServerSelection
    end
    return M.NetworkBlockReason.Allowed
end

function M.Connection:getBlockReason()
    if self:isBlocked() then
        return "Server selection required"
    end
    return ""
end

function M.Connection:getBaseUrl()
    if self.savedServerBaseUrl and self.savedServerBaseUrl ~= "" then
        return self.savedServerBaseUrl
    end
    return DEFAULT_SERVER_URL
end

function M.Connection:getCharacterName()
    return getCharacterName()
end

function M.Connection:isGameAlive()
    return isGameAlive()
end

function M.Connection:getHeaders(characterName)
    characterName = characterName or ""
    local ProtocolVersion = require("protocol.ProtocolVersion")
    local addonVersion = addon and addon.version or "0.9.9"
    
    local headers = {
        ["Content-Type"] = "application/json",
        ["User-Agent"] = "FFXIFriendList/" .. addonVersion,
        ["X-Protocol-Version"] = ProtocolVersion.PROTOCOL_VERSION
    }
    
    local realmIdForHeader = nil
    if self.savedRealmId and self.savedRealmId ~= "" then
        realmIdForHeader = self.savedRealmId
    elseif self.realmDetector and self.realmDetector.getRealmId then
        realmIdForHeader = self.realmDetector:getRealmId()
    end
    if realmIdForHeader and realmIdForHeader ~= "" then
        headers["X-Realm-Id"] = realmIdForHeader
    end
    
    if characterName ~= "" then
        local apiKey = self:getApiKey(characterName)
        if apiKey ~= "" then
            headers["X-API-Key"] = apiKey
            headers["characterName"] = characterName
            if self.logger and self.logger.debug then
                self.logger.debug("[Connection] Headers: API key present for " .. characterName)
            end
        else
            if self.logger and self.logger.warn then
                self.logger.warn("[Connection] Headers: No API key for " .. characterName)
            end
        end
        
        if self.session and self.session.getSessionId then
            local sessionId = self.session:getSessionId()
            if sessionId and sessionId ~= "" then
                headers["X-Session-Id"] = sessionId
            end
        end
    end
    
    return headers
end

function M.Connection:autoConnect(characterName)
    if not characterName or characterName == "" then
        return false
    end
    
    if not self:hasSavedServer() then
        return false
    end
    
    if self.autoConnectAttempted or self.autoConnectInProgress then
        return false
    end
    
    self.autoConnectAttempted = true
    self.autoConnectInProgress = true
    
    local normalizedName = string.lower(characterName)
    local realmId = "notARealServer"
    if self.savedRealmId and self.savedRealmId ~= "" then
        realmId = self.savedRealmId
    elseif self.realmDetector and self.realmDetector.getRealmId then
        realmId = self.realmDetector:getRealmId() or "notARealServer"
    end
    
    if self.logger and self.logger.echo then
        self.logger.echo("Auto-connecting as " .. normalizedName .. "...")
    elseif self.logger and self.logger.info then
        self.logger.info("[Connection] Auto-connecting as " .. normalizedName .. " (server: " .. self:getBaseUrl() .. ")")
    end
    
    self:startConnecting()
    
    local storedApiKey = self:getApiKey(normalizedName)
    
    -- If no API key for this character, use any available API key from linked characters
    -- This allows alt registration to link to the existing account
    if not storedApiKey or storedApiKey == "" then
        for charName, apiKey in pairs(self.apiKeys) do
            if apiKey and apiKey ~= "" then
                storedApiKey = apiKey
                if self.logger and self.logger.debug then
                    self.logger.debug("[Connection] Using API key from " .. charName .. " for alt registration")
                end
                break
            end
        end
    end
    
    local function attemptConnect()
        local baseUrl = self:getBaseUrl()
        baseUrl = baseUrl:gsub("/+$", "")
        
        -- If we have an API key, use /api/characters/active (auto-creates character on same account)
        -- If no API key, use /api/auth/ensure (new registration or recovery)
        local hasApiKey = storedApiKey and storedApiKey ~= ""
        local url, requestBody
        
        if hasApiKey then
            -- Use characters/active endpoint (matches C++ behavior)
            url = baseUrl .. Endpoints.CHARACTERS.ACTIVE
            requestBody = RequestEncoder.encodeSetActiveCharacter(normalizedName, realmId)
        else
            -- Use auth/ensure for new registrations
            url = baseUrl .. Endpoints.AUTH.ENSURE
            requestBody = RequestEncoder.encodeAuthEnsure(normalizedName, realmId)
        end
        
        local ProtocolVersion = require("protocol.ProtocolVersion")
        local addonVersion = addon and addon.version or "0.9.9"
        local headers = {
            ["Content-Type"] = "application/json",
            ["User-Agent"] = "FFXIFriendList/" .. addonVersion,
            ["X-Protocol-Version"] = ProtocolVersion.PROTOCOL_VERSION
        }
        
        if hasApiKey then
            headers["X-API-Key"] = storedApiKey
            headers["characterName"] = normalizedName
        end
        
        if self.session and self.session.getSessionId then
            local sessionId = self.session:getSessionId()
            if sessionId and sessionId ~= "" then
                headers["X-Session-Id"] = sessionId
            end
        end
        
        local realmIdForHeader = nil
        if self.savedRealmId and self.savedRealmId ~= "" then
            realmIdForHeader = self.savedRealmId
        elseif self.realmDetector and self.realmDetector.getRealmId then
            realmIdForHeader = self.realmDetector:getRealmId()
        end
        if realmIdForHeader and realmIdForHeader ~= "" then
            headers["X-Realm-Id"] = realmIdForHeader
        end
        
        if self.logger and self.logger.echo then
            self.logger.echo("Auth URL: " .. url)
            self.logger.echo("Auth body: " .. tostring(requestBody))
            self.logger.echo("Auth headers:")
            for k, v in pairs(headers) do
                self.logger.echo("  " .. tostring(k) .. ": " .. tostring(v))
            end
        elseif self.logger and self.logger.info then
            self.logger.info("[Connection] Auth URL: " .. url)
            self.logger.info("[Connection] Auth body: " .. tostring(requestBody))
            self.logger.info("[Connection] Auth headers:")
            for k, v in pairs(headers) do
                self.logger.info("[Connection]   " .. tostring(k) .. ": " .. tostring(v))
            end
        end
        
        local requestId = self.net.request({
            url = url,
            method = "POST",
            headers = headers,
            body = requestBody,
            callback = function(success, response)
                self:handleAuthResponse(success, response, normalizedName, storedApiKey)
            end
        })
        
        return requestId ~= nil
    end
    
    return attemptConnect()
end

function M.Connection:handleAuthResponse(success, response, characterName, fallbackApiKey)
    self.autoConnectInProgress = false
    
    if not success then
        self:setFailed()
        self.lastError = response or "Network error"
        if self.logger and self.logger.echo then
            self.logger.echo("Connection failed: " .. tostring(self.lastError))
        elseif self.logger and self.logger.error then
            self.logger.error("[Connection] Auth failed: " .. tostring(self.lastError))
        end
        return
    end
    
    local ok, envelope = Envelope.decode(response)
    if not ok then
        self:setFailed()
        self.lastError = envelope or "Failed to decode response"
        if self.logger and self.logger.error then
            self.logger.error("[Connection] Envelope decode failed: " .. tostring(self.lastError))
        end
        return
    end
    
    -- Accept both AuthEnsureResponse (new registration) and SetActiveCharacterResponse (alt linking)
    local validResponseTypes = {
        [MessageTypes.ResponseType.AuthEnsureResponse] = true,
        [MessageTypes.ResponseType.SetActiveCharacterResponse] = true
    }
    if not envelope.success or not validResponseTypes[envelope.type] then
        self:setFailed()
        self.lastError = envelope.error or "Authentication failed"
        if self.logger and self.logger.echo then
            self.logger.echo("Authentication failed: " .. tostring(self.lastError))
        elseif self.logger and self.logger.error then
            self.logger.error("[Connection] Auth failed: " .. tostring(self.lastError))
        end
        return
    end
    
    local decodeOk, result = DecodeRouter.decode(envelope)
    if not decodeOk then
        self:setFailed()
        self.lastError = result or "Failed to decode response"
        return
    end
    
    local apiKey = ""
    if result and type(result) == "table" then
        apiKey = result.apiKey or ""
    end
    if apiKey == "" and envelope.payload and type(envelope.payload) == "table" then
        apiKey = envelope.payload.apiKey or ""
    end
    
    if apiKey == "" then
        apiKey = fallbackApiKey or ""
    end
    
    if apiKey and apiKey ~= "" then
        self:setApiKey(characterName, apiKey)
        if _G.gConfig then
            if not _G.gConfig.data then
                _G.gConfig.data = {}
            end
            if not _G.gConfig.data.apiKeys then
                _G.gConfig.data.apiKeys = {}
            end
            _G.gConfig.data.apiKeys[characterName] = apiKey
            local settings = require("libs.settings")
            if settings and settings.save then
                settings.save()
            end
        end
    end
    
    self:setConnected()
    if self.logger and self.logger.echo then
        self.logger.echo("Connected to server as " .. characterName)
    elseif self.logger and self.logger.info then
        self.logger.info("[Connection] Successfully authenticated " .. characterName)
    end
    
    local app = _G.FFXIFriendListApp
    if app and not app._startupRefreshCompleted then
        if self.logger and self.logger.debug then
            local timeMs = 0
            if app.deps and app.deps.time then
                timeMs = app.deps.time()
            else
                timeMs = os.time() * 1000
            end
            self.logger.debug(string.format("[Connection] [%d] Triggering immediate startup refresh", timeMs))
        end
        local App = require("app.App")
        if App and App._triggerStartupRefresh then
            App._triggerStartupRefresh(app)
            app._startupRefreshCompleted = true
        end
    end
end

function M.Connection:connect(characterName)
    characterName = characterName or getCharacterName()
    if characterName == "" then
        if self.logger and self.logger.warn then
            self.logger.warn("[Connection] Cannot connect: character name not available")
        end
        return false
    end
    
    if not self:canConnect() then
        if self.logger and self.logger.warn then
            self.logger.warn("[Connection] Cannot connect: already connecting or connected")
        end
        return false
    end
    
    self.autoConnectAttempted = false
    self.autoConnectInProgress = false
    
    return self:autoConnect(characterName)
end

function M.Connection:disconnect()
    self:setDisconnected()
end

function M.Connection:setServer(serverId, baseUrl)
    self:setSavedServer(serverId, baseUrl)
end

function M.Connection:getStatus()
    return {
        state = self.state,
        isConnected = self:isConnected(),
        isConnecting = self:isConnecting(),
        hasServer = self:hasSavedServer(),
        baseUrl = self:getBaseUrl()
    }
end

return M
