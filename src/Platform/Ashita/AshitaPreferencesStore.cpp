#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <map>
#include <mutex>
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::map<std::string, bool> AshitaPreferencesStore::windowLockCache_;
bool AshitaPreferencesStore::windowLockCacheValid_ = false;
static std::mutex windowLockCacheMutex_;

std::map<std::string, bool> AshitaPreferencesStore::collapsibleSectionCache_;
bool AshitaPreferencesStore::collapsibleSectionCacheValid_ = false;
static std::mutex collapsibleSectionCacheMutex_;

AshitaPreferencesStore::AshitaPreferencesStore() {
}

std::string AshitaPreferencesStore::getSettingsJsonPath() const {
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
                    return gameDir + "config\\FFXIFriendList\\settings.json";
                }
            }
        }
    }
    
    std::string defaultPath = PathUtils::getDefaultConfigPath("settings.json");
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\settings.json" : defaultPath;
}

std::string AshitaPreferencesStore::getCacheJsonPath() const {
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
    
    return "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\cache.json";
}

std::string AshitaPreferencesStore::getConfigPath() const {
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
    
    std::string defaultPath = PathUtils::getDefaultIniPath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.ini" : defaultPath;
}

void AshitaPreferencesStore::ensureConfigDirectory(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

void AshitaPreferencesStore::trimString(std::string& str) {
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

bool AshitaPreferencesStore::parseBoolean(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    trimString(lower);
    return (lower == "true" || lower == "1" || lower == "yes");
}

float AshitaPreferencesStore::parseFloat(const std::string& value) {
    try {
        return std::stof(value);
    } catch (...) {
        return 0.0f;
    }
}

int AshitaPreferencesStore::loadCustomCloseKeyCodeFromConfig() const {
    try {
        std::string filePath = getConfigPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return 0;
        }
        
        std::string line;
        bool isInSettingsSection = false;
        
        while (std::getline(file, line)) {
            trimString(line);
            
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }
            
            if (line[0] == '[' && line.back() == ']') {
                std::string section = line.substr(1, line.length() - 2);
                isInSettingsSection = (section == "Settings");
                continue;
            }
            
            if (!isInSettingsSection) {
                continue;
            }
            
            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }
            
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            trimString(key);
            trimString(value);
            
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), 
                          [](unsigned char c) { return static_cast<char>(::tolower(c)); });
            
            if (keyLower == "closekey" || keyLower == "close_key" || keyLower == "customclosekeycode" || keyLower == "custom_close_key_code") {
                if (!value.empty()) {
                    try {
                        int keyCode = std::stoi(value);
                        if (keyCode >= 0 && keyCode < 256) {
                            file.close();
                            return keyCode;
                        }
                    } catch (...) {
                    }
                }
            }
        }
        
        file.close();
    } catch (...) {
    }
    
    return 0;
}

int AshitaPreferencesStore::loadDebugModeFromConfig() const {
    try {
        std::string filePath = getConfigPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return -1;
        }

        std::string line;
        bool isInSettingsSection = false;

        while (std::getline(file, line)) {
            trimString(line);

            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                std::string section = line.substr(1, line.length() - 2);
                isInSettingsSection = (section == "Settings");
                continue;
            }

            if (!isInSettingsSection) {
                continue;
            }

            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            trimString(key);
            trimString(value);

            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                          [](unsigned char c) { return static_cast<char>(::tolower(c)); });

            if (keyLower == "debugmode" || keyLower == "debug_mode") {
                file.close();
                return parseBoolean(value) ? 1 : 0;
            }
        }

        file.close();
    } catch (...) {
    }
    
    return -1;
}

