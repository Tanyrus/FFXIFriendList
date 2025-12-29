#include "Platform/Ashita/ApiKeyPersistence.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <map>
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::mutex ApiKeyPersistence::ioMutex_;

std::string ApiKeyPersistence::getMainJsonPath() {
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
                    return gameDir + "config\\FFXIFriendList\\ffxifriendlist.json";
                }
            }
        }
    }
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultMainJsonPath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.json" : defaultPath;
}

void ApiKeyPersistence::ensureConfigDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

std::string ApiKeyPersistence::normalizeCharacterName(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}

void ApiKeyPersistence::trimString(std::string& str) {
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

static void extractAllFieldsFromJsonObject(const std::string& jsonObj, std::map<std::string, std::string>& out) {
    if (jsonObj.empty() || jsonObj[0] != '{') {
        return;
    }
    
    size_t pos = 1;
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
        } else {
            while (valueEnd < jsonObj.length() && 
                   jsonObj[valueEnd] != ',' && 
                   jsonObj[valueEnd] != '}' &&
                   !std::isspace(static_cast<unsigned char>(jsonObj[valueEnd]))) {
                ++valueEnd;
            }
        }
        
        std::string value = jsonObj.substr(valueStart, valueEnd - valueStart);
        std::string decodedValue;
        if (::XIFriendList::Protocol::JsonUtils::decodeString(value, decodedValue)) {
            out[key] = decodedValue;
        }
        
        pos = valueEnd;
    }
}

std::string ApiKeyPersistence::loadApiKeyFromJson(const std::string& characterName) {
    try {
        std::string filePath = getMainJsonPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        if (jsonContent.empty() || !::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            return "";
        }
        
        std::string dataJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "data", dataJson)) {
            return "";
        }
        
        std::string apiKeysJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "apiKeys", apiKeysJson)) {
            return "";
        }
        
        std::string normalizedChar = normalizeCharacterName(characterName);
        std::string apiKey;
        if (::XIFriendList::Protocol::JsonUtils::extractStringField(apiKeysJson, normalizedChar, apiKey)) {
            return apiKey;
        }
        
        return "";
    } catch (...) {
        return "";
    }
}


bool ApiKeyPersistence::saveApiKeyToJson(const std::string& characterName, const std::string& apiKey) {
    if (characterName.empty()) {
        return false;
    }
    
    try {
        std::string filePath = getMainJsonPath();
        ensureConfigDirectory(filePath);
        
        std::string existingJson;
        std::map<std::string, std::string> apiKeys;
        std::string notifiedMailJson;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                std::string dataJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                    std::string apiKeysJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "apiKeys", apiKeysJson)) {
                        extractAllFieldsFromJsonObject(apiKeysJson, apiKeys);
                    }
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "notifiedMail", notifiedMailJson);
                }
            }
        }
        
        std::string normalizedChar = normalizeCharacterName(characterName);
        apiKeys[normalizedChar] = apiKey;
        
        std::vector<std::pair<std::string, std::string>> apiKeyFields;
        for (const auto& kv : apiKeys) {
            apiKeyFields.push_back({kv.first, ::XIFriendList::Protocol::JsonUtils::encodeString(kv.second)});
        }
        std::string apiKeysJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(apiKeyFields);
        
        std::string windowLocksJson;
        std::string collapsibleSectionsJson;
        std::string serverSelectionJson;
        std::string settingsJson;
        
        if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
            std::string dataJson;
            if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "windowLocks", windowLocksJson);
                ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "collapsibleSections", collapsibleSectionsJson);
                ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "serverSelection", serverSelectionJson);
                ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "settings", settingsJson);
            }
        }
        
        std::vector<std::pair<std::string, std::string>> dataFields;
        dataFields.push_back({"apiKeys", apiKeysJson});
        if (!notifiedMailJson.empty()) {
            dataFields.push_back({"notifiedMail", notifiedMailJson});
        }
        if (!windowLocksJson.empty()) {
            dataFields.push_back({"windowLocks", windowLocksJson});
        }
        if (!collapsibleSectionsJson.empty()) {
            dataFields.push_back({"collapsibleSections", collapsibleSectionsJson});
        }
        if (!serverSelectionJson.empty()) {
            dataFields.push_back({"serverSelection", serverSelectionJson});
        }
        if (!settingsJson.empty()) {
            dataFields.push_back({"settings", settingsJson});
        }
        
        std::string newDataJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(dataFields);
        
        std::vector<std::pair<std::string, std::string>> rootFields;
        rootFields.push_back({"schema", ::XIFriendList::Protocol::JsonUtils::encodeString("XIFriendList/v1")});
        rootFields.push_back({"migrationCompleted", ::XIFriendList::Protocol::JsonUtils::encodeString("1")});
        rootFields.push_back({"data", newDataJson});
        
        std::string jsonContent = ::XIFriendList::Protocol::JsonUtils::encodeObject(rootFields);
        
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

