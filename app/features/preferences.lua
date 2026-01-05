local Models = require("core.models")
local RequestEncoder = require("protocol.Encoding.RequestEncoder")
local Envelope = require("protocol.Envelope")
local DecodeRouter = require("protocol.DecodeRouter")
local MessageTypes = require("protocol.MessageTypes")
local Endpoints = require("protocol.Endpoints")
local UIConstants = require("core.UIConstants")
local Settings = require("libs.settings")

local M = {}

M.Preferences = {}
M.Preferences.__index = M.Preferences

function M.Preferences.new(deps)
    local self = setmetatable({}, M.Preferences)
    self.deps = deps or {}
    
    self.prefs = Models.Preferences.new()
    self.lastUpdatedAt = nil
    
    return self
end

function M.Preferences:getState()
    return {
        prefs = self.prefs,
        lastUpdatedAt = self.lastUpdatedAt
    }
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

-- Helper to safely get a value with a default
local function getVal(val, defaultVal)
    if val == nil then return defaultVal end
    return val
end

function M.Preferences:load()
    -- Use Settings module (Lua table persistence - preserves types)
    local data = Settings.get("data")
    if not data or type(data) ~= "table" then
        return false
    end
    
    local prefs = data.preferences
    if not prefs or type(prefs) ~= "table" then
        return false
    end
    
    -- Load from settings (types are preserved as Lua tables)
    self.prefs.shareFriendsAcrossAlts = getVal(prefs.shareFriendsAcrossAlts, true)
    self.prefs.mainFriendView.showJob = getVal(prefs.mainShowJob, true)
    self.prefs.mainFriendView.showZone = getVal(prefs.mainShowZone, false)
    self.prefs.mainFriendView.showNationRank = getVal(prefs.mainShowNationRank, false)
    self.prefs.mainFriendView.showLastSeen = getVal(prefs.mainShowLastSeen, false)
    self.prefs.mainHoverTooltip.showJob = getVal(prefs.mainHoverShowJob, true)
    self.prefs.mainHoverTooltip.showZone = getVal(prefs.mainHoverShowZone, true)
    self.prefs.mainHoverTooltip.showNationRank = getVal(prefs.mainHoverShowNationRank, true)
    self.prefs.mainHoverTooltip.showLastSeen = getVal(prefs.mainHoverShowLastSeen, false)
    self.prefs.mainHoverTooltip.showFriendedAs = getVal(prefs.mainHoverShowFriendedAs, true)
    self.prefs.quickOnlineHoverTooltip.showJob = getVal(prefs.quickHoverShowJob, true)
    self.prefs.quickOnlineHoverTooltip.showZone = getVal(prefs.quickHoverShowZone, true)
    self.prefs.quickOnlineHoverTooltip.showNationRank = getVal(prefs.quickHoverShowNationRank, true)
    self.prefs.quickOnlineHoverTooltip.showLastSeen = getVal(prefs.quickHoverShowLastSeen, false)
    self.prefs.quickOnlineHoverTooltip.showFriendedAs = getVal(prefs.quickHoverShowFriendedAs, true)
    self.prefs.debugMode = getVal(prefs.debugMode, false)
    self.prefs.shareJobWhenAnonymous = getVal(prefs.shareJobWhenAnonymous, false)
    self.prefs.showOnlineStatus = getVal(prefs.showOnlineStatus, true)
    self.prefs.shareLocation = getVal(prefs.shareLocation, true)
    self.prefs.notificationDuration = getVal(prefs.notificationDuration, 8.0)
    local DEFAULT_POS_X = UIConstants.NOTIFICATION_POSITION[1]
    local DEFAULT_POS_Y = UIConstants.NOTIFICATION_POSITION[2]
    local posX = prefs.notificationPositionX
    local posY = prefs.notificationPositionY
    self.prefs.notificationPositionX = (posX == nil or posX < 0) and DEFAULT_POS_X or posX
    self.prefs.notificationPositionY = (posY == nil or posY < 0) and DEFAULT_POS_Y or posY
    self.prefs.customCloseKeyCode = getVal(prefs.customCloseKeyCode, 0)
    self.prefs.controllerCloseButton = getVal(prefs.controllerCloseButton, UIConstants.CONTROLLER_CLOSE_BUTTON)
    self.prefs.windowsLocked = getVal(prefs.windowsLocked, false)
    self.prefs.notificationSoundsEnabled = getVal(prefs.notificationSoundsEnabled, true)
    self.prefs.soundOnFriendOnline = getVal(prefs.soundOnFriendOnline, true)
    self.prefs.soundOnFriendRequest = getVal(prefs.soundOnFriendRequest, true)
    self.prefs.notificationSoundVolume = getVal(prefs.notificationSoundVolume, 0.6)
    self.prefs.controllerLayout = getVal(prefs.controllerLayout, 'xinput')
    self.prefs.flistBindButton = getVal(prefs.flistBindButton, '')
    self.prefs.closeBindButton = getVal(prefs.closeBindButton, '')
    self.prefs.flBindButton = getVal(prefs.flBindButton, '')
    return true
