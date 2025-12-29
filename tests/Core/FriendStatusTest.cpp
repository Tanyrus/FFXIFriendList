// FriendStatusTest.cpp
// Unit tests for Core::FriendStatus

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("FriendStatus - Construction", "[Core][FriendStatus]") {
    FriendStatus status;
    REQUIRE(status.characterName.empty());
    REQUIRE_FALSE(status.isOnline);
    REQUIRE(status.nation == -1);  // -1 = hidden/not set (default to most private)
    REQUIRE(status.lastSeenAt == 0);
    REQUIRE(status.showOnlineStatus);
    REQUIRE_FALSE(status.isLinkedCharacter);
    REQUIRE_FALSE(status.isOnAltCharacter);
}

TEST_CASE("FriendStatus - Equality", "[Core][FriendStatus]") {
    FriendStatus status1;
    status1.characterName = "TestName";
    status1.isOnline = true;
    status1.job = "WAR";
    status1.rank = "1";
    status1.nation = 1;
    status1.zone = "San d'Oria";
    
    FriendStatus status2 = status1;
    REQUIRE(status1 == status2);
    
    status2.isOnline = false;
    REQUIRE(status1 != status2);
}

TEST_CASE("FriendStatus - HasStatusChanged", "[Core][FriendStatus]") {
    FriendStatus status1;
    status1.characterName = "TestName";
    status1.isOnline = true;
    status1.job = "WAR";
    
    FriendStatus status2 = status1;
    REQUIRE_FALSE(status1.hasStatusChanged(status2));
    
    status2.job = "MNK";
    REQUIRE(status1.hasStatusChanged(status2));
    
    status2 = status1;
    status2.lastSeenAt = 12345;  // Should not affect hasStatusChanged
    REQUIRE_FALSE(status1.hasStatusChanged(status2));
}

TEST_CASE("FriendStatus - HasOnlineStatusChanged", "[Core][FriendStatus]") {
    FriendStatus status1;
    status1.isOnline = true;
    
    FriendStatus status2 = status1;
    REQUIRE_FALSE(status1.hasOnlineStatusChanged(status2));
    
    status2.isOnline = false;
    REQUIRE(status1.hasOnlineStatusChanged(status2));
}

