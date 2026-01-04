local M = {}

M.Notes = {}
M.Notes.__index = M.Notes

function M.Notes.new(deps)
    local self = setmetatable({}, M.Notes)
    self.deps = deps or {}
    
    self.notes = {}
    self.dirty = false
    
    return self
end

function M.Notes:getState()
    return {
        noteCount = 0,
        dirty = self.dirty
    }
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

function M.Notes:load()
    if not self.deps.storage then
        return false
    end
    
    local data = self.deps.storage.load("notes")
    if data and type(data) == "table" then
        self.notes = data
        self.dirty = false
        return true
    end
    return false
end

function M.Notes:save()
    if not self.deps.storage then
        return false
    end
    
    if self.dirty then
        self.deps.storage.save("notes", self.notes)
        self.dirty = false
        return true
    end
    return false
end

function M.Notes:getNote(friendKey)
    return self.notes[friendKey] or ""
end

function M.Notes:setNote(friendKey, note)
    if note and note ~= "" then
        self.notes[friendKey] = note
    else
        self.notes[friendKey] = nil
    end
    self.dirty = true
end

function M.Notes:deleteNote(friendKey)
    self.notes[friendKey] = nil
    self.dirty = true
end

function M.Notes:hasNote(friendKey)
    return self.notes[friendKey] ~= nil and self.notes[friendKey] ~= ""
end

return M
