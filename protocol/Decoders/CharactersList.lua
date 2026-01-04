-- Decoders/CharactersList.lua
-- Decode CharactersListResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        characters = {},
        activeCharacterId = nil,
        mainCharacterId = nil
    }
    
    if payload.characters and type(payload.characters) == "table" then
        result.characters = payload.characters
    end
    
    if payload.activeCharacterId ~= nil then
        result.activeCharacterId = tonumber(payload.activeCharacterId)
    end
    
    if payload.mainCharacterId ~= nil then
        result.mainCharacterId = tonumber(payload.mainCharacterId)
    end
    
    return true, result
end

return M

