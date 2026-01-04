-- SessionManager.lua

local M = {}
M.SessionManager = {}
M.SessionManager.__index = M.SessionManager

function M.SessionManager.new()
    local self = setmetatable({}, M.SessionManager)
    self.sessionId = ""
    self.accountId = 0
    self.characterId = 0
    self.activeCharacterId = 0
    return self
end

function M.SessionManager:getSessionId()
    return self.sessionId
end

function M.SessionManager:setSessionId(sessionId)
    self.sessionId = sessionId
end

function M.SessionManager:getAccountId()
    return self.accountId
end

function M.SessionManager:setAccountId(accountId)
    self.accountId = accountId
end

function M.SessionManager:getCharacterId()
    return self.characterId
end

function M.SessionManager:setCharacterId(characterId)
    self.characterId = characterId
end

function M.SessionManager:getActiveCharacterId()
    return self.activeCharacterId
end

function M.SessionManager:setActiveCharacterId(activeCharacterId)
    self.activeCharacterId = activeCharacterId
end

function M.SessionManager:clearSession()
    self.sessionId = ""
    self.accountId = 0
    self.characterId = 0
    self.activeCharacterId = 0
end

function M.SessionManager:isAuthenticated()
    return self.sessionId ~= "" and self.accountId > 0
end

return M

