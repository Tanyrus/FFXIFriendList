
#include "MessageValidator.h"
#include "ProtocolVersion.h"
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace Protocol {

ValidationResult MessageValidator::validateRequest(const RequestMessage& request) {
    ValidationResult versionResult = validateVersion(request.protocolVersion);
    if (versionResult != ValidationResult::Valid) {
        return versionResult;
    }
    
    std::string typeStr = requestTypeToString(request.type);
    if (typeStr == "Unknown") {
        return ValidationResult::InvalidType;
    }
    
    if (request.payload.size() > MAX_JSON_SIZE) {
        return ValidationResult::PayloadTooLarge;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::validateResponse(const ResponseMessage& response) {
    ValidationResult versionResult = validateVersion(response.protocolVersion);
    if (versionResult != ValidationResult::Valid) {
        return versionResult;
    }
    
    std::string typeStr = responseTypeToString(response.type);
    if (typeStr == "Unknown") {
        return ValidationResult::InvalidType;
    }
    
    if (response.payload.size() > MAX_JSON_SIZE) {
        return ValidationResult::PayloadTooLarge;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::validateVersion(const std::string& version) {
    if (version.empty()) {
        return ValidationResult::MissingRequiredField;
    }
    
    Version v;
    if (!Version::parse(version, v)) {
        return ValidationResult::InvalidVersion;
    }
    
    Version current = getCurrentVersion();
    if (!v.isCompatibleWith(current)) {
        return ValidationResult::InvalidVersion;
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::validateCharacterName(const std::string& name) {
    if (name.empty()) {
        return ValidationResult::MissingRequiredField;
    }
    
    if (name.length() > MAX_CHARACTER_NAME_LENGTH) {
        return ValidationResult::InvalidFieldValue;
    }
    
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != ' ' && c != '-' && c != '_') {
            return ValidationResult::InvalidFieldValue;
        }
    }
    
    return ValidationResult::Valid;
}

ValidationResult MessageValidator::validateFriendListSize(size_t count) {
    if (count > MAX_FRIEND_LIST_SIZE) {
        return ValidationResult::InvalidFieldValue;
    }
    return ValidationResult::Valid;
}

std::string MessageValidator::getErrorMessage(ValidationResult result) {
    switch (result) {
        case ValidationResult::Valid:
            return "Valid";
        case ValidationResult::InvalidVersion:
            return "Invalid protocol version";
        case ValidationResult::InvalidType:
            return "Invalid message type";
        case ValidationResult::MissingRequiredField:
            return "Missing required field";
        case ValidationResult::InvalidFieldValue:
            return "Invalid field value";
        case ValidationResult::InvalidJson:
            return "Invalid JSON format";
        case ValidationResult::PayloadTooLarge:
            return "Payload too large";
        default:
            return "Unknown validation error";
    }
}

} // namespace Protocol
} // namespace XIFriendList