::XIFriendList::Core::Preferences AshitaPreferencesStore::loadPreferencesFromJson(bool serverPrefs) {
    ::XIFriendList::Core::Preferences prefs;
    
    try {
        std::string filePath = PathUtils::getDefaultMainJsonPath();
        if (filePath.empty()) {
            char dllPath[MAX_PATH] = {0};
            HMODULE hModule = GetModuleHandleA(nullptr);
            if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
                std::string path(dllPath);
                size_t lastSlash = path.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    std::string gameDir = path.substr(0, lastSlash);
                    lastSlash = gameDir.find_last_of("\\/");
                    if (lastSlash != std::string::npos) {
                        gameDir = gameDir.substr(0, lastSlash + 1);
                        filePath = gameDir + "config\\FFXIFriendList\\ffxifriendlist.json";
                    }
                }
            }
            if (filePath.empty()) {
                filePath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.json";
            }
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return prefs;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
            
        if (jsonContent.empty()) {
            return prefs;
        }
            
        if (!::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            return prefs;
        }
        
        std::string dataJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "data", dataJson)) {
            return prefs;
        }
        
        std::string prefsJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "preferences", prefsJson)) {
            return prefs;
        }
        
        if (serverPrefs) {
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "useServerNotes", prefs.useServerNotes);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "shareFriendsAcrossAlts", prefs.shareFriendsAcrossAlts);
        } else {
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "debugMode", prefs.debugMode);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "overwriteNotesOnUpload", prefs.overwriteNotesOnUpload);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "overwriteNotesOnDownload", prefs.overwriteNotesOnDownload);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "shareJobWhenAnonymous", prefs.shareJobWhenAnonymous);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "showOnlineStatus", prefs.showOnlineStatus);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "shareLocation", prefs.shareLocation);
            ::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "notificationDuration", prefs.notificationDuration);
            ::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "notificationPositionX", prefs.notificationPositionX);
            ::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "notificationPositionY", prefs.notificationPositionY);
            int customCloseKeyCode = 0;
            if (::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "customCloseKeyCode", customCloseKeyCode)) {
                prefs.customCloseKeyCode = customCloseKeyCode;
            }
            int controllerCloseButton = 0;
            if (::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "controllerCloseButton", controllerCloseButton)) {
                prefs.controllerCloseButton = controllerCloseButton;
            }
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "windowsLocked", prefs.windowsLocked);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "notificationSoundsEnabled", prefs.notificationSoundsEnabled);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "soundOnFriendOnline", prefs.soundOnFriendOnline);
            ::XIFriendList::Protocol::JsonUtils::extractBooleanField(prefsJson, "soundOnFriendRequest", prefs.soundOnFriendRequest);
            ::XIFriendList::Protocol::JsonUtils::extractNumberField(prefsJson, "notificationSoundVolume", prefs.notificationSoundVolume);
            
            ::XIFriendList::Protocol::JsonUtils::extractFriendViewSettingsField(prefsJson, "mainFriendView", prefs.mainFriendView);
            ::XIFriendList::Protocol::JsonUtils::extractFriendViewSettingsField(prefsJson, "quickOnlineFriendView", prefs.quickOnlineFriendView);
        }
    } catch (...) {
    }
    
    return prefs;
}

