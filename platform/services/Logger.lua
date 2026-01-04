local LogLevel = require("common.LogLevel")

local M = {}
M.Logger = {}
M.Logger.__index = M.Logger

function M.Logger.new(logManager)
    local self = setmetatable({}, M.Logger)
    self.logManager = logManager or (ashita and ashita.log) or nil
    return self
end

function M.Logger:debug(message)
    self:log(LogLevel.LogLevel.Debug, message)
end

function M.Logger:info(message)
    self:log(LogLevel.LogLevel.Info, message)
end

function M.Logger:warning(message)
    self:log(LogLevel.LogLevel.Warning, message)
end

function M.Logger:error(message)
    self:log(LogLevel.LogLevel.Error, message)
end

function M.Logger:log(level, message)
    if not self.logManager or not self.logManager.Log then
        print(string.format("[%s] %s", level, message))
        return
    end
    
    local ashitaLevel = 1
    if level == LogLevel.LogLevel.Debug then
        ashitaLevel = 0
    elseif level == LogLevel.LogLevel.Warning then
        ashitaLevel = 2
    elseif level == LogLevel.LogLevel.Error then
        ashitaLevel = 3
    end
    
    -- Ashita's ILogManager.Log() automatically writes to log files
    -- Log files are typically in: <GameDirectory>/logs/
    -- Format: <GameDirectory>/logs/ashita_YYYYMMDD.log
    -- Module name "FFXIFriendList" will appear in the log entries
    self.logManager.Log(ashitaLevel, "FFXIFriendList", message)
end

-- Write message to echo chat (like the "Initialized" message)
function M.Logger:echo(message)
    -- Write to log first
    self:info(message)
    
    -- Also write to echo chat
    if AshitaCore and AshitaCore:GetChatManager then
        local chatManager = AshitaCore:GetChatManager()
        if chatManager and chatManager.Write then
            local fullMessage = "FriendList: " .. message
            pcall(function()
                chatManager:Write(200, false, fullMessage)
            end)
        end
    end
end

return M

