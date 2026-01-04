-- Decoders/FriendRequests.lua
-- Decode FriendRequestsResponse payload
-- Normalizes server fields to UI-friendly format

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        incoming = {},
        outgoing = {}
    }
    
    -- Normalize incoming requests
    -- Server: { requestId, fromCharacterName, fromAccountId, status, createdAt }
    -- UI expects: { id, name, ... }
    if payload.incoming and type(payload.incoming) == "table" then
        for _, req in ipairs(payload.incoming) do
            table.insert(result.incoming, {
                id = req.requestId,
                name = req.fromCharacterName,
                accountId = req.fromAccountId,
                status = req.status,
                createdAt = req.createdAt
            })
        end
    end
    
    -- Normalize outgoing requests
    -- Server: { requestId, toCharacterName, toAccountId, status, createdAt }
    -- UI expects: { id, name, ... }
    if payload.outgoing and type(payload.outgoing) == "table" then
        for _, req in ipairs(payload.outgoing) do
            table.insert(result.outgoing, {
                id = req.requestId,
                name = req.toCharacterName,
                accountId = req.toAccountId,
                status = req.status,
                createdAt = req.createdAt
            })
        end
    end
    
    return true, result
end

return M

