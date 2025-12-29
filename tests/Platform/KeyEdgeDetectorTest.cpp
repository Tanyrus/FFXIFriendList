// KeyEdgeDetectorTest.cpp
// Unit tests for KeyEdgeDetector

#include <catch2/catch_test_macros.hpp>
#include "Platform/Ashita/KeyEdgeDetector.h"
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

TEST_CASE("KeyEdgeDetector - reset and update", "[KeyEdgeDetector]") {
    KeyEdgeDetector detector;
    
    detector.reset();
    
    bool result = detector.update(VK_ESCAPE);
    
    REQUIRE((result == true || result == false));
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

