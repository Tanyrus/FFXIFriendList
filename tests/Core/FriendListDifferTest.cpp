// FriendListDifferTest.cpp
// Unit tests for Core::FriendListDiffer

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("FriendListDiffer - Diff", "[Core][FriendListDiffer]") {
    std::vector<std::string> oldFriends = {"Friend1", "Friend2", "Friend3"};
    std::vector<std::string> newFriends = {"Friend2", "Friend3", "Friend4"};
    
    auto diff = FriendListDiffer::diff(oldFriends, newFriends);
    
    REQUIRE(diff.addedFriends.size() == 1);
    REQUIRE(diff.addedFriends[0] == "Friend4");
    
    REQUIRE(diff.removedFriends.size() == 1);
    REQUIRE(diff.removedFriends[0] == "Friend1");
    
    REQUIRE(diff.hasChanges());
}

TEST_CASE("FriendListDiffer - DiffStatuses", "[Core][FriendListDiffer]") {
    std::vector<FriendStatus> oldStatuses;
    FriendStatus status1;
    status1.characterName = "Friend1";
    status1.job = "WAR";
    oldStatuses.push_back(status1);
    
    std::vector<FriendStatus> newStatuses;
    FriendStatus status2;
    status2.characterName = "Friend1";
    status2.job = "MNK";  // Changed
    newStatuses.push_back(status2);
    
    auto changed = FriendListDiffer::diffStatuses(oldStatuses, newStatuses);
    REQUIRE(changed.size() == 1);
    REQUIRE(changed[0] == "Friend1");
}

TEST_CASE("FriendListDiffer - DiffOnlineStatus", "[Core][FriendListDiffer]") {
    std::vector<FriendStatus> oldStatuses;
    FriendStatus status1;
    status1.characterName = "Friend1";
    status1.isOnline = false;
    oldStatuses.push_back(status1);
    
    std::vector<FriendStatus> newStatuses;
    FriendStatus status2;
    status2.characterName = "Friend1";
    status2.isOnline = true;  // Changed
    newStatuses.push_back(status2);
    
    auto changed = FriendListDiffer::diffOnlineStatus(oldStatuses, newStatuses);
    REQUIRE(changed.size() == 1);
    REQUIRE(changed[0] == "Friend1");
}

TEST_CASE("FriendListDiffer - NoChanges", "[Core][FriendListDiffer]") {
    std::vector<std::string> friends = {"Friend1", "Friend2"};
    auto diff = FriendListDiffer::diff(friends, friends);
    
    REQUIRE_FALSE(diff.hasChanges());
    REQUIRE(diff.addedFriends.empty());
    REQUIRE(diff.removedFriends.empty());
}

