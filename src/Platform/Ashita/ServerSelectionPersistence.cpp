#include "Platform/Ashita/ServerSelectionPersistence.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::mutex ServerSelectionPersistence::ioMutex_;

std::string ServerSelectionPersistence::getJsonPath() {
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

void ServerSelectionPersistence::ensureConfigDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

bool ServerSelectionPersistence::loadFromFile(::XIFriendList::App::State::ServerSelectionState& state) {
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    state.savedServerId = std::nullopt;
    state.savedServerBaseUrl = std::nullopt;
    state.draftServerId = std::nullopt;
    state.detectedServerSuggestion = std::nullopt;
    state.detectedServerShownOnce = false;
    
    try {
        std::string filePath = getJsonPath();
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
        
        std::string serverSelectionJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "serverSelection", serverSelectionJson)) {
            return true;
        }
        
        std::string savedServerId;
        if (::XIFriendList::Protocol::JsonUtils::extractStringField(serverSelectionJson, "savedServerId", savedServerId) && !savedServerId.empty()) {
            state.savedServerId = savedServerId;
        }
        
        std::string savedServerBaseUrl;
        if (::XIFriendList::Protocol::JsonUtils::extractStringField(serverSelectionJson, "savedServerBaseUrl", savedServerBaseUrl) && !savedServerBaseUrl.empty()) {
            state.savedServerBaseUrl = savedServerBaseUrl;
        }
        
        bool detectedShownOnce = false;
        if (::XIFriendList::Protocol::JsonUtils::extractBooleanField(serverSelectionJson, "detectedServerShownOnce", detectedShownOnce)) {
            state.detectedServerShownOnce = detectedShownOnce;
        }
        
        return true;
    } catch (...) {
        state.savedServerId = std::nullopt;
        state.savedServerBaseUrl = std::nullopt;
        state.detectedServerShownOnce = false;
        return false;
    }
}

bool ServerSelectionPersistence::saveToFile(const ::XIFriendList::App::State::ServerSelectionState& state) {
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    try {
        std::string filePath = getJsonPath();
        ensureConfigDirectory(filePath);
        
        std::string existingJson;
        std::string dataJson;
        std::string apiKeysJson;
        std::string notifiedMailJson;
        std::string windowLocksJson;
        std::string collapsibleSectionsJson;
        std::string settingsJson;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "apiKeys", apiKeysJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "notifiedMail", notifiedMailJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "windowLocks", windowLocksJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "collapsibleSections", collapsibleSectionsJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "settings", settingsJson);
                }
            }
        }
        
        std::vector<std::pair<std::string, std::string>> serverSelectionFields;
        if (state.savedServerId.has_value() && !state.savedServerId->empty()) {
            serverSelectionFields.push_back({"savedServerId", ::XIFriendList::Protocol::JsonUtils::encodeString(state.savedServerId.value())});
        }
        if (state.savedServerBaseUrl.has_value() && !state.savedServerBaseUrl->empty()) {
            serverSelectionFields.push_back({"savedServerBaseUrl", ::XIFriendList::Protocol::JsonUtils::encodeString(state.savedServerBaseUrl.value())});
        }
        serverSelectionFields.push_back({"detectedServerShownOnce", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(state.detectedServerShownOnce)});
        std::string serverSelectionJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(serverSelectionFields);
        
        std::vector<std::pair<std::string, std::string>> dataFields;
        if (!apiKeysJson.empty()) {
            dataFields.push_back({"apiKeys", apiKeysJson});
        }
        if (!notifiedMailJson.empty()) {
            dataFields.push_back({"notifiedMail", notifiedMailJson});
        }
        if (!windowLocksJson.empty()) {
            dataFields.push_back({"windowLocks", windowLocksJson});
        }
        if (!collapsibleSectionsJson.empty()) {
            dataFields.push_back({"collapsibleSections", collapsibleSectionsJson});
        }
        if (!settingsJson.empty()) {
            dataFields.push_back({"settings", settingsJson});
        }
        dataFields.push_back({"serverSelection", serverSelectionJson});
        
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

