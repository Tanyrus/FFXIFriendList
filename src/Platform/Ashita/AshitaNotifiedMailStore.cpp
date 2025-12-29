#include "Platform/Ashita/AshitaNotifiedMailStore.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <sstream>
#include <map>
#include <windows.h>

#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaNotifiedMailStore::AshitaNotifiedMailStore() {
}

std::string AshitaNotifiedMailStore::getCacheJsonPath() const {
    char dllPath[MAX_PATH] = {0};
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\cache.json";
                }
            }
        }
    }
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultCachePath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\cache.json" : defaultPath;
}

std::string AshitaNotifiedMailStore::getConfigPath() const {
    char dllPath[MAX_PATH] = {0};
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\ffxifriendlist.ini";
                }
            }
        }
    }
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultIniPath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.ini" : defaultPath;
}

void AshitaNotifiedMailStore::ensureConfigDirectory(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

std::string AshitaNotifiedMailStore::normalizeCharacterName(const std::string& name) const {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}

void AshitaNotifiedMailStore::trimString(std::string& str) const {
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
}

std::set<std::string> AshitaNotifiedMailStore::splitCommaSeparated(const std::string& str) const {
    std::set<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        trimString(item);
        if (!item.empty()) {
            result.insert(item);
        }
    }
    
    return result;
}

std::string AshitaNotifiedMailStore::joinCommaSeparated(const std::set<std::string>& ids) const {
    if (ids.empty()) {
        return "";
    }
    
    std::stringstream ss;
    bool first = true;
    for (const auto& id : ids) {
        if (!first) {
            ss << ",";
        }
        ss << id;
        first = false;
    }
    return ss.str();
}

// Helper: Extract all key-value pairs from a JSON object (same as in AshitaApiKeyStore)
static void extractAllFieldsFromJsonObject(const std::string& jsonObj, std::map<std::string, std::string>& out) {
    if (jsonObj.empty() || jsonObj[0] != '{') {
        return;
    }
    
    size_t pos = 1;  // Skip opening '{'
    while (pos < jsonObj.length() && jsonObj[pos] != '}') {
        while (pos < jsonObj.length() && std::isspace(static_cast<unsigned char>(jsonObj[pos]))) {
            ++pos;
        }
        if (pos >= jsonObj.length() || jsonObj[pos] == '}') break;
        
        if (jsonObj[pos] == ',') {
            ++pos;
            while (pos < jsonObj.length() && std::isspace(static_cast<unsigned char>(jsonObj[pos]))) {
                ++pos;
            }
        }
        
        if (jsonObj[pos] != '"') break;
        size_t keyStart = pos + 1;
        size_t keyEnd = keyStart;
        while (keyEnd < jsonObj.length() && jsonObj[keyEnd] != '"') {
            if (jsonObj[keyEnd] == '\\' && keyEnd + 1 < jsonObj.length()) {
                keyEnd += 2;
            } else {
                ++keyEnd;
            }
        }
        if (keyEnd >= jsonObj.length()) break;
        std::string key = jsonObj.substr(keyStart, keyEnd - keyStart);
        
        pos = keyEnd + 1;
        while (pos < jsonObj.length() && jsonObj[pos] != ':') ++pos;
        if (pos >= jsonObj.length()) break;
        ++pos;
        while (pos < jsonObj.length() && std::isspace(static_cast<unsigned char>(jsonObj[pos]))) {
            ++pos;
        }
        
        if (pos >= jsonObj.length()) break;
        size_t valueStart = pos;
        size_t valueEnd = valueStart;
        if (jsonObj[valueStart] == '"') {
            // String value
            valueEnd = valueStart + 1;
            while (valueEnd < jsonObj.length() && jsonObj[valueEnd] != '"') {
                if (jsonObj[valueEnd] == '\\' && valueEnd + 1 < jsonObj.length()) {
                    valueEnd += 2;
                } else {
                    ++valueEnd;
                }
            }
            if (valueEnd < jsonObj.length()) {
                ++valueEnd;  // Include closing quote
            }
        } else if (jsonObj[valueStart] == '[') {
            // Array value
            int depth = 1;
            valueEnd = valueStart + 1;
            while (valueEnd < jsonObj.length() && depth > 0) {
                if (jsonObj[valueEnd] == '[') ++depth;
                else if (jsonObj[valueEnd] == ']') --depth;
                else if (jsonObj[valueEnd] == '"') {
                    ++valueEnd;
                    while (valueEnd < jsonObj.length() && jsonObj[valueEnd] != '"') {
                        if (jsonObj[valueEnd] == '\\' && valueEnd + 1 < jsonObj.length()) {
                            valueEnd += 2;
                        } else {
                            ++valueEnd;
                        }
                    }
                }
                ++valueEnd;
            }
        } else {
            // Not a string/array value, skip to next comma or }
            while (valueEnd < jsonObj.length() && 
                   jsonObj[valueEnd] != ',' && 
                   jsonObj[valueEnd] != '}' &&
                   !std::isspace(static_cast<unsigned char>(jsonObj[valueEnd]))) {
                ++valueEnd;
            }
        }
        
        std::string value = jsonObj.substr(valueStart, valueEnd - valueStart);
        out[key] = value;  // Store raw JSON value
        
        pos = valueEnd;
    }
}

