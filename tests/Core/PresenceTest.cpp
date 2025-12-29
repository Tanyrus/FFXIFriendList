// PresenceTest.cpp
// Unit tests for Core::Presence

#include <catch2/catch_test_macros.hpp>
#include "Core/ModelsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("Presence - Construction", "[Core][Presence]") {
    Presence presence;
    REQUIRE(presence.characterName.empty());
    REQUIRE(presence.nation == 0);
    REQUIRE_FALSE(presence.isAnonymous);
    REQUIRE(presence.timestamp == 0);
}

TEST_CASE("Presence - HasChanged", "[Core][Presence]") {
    Presence presence1;
    presence1.characterName = "TestName";
    presence1.job = "WAR";
    presence1.zone = "San d'Oria";
    
    Presence presence2 = presence1;
    REQUIRE_FALSE(presence1.hasChanged(presence2));
    
    presence2.job = "MNK";
    REQUIRE(presence1.hasChanged(presence2));
    
    presence2 = presence1;
    presence2.timestamp = 12345;  // Should not affect hasChanged
    REQUIRE_FALSE(presence1.hasChanged(presence2));
}

TEST_CASE("Presence - IsValid", "[Core][Presence]") {
    Presence presence1;
    REQUIRE_FALSE(presence1.isValid());
    
    Presence presence2;
    presence2.characterName = "TestName";
    REQUIRE(presence2.isValid());
}