bool AshitaPreferencesStore::savePreferencesToJson(const ::XIFriendList::Core::Preferences& prefs, bool serverPrefs) {
    try {
        std::string filePath = PathUtils::getDefaultMainJsonPath();
        if (filePath.empty()) {
            char dllPath[MAX_PATH] = {0};
            HMODULE hModule = GetModuleHandleA(nullptr);
            if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
                std::string path(dllPath);
                size_t lastSlash = path.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    std::string gameDir = path.substr(0, lastSlash);
                    lastSlash = gameDir.find_last_of("\\/");
                    if (lastSlash != std::string::npos) {
                        gameDir = gameDir.substr(0, lastSlash + 1);
                        filePath = gameDir + "config\\FFXIFriendList\\ffxifriendlist.json";
                    }
                }
            }
            if (filePath.empty()) {
                filePath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.json";
            }
        }
        
        ensureConfigDirectory(filePath);
        
        std::string existingJson;
        std::string dataJson;
        std::string apiKeysJson;
        std::string notifiedMailJson;
        std::string windowLocksJson;
        std::string collapsibleSectionsJson;
        std::string serverSelectionJson;
        std::string existingPrefsJson;
        
        ::XIFriendList::Core::Preferences existingPrefs;
        bool hasExisting = false;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                    if (::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "preferences", existingPrefsJson)) {
                        existingPrefs = loadPreferencesFromJson(false);
                        ::XIFriendList::Core::Preferences existingServerPrefs = loadPreferencesFromJson(true);
                        existingPrefs.useServerNotes = existingServerPrefs.useServerNotes;
                        hasExisting = true;
                    }
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "apiKeys", apiKeysJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "notifiedMail", notifiedMailJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "windowLocks", windowLocksJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "collapsibleSections", collapsibleSectionsJson);
                    ::XIFriendList::Protocol::JsonUtils::extractField(dataJson, "serverSelection", serverSelectionJson);
                }
            }
        }
        
        ::XIFriendList::Core::Preferences mergedPrefs = hasExisting ? existingPrefs : ::XIFriendList::Core::Preferences();
        if (serverPrefs) {
            mergedPrefs.useServerNotes = prefs.useServerNotes;
            mergedPrefs.shareFriendsAcrossAlts = prefs.shareFriendsAcrossAlts;
        } else {
            mergedPrefs.debugMode = prefs.debugMode;
            mergedPrefs.overwriteNotesOnUpload = prefs.overwriteNotesOnUpload;
            mergedPrefs.overwriteNotesOnDownload = prefs.overwriteNotesOnDownload;
            mergedPrefs.shareJobWhenAnonymous = prefs.shareJobWhenAnonymous;
            mergedPrefs.showOnlineStatus = prefs.showOnlineStatus;
            mergedPrefs.shareLocation = prefs.shareLocation;
            mergedPrefs.notificationDuration = prefs.notificationDuration;
            mergedPrefs.notificationPositionX = prefs.notificationPositionX;
            mergedPrefs.notificationPositionY = prefs.notificationPositionY;
            mergedPrefs.customCloseKeyCode = prefs.customCloseKeyCode;
            mergedPrefs.controllerCloseButton = prefs.controllerCloseButton;
            mergedPrefs.windowsLocked = prefs.windowsLocked;
            mergedPrefs.notificationSoundsEnabled = prefs.notificationSoundsEnabled;
            mergedPrefs.soundOnFriendOnline = prefs.soundOnFriendOnline;
            mergedPrefs.soundOnFriendRequest = prefs.soundOnFriendRequest;
            mergedPrefs.notificationSoundVolume = prefs.notificationSoundVolume;
            mergedPrefs.mainFriendView = prefs.mainFriendView;
            mergedPrefs.quickOnlineFriendView = prefs.quickOnlineFriendView;
        }
        
        std::vector<std::pair<std::string, std::string>> prefsFields;
        
        prefsFields.push_back({"useServerNotes", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.useServerNotes)});
        
        prefsFields.push_back({"debugMode", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.debugMode)});
        prefsFields.push_back({"overwriteNotesOnUpload", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.overwriteNotesOnUpload)});
        prefsFields.push_back({"overwriteNotesOnDownload", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.overwriteNotesOnDownload)});
        prefsFields.push_back({"shareJobWhenAnonymous", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.shareJobWhenAnonymous)});
        prefsFields.push_back({"showOnlineStatus", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.showOnlineStatus)});
        prefsFields.push_back({"shareLocation", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.shareLocation)});
        std::ostringstream durationStream;
        durationStream.precision(1);
        durationStream << std::fixed << mergedPrefs.notificationDuration;
        prefsFields.push_back({"notificationDuration", durationStream.str()});
        std::ostringstream posXStream;
        posXStream.precision(1);
        posXStream << std::fixed << mergedPrefs.notificationPositionX;
        prefsFields.push_back({"notificationPositionX", posXStream.str()});
        std::ostringstream posYStream;
        posYStream.precision(1);
        posYStream << std::fixed << mergedPrefs.notificationPositionY;
        prefsFields.push_back({"notificationPositionY", posYStream.str()});
        prefsFields.push_back({"customCloseKeyCode", ::XIFriendList::Protocol::JsonUtils::encodeNumber(mergedPrefs.customCloseKeyCode)});
        prefsFields.push_back({"controllerCloseButton", ::XIFriendList::Protocol::JsonUtils::encodeNumber(mergedPrefs.controllerCloseButton)});
        prefsFields.push_back({"windowsLocked", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.windowsLocked)});
        prefsFields.push_back({"notificationSoundsEnabled", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.notificationSoundsEnabled)});
        prefsFields.push_back({"soundOnFriendOnline", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.soundOnFriendOnline)});
        prefsFields.push_back({"soundOnFriendRequest", ::XIFriendList::Protocol::JsonUtils::encodeBoolean(mergedPrefs.soundOnFriendRequest)});
        std::ostringstream volumeStream;
        volumeStream.precision(2);
        volumeStream << std::fixed << mergedPrefs.notificationSoundVolume;
        prefsFields.push_back({"notificationSoundVolume", volumeStream.str()});
        prefsFields.push_back({"mainFriendView", ::XIFriendList::Protocol::JsonUtils::encodeFriendViewSettings(mergedPrefs.mainFriendView)});
        prefsFields.push_back({"quickOnlineFriendView", ::XIFriendList::Protocol::JsonUtils::encodeFriendViewSettings(mergedPrefs.quickOnlineFriendView)});
        
        std::string prefsJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(prefsFields);
        
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
        if (!serverSelectionJson.empty()) {
            dataFields.push_back({"serverSelection", serverSelectionJson});
        }
        dataFields.push_back({"preferences", prefsJson});
        
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


