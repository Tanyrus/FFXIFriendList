-- MessageValidator.lua
-- Message-level validation

local ProtocolVersion = require("protocol.ProtocolVersion")
local MessageTypes = require("protocol.MessageTypes")
local FieldChecks = require("protocol.Validation.FieldChecks")

local M = {}

-- ValidationResult enum
M.ValidationResult = {
    Valid = "Valid",
    InvalidVersion = "InvalidVersion",
    InvalidType = "InvalidType",
    MissingRequiredField = "MissingRequiredField",
    InvalidFieldValue = "InvalidFieldValue",
    InvalidJson = "InvalidJson",
    PayloadTooLarge = "PayloadTooLarge"
}

-- Constants
M.MAX_JSON_SIZE = 1024 * 1024  -- 1MB

-- Validate request message
function M.validateRequest(request)
    local versionResult = M.validateVersion(request.protocolVersion)
    if versionResult ~= M.ValidationResult.Valid then
        return versionResult
    end
    
    local typeStr = MessageTypes.requestTypeToString(request.type)
    if typeStr == "Unknown" then
        return M.ValidationResult.InvalidType
    end
    
    if request.payload and #request.payload > M.MAX_JSON_SIZE then
        return M.ValidationResult.PayloadTooLarge
    end
    
    return M.ValidationResult.Valid
end

-- Validate response message
function M.validateResponse(response)
    local versionResult = M.validateVersion(response.protocolVersion)
    if versionResult ~= M.ValidationResult.Valid then
        return versionResult
    end
    
    local typeStr = MessageTypes.responseTypeToString(response.type)
    if typeStr == "Unknown" then
        return M.ValidationResult.InvalidType
    end
    
    if response.payload and #response.payload > M.MAX_JSON_SIZE then
        return M.ValidationResult.PayloadTooLarge
    end
    
    return M.ValidationResult.Valid
end

-- Validate version string
function M.validateVersion(versionStr)
    if not versionStr or versionStr == "" then
        return M.ValidationResult.MissingRequiredField
    end
    
    local version = ProtocolVersion.Version.parse(versionStr)
    if not version then
        return M.ValidationResult.InvalidVersion
    end
    
    local current = ProtocolVersion.getCurrentVersion()
    if not version:isCompatibleWith(current) then
        return M.ValidationResult.InvalidVersion
    end
    
    return M.ValidationResult.Valid
end

-- Validate character name (delegates to FieldChecks)
function M.validateCharacterName(name)
    local valid, error = FieldChecks.validateCharacterName(name)
    if not valid then
        return M.ValidationResult.InvalidFieldValue
    end
    return M.ValidationResult.Valid
end

-- Validate friend list size (delegates to FieldChecks)
function M.validateFriendListSize(count)
    local valid, error = FieldChecks.validateFriendListSize(count)
    if not valid then
        return M.ValidationResult.InvalidFieldValue
    end
    return M.ValidationResult.Valid
end

-- Get error message for validation result
function M.getErrorMessage(result)
    if result == M.ValidationResult.Valid then
        return "Valid"
    elseif result == M.ValidationResult.InvalidVersion then
        return "Invalid protocol version"
    elseif result == M.ValidationResult.InvalidType then
        return "Invalid message type"
    elseif result == M.ValidationResult.MissingRequiredField then
        return "Missing required field"
    elseif result == M.ValidationResult.InvalidFieldValue then
        return "Invalid field value"
    elseif result == M.ValidationResult.InvalidJson then
        return "Invalid JSON format"
    elseif result == M.ValidationResult.PayloadTooLarge then
        return "Payload too large"
    else
        return "Unknown validation error"
    end
end

return M

