-- Envelope.lua
-- Envelope decoding and validation (compatible with server's actual JSON format)
-- Server returns: { protocolVersion, type, success, ...data fields... }
-- We normalize to: { protocolVersion, type, success, payload: {...data fields...} }

local Json = require("protocol.Json")
local ProtocolVersion = require("protocol.ProtocolVersion")

local M = {}

M.DecodeError = {
    InvalidJson = "InvalidJson",
    MissingProtocolVersion = "MissingProtocolVersion",
    MissingType = "MissingType",
    MissingSuccess = "MissingSuccess",
    InvalidVersion = "InvalidVersion"
}

-- Envelope fields (not part of payload)
local ENVELOPE_FIELDS = {
    protocolVersion = true,
    type = true,
    success = true,
    error = true,
    errorCode = true,
    details = true,
    requestId = true,
    payload = true  -- If server includes payload, use it
}

function M.decode(jsonString)
    if not jsonString or jsonString == "" then
        return false, M.DecodeError.InvalidJson, "Empty JSON string"
    end
    
    local ok, data = Json.decode(jsonString)
    if not ok then
        -- Log decode failure
        local errorMsg = data
        if type(errorMsg) == "table" then
            errorMsg = "Invalid JSON"
        else
            errorMsg = tostring(errorMsg or "Invalid JSON")
        end
        local logger = _G.FFXIFriendListApp and _G.FFXIFriendListApp.deps and _G.FFXIFriendListApp.deps.logger
        if logger and logger.warn then
            logger.warn("[Envelope] JSON decode failed: " .. errorMsg)
        end
        return false, M.DecodeError.InvalidJson, errorMsg
    end
    
    if type(data) ~= "table" then
        return false, M.DecodeError.InvalidJson, "JSON root is not an object"
    end
    
    if not data.protocolVersion or type(data.protocolVersion) ~= "string" then
        return false, M.DecodeError.MissingProtocolVersion, "protocolVersion is required"
    end
    
    local version = ProtocolVersion.Version.parse(data.protocolVersion)
    if not version then
        return false, M.DecodeError.InvalidVersion, "Invalid protocol version format"
    end
    
    local currentVersion = ProtocolVersion.getCurrentVersion()
    if not version:isCompatibleWith(currentVersion) then
        return false, M.DecodeError.InvalidVersion, "Incompatible protocol version"
    end
    
    if not data.type or type(data.type) ~= "string" then
        return false, M.DecodeError.MissingType, "type is required"
    end
    
    if data.success == nil or type(data.success) ~= "boolean" then
        return false, M.DecodeError.MissingSuccess, "success is required"
    end
    
    -- Extract payload: if server includes payload field, use it; otherwise extract all non-envelope fields
    local payload = {}
    if data.payload and type(data.payload) == "table" then
        -- Server included payload field (future compatibility)
        payload = data.payload
    else
        -- Server returns data directly in response (current format)
        -- Extract all fields except envelope fields into payload
        for key, value in pairs(data) do
            if not ENVELOPE_FIELDS[key] then
                payload[key] = value
            end
        end
    end
    
    local envelope = {
        protocolVersion = data.protocolVersion,
        type = data.type,
        success = data.success,
        payload = payload,
        error = data.error or "",
        errorCode = data.errorCode or "",
        details = data.details or "",
        requestId = data.requestId or ""
    }
    
    return true, envelope
end

return M

