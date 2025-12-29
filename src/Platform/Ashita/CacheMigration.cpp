#include "Platform/Ashita/CacheMigration.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <windows.h>
#include <map>
#include <filesystem>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

void CacheMigration::ensureConfigDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

std::string CacheMigration::readIniValue(const std::string& filePath, const std::string& section, const std::string& key) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string line;
        bool isInSection = false;
        
        while (std::getline(file, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
                continue;
            }
            
            if (trimmed[0] == '[' && trimmed.back() == ']') {
                std::string sectionName = trimmed.substr(1, trimmed.length() - 2);
                std::transform(sectionName.begin(), sectionName.end(), sectionName.begin(), ::tolower);
                std::string lowerSection = section;
                std::transform(lowerSection.begin(), lowerSection.end(), lowerSection.begin(), ::tolower);
                isInSection = (sectionName == lowerSection);
                continue;
            }
            
            if (!isInSection) {
                continue;
            }
            
            size_t equalsPos = trimmed.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }
            
            std::string lineKey = trimmed.substr(0, equalsPos);
            std::string value = trimmed.substr(equalsPos + 1);
            lineKey.erase(0, lineKey.find_first_not_of(" \t\r\n"));
            lineKey.erase(lineKey.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            std::string lowerKey = lineKey;
            std::string lowerSearchKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            std::transform(lowerSearchKey.begin(), lowerSearchKey.end(), lowerSearchKey.begin(), ::tolower);
            
            if (lowerKey == lowerSearchKey) {
                return value;
            }
        }
    } catch (...) {
    }
    
    return "";
}