::XIFriendList::Core::Preferences AshitaPreferencesStore::loadServerPreferences() {
    return loadPreferencesFromJson(true);
}

bool AshitaPreferencesStore::saveServerPreferences(const ::XIFriendList::Core::Preferences& prefs) {
    return savePreferencesToJson(prefs, true);
}

::XIFriendList::Core::Preferences AshitaPreferencesStore::loadLocalPreferences() {
    ::XIFriendList::Core::Preferences prefs = loadPreferencesFromJson(false);

    prefs.debugMode = false;
    int configDebugMode = loadDebugModeFromConfig();
    if (configDebugMode >= 0) {
        prefs.debugMode = (configDebugMode == 1);
    }
    
    int configKeyCode = loadCustomCloseKeyCodeFromConfig();
    if (configKeyCode != 0) {
        prefs.customCloseKeyCode = configKeyCode;
    }
    
    return prefs;
}

bool AshitaPreferencesStore::saveLocalPreferences(const ::XIFriendList::Core::Preferences& prefs) {
    // H1: Save to JSON only
    return savePreferencesToJson(prefs, false);
}

std::string AshitaPreferencesStore::getCacheJsonPathStatic() {
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
    
    std::string defaultPath = PathUtils::getDefaultCachePath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\cache.json" : defaultPath;
}

void AshitaPreferencesStore::ensureConfigDirectoryStatic(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

// Window lock state management (stored in cache.json)
// FIX: Cache lock states to avoid file I/O on every render call
void AshitaPreferencesStore::loadWindowLockCache() {
    std::lock_guard<std::mutex> lock(windowLockCacheMutex_);
    
    if (windowLockCacheValid_) {
        return;
    }
    
    windowLockCache_.clear();
    
    try {
        std::string filePath = getCacheJsonPathStatic();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            windowLockCacheValid_ = true;
            return;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
            
        if (jsonContent.empty() || !::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            windowLockCacheValid_ = true;
            return;
        }
            
        std::string schema;
        if (!::XIFriendList::Protocol::JsonUtils::extractStringField(jsonContent, "schema", schema) || 
            schema != "XIFriendListCache/v1") {
            windowLockCacheValid_ = true;
            return;
        }
        
        std::string cacheJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "cache", cacheJson)) {
            windowLockCacheValid_ = true;
            return;
        }
        
        std::string windowLocksJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "windowLocks", windowLocksJson)) {
            windowLockCacheValid_ = true;
            return;
        }
        
        size_t pos = 1;
        while (pos < windowLocksJson.length() && windowLocksJson[pos] != '}') {
            while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                ++pos;
            }
            if (pos >= windowLocksJson.length() || windowLocksJson[pos] == '}') break;
            
            if (windowLocksJson[pos] == ',') {
                ++pos;
                while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                    ++pos;
                }
            }
            
            if (windowLocksJson[pos] != '"') break;
            size_t keyStart = pos + 1;
            size_t keyEnd = keyStart;
            while (keyEnd < windowLocksJson.length() && windowLocksJson[keyEnd] != '"') {
                if (windowLocksJson[keyEnd] == '\\' && keyEnd + 1 < windowLocksJson.length()) {
                    keyEnd += 2;
                } else {
                    ++keyEnd;
                }
            }
            if (keyEnd >= windowLocksJson.length()) break;
            
            std::string key = windowLocksJson.substr(keyStart, keyEnd - keyStart);
            
            pos = keyEnd + 1;
            while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                ++pos;
            }
            if (pos >= windowLocksJson.length() || windowLocksJson[pos] != ':') break;
            ++pos;
            while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                ++pos;
            }
            
            if (pos + 4 <= windowLocksJson.length() && windowLocksJson.substr(pos, 4) == "true") {
                windowLockCache_[key] = true;
                pos += 4;
            } else if (pos + 5 <= windowLocksJson.length() && windowLocksJson.substr(pos, 5) == "false") {
                windowLockCache_[key] = false;
                pos += 5;
            } else {
                break;
            }
        }
        
        windowLockCacheValid_ = true;
    } catch (...) {
        windowLockCacheValid_ = true;
    }
}

