-- KeyEdgeDetector.lua

local M = {}
M.KeyEdgeDetector = {}
M.KeyEdgeDetector.__index = M.KeyEdgeDetector

function M.KeyEdgeDetector.new()
    local self = setmetatable({}, M.KeyEdgeDetector)
    self.lastState = false
    return self
end

function M.KeyEdgeDetector:update(currentState)
    local rising = false
    local falling = false
    
    if currentState and not self.lastState then
        rising = true
    elseif not currentState and self.lastState then
        falling = true
    end
    
    self.lastState = currentState
    return rising, falling
end

function M.KeyEdgeDetector:isPressed()
    return self.lastState
end

return M

