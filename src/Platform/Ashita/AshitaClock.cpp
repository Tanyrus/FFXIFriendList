#include "AshitaClock.h"
#include <chrono>
#include <thread>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

uint64_t AshitaClock::nowMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return static_cast<uint64_t>(milliseconds.count());
}

uint64_t AshitaClock::nowSeconds() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return static_cast<uint64_t>(seconds.count());
}

void AshitaClock::sleepMs(uint64_t milliseconds) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

