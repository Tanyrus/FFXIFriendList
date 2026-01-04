-- EventQueue.lua

local M = {}
M.EventQueue = {}
M.EventQueue.__index = M.EventQueue

function M.EventQueue.new()
    local self = setmetatable({}, M.EventQueue)
    self.characterChangedQueue = {}
    self.zoneChangedQueue = {}
    self.characterChangedHandler = nil
    self.zoneChangedHandler = nil
    return self
end

function M.EventQueue:pushCharacterChanged(event)
    table.insert(self.characterChangedQueue, event)
end

function M.EventQueue:pushZoneChanged(event)
    table.insert(self.zoneChangedQueue, event)
end

function M.EventQueue:processEvents()
    local processed = 0
    
    while #self.characterChangedQueue > 0 do
        local event = table.remove(self.characterChangedQueue, 1)
        if self.characterChangedHandler then
            self.characterChangedHandler(event)
            processed = processed + 1
        end
    end
    
    while #self.zoneChangedQueue > 0 do
        local event = table.remove(self.zoneChangedQueue, 1)
        if self.zoneChangedHandler then
            self.zoneChangedHandler(event)
            processed = processed + 1
        end
    end
    
    return processed
end

function M.EventQueue:empty()
    return #self.characterChangedQueue == 0 and #self.zoneChangedQueue == 0
end

function M.EventQueue:setCharacterChangedHandler(handler)
    self.characterChangedHandler = handler
end

function M.EventQueue:setZoneChangedHandler(handler)
    self.zoneChangedHandler = handler
end

return M

