-- Decoders/ServerList.lua
-- Decode ServerListResponse payload (special case: may not have envelope)

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        servers = {}
    }
    
    if payload.servers and type(payload.servers) == "table" then
        result.servers = payload.servers
    end
    
    return true, result
end

return M

