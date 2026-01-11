-- characterVisibility.lua
-- Per-Character Visibility feature
-- Manages visibility settings for each character on the account

local Endpoints = require("protocol.Endpoints")
local Envelope = require("protocol.Envelope")

local M = {}

M.CharacterVisibility = {}
M.CharacterVisibility.__index = M.CharacterVisibility

function M.CharacterVisibility.new(deps)
    local self = setmetatable({}, M.CharacterVisibility)
    self.deps = deps or {}
    
    -- Character visibility data
    self.characters = {}
    self.isLoading = false
    self.lastError = nil
    self.lastUpdateTime = 0
    self.lastFetchTime = 0
    
    return self
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

function M.CharacterVisibility:getState()
    return {
        characters = self.characters,
        isLoading = self.isLoading,
        lastError = self.lastError,
        lastUpdateTime = self.lastUpdateTime,
        lastFetchTime = self.lastFetchTime
    }
end

function M.CharacterVisibility:getCharacters()
    return self.characters
end

function M.CharacterVisibility:isCharacterVisible(characterId)
    for _, char in ipairs(self.characters) do
        if char.characterId == characterId then
            return char.shareVisibility
        end
    end
    -- Default to visible if not found
    return true
end

function M.CharacterVisibility:getActiveCharacterVisibility()
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.connection then
        return true -- Default to visible
    end
    
    local connection = app.features.connection
    local auth = connection:getAuth()
    if not auth or not auth.characterId then
        return true
    end
    
    return self:isCharacterVisible(auth.characterId)
end

-- Fetch visibility status for all characters from server
function M.CharacterVisibility:fetch()
    if self.isLoading then
        return
    end
    
    local net = self.deps.net
    if not net then
        self:_logError("No network dependency")
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
    
    self.isLoading = true
    self.lastError = nil
    
    local url = Endpoints.CHARACTERS.VISIBILITY
    
    net.request({
        method = "GET",
        url = url,
        callback = function(success, response)
            self.isLoading = false
            self.lastFetchTime = getTime(self)
            
            if not success then
                self.lastError = "Failed to fetch character visibility"
                self:_logError(self.lastError)
                return
            end
            
            local envelope = Envelope.parse(response)
            if not envelope.success then
                self.lastError = envelope.error or "Failed to parse response"
                self:_logError(self.lastError)
                return
            end
            
            local data = envelope.data
            if data and data.characters then
                self:_updateFromServer(data.characters)
            end
        end
    })
end

-- Update visibility for a specific character
function M.CharacterVisibility:setVisibility(characterId, shareVisibility)
    if self.isLoading then
        return
    end
    
    local net = self.deps.net
    if not net then
        self:_logError("No network dependency")
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
    
    self.isLoading = true
    self.lastError = nil
    
    -- Optimistic update
    self:_setLocalVisibility(characterId, shareVisibility)
    
    local url = Endpoints.characterVisibilityUpdate(characterId)
    
    net.request({
        method = "PATCH",
        url = url,
        body = {
            shareVisibility = shareVisibility
        },
        callback = function(success, response)
            self.isLoading = false
            self.lastUpdateTime = getTime(self)
            
            if not success then
                -- Revert optimistic update
                self:_setLocalVisibility(characterId, not shareVisibility)
                self.lastError = "Failed to update character visibility"
                self:_logError(self.lastError)
                return
            end
            
            local envelope = Envelope.parse(response)
            if not envelope.success then
                -- Revert optimistic update
                self:_setLocalVisibility(characterId, not shareVisibility)
                self.lastError = envelope.error or "Failed to parse response"
                self:_logError(self.lastError)
                return
            end
            
            local data = envelope.data
            if data and data.characters then
                self:_updateFromServer(data.characters)
            end
            
            self:_log("Character visibility updated: " .. characterId .. " = " .. tostring(shareVisibility))
        end
    })
end

-- Toggle visibility for a character
function M.CharacterVisibility:toggleVisibility(characterId)
    local currentVisibility = self:isCharacterVisible(characterId)
    self:setVisibility(characterId, not currentVisibility)
end

-- Internal: update from server response
function M.CharacterVisibility:_updateFromServer(serverCharacters)
    self.characters = {}
    
    for _, char in ipairs(serverCharacters) do
        table.insert(self.characters, {
            characterId = char.characterId,
            characterName = char.characterName,
            realmId = char.realmId,
            shareVisibility = char.shareVisibility
        })
    end
    
    -- Sort by character name
    table.sort(self.characters, function(a, b)
        return string.lower(a.characterName) < string.lower(b.characterName)
    end)
end

-- Internal: set local visibility (for optimistic updates)
function M.CharacterVisibility:_setLocalVisibility(characterId, shareVisibility)
    for i, char in ipairs(self.characters) do
        if char.characterId == characterId then
            self.characters[i].shareVisibility = shareVisibility
            return
        end
    end
end

-- Logging helpers
function M.CharacterVisibility:_log(message)
    local log = self.deps.log
    if log then
        log.info("[CharacterVisibility] " .. message)
    end
end

function M.CharacterVisibility:_logError(message)
    local log = self.deps.log
    if log then
        log.error("[CharacterVisibility] " .. message)
    end
end

return M
