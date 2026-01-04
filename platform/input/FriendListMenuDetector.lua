-- FriendListMenuDetector.lua

local M = {}
M.FriendListMenuDetector = {}
M.FriendListMenuDetector.__index = M.FriendListMenuDetector

function M.FriendListMenuDetector.new()
    local self = setmetatable({}, M.FriendListMenuDetector)
    self.initialized = false
    self.isMenuOpen = false
    return self
end

function M.FriendListMenuDetector:initialize(ashitaCore, logger, callback)
    self.ashitaCore = ashitaCore
    self.logger = logger
    self.callback = callback
    self.initialized = true
    return true
end

function M.FriendListMenuDetector:shutdown()
    self.initialized = false
end

function M.FriendListMenuDetector:isMenuOpen()
    return self.isMenuOpen
end

function M.FriendListMenuDetector:update()
end

return M

