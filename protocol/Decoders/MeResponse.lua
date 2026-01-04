-- Decoders/MeResponse.lua
-- Decode MeResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        accountId = 0,
        currentCharacterId = 0,
        currentCharacterName = "",
        currentRealmId = "",
        activeCharacterId = nil,
        activeCharacterName = nil,
        mainCharacterId = nil,
        characters = {},
        preferences = {},
        privacy = {}
    }
    
    if payload.accountId ~= nil then
        result.accountId = tonumber(payload.accountId) or 0
    end
    
    if payload.currentCharacterId ~= nil then
        result.currentCharacterId = tonumber(payload.currentCharacterId) or 0
    end
    
    if payload.currentCharacterName then
        result.currentCharacterName = payload.currentCharacterName
    end
    
    if payload.currentRealmId then
        result.currentRealmId = payload.currentRealmId
    end
    
    if payload.activeCharacterId ~= nil then
        result.activeCharacterId = tonumber(payload.activeCharacterId)
    end
    
    if payload.activeCharacterName then
        result.activeCharacterName = payload.activeCharacterName
    end
    
    if payload.mainCharacterId ~= nil then
        result.mainCharacterId = tonumber(payload.mainCharacterId)
    end
    
    if payload.characters and type(payload.characters) == "table" then
        result.characters = payload.characters
    end
    
    if payload.preferences and type(payload.preferences) == "table" then
        result.preferences = payload.preferences
    end
    
    if payload.privacy and type(payload.privacy) == "table" then
        result.privacy = payload.privacy
    end
    
    return true, result
end

return M

