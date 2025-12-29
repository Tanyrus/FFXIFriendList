// Platform implementation of realm detection

#include "Platform/Ashita/AshitaRealmDetector.h"
#include "Platform/Ashita/PathUtils.h"
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaRealmDetector::AshitaRealmDetector() {
    cachedRealmId_ = detectRealm();
}

std::string AshitaRealmDetector::detectRealm() const {
    std::string configDir = getConfigDirectory();
    if (configDir.empty()) {
        return "horizon";
    }
    if (fileExists(configDir + "Nasomi")) {
        return "nasomi";
    }
    if (fileExists(configDir + "Eden")) {
        return "eden";
    }
    if (fileExists(configDir + "Catseye")) {
        return "catseye";
    }
    if (fileExists(configDir + "Horizon")) {
        return "horizon";
    }
    if (fileExists(configDir + "Gaia")) {
        return "gaia";
    }
    if (fileExists(configDir + "Phoenix")) {
        return "phoenix";
    }
    if (fileExists(configDir + "LevelDown99")) {
        return "leveldown99";
    }
    return "horizon";
}

std::string AshitaRealmDetector::getRealmId() const {
    return cachedRealmId_;
}

std::string AshitaRealmDetector::getConfigDirectory() const {
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
                    return gameDir + "config\\FFXIFriendList\\";
                }
            }
        }
    }
    std::string defaultDir = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultConfigDirectory();
    return defaultDir.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\" : defaultDir + "\\";
}

bool AshitaRealmDetector::fileExists(const std::string& path) const {
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES);
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

