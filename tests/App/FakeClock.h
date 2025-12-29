// FakeClock.h
// Fake implementation of IClock for testing

#ifndef APP_FAKE_CLOCK_H
#define APP_FAKE_CLOCK_H

#include "App/Interfaces/IClock.h"

namespace XIFriendList {
namespace App {

// Fake clock for testing
class FakeClock : public IClock {
public:
    FakeClock() : currentTime_(0) {}
    
    // Set current time
    void setTime(uint64_t timeMs) { currentTime_ = timeMs; }
    
    // Advance time
    void advance(uint64_t milliseconds) { currentTime_ += milliseconds; }
    
    // IClock interface
    uint64_t nowMs() const override { return currentTime_; }
    
    uint64_t nowSeconds() const override { return currentTime_ / 1000; }
    
    void sleepMs(uint64_t milliseconds) const override {
        // No-op in fake clock
        (void)milliseconds;
    }

private:
    uint64_t currentTime_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_FAKE_CLOCK_H