bool AshitaPreferencesStore::loadWindowLockState(const std::string& windowId) {
    if (windowId.empty()) {
        return false;
    }
    
    if (!windowLockCacheValid_) {
        loadWindowLockCache();
    }
    
    std::lock_guard<std::mutex> lock(windowLockCacheMutex_);
    auto it = windowLockCache_.find(windowId);
    return (it != windowLockCache_.end()) ? it->second : false;
}

bool AshitaPreferencesStore::saveWindowLockState(const std::string& windowId, bool locked) {
    if (windowId.empty()) {
        return false;
    }
    
    if (!windowLockCacheValid_) {
        loadWindowLockCache();
    }
    
    {
        std::lock_guard<std::mutex> lock(windowLockCacheMutex_);
        windowLockCache_[windowId] = locked;
    }
    
    try {
        std::string filePath = getCacheJsonPathStatic();
        ensureConfigDirectoryStatic(filePath);
        
        std::string existingJson;
        std::map<std::string, std::string> apiKeys;
        std::map<std::string, std::string> notifiedMail;
        std::map<std::string, bool> windowLocks;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                std::string cacheJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "cache", cacheJson)) {
                    std::string apiKeysJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                    }
                    std::string notifiedMailJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                    }
                    std::string windowLocksJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "windowLocks", windowLocksJson)) {
                        size_t pos = 1;
                        while (pos < windowLocksJson.length() && windowLocksJson[pos] != '}') {
                            while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                                ++pos;
                            }
                            if (pos >= windowLocksJson.length() || windowLocksJson[pos] == '}') break;
                            
                            if (windowLocksJson[pos] == ',') {
                                ++pos;
                                while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                                    ++pos;
                                }
                            }
                            
                            if (windowLocksJson[pos] != '"') break;
                            size_t keyStart = pos + 1;
                            size_t keyEnd = keyStart;
                            while (keyEnd < windowLocksJson.length() && windowLocksJson[keyEnd] != '"') {
                                if (windowLocksJson[keyEnd] == '\\' && keyEnd + 1 < windowLocksJson.length()) {
                                    keyEnd += 2;
                                } else {
                                    ++keyEnd;
                                }
                            }
                            if (keyEnd >= windowLocksJson.length()) break;
                            std::string key = windowLocksJson.substr(keyStart, keyEnd - keyStart);
                            
                            pos = keyEnd + 1;
                            while (pos < windowLocksJson.length() && windowLocksJson[pos] != ':') ++pos;
                            if (pos >= windowLocksJson.length()) break;
                            ++pos;
                            while (pos < windowLocksJson.length() && std::isspace(static_cast<unsigned char>(windowLocksJson[pos]))) {
                                ++pos;
                            }
                            
                            if (pos >= windowLocksJson.length()) break;
                            bool value = false;
                            if (windowLocksJson.substr(pos, 4) == "true") {
                                value = true;
                                pos += 4;
                            } else if (windowLocksJson.substr(pos, 5) == "false") {
                                value = false;
                                pos += 5;
                            } else {
                                break;
                            }
                            
                            windowLocks[key] = value;
                            
                            while (pos < windowLocksJson.length() && windowLocksJson[pos] != ',' && windowLocksJson[pos] != '}') {
                                ++pos;
                            }
                        }
                    }
                }
            }
        }
        
        windowLocks[windowId] = locked;
        
        std::vector<std::pair<std::string, std::string>> windowLocksFields;
        for (const auto& kv : windowLocks) {
            windowLocksFields.push_back({kv.first, kv.second ? "true" : "false"});
        }
        std::string windowLocksJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(windowLocksFields);
        
        std::vector<std::pair<std::string, std::string>> cacheFields;
        
        if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
            std::string cacheJson;
            if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "cache", cacheJson)) {
                std::string apiKeysJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                    cacheFields.push_back({"apiKeys", apiKeysJson});
                }
                std::string notifiedMailJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                    cacheFields.push_back({"notifiedMail", notifiedMailJson});
                }
            }
        }
        
        cacheFields.push_back({"windowLocks", windowLocksJson});
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

