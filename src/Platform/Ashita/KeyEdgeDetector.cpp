#include "KeyEdgeDetector.h"
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

KeyEdgeDetector::KeyEdgeDetector()
    : lastKeyState_(false)
{
}

bool KeyEdgeDetector::update(int32_t virtualKeyCode) {
    bool currentState = (::GetAsyncKeyState(virtualKeyCode) & 0x8000) != 0;
    bool edgeDetected = currentState && !lastKeyState_;
    lastKeyState_ = currentState;
    
    return edgeDetected;
}

void KeyEdgeDetector::reset() {
    lastKeyState_ = false;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