// H1: Load notified mail IDs from JSON cache
std::set<std::string> AshitaNotifiedMailStore::loadNotifiedMailIdsFromJson(const std::string& characterName) const {
    if (characterName.empty()) {
        return std::set<std::string>();
    }
    
    try {
        std::string filePath = getCacheJsonPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return std::set<std::string>();
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        if (jsonContent.empty() || !::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            return std::set<std::string>();
        }
        
        std::string schema;
        if (!::XIFriendList::Protocol::JsonUtils::extractStringField(jsonContent, "schema", schema) || 
            schema != "XIFriendListCache/v1") {
            return std::set<std::string>();
        }
        
        std::string cacheJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "cache", cacheJson)) {
            return std::set<std::string>();
        }
        
        std::string notifiedMailJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
            return std::set<std::string>();
        }
        
        std::string normalizedChar = normalizeCharacterName(characterName);
        std::string charArrayJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(notifiedMailJson, normalizedChar, charArrayJson)) {
            return std::set<std::string>();
        }
        
        std::vector<std::string> ids;
        if (::XIFriendList::Protocol::JsonUtils::decodeStringArray(charArrayJson, ids)) {
            return std::set<std::string>(ids.begin(), ids.end());
        }
        
        return std::set<std::string>();
    } catch (...) {
        return std::set<std::string>();
    }
}

// H1: Save notified mail IDs to JSON cache
bool AshitaNotifiedMailStore::saveNotifiedMailIdsToJson(const std::string& characterName, const std::set<std::string>& messageIds) {
    if (characterName.empty()) {
        return false;
    }
    
    try {
        std::string filePath = getCacheJsonPath();
        ensureConfigDirectory(filePath);
        
        std::string existingJson;
        std::map<std::string, std::string> apiKeys;  // Preserve API keys
        std::map<std::string, std::string> notifiedMail;  // All character mail IDs
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                std::string cacheJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "cache", cacheJson)) {
                    std::string apiKeysJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                        extractAllFieldsFromJsonObject(apiKeysJson, apiKeys);
                    }
                    std::string notifiedMailJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                        extractAllFieldsFromJsonObject(notifiedMailJson, notifiedMail);
                    }
                }
            }
        }
        
        std::string normalizedChar = normalizeCharacterName(characterName);
        std::vector<std::string> idsVec(messageIds.begin(), messageIds.end());
        std::string idsArrayJson = ::XIFriendList::Protocol::JsonUtils::encodeStringArray(idsVec);
        notifiedMail[normalizedChar] = idsArrayJson;
        
        std::vector<std::pair<std::string, std::string>> notifiedMailFields;
        for (const auto& kv : notifiedMail) {
            notifiedMailFields.push_back({kv.first, kv.second});  // Value is already JSON array
        }
        std::string notifiedMailJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(notifiedMailFields);
        
        std::vector<std::pair<std::string, std::string>> apiKeyFields;
        for (const auto& kv : apiKeys) {
            apiKeyFields.push_back({kv.first, ::XIFriendList::Protocol::JsonUtils::encodeString(kv.second)});
        }
        std::string apiKeysJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(apiKeyFields);
        
        std::vector<std::pair<std::string, std::string>> cacheFields;
        cacheFields.push_back({"apiKeys", apiKeysJson});
        cacheFields.push_back({"notifiedMail", notifiedMailJson});
        std::string cacheJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(cacheFields);
        
        std::vector<std::pair<std::string, std::string>> rootFields;
        rootFields.push_back({"schema", ::XIFriendList::Protocol::JsonUtils::encodeString("XIFriendListCache/v1")});
        rootFields.push_back({"version", ::XIFriendList::Protocol::JsonUtils::encodeNumber(1)});
        rootFields.push_back({"cache", cacheJson});
        
        std::string jsonContent = ::XIFriendList::Protocol::JsonUtils::encodeObject(rootFields);
        
        // Write to file
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            return false;
        }
        
        outFile << jsonContent;
        outFile.close();
        return true;
    } catch (...) {
        return false;
    }
}

