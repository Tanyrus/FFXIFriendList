#include "AshitaEventQueue.h"

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaEventQueue::AshitaEventQueue() {
}

void AshitaEventQueue::pushCharacterChanged(const ::XIFriendList::App::Events::CharacterChanged& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    characterChangedQueue_.push(event);
}

void AshitaEventQueue::pushZoneChanged(const ::XIFriendList::App::Events::ZoneChanged& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    zoneChangedQueue_.push(event);
}

size_t AshitaEventQueue::processEvents() {
    size_t processed = 0;
    
    while (true) {
        ::XIFriendList::App::Events::CharacterChanged event("");
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (characterChangedQueue_.empty()) {
                break;
            }
            event = characterChangedQueue_.front();
            characterChangedQueue_.pop();
        }
        
        if (characterChangedHandler_) {
            characterChangedHandler_(event);
            processed++;
        }
    }
    
    while (true) {
        ::XIFriendList::App::Events::ZoneChanged event(0);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (zoneChangedQueue_.empty()) {
                break;
            }
            event = zoneChangedQueue_.front();
            zoneChangedQueue_.pop();
        }
        
        if (zoneChangedHandler_) {
            zoneChangedHandler_(event);
            processed++;
        }
    }
    
    return processed;
}

bool AshitaEventQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return characterChangedQueue_.empty() && zoneChangedQueue_.empty();
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