bool CacheMigration::writeIniValue(const std::string& filePath, const std::string& section, const std::string& key, const std::string& value) {
    try {
        ensureConfigDirectory(filePath);
        
        std::vector<std::string> lines;
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            std::string line;
            while (std::getline(inFile, line)) {
                lines.push_back(line);
            }
            inFile.close();
        }
        
        bool foundSection = false;
        bool foundKey = false;
        size_t sectionIndex = 0;
        
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string trimmed = lines[i];
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            
            if (trimmed[0] == '[' && trimmed.back() == ']') {
                std::string sectionName = trimmed.substr(1, trimmed.length() - 2);
                std::transform(sectionName.begin(), sectionName.end(), sectionName.begin(), ::tolower);
                std::string lowerSection = section;
                std::transform(lowerSection.begin(), lowerSection.end(), lowerSection.begin(), ::tolower);
                if (sectionName == lowerSection) {
                    foundSection = true;
                    sectionIndex = i;
                    for (size_t j = i + 1; j < lines.size(); ++j) {
                        std::string nextLine = lines[j];
                        std::string nextTrimmed = nextLine;
                        nextTrimmed.erase(0, nextTrimmed.find_first_not_of(" \t\r\n"));
                        nextTrimmed.erase(nextTrimmed.find_last_not_of(" \t\r\n") + 1);
                        if (nextTrimmed.empty() || nextTrimmed[0] == '[') {
                            break;
                        }
                        size_t equalsPos = nextTrimmed.find('=');
                        if (equalsPos != std::string::npos) {
                            std::string lineKey = nextTrimmed.substr(0, equalsPos);
                            lineKey.erase(0, lineKey.find_first_not_of(" \t\r\n"));
                            lineKey.erase(lineKey.find_last_not_of(" \t\r\n") + 1);
                            std::string lowerKey = lineKey;
                            std::string lowerSearchKey = key;
                            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                            std::transform(lowerSearchKey.begin(), lowerSearchKey.end(), lowerSearchKey.begin(), ::tolower);
                            if (lowerKey == lowerSearchKey) {
                                lines[j] = key + "=" + value;
                                foundKey = true;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }
        
        if (!foundSection) {
            if (!lines.empty() && !lines.back().empty()) {
                lines.push_back("");
            }
            lines.push_back("[" + section + "]");
            lines.push_back(key + "=" + value);
        } else if (!foundKey) {
            lines.insert(lines.begin() + sectionIndex + 1, key + "=" + value);
        }
        
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            return false;
        }
        
        for (const auto& line : lines) {
            outFile << line << "\n";
        }
        outFile.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool CacheMigration::hasMigrationCompleted(const std::string& jsonPath) {
    try {
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            return false;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        if (jsonContent.empty() || !Protocol::JsonUtils::isValidJson(jsonContent)) {
            return false;
        }
        
        std::string migrated;
        if (Protocol::JsonUtils::extractStringField(jsonContent, "migrationCompleted", migrated)) {
            return (migrated == "1" || migrated == "true");
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

void CacheMigration::markMigrationCompleted(const std::string& jsonPath) {
    try {
        ensureConfigDirectory(jsonPath);
        
        std::ifstream inFile(jsonPath);
        std::string existingJson;
        if (inFile.is_open()) {
            existingJson = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
        }
        
        std::string dataJson;
        if (!existingJson.empty() && Protocol::JsonUtils::isValidJson(existingJson)) {
            Protocol::JsonUtils::extractField(existingJson, "data", dataJson);
        }
        
        std::vector<std::pair<std::string, std::string>> rootFields;
        rootFields.push_back({"schema", Protocol::JsonUtils::encodeString("XIFriendList/v1")});
        rootFields.push_back({"migrationCompleted", Protocol::JsonUtils::encodeString("1")});
        if (!dataJson.empty()) {
            rootFields.push_back({"data", dataJson});
        }
        
        std::string jsonContent = Protocol::JsonUtils::encodeObject(rootFields);
        
        std::ofstream outFile(jsonPath);
        if (outFile.is_open()) {
            outFile << jsonContent;
            outFile.close();
        }
    } catch (...) {
    }
}

bool CacheMigration::migrateCacheAndIniToJson(const std::string& jsonPath, const std::string& cacheJsonPath, const std::string& iniPath, const std::string& settingsJsonPath) {
    bool migrationCompleted = hasMigrationCompleted(jsonPath);
    
    std::vector<std::pair<std::string, std::string>> dataFields;
    
    if (migrationCompleted) {
        std::string existingDataJson;
        std::ifstream checkFile(jsonPath);
        if (checkFile.is_open()) {
            std::string existingJson((std::istreambuf_iterator<char>(checkFile)), std::istreambuf_iterator<char>());
            checkFile.close();
            if (!existingJson.empty() && Protocol::JsonUtils::isValidJson(existingJson)) {
                std::string dataJson;
                if (Protocol::JsonUtils::extractField(existingJson, "data", dataJson)) {
                    std::string prefsJson;
                    if (Protocol::JsonUtils::extractField(dataJson, "preferences", prefsJson) && !prefsJson.empty()) {
                        return true;
                    }
                    
                    std::vector<std::string> fieldsToPreserve = {"apiKeys", "notifiedMail", "windowLocks", "collapsibleSections", "serverSelection", "settings"};
                    for (const auto& fieldName : fieldsToPreserve) {
                        std::string existingFieldJson;
                        if (Protocol::JsonUtils::extractField(dataJson, fieldName, existingFieldJson) && !existingFieldJson.empty()) {
                            dataFields.push_back({fieldName, existingFieldJson});
                        }
                    }
                }
            }
        }
    }
    
    ensureConfigDirectory(jsonPath);
    
    std::ifstream cacheFile(cacheJsonPath);
    if (cacheFile.is_open()) {
        std::string jsonContent((std::istreambuf_iterator<char>(cacheFile)), std::istreambuf_iterator<char>());
        cacheFile.close();
        
        if (!jsonContent.empty() && Protocol::JsonUtils::isValidJson(jsonContent)) {
            std::string cacheJson;
            std::string schema;
            Protocol::JsonUtils::extractStringField(jsonContent, "schema", schema);
            
            if (schema == "XIFriendListCache/v1") {
                if (Protocol::JsonUtils::extractField(jsonContent, "cache", cacheJson)) {
                    std::string apiKeysJson;
                    if (Protocol::JsonUtils::extractField(cacheJson, "apiKeys", apiKeysJson)) {
                        bool found = false;
                        for (auto& field : dataFields) {
                            if (field.first == "apiKeys") {
                                field.second = apiKeysJson;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            dataFields.push_back({"apiKeys", apiKeysJson});
                        }
                    }
                    
                    std::string notifiedMailJson;
                    if (Protocol::JsonUtils::extractField(cacheJson, "notifiedMail", notifiedMailJson)) {
                        bool found = false;
                        for (auto& field : dataFields) {
                            if (field.first == "notifiedMail") {
                                field.second = notifiedMailJson;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            dataFields.push_back({"notifiedMail", notifiedMailJson});
                        }
                    }
                    
                    std::string windowLocksJson;
                    if (Protocol::JsonUtils::extractField(cacheJson, "windowLocks", windowLocksJson)) {
                        bool found = false;
                        for (auto& field : dataFields) {
                            if (field.first == "windowLocks") {
                                field.second = windowLocksJson;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            dataFields.push_back({"windowLocks", windowLocksJson});
                        }
                    }
                    
                    std::string collapsibleSectionsJson;
                    if (Protocol::JsonUtils::extractField(cacheJson, "collapsibleSections", collapsibleSectionsJson)) {
                        bool found = false;
                        for (auto& field : dataFields) {
                            if (field.first == "collapsibleSections") {
                                field.second = collapsibleSectionsJson;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            dataFields.push_back({"collapsibleSections", collapsibleSectionsJson});
                        }
                    }
                    
                    std::string serverSelectionJson;
                    if (Protocol::JsonUtils::extractField(cacheJson, "serverSelection", serverSelectionJson)) {
                        bool found = false;
                        for (auto& field : dataFields) {
                            if (field.first == "serverSelection") {
                                field.second = serverSelectionJson;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            dataFields.push_back({"serverSelection", serverSelectionJson});
                        }
                    }
                }
            }
        }
    }
    
    std::ifstream iniFile(iniPath);
    if (iniFile.is_open()) {
        std::string line;
        bool inSettings = false;
        std::string debugMode, customCloseKeyCode;
        
        while (std::getline(iniFile, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
            
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
                continue;
            }
            
            if (trimmed[0] == '[' && trimmed.back() == ']') {
                std::string section = trimmed.substr(1, trimmed.length() - 2);
                std::transform(section.begin(), section.end(), section.begin(), ::tolower);
                inSettings = (section == "settings");
                continue;
            }
            
            if (inSettings) {
                size_t equalsPos = trimmed.find('=');
                if (equalsPos != std::string::npos) {
                    std::string key = trimmed.substr(0, equalsPos);
                    std::string value = trimmed.substr(equalsPos + 1);
                    key.erase(0, key.find_first_not_of(" \t\r\n"));
                    key.erase(key.find_last_not_of(" \t\r\n") + 1);
                    value.erase(0, value.find_first_not_of(" \t\r\n"));
                    value.erase(value.find_last_not_of(" \t\r\n") + 1);
                    
                    std::string lowerKey = key;
                    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                    
                    if (lowerKey == "debugmode") {
                        debugMode = value;
                    } else if (lowerKey == "customclosekeycode") {
                        customCloseKeyCode = value;
                    }
                }
            }
        }
        iniFile.close();
        
        if (!debugMode.empty() || !customCloseKeyCode.empty()) {
            std::vector<std::pair<std::string, std::string>> settingsFields;
            if (!debugMode.empty()) {
                settingsFields.push_back({"debugMode", Protocol::JsonUtils::encodeString(debugMode)});
            }
            if (!customCloseKeyCode.empty()) {
                settingsFields.push_back({"customCloseKeyCode", Protocol::JsonUtils::encodeString(customCloseKeyCode)});
            }
            if (!settingsFields.empty()) {
                std::string settingsJson = Protocol::JsonUtils::encodeObject(settingsFields);
                dataFields.push_back({"settings", settingsJson});
            }
        }
    }
    
    std::ifstream settingsFile(settingsJsonPath);
    if (settingsFile.is_open()) {
        std::string jsonContent((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
        settingsFile.close();
        
        if (!jsonContent.empty() && Protocol::JsonUtils::isValidJson(jsonContent)) {
            std::string prefsJson;
            if (Protocol::JsonUtils::extractField(jsonContent, "preferences", prefsJson) && !prefsJson.empty()) {
                bool found = false;
                for (auto& field : dataFields) {
                    if (field.first == "preferences") {
                        field.second = prefsJson;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    dataFields.push_back({"preferences", prefsJson});
                }
            }
        }
    }
    
    if (!dataFields.empty()) {
        std::string dataJson = Protocol::JsonUtils::encodeObject(dataFields);
        
        std::vector<std::pair<std::string, std::string>> rootFields;
        rootFields.push_back({"schema", Protocol::JsonUtils::encodeString("XIFriendList/v1")});
        rootFields.push_back({"migrationCompleted", Protocol::JsonUtils::encodeString("1")});
        rootFields.push_back({"data", dataJson});
        
        std::string jsonContent = Protocol::JsonUtils::encodeObject(rootFields);
        
        std::ofstream outFile(jsonPath);
        if (outFile.is_open()) {
            outFile << jsonContent;
            outFile.close();
        }
    }
    
    markMigrationCompleted(jsonPath);
    return true;
}

bool CacheMigration::migrateConfigDirectory(const std::filesystem::path& oldConfigDir, const std::filesystem::path& newConfigDir) {
    try {
        if (!std::filesystem::exists(oldConfigDir)) {
            return false;
        }
        
        if (!std::filesystem::is_directory(oldConfigDir)) {
            return false;
        }
        
        if (std::filesystem::equivalent(oldConfigDir, newConfigDir)) {
            return false;
        }
        
        std::filesystem::create_directories(newConfigDir);
        
        bool anyMigrated = false;
        
        for (const auto& entry : std::filesystem::directory_iterator(oldConfigDir)) {
            if (!entry.is_regular_file() && !entry.is_directory()) {
                continue;
            }
            
            std::filesystem::path oldPath = entry.path();
            std::filesystem::path filename = oldPath.filename();
            std::string filenameStr = filename.string();
            std::filesystem::path newPath = newConfigDir / filename;
            
            if (filenameStr == "XIFriendList.ini") {
                newPath = newConfigDir / "ffxifriendlist.ini";
            } else if (filenameStr == "xifriendlist.json") {
                newPath = newConfigDir / "ffxifriendlist.json";
            }
            
            if (std::filesystem::exists(newPath)) {
                continue;
            }
            
            try {
                if (entry.is_regular_file()) {
                    std::filesystem::copy_file(oldPath, newPath, std::filesystem::copy_options::overwrite_existing);
                    anyMigrated = true;
                } else if (entry.is_directory()) {
                    std::filesystem::copy(oldPath, newPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                    anyMigrated = true;
                }
            } catch (const std::exception&) {
            }
        }
        
        return anyMigrated;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