// Collapsible section state management (stored in cache.json)
// FIX: Cache section states to avoid file I/O on every render call
void AshitaPreferencesStore::loadCollapsibleSectionCache() {
    std::lock_guard<std::mutex> lock(collapsibleSectionCacheMutex_);
    
    if (collapsibleSectionCacheValid_) {
        return;  // Cache already loaded
    }
    
    collapsibleSectionCache_.clear();
    
    try {
        std::string filePath = getCacheJsonPathStatic();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            collapsibleSectionCacheValid_ = true;  // Mark as valid (empty cache)
            return;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
            
        if (jsonContent.empty() || !::XIFriendList::Protocol::JsonUtils::isValidJson(jsonContent)) {
            collapsibleSectionCacheValid_ = true;
            return;
        }
            
        std::string schema;
        if (!::XIFriendList::Protocol::JsonUtils::extractStringField(jsonContent, "schema", schema) || 
            schema != "XIFriendListCache/v1") {
            collapsibleSectionCacheValid_ = true;
            return;
        }
        
        std::string cacheJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(jsonContent, "cache", cacheJson)) {
            collapsibleSectionCacheValid_ = true;
            return;
        }
        
        std::string collapsibleSectionsJson;
        if (!::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "collapsibleSections", collapsibleSectionsJson)) {
            collapsibleSectionCacheValid_ = true;
            return;
        }
        
        size_t pos = 1;  // Skip opening '{'
        while (pos < collapsibleSectionsJson.length() && collapsibleSectionsJson[pos] != '}') {
            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                ++pos;
            }
            if (pos >= collapsibleSectionsJson.length() || collapsibleSectionsJson[pos] == '}') break;
            
            if (collapsibleSectionsJson[pos] == ',') {
                ++pos;
                while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                    ++pos;
                }
            }
            
            if (collapsibleSectionsJson[pos] != '"') break;
            size_t keyStart = pos + 1;
            size_t keyEnd = keyStart;
            while (keyEnd < collapsibleSectionsJson.length() && collapsibleSectionsJson[keyEnd] != '"') {
                if (collapsibleSectionsJson[keyEnd] == '\\' && keyEnd + 1 < collapsibleSectionsJson.length()) {
                    keyEnd += 2;
                } else {
                    ++keyEnd;
                }
            }
            if (keyEnd >= collapsibleSectionsJson.length()) break;
            
            std::string key = collapsibleSectionsJson.substr(keyStart, keyEnd - keyStart);
            
            pos = keyEnd + 1;
            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                ++pos;
            }
            if (pos >= collapsibleSectionsJson.length() || collapsibleSectionsJson[pos] != ':') break;
            ++pos;
            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                ++pos;
            }
            
            if (pos + 4 <= collapsibleSectionsJson.length() && collapsibleSectionsJson.substr(pos, 4) == "true") {
                collapsibleSectionCache_[key] = true;
                pos += 4;
            } else if (pos + 5 <= collapsibleSectionsJson.length() && collapsibleSectionsJson.substr(pos, 5) == "false") {
                collapsibleSectionCache_[key] = false;
                pos += 5;
            } else {
                break;  // Invalid value
            }
        }
        
        collapsibleSectionCacheValid_ = true;
    } catch (...) {
        collapsibleSectionCacheValid_ = true;  // Mark as valid even on error (empty cache)
    }
}

bool AshitaPreferencesStore::loadCollapsibleSectionState(const std::string& windowId, const std::string& sectionId) {
    if (windowId.empty() || sectionId.empty()) {
        return false;  // Default to collapsed
    }
    
    std::string key = windowId + "." + sectionId;
    
    if (!collapsibleSectionCacheValid_) {
        loadCollapsibleSectionCache();
    }
    
    std::lock_guard<std::mutex> lock(collapsibleSectionCacheMutex_);
    auto it = collapsibleSectionCache_.find(key);
    return (it != collapsibleSectionCache_.end()) ? it->second : false;
}