end

function M.Preferences:save()
    -- Use Settings module (Lua table persistence - preserves types)
    local prefsData = {
        shareFriendsAcrossAlts = self.prefs.shareFriendsAcrossAlts,
        mainShowJob = self.prefs.mainFriendView.showJob,
        mainShowZone = self.prefs.mainFriendView.showZone,
        mainShowNationRank = self.prefs.mainFriendView.showNationRank,
        mainShowLastSeen = self.prefs.mainFriendView.showLastSeen,
        mainHoverShowJob = self.prefs.mainHoverTooltip.showJob,
        mainHoverShowZone = self.prefs.mainHoverTooltip.showZone,
        mainHoverShowNationRank = self.prefs.mainHoverTooltip.showNationRank,
        mainHoverShowLastSeen = self.prefs.mainHoverTooltip.showLastSeen,
        mainHoverShowFriendedAs = self.prefs.mainHoverTooltip.showFriendedAs,
        quickHoverShowJob = self.prefs.quickOnlineHoverTooltip.showJob,
        quickHoverShowZone = self.prefs.quickOnlineHoverTooltip.showZone,
        quickHoverShowNationRank = self.prefs.quickOnlineHoverTooltip.showNationRank,
        quickHoverShowLastSeen = self.prefs.quickOnlineHoverTooltip.showLastSeen,
        quickHoverShowFriendedAs = self.prefs.quickOnlineHoverTooltip.showFriendedAs,
        debugMode = self.prefs.debugMode,
        shareJobWhenAnonymous = self.prefs.shareJobWhenAnonymous,
        showOnlineStatus = self.prefs.showOnlineStatus,
        shareLocation = self.prefs.shareLocation,
        notificationDuration = self.prefs.notificationDuration,
        notificationPositionX = self.prefs.notificationPositionX,
        notificationPositionY = self.prefs.notificationPositionY,
        customCloseKeyCode = self.prefs.customCloseKeyCode,
        controllerCloseButton = self.prefs.controllerCloseButton,
        windowsLocked = self.prefs.windowsLocked,
        notificationSoundsEnabled = self.prefs.notificationSoundsEnabled,
        soundOnFriendOnline = self.prefs.soundOnFriendOnline,
        soundOnFriendRequest = self.prefs.soundOnFriendRequest,
        notificationSoundVolume = self.prefs.notificationSoundVolume,
        controllerLayout = self.prefs.controllerLayout,
        flistBindButton = self.prefs.flistBindButton,
        closeBindButton = self.prefs.closeBindButton,
        flBindButton = self.prefs.flBindButton
    }
    
    -- Update preferences in the data section (preserves apiKeys, serverSelection, etc.)
    local data = Settings.get("data") or {}
    data.preferences = data.preferences or {}
    for k, v in pairs(prefsData) do
        data.preferences[k] = v
    end
    Settings.save()
    
    self.lastUpdatedAt = getTime(self)
    return true
end

function M.Preferences:getPrefs()
    return self.prefs
end

function M.Preferences:setPref(key, value)
    if self.prefs[key] ~= nil then
        self.prefs[key] = value
        return true
    end
    return false
end