bool ApiKeyPersistence::loadFromFile(::XIFriendList::App::State::ApiKeyState& state) {
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    state.apiKeys.clear();
    
    try {
        std::string filePath = getMainJsonPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return true;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        if (jsonContent.empty() || !::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            return true;
        }
        
        std::string dataJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "data", dataJson)) {
            return true;
        }
        
        std::string apiKeysJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "apiKeys", apiKeysJson)) {
            return true;
        }
        
        extractAllFieldsFromJsonObject(apiKeysJson, state.apiKeys);
        
        return true;
    } catch (...) {
        state.apiKeys.clear();
        return false;
    }
}

bool ApiKeyPersistence::saveToFile(const ::XIFriendList::App::State::ApiKeyState& state) {
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    try {
        std::string filePath = getMainJsonPath();
        ensureConfigDirectory(filePath);
        
        std::string existingJson;
        std::string dataJson;
        std::string notifiedMailJson;
        std::string windowLocksJson;
        std::string collapsibleSectionsJson;
        std::string serverSelectionJson;
        std::string settingsJson;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "notifiedMail", notifiedMailJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "windowLocks", windowLocksJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "collapsibleSections", collapsibleSectionsJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "serverSelection", serverSelectionJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "settings", settingsJson);
                }
            }
        }
        
        std::vector<std::pair<std::string, std::string>> apiKeyFields;
        for (const auto& kv : state.apiKeys) {
            apiKeyFields.push_back({kv.first, ::XIFriendList::Protocol::JsonUtils::encodeString(kv.second)});
        }
        std::string apiKeysJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(apiKeyFields);
        
        std::vector<std::pair<std::string, std::string>> dataFields;
        dataFields.push_back({"apiKeys", apiKeysJson});
        if (!notifiedMailJson.empty()) {
            dataFields.push_back({"notifiedMail", notifiedMailJson});
        }
        if (!windowLocksJson.empty()) {
            dataFields.push_back({"windowLocks", windowLocksJson});
        }
        if (!collapsibleSectionsJson.empty()) {
            dataFields.push_back({"collapsibleSections", collapsibleSectionsJson});
        }
        if (!serverSelectionJson.empty()) {
            dataFields.push_back({"serverSelection", serverSelectionJson});
        }
        if (!settingsJson.empty()) {
            dataFields.push_back({"settings", settingsJson});
        }
        
        std::string newDataJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(dataFields);
        
        std::vector<std::pair<std::string, std::string>> rootFields;
        rootFields.push_back({"schema", ::XIFriendList::Protocol::JsonUtils::encodeString("XIFriendList/v1")});
        rootFields.push_back({"migrationCompleted", ::XIFriendList::Protocol::JsonUtils::encodeString("1")});
        rootFields.push_back({"data", newDataJson});
        
        std::string jsonContent = ::XIFriendList::Protocol::JsonUtils::encodeObject(rootFields);
        
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

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

