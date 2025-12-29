
#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include <string>
#include <cstdint>

namespace XIFriendList {
namespace App {
namespace Events {

struct CharacterChanged {
    std::string newCharacterName;
    std::string previousCharacterName;
    uint64_t timestamp;
    
    CharacterChanged(const std::string& newName, const std::string& prevName = "", uint64_t ts = 0)
        : newCharacterName(newName)
        , previousCharacterName(prevName)
        , timestamp(ts)
    {}
};

struct ZoneChanged {
    uint16_t zoneId;
    std::string zoneName;
    uint64_t timestamp;
    
    ZoneChanged(uint16_t id, const std::string& name = "", uint64_t ts = 0)
        : zoneId(id)
        , zoneName(name)
        , timestamp(ts)
    {}
};

} // namespace Events
} // namespace App
} // namespace XIFriendList

#endif // APP_EVENTS_H

