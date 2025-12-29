
#ifndef APP_I_EVENT_QUEUE_H
#define APP_I_EVENT_QUEUE_H

#include "App/Events/AppEvents.h"
#include <functional>
#include <memory>

namespace XIFriendList {
namespace App {

using EventHandler = std::function<void()>;

class IEventQueue {
public:
    virtual ~IEventQueue() = default;
    virtual void pushCharacterChanged(const XIFriendList::App::Events::CharacterChanged& event) = 0;
    virtual void pushZoneChanged(const XIFriendList::App::Events::ZoneChanged& event) = 0;
    virtual size_t processEvents() = 0;
    virtual bool empty() const = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_I_EVENT_QUEUE_H

