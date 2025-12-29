
#ifndef PLATFORM_ASHITA_SERVER_SELECTION_PERSISTENCE_H
#define PLATFORM_ASHITA_SERVER_SELECTION_PERSISTENCE_H

#include "App/State/ServerSelectionState.h"
#include <string>
#include <mutex>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class ServerSelectionPersistence {
public:
    static bool loadFromFile(::XIFriendList::App::State::ServerSelectionState& state);
    static bool saveToFile(const ::XIFriendList::App::State::ServerSelectionState& state);

private:
    static std::string getJsonPath();
    static void ensureConfigDirectory(const std::string& filePath);
    
    static std::mutex ioMutex_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_SERVER_SELECTION_PERSISTENCE_H

