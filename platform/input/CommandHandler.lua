-- CommandHandler.lua

local M = {}
M.CommandHandler = {}
M.CommandHandler.__index = M.CommandHandler

function M.CommandHandler.new()
    local self = setmetatable({}, M.CommandHandler)
    return self
end

function M.CommandHandler:parseCommand(command)
    if not command or command == "" then
        return nil, nil
    end
    
    local cmd = command:match("^/(.+)")
    if not cmd then
        return nil, nil
    end
    
    local parts = {}
    for part in cmd:gmatch("%S+") do
        table.insert(parts, part)
    end
    
    if #parts == 0 then
        return nil, nil
    end
    
    local action = parts[1]:lower()
    local args = {}
    for i = 2, #parts do
        table.insert(args, parts[i])
    end
    
    return action, args
end

function M.CommandHandler:handleCommand(action, args, handler)
    if handler then
        return handler(action, args)
    end
    return false
end

return M