function M.Preferences:syncToServer(onComplete)
    if not self.deps.net or not self.deps.connection then
        if onComplete then onComplete(false) end
        return false
    end
    
    if not self.deps.connection:isConnected() then
        if onComplete then onComplete(false) end
        return false
    end
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.PREFERENCES
    
    local characterName = ""
    if self.deps.connection.getCharacterName then
        characterName = self.deps.connection:getCharacterName()
    elseif self.deps.connection.lastCharacterName then
        characterName = self.deps.connection.lastCharacterName
    end
    
    local headers = self.deps.connection:getHeaders(characterName)
    
    local json = require("protocol.Json")
    local body = json.encode({
        preferences = {
            shareFriendsAcrossAlts = self.prefs.shareFriendsAcrossAlts
        },
        privacy = {
            shareOnlineStatus = self.prefs.showOnlineStatus,
            shareLocation = self.prefs.shareLocation,
            shareCharacterData = self.prefs.shareJobWhenAnonymous
        }
    })
    
    local selfRef = self
    
    self.deps.net.request({
        url = url,
        method = "PATCH",
        headers = headers,
        body = body,
        callback = function(success, response)
            local syncSuccess = false
            if success then
                local ok, envelope = Envelope.decode(response)
                if ok and envelope.success then
                    if selfRef.deps.logger and selfRef.deps.logger.debug then
                        selfRef.deps.logger.debug("[Preferences] Privacy settings synced to server")
                    end
                    syncSuccess = true
                else
                    if selfRef.deps.logger and selfRef.deps.logger.debug then
                        selfRef.deps.logger.debug("[Preferences] Server sync not available (local settings saved)")
                    end
                end
            else
                if selfRef.deps.logger and selfRef.deps.logger.debug then
                    selfRef.deps.logger.debug("[Preferences] Server sync failed - local settings saved")
                end
            end
            
            if onComplete then
                onComplete(syncSuccess)
            end
        end
    })
    
    return true
end

function M.Preferences:refresh()
    if not self.deps.net or not self.deps.connection then
        return false
    end
    
    if not self.deps.connection:isConnected() then
        return false
    end
    
    local url = self.deps.connection:getBaseUrl() .. Endpoints.PREFERENCES
    
    local characterName = ""
    if self.deps.connection.getCharacterName then
        characterName = self.deps.connection:getCharacterName()
    elseif self.deps.connection.lastCharacterName then
        characterName = self.deps.connection.lastCharacterName
    end
    
    local headers = self.deps.connection:getHeaders(characterName)
    
    local timeMs = 0
    if self.deps.time then
        timeMs = self.deps.time()
    else
        timeMs = os.time() * 1000
    end
    if self.deps.logger and self.deps.logger.debug then
        self.deps.logger.debug(string.format("[Preferences] [%d] Refresh: GET %s", timeMs, url))
    end
    
    local requestId = self.deps.net.request({
        url = url,
        method = "GET",
        headers = headers,
        body = "",
        callback = function(success, response)
            local callbackTimeMs = 0
            if self.deps.time then
                callbackTimeMs = self.deps.time()
            else
                callbackTimeMs = os.time() * 1000
            end
            
            if success then
                local ok, envelope = Envelope.decode(response)
                if ok and envelope.success then
                    local decodeOk, result = DecodeRouter.decode(envelope)
                    if decodeOk then
                        local serverPrefs = result.preferences or {}
                        if serverPrefs.shareFriendsAcrossAlts ~= nil then
                            self.prefs.shareFriendsAcrossAlts = serverPrefs.shareFriendsAcrossAlts
                        end
                        if serverPrefs.useServerNotes ~= nil then
                            self.prefs.useServerNotes = serverPrefs.useServerNotes
                        end
                        
                        local serverPrivacy = result.privacy or {}
                        if serverPrivacy.shareOnlineStatus ~= nil then
                            self.prefs.showOnlineStatus = serverPrivacy.shareOnlineStatus
                        end
                        if serverPrivacy.shareLocation ~= nil then
                            self.prefs.shareLocation = serverPrivacy.shareLocation
                        end
                        if serverPrivacy.shareCharacterData ~= nil then
                            self.prefs.shareJobWhenAnonymous = serverPrivacy.shareCharacterData
                        end
                        
                        self:save()
                        
                        if self.deps.logger and self.deps.logger.debug then
                            self.deps.logger.debug(string.format("[Preferences] [%d] Refresh complete: server preferences applied", callbackTimeMs))
                        end
                    else
                        if self.deps.logger and self.deps.logger.warn then
                            self.deps.logger.warn(string.format("[Preferences] [%d] Refresh: Failed to decode preferences", callbackTimeMs))
                        end
                    end
                else
                    if self.deps.logger and self.deps.logger.warn then
                        local errorMsg = envelope and envelope.error or "Unknown error"
                        self.deps.logger.warn(string.format("[Preferences] [%d] Refresh: Server error: %s", callbackTimeMs, tostring(errorMsg)))
                    end
                end
            else
                if self.deps.logger and self.deps.logger.warn then
                    self.deps.logger.warn(string.format("[Preferences] [%d] Refresh: Request failed: %s", callbackTimeMs, tostring(response)))
                end
            end
        end
    })
    
    return requestId ~= nil
end

return M
