
#ifndef PLATFORM_ASHITA_KEY_EDGE_DETECTOR_H
#define PLATFORM_ASHITA_KEY_EDGE_DETECTOR_H

#include <windows.h>
#include <cstdint>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class KeyEdgeDetector {
public:
    KeyEdgeDetector();
    bool update(int32_t virtualKeyCode);
    void reset();

private:
    bool lastKeyState_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_KEY_EDGE_DETECTOR_H