bool AshitaPreferencesStore::saveCollapsibleSectionState(const std::string& windowId, const std::string& sectionId, bool expanded) {
    if (windowId.empty() || sectionId.empty()) {
        return false;
    }
    
    std::string key = windowId + "." + sectionId;
    
    {
        std::lock_guard<std::mutex> lock(collapsibleSectionCacheMutex_);
        if (!collapsibleSectionCacheValid_) {
            loadCollapsibleSectionCache();  // Load cache if not already loaded
        }
        collapsibleSectionCache_[key] = expanded;
    }
    
    try {
        std::string filePath = getCacheJsonPathStatic();
        ensureConfigDirectoryStatic(filePath);
        
        std::string existingJson;
        std::map<std::string, std::string> apiKeys;
        std::map<std::string, std::string> notifiedMail;
        std::map<std::string, bool> windowLocks;
        std::map<std::string, bool> collapsibleSections;
        
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            
            if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
                std::string cacheJson;
                if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "cache", cacheJson)) {
                    std::string apiKeysJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                    }
                    std::string notifiedMailJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                    }
                    std::string windowLocksJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "windowLocks", windowLocksJson)) {
                    }
                    std::string collapsibleSectionsJson;
                    if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "collapsibleSections", collapsibleSectionsJson)) {
                        size_t pos = 1;  // Skip opening '{'
                        while (pos < collapsibleSectionsJson.length() && collapsibleSectionsJson[pos] != '}') {
                            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                                ++pos;
                            }
                            if (pos >= collapsibleSectionsJson.length() || collapsibleSectionsJson[pos] == '}') break;
                            
                            if (collapsibleSectionsJson[pos] == ',') {
                                ++pos;
                                while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                                    ++pos;
                                }
                            }
                            
                            if (collapsibleSectionsJson[pos] != '"') break;
                            size_t keyStart = pos + 1;
                            size_t keyEnd = keyStart;
                            while (keyEnd < collapsibleSectionsJson.length() && collapsibleSectionsJson[keyEnd] != '"') {
                                if (collapsibleSectionsJson[keyEnd] == '\\' && keyEnd + 1 < collapsibleSectionsJson.length()) {
                                    keyEnd += 2;
                                } else {
                                    ++keyEnd;
                                }
                            }
                            if (keyEnd >= collapsibleSectionsJson.length()) break;
                            
                            std::string existingKey = collapsibleSectionsJson.substr(keyStart, keyEnd - keyStart);
                            
                            pos = keyEnd + 1;
                            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                                ++pos;
                            }
                            if (pos >= collapsibleSectionsJson.length() || collapsibleSectionsJson[pos] != ':') break;
                            ++pos;
                            while (pos < collapsibleSectionsJson.length() && std::isspace(static_cast<unsigned char>(collapsibleSectionsJson[pos]))) {
                                ++pos;
                            }
                            
                            if (pos + 4 <= collapsibleSectionsJson.length() && collapsibleSectionsJson.substr(pos, 4) == "true") {
                                collapsibleSections[existingKey] = true;
                                pos += 4;
                            } else if (pos + 5 <= collapsibleSectionsJson.length() && collapsibleSectionsJson.substr(pos, 5) == "false") {
                                collapsibleSections[existingKey] = false;
                                pos += 5;
                            } else {
                                break;  // Invalid value
                            }
                        }
                    }
                }
            }
        }
        
        collapsibleSections[key] = expanded;
        
        std::vector<std::pair<std::string, std::string>> collapsibleSectionsFields;
        for (const auto& pair : collapsibleSections) {
            collapsibleSectionsFields.push_back({pair.first, pair.second ? "true" : "false"});
        }
        std::string collapsibleSectionsJson = ::XIFriendList::Protocol::JsonUtils::encodeObject(collapsibleSectionsFields);
        
        std::vector<std::pair<std::string, std::string>> cacheFields;
        std::string apiKeysJson;
        std::string notifiedMailJson;
        std::string windowLocksJson;
        
        // Re-extract existing fields to preserve them
        if (!existingJson.empty() && ::XIFriendList::Protocol::JsonUtils::isValidJson(existingJson)) {
            std::string cacheJson;
            if (::XIFriendList::Protocol::JsonUtils::extractField(existingJson, "cache", cacheJson)) {
                if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                    cacheFields.push_back({"apiKeys", apiKeysJson});
                }
                if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                    cacheFields.push_back({"notifiedMail", notifiedMailJson});
                }
                if (::XIFriendList::Protocol::JsonUtils::extractField(cacheJson, "windowLocks", windowLocksJson)) {
                    cacheFields.push_back({"windowLocks", windowLocksJson});
                }
            }
        }
        
        cacheFields.push_back({"collapsibleSections", collapsibleSectionsJson});
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

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

