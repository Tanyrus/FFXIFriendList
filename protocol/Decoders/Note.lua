-- Decoders/Note.lua
-- Decode NoteResponse / NoteUpdateResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        friendAccountId = nil,
        friendName = "",
        noteText = nil,
        updatedAt = nil,
        deleted = false
    }
    
    if payload.friendAccountId ~= nil then
        result.friendAccountId = tonumber(payload.friendAccountId)
    end
    
    if payload.friendName then
        result.friendName = payload.friendName
    end
    
    if payload.noteText ~= nil then
        result.noteText = payload.noteText
    end
    
    if payload.updatedAt ~= nil then
        result.updatedAt = tonumber(payload.updatedAt)
    end
    
    if payload.deleted ~= nil then
        result.deleted = payload.deleted == true
    end
    
    return true, result
end

return M

