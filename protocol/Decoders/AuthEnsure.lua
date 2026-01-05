-- Decoders/AuthEnsure.lua
-- Decode AuthEnsureResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        characterName = "",
        apiKey = "",
        accountId = 0,
        characterId = 0,
        activeCharacterId = 0,
        realmId = "",
        isNewAccount = false,
        isNewCharacter = false,
        wasRecovered = false,
        wasMerged = false,
        preferences = {},
        privacy = {}
    }
    
    if payload.characterName then
        result.characterName = payload.characterName
    end
    
    -- SetActiveCharacterResponse uses activeCharacterName instead of characterName
    if payload.activeCharacterName then
        result.characterName = payload.activeCharacterName
    end
    
    if payload.apiKey then
        result.apiKey = payload.apiKey
    end
    
    if payload.accountId ~= nil then
        result.accountId = tonumber(payload.accountId) or 0
    end
    
    if payload.characterId ~= nil then
        result.characterId = tonumber(payload.characterId) or 0
    end
    
    if payload.activeCharacterId ~= nil then
        result.activeCharacterId = tonumber(payload.activeCharacterId) or 0
    end
    
    if payload.realmId then
        result.realmId = payload.realmId
    end
    
    if payload.isNewAccount ~= nil then
        result.isNewAccount = payload.isNewAccount == true
    end
    
    if payload.isNewCharacter ~= nil then
        result.isNewCharacter = payload.isNewCharacter == true
    end
    
    if payload.wasRecovered ~= nil then
        result.wasRecovered = payload.wasRecovered == true
    end
    
    -- SetActiveCharacterResponse uses wasCreated/wasMerged instead of isNewAccount/isNewCharacter
    if payload.wasCreated ~= nil then
        result.isNewCharacter = payload.wasCreated == true
    end
    
    if payload.wasMerged ~= nil then
        result.wasMerged = payload.wasMerged == true
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

