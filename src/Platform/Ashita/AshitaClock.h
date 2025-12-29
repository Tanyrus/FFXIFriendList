
#ifndef PLATFORM_ASHITA_CLOCK_H
#define PLATFORM_ASHITA_CLOCK_H

#include "App/Interfaces/IClock.h"

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaClock : public ::XIFriendList::App::IClock {
public:
    AshitaClock() = default;
    ~AshitaClock() = default;
    uint64_t nowMs() const override;
    uint64_t nowSeconds() const override;
    void sleepMs(uint64_t milliseconds) const override;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_CLOCK_H

