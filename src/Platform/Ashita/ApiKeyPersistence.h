
#ifndef PLATFORM_ASHITA_API_KEY_PERSISTENCE_H
#define PLATFORM_ASHITA_API_KEY_PERSISTENCE_H

#include "App/State/ApiKeyState.h"
#include <string>
#include <map>
#include <mutex>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class ApiKeyPersistence {
public:
    static bool loadFromFile(::XIFriendList::App::State::ApiKeyState& state);
    static bool saveToFile(const ::XIFriendList::App::State::ApiKeyState& state);

private:
    static std::string getMainJsonPath();
    static void ensureConfigDirectory(const std::string& filePath);
    static std::string normalizeCharacterName(const std::string& name);
    static void trimString(std::string& str);
    static std::string loadApiKeyFromJson(const std::string& characterName);
    static bool saveApiKeyToJson(const std::string& characterName, const std::string& apiKey);
    
    // Mutex for thread-safe file I/O
    static std::mutex ioMutex_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_API_KEY_PERSISTENCE_H

