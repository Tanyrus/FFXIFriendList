-- Decoders/NotesList.lua
-- Decode NotesListResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        notes = {}
    }
    
    if payload.notes and type(payload.notes) == "table" then
        result.notes = payload.notes
    end
    
    return true, result
end

return M

