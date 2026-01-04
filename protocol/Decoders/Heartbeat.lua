-- Decoders/Heartbeat.lua
-- Decode HeartbeatResponse payload

local M = {}

function M.decode(payload)
    if not payload or type(payload) ~= "table" then
        return false, "Invalid payload: expected table"
    end
    
    local result = {
        serverTime = 0,
        nextHeartbeatMs = 30000,
        onlineThresholdMs = 60000,
        is_outdated = false,
        latest_version = nil,
        release_url = nil,
        statuses = {},
        lastEventTimestamp = 0,
        lastRequestEventTimestamp = 0
    }
    
    if payload.serverTime then
        result.serverTime = tonumber(payload.serverTime) or 0
    end
    
    if payload.nextHeartbeatMs then
        result.nextHeartbeatMs = tonumber(payload.nextHeartbeatMs) or 30000
    end
    
    if payload.onlineThresholdMs then
        result.onlineThresholdMs = tonumber(payload.onlineThresholdMs) or 60000
    end
    
    if payload.is_outdated ~= nil then
        result.is_outdated = payload.is_outdated == true
    end
    
    if payload.latest_version then
        result.latest_version = payload.latest_version
    end
    
    if payload.release_url then
        result.release_url = payload.release_url
    end
    
    -- Friend statuses array (online/offline status updates)
    if payload.statuses and type(payload.statuses) == "table" then
        result.statuses = payload.statuses
    end
    
    -- Timestamps for tracking events (used in next heartbeat request)
    if payload.lastEventTimestamp then
        result.lastEventTimestamp = tonumber(payload.lastEventTimestamp) or 0
    end
    
    if payload.lastRequestEventTimestamp then
        result.lastRequestEventTimestamp = tonumber(payload.lastRequestEventTimestamp) or 0
    end
    
    return true, result
end

return M

