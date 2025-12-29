
#ifndef PROTOCOL_MESSAGE_VALIDATOR_H
#define PROTOCOL_MESSAGE_VALIDATOR_H

#include "MessageTypes.h"
#include "ProtocolVersion.h"
#include <string>

namespace XIFriendList {
namespace Protocol {

enum class ValidationResult {
    Valid,
    InvalidVersion,
    InvalidType,
    MissingRequiredField,
    InvalidFieldValue,
    InvalidJson,
    PayloadTooLarge
};

class MessageValidator {
public:
    static ValidationResult validateRequest(const RequestMessage& request);
    static ValidationResult validateResponse(const ResponseMessage& response);
    static ValidationResult validateVersion(const std::string& version);
    static ValidationResult validateCharacterName(const std::string& name);
    static ValidationResult validateFriendListSize(size_t count);
    static std::string getErrorMessage(ValidationResult result);

private:
    static constexpr size_t MAX_FRIEND_LIST_SIZE = 1000;
    static constexpr size_t MAX_CHARACTER_NAME_LENGTH = 16;
    static constexpr size_t MAX_JSON_SIZE = 1024 * 1024;
};

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_MESSAGE_VALIDATOR_H

