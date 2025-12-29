
#include "JsonUtils.h"
#include "Core/ModelsCore.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <string>

namespace XIFriendList {
namespace Protocol {
namespace JsonUtils {

std::string escapeString(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                        << static_cast<int>(c) << std::dec;
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string encodeString(const std::string& value) {
    return "\"" + escapeString(value) + "\"";
}

std::string encodeNumber(int64_t value) {
    return std::to_string(value);
}

std::string encodeNumber(uint64_t value) {
    return std::to_string(value);
}

std::string encodeNumber(int value) {
    return std::to_string(value);
}

std::string encodeBoolean(bool value) {
    return value ? "true" : "false";
}

std::string encodeStringArray(const std::vector<std::string>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << encodeString(arr[i]);
    }
    oss << "]";
    return oss.str();
}

std::string encodeObject(const std::vector<std::pair<std::string, std::string>>& fields) {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) oss << ",";
        oss << encodeString(fields[i].first) << ":" << fields[i].second;
    }
    oss << "}";
    return oss.str();
}

bool decodeString(const std::string& json, std::string& out) {
    if (json.empty() || json[0] != '"') {
        return false;
    }
    
    out.clear();
    bool escaped = false;
    for (size_t i = 1; i < json.length(); ++i) {
        char c = json[i];
        if (escaped) {
            switch (c) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (i + 4 < json.length()) {
                        std::string hexStr = json.substr(i + 1, 4);
                        unsigned int codePoint = 0;
                        bool valid = true;
                        for (char hexChar : hexStr) {
                            codePoint <<= 4;
                            if (hexChar >= '0' && hexChar <= '9') {
                                codePoint += hexChar - '0';
                            } else if (hexChar >= 'A' && hexChar <= 'F') {
                                codePoint += hexChar - 'A' + 10;
                            } else if (hexChar >= 'a' && hexChar <= 'f') {
                                codePoint += hexChar - 'a' + 10;
                            } else {
                                valid = false;
                                break;
                            }
                        }
                        if (valid && codePoint <= 0x7F) {
                            out += static_cast<char>(codePoint);
                        } else if (valid) {
                            out += '?';
                        } else {
                            out += '?';
                        }
                        i += 4;
                    } else {
                        out += '?';
                    }
                    break;
                }
                default: out += c; break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return true;
        } else {
            out += c;
        }
    }
    return false;
}

bool decodeNumber(const std::string& json, int64_t& out) {
    try {
        out = std::stoll(json);
        return true;
    } catch (...) {
        return false;
    }
}

bool decodeNumber(const std::string& json, uint64_t& out) {
    try {
        out = std::stoull(json);
        return true;
    } catch (...) {
        return false;
    }
}

bool decodeNumber(const std::string& json, int& out) {
    try {
        out = std::stoi(json);
        return true;
    } catch (...) {
        return false;
    }
}

bool decodeBoolean(const std::string& json, bool& out) {
    if (json == "true") {
        out = true;
        return true;
    } else if (json == "false") {
        out = false;
        return true;
    }
    return false;
}

bool decodeStringArray(const std::string& json, std::vector<std::string>& out) {
    out.clear();
    if (json.empty() || json[0] != '[') {
        return false;
    }
    
    size_t pos = 1;
    while (pos < json.length() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    
    if (pos >= json.length()) {
        return false;
    }
    
    if (json[pos] == ']') {
        return true;  // Empty array
    }
    
    while (pos < json.length()) {
        if (json[pos] != '"') {
            return false;
        }
        
        std::string value;
        size_t start = pos;
        size_t end = start + 1;
        while (end < json.length() && json[end] != '"') {
            if (json[end] == '\\' && end + 1 < json.length()) {
                end += 2;  // Skip escaped character
            } else {
                ++end;
            }
        }
        if (end >= json.length()) {
            return false;
        }
        
        if (!decodeString(json.substr(start, end - start + 1), value)) {
            return false;
        }
        out.push_back(value);
        
        pos = end + 1;
        while (pos < json.length() && std::isspace(static_cast<unsigned char>(json[pos]))) {
            ++pos;
        }
        
        if (pos >= json.length()) {
            return false;
        }
        
        if (json[pos] == ']') {
            return true;
        } else if (json[pos] == ',') {
            ++pos;
            while (pos < json.length() && std::isspace(static_cast<unsigned char>(json[pos]))) {
                ++pos;
            }
        } else {
            return false;
        }
    }
    
    return false;
}

bool extractField(const std::string& json, const std::string& fieldName, std::string& out) {
    std::string searchKey = "\"" + fieldName + "\":";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return false;
    }
    
    size_t valueStart = keyPos + searchKey.length();
    while (valueStart < json.length() && std::isspace(static_cast<unsigned char>(json[valueStart]))) {
        ++valueStart;
    }
    
    if (valueStart >= json.length()) {
        return false;
    }
    
    size_t valueEnd = valueStart;
    if (json[valueStart] == '"') {
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && json[valueEnd] != '"') {
            if (json[valueEnd] == '\\' && valueEnd + 1 < json.length()) {
                valueEnd += 2;
            } else {
                ++valueEnd;
            }
        }
        if (valueEnd < json.length()) {
            ++valueEnd;
        }
    } else if (json[valueStart] == '[') {
        int depth = 1;
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && depth > 0) {
            if (json[valueEnd] == '[') ++depth;
            else if (json[valueEnd] == ']') --depth;
            else if (json[valueEnd] == '"') {
                ++valueEnd;
                while (valueEnd < json.length() && json[valueEnd] != '"') {
                    if (json[valueEnd] == '\\' && valueEnd + 1 < json.length()) {
                        valueEnd += 2;
                    } else {
                        ++valueEnd;
                    }
                }
            }
            ++valueEnd;
        }
    } else if (json[valueStart] == '{') {
        int depth = 1;
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && depth > 0) {
            if (json[valueEnd] == '{') ++depth;
            else if (json[valueEnd] == '}') --depth;
            else if (json[valueEnd] == '"') {
                ++valueEnd;
                while (valueEnd < json.length() && json[valueEnd] != '"') {
                    if (json[valueEnd] == '\\' && valueEnd + 1 < json.length()) {
                        valueEnd += 2;
                    } else {
                        ++valueEnd;
                    }
                }
            }
            ++valueEnd;
        }
    } else {
        valueEnd = valueStart;
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && 
               json[valueEnd] != '}' && 
               json[valueEnd] != ']' &&
               !std::isspace(static_cast<unsigned char>(json[valueEnd]))) {
            ++valueEnd;
        }
    }
    
    if (valueEnd > valueStart) {
        out = json.substr(valueStart, valueEnd - valueStart);
        return true;
    }
    
    return false;
}

