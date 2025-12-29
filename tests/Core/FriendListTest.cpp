// FriendListTest.cpp
// Unit tests for Core::FriendList

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("FriendList - Construction", "[Core][FriendList]") {
    FriendList list;
    REQUIRE(list.empty());
    REQUIRE(list.size() == 0);
}

TEST_CASE("FriendList - AddFriend", "[Core][FriendList]") {
    FriendList list;
    
    REQUIRE(list.addFriend("Friend1", "Friend1"));
    REQUIRE(list.size() == 1);
    REQUIRE(list.hasFriend("Friend1"));
    REQUIRE(list.hasFriend("friend1"));  // Case-insensitive
    
    REQUIRE_FALSE(list.addFriend("Friend1"));  // Already exists
    REQUIRE(list.size() == 1);
    
    REQUIRE(list.addFriend("Friend2"));
    REQUIRE(list.size() == 2);
}

TEST_CASE("FriendList - RemoveFriend", "[Core][FriendList]") {
    FriendList list;
    list.addFriend("Friend1");
    list.addFriend("Friend2");
    
    REQUIRE(list.removeFriend("Friend1"));
    REQUIRE(list.size() == 1);
    REQUIRE_FALSE(list.hasFriend("Friend1"));
    REQUIRE(list.hasFriend("Friend2"));
    
    REQUIRE_FALSE(list.removeFriend("Friend1"));  // Not found
    REQUIRE(list.size() == 1);
}

TEST_CASE("FriendList - UpdateFriend", "[Core][FriendList]") {
    FriendList list;
    list.addFriend("Friend1", "Original");
    
    Friend updated;
    updated.name = "Friend1";
    updated.friendedAs = "Updated";
    updated.linkedCharacters.push_back("Alt1");
    
    REQUIRE(list.updateFriend(updated));
    const Friend* f = list.findFriend("Friend1");
    REQUIRE(f != nullptr);
    REQUIRE(f->friendedAs == "updated");  // Normalized to lowercase
    REQUIRE(f->linkedCharacters.size() == 1);
    
    REQUIRE_FALSE(list.updateFriend(Friend("NonExistent", "")));
}

TEST_CASE("FriendList - FindFriend", "[Core][FriendList]") {
    FriendList list;
    list.addFriend("Friend1", "Original");
    
    const Friend* friend1 = list.findFriend("Friend1");
    REQUIRE(friend1 != nullptr);
    REQUIRE(friend1->name == "friend1");  // Normalized to lowercase
    REQUIRE(friend1->friendedAs == "original");
    
    const Friend* friend2 = list.findFriend("friend1");  // Case-insensitive
    REQUIRE(friend2 != nullptr);
    
    const Friend* friend3 = list.findFriend("NonExistent");
    REQUIRE(friend3 == nullptr);
}

TEST_CASE("FriendList - GetFriendNames", "[Core][FriendList]") {
    FriendList list;
    list.addFriend("Friend1");
    list.addFriend("Friend2");
    list.addFriend("Friend3");
    
    auto names = list.getFriendNames();
    REQUIRE(names.size() == 3);
    REQUIRE(std::find(names.begin(), names.end(), "friend1") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "friend2") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "friend3") != names.end());
}

TEST_CASE("FriendList - UpdateFriendStatus", "[Core][FriendList]") {
    FriendList list;
    
    FriendStatus status1;
    status1.characterName = "Friend1";
    status1.isOnline = true;
    status1.job = "WAR";
    
    list.updateFriendStatus(status1);
    const FriendStatus* status = list.getFriendStatus("Friend1");
    REQUIRE(status != nullptr);
    REQUIRE(status->isOnline);
    REQUIRE(status->job == "WAR");
    
    // Update existing status
    FriendStatus status2;
    status2.characterName = "Friend1";
    status2.isOnline = false;
    status2.job = "MNK";
    
    list.updateFriendStatus(status2);
    status = list.getFriendStatus("Friend1");
    REQUIRE(status != nullptr);
    REQUIRE_FALSE(status->isOnline);
    REQUIRE(status->job == "MNK");
}

TEST_CASE("FriendList - Clear", "[Core][FriendList]") {
    FriendList list;
    list.addFriend("Friend1");
    list.addFriend("Friend2");
    
    FriendStatus status;
    status.characterName = "Friend1";
    list.updateFriendStatus(status);
    
    list.clear();
    REQUIRE(list.empty());
    REQUIRE(list.getFriendStatuses().empty());
}

