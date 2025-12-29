
#ifndef PROTOCOL_JSON_UTILS_H
#define PROTOCOL_JSON_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

// Forward declaration
namespace XIFriendList {
namespace Core {
    struct FriendViewSettings;
}
}

namespace XIFriendList {
namespace Protocol {

namespace JsonUtils {
std::string escapeString(const std::string& str);
std::string encodeString(const std::string& value);
std::string encodeNumber(int64_t value);
std::string encodeNumber(uint64_t value);
std::string encodeNumber(int value);
std::string encodeBoolean(bool value);
std::string encodeStringArray(const std::vector<std::string>& arr);
std::string encodeObject(const std::vector<std::pair<std::string, std::string>>& fields);
bool decodeString(const std::string& json, std::string& out);
bool decodeNumber(const std::string& json, int64_t& out);
bool decodeNumber(const std::string& json, uint64_t& out);
bool decodeNumber(const std::string& json, int& out);
bool decodeBoolean(const std::string& json, bool& out);
bool decodeStringArray(const std::string& json, std::vector<std::string>& out);
bool extractField(const std::string& json, const std::string& fieldName, std::string& out);
bool extractStringField(const std::string& json, const std::string& fieldName, std::string& out);
bool extractNumberField(const std::string& json, const std::string& fieldName, int64_t& out);
bool extractNumberField(const std::string& json, const std::string& fieldName, uint64_t& out);
bool extractNumberField(const std::string& json, const std::string& fieldName, int& out);
bool extractNumberField(const std::string& json, const std::string& fieldName, float& out);
bool extractBooleanField(const std::string& json, const std::string& fieldName, bool& out);
bool extractStringArrayField(const std::string& json, const std::string& fieldName, std::vector<std::string>& out);
bool isValidJson(const std::string& json);

// Helper to serialize FriendViewSettings to JSON object string
std::string encodeFriendViewSettings(const XIFriendList::Core::FriendViewSettings& settings);

// Helper to deserialize FriendViewSettings from JSON object string
bool extractFriendViewSettingsField(const std::string& json, const std::string& fieldName, XIFriendList::Core::FriendViewSettings& out);

} // namespace JsonUtils

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_JSON_UTILS_H