bool extractStringField(const std::string& json, const std::string& fieldName, std::string& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeString(value, out);
}

bool extractNumberField(const std::string& json, const std::string& fieldName, int64_t& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeNumber(value, out);
}

bool extractNumberField(const std::string& json, const std::string& fieldName, uint64_t& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeNumber(value, out);
}

bool extractNumberField(const std::string& json, const std::string& fieldName, int& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeNumber(value, out);
}

bool extractNumberField(const std::string& json, const std::string& fieldName, float& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    try {
        out = std::stof(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool extractBooleanField(const std::string& json, const std::string& fieldName, bool& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeBoolean(value, out);
}

bool extractStringArrayField(const std::string& json, const std::string& fieldName, std::vector<std::string>& out) {
    std::string value;
    if (!extractField(json, fieldName, value)) {
        return false;
    }
    return decodeStringArray(value, out);
}

bool isValidJson(const std::string& json) {
    if (json.empty()) {
        return false;
    }
    
    if (json[0] != '{' && json[0] != '[') {
        return false;
    }
    
    int depth = 0;
    int arrayDepth = 0;
    bool inString = false;
    bool escaped = false;
    bool expectingKey = true;  // After {, expect a key or }
    bool expectingValue = false;  // After :, expect a value
    
    for (size_t i = 0; i < json.length(); ++i) {
        char c = json[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            continue;
        }
        
        if (c == '"') {
            inString = !inString;
            if (!inString) {
                if (expectingKey && depth > 0) {
                    expectingKey = false;
                    expectingValue = false;
                } else if (expectingValue) {
                    expectingValue = false;
                }
            }
            continue;
        }
        
        if (inString) {
            continue;
        }
        
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        
        if (c == '{') {
            ++depth;
            expectingKey = true;
            expectingValue = false;
        } else if (c == '}') {
            if (expectingValue) return false;
            --depth;
            if (depth < 0) return false;
            expectingKey = false;
            expectingValue = false;
        } else if (c == '[') {
            ++arrayDepth;
            expectingValue = false;
        } else if (c == ']') {
            --arrayDepth;
            if (arrayDepth < 0) return false;
            expectingValue = false;
        } else if (c == ':') {
            if (expectingKey) return false;  // Can't have : when expecting a key
            expectingValue = true;
            expectingKey = false;
        } else if (c == ',') {
            if (depth > 0) {
                expectingKey = true;  // After comma in object, expect next key
            }
            expectingValue = false;
        } else if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
            if (expectingKey && depth > 0) return false;
            expectingValue = false;
        } else if ((c == 't' || c == 'f' || c == 'n') && expectingValue) {
            expectingValue = false;
        } else {
            if (expectingKey && depth > 0) {
                return false;
            }
        }
    }
    
    return depth == 0 && arrayDepth == 0 && !inString;
}

std::string encodeFriendViewSettings(const XIFriendList::Core::FriendViewSettings& settings) {
    std::vector<std::pair<std::string, std::string>> fields;
    fields.push_back({"showJob", encodeBoolean(settings.showJob)});
    fields.push_back({"showZone", encodeBoolean(settings.showZone)});
    fields.push_back({"showNationRank", encodeBoolean(settings.showNationRank)});
    fields.push_back({"showLastSeen", encodeBoolean(settings.showLastSeen)});
    return encodeObject(fields);
}

bool extractFriendViewSettingsField(const std::string& json, const std::string& fieldName, XIFriendList::Core::FriendViewSettings& out) {
    std::string settingsJson;
    if (!extractField(json, fieldName, settingsJson)) {
        return false;
    }
    
    if (!extractBooleanField(settingsJson, "showJob", out.showJob)) {
        out.showJob = true;  // Default
    }
    if (!extractBooleanField(settingsJson, "showZone", out.showZone)) {
        out.showZone = false;  // Default
    }
    if (!extractBooleanField(settingsJson, "showNationRank", out.showNationRank)) {
        out.showNationRank = false;  // Default
    }
    if (!extractBooleanField(settingsJson, "showLastSeen", out.showLastSeen)) {
        out.showLastSeen = false;  // Default
    }
    
    return true;
}

} // namespace JsonUtils
} // namespace Protocol
} // namespace XIFriendList

