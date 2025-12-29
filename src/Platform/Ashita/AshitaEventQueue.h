
#ifndef PLATFORM_ASHITA_EVENT_QUEUE_H
#define PLATFORM_ASHITA_EVENT_QUEUE_H

#include "App/Interfaces/IEventQueue.h"
#include "App/Events/AppEvents.h"
#include <queue>
#include <mutex>
#include <vector>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaEventQueue : public ::XIFriendList::App::IEventQueue {
public:
    AshitaEventQueue();
    ~AshitaEventQueue() = default;
    void pushCharacterChanged(const ::XIFriendList::App::Events::CharacterChanged& event) override;
    void pushZoneChanged(const ::XIFriendList::App::Events::ZoneChanged& event) override;
    size_t processEvents() override;
    bool empty() const override;
    void setCharacterChangedHandler(std::function<void(const ::XIFriendList::App::Events::CharacterChanged&)> handler) {
        characterChangedHandler_ = handler;
    }
    void setZoneChangedHandler(std::function<void(const ::XIFriendList::App::Events::ZoneChanged&)> handler) {
        zoneChangedHandler_ = handler;
    }

private:
    mutable std::mutex mutex_;
    std::queue<::XIFriendList::App::Events::CharacterChanged> characterChangedQueue_;
    std::queue<::XIFriendList::App::Events::ZoneChanged> zoneChangedQueue_;
    std::function<void(const ::XIFriendList::App::Events::CharacterChanged&)> characterChangedHandler_;
    std::function<void(const ::XIFriendList::App::Events::ZoneChanged&)> zoneChangedHandler_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_EVENT_QUEUE_H

