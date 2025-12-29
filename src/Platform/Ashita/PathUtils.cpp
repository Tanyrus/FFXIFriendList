#include "PathUtils.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::string PathUtils::getDefaultConfigDirectory() {
    char appDataPath[MAX_PATH] = {0};
    
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath) == S_OK) {
        std::filesystem::path configDir = std::filesystem::path(appDataPath) / "HorizonXI-Launcher" / "config" / "FFXIFriendList";
        return configDir.string();
    }
    
    return "";
}

std::string PathUtils::getDefaultConfigPath(const std::string& filename) {
    std::string configDir = getDefaultConfigDirectory();
    if (!configDir.empty()) {
        return configDir + "\\" + filename;
    }
    return "";
}

std::string PathUtils::getDefaultCachePath() {
    return getDefaultConfigPath("cache.json");
}

std::string PathUtils::getDefaultIniPath() {
    return getDefaultConfigPath("ffxifriendlist.ini");
}

std::string PathUtils::getDefaultMainJsonPath() {
    char dllPath[MAX_PATH] = {0};
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
        std::string dllPathStr(dllPath);
        size_t lastSlash = dllPathStr.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string gameDir = dllPathStr.substr(0, lastSlash);
            lastSlash = gameDir.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                gameDir = gameDir.substr(0, lastSlash + 1);
                return gameDir + "config\\FFXIFriendList\\ffxifriendlist.json";
            }
        }
    }
    
    std::string appDataPath = getDefaultConfigPath("ffxifriendlist.json");
    if (!appDataPath.empty()) {
        return appDataPath;
    }
    
    return "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.json";
}

std::string PathUtils::getDefaultThemesIniPath() {
    return getDefaultConfigPath("CustomThemes.ini");
}

std::string PathUtils::getDefaultNotesJsonPath() {
    return getDefaultConfigPath("notes.json");
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