std::set<std::string> AshitaNotifiedMailStore::loadNotifiedMailIds(const std::string& characterName) {
    if (characterName.empty()) {
        return std::set<std::string>();
    }
    
    // H1: Load from JSON only
    return loadNotifiedMailIdsFromJson(characterName);
}

bool AshitaNotifiedMailStore::saveNotifiedMailId(const std::string& characterName, const std::string& messageId) {
    if (characterName.empty() || messageId.empty()) {
        return false;
    }
    
    std::set<std::string> existingIds = loadNotifiedMailIds(characterName);
    existingIds.insert(messageId);
    return saveNotifiedMailIds(characterName, existingIds);
}

bool AshitaNotifiedMailStore::saveNotifiedMailIds(const std::string& characterName, const std::set<std::string>& messageIds) {
    if (characterName.empty()) {
        return false;
    }
    
    // H1: Save to JSON (deprecate INI writes)
    return saveNotifiedMailIdsToJson(characterName, messageIds);
    
    // H2: Deprecated INI write code (kept for reference, not executed)
    /*
    try {
        std::string filePath = getConfigPath();
        ensureConfigDirectory(filePath);
        
        std::ifstream inFile(filePath);
        std::vector<std::string> lines;
        std::string line;
        bool isInSettingsSection = false;
        bool keyFound = false;
        
        std::string normalizedChar = normalizeCharacterName(characterName);
        std::string keyToSave = "NotifiedMail_" + normalizedChar;
        std::string keyToSaveLower = keyToSave;
        std::transform(keyToSaveLower.begin(), keyToSaveLower.end(), keyToSaveLower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        std::string valueToSave = joinCommaSeparated(messageIds);
        
        // Read existing file
        while (std::getline(inFile, line)) {
            if (line[0] == '[' && line.back() == ']') {
                std::string section = line.substr(1, line.length() - 2);
                isInSettingsSection = (section == "Settings");
                lines.push_back(line);
                continue;
            }
            
            if (isInSettingsSection) {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = line.substr(0, eqPos);
                    trimString(key);
                    std::string keyLower = key;
                    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    
                    if (keyLower == keyToSaveLower) {
                        // Replace existing key
                        lines.push_back(keyToSave + "=" + valueToSave);
                        keyFound = true;
                        continue;
                    }
                }
            }
            
            lines.push_back(line);
        }
        
        inFile.close();
        
        if (!keyFound) {
            // Ensure Settings section exists
            bool hasSettingsSection = false;
            for (const auto& l : lines) {
                if (l == "[Settings]") {
                    hasSettingsSection = true;
                    break;
                }
            }
            
            if (!hasSettingsSection) {
                lines.push_back("[Settings]");
            }
            
            lines.push_back(keyToSave + "=" + valueToSave);
        }
        
        // Write file
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            return false;
        }
        
        for (const auto& l : lines) {
            outFile << l << std::endl;
        }
        
        outFile.close();
        return true;
    }
    catch (...) {
        return false;
    }
    */
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

