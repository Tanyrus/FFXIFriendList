
#ifndef APP_ICLOCK_H
#define APP_ICLOCK_H

#include <cstdint>

namespace XIFriendList {
namespace App {

class IClock {
public:
    virtual ~IClock() = default;
    virtual uint64_t nowMs() const = 0;
    virtual uint64_t nowSeconds() const = 0;
    virtual void sleepMs(uint64_t milliseconds) const = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_ICLOCK_H

