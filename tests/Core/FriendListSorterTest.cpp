// FriendListSorterTest.cpp
// Unit tests for Core::FriendListSorter

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("FriendListSorter - SortFriendsAlphabetically", "[Core][FriendListSorter]") {
    std::vector<std::string> names = {"Charlie", "Alice", "Bob"};
    FriendListSorter::sortFriendsAlphabetically(names);
    
    REQUIRE(names[0] == "Alice");
    REQUIRE(names[1] == "Bob");
    REQUIRE(names[2] == "Charlie");
}

TEST_CASE("FriendListSorter - SortFriendsByStatus", "[Core][FriendListSorter]") {
    std::vector<std::string> names = {"Offline1", "Online1", "Offline2", "Online2"};
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "Online1";
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "Online2";
    status2.isOnline = true;
    status2.showOnlineStatus = true;
    statuses.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "Offline1";
    status3.isOnline = false;
    status3.showOnlineStatus = true;
    statuses.push_back(status3);
    
    FriendStatus status4;
    status4.characterName = "Offline2";
    status4.isOnline = false;
    status4.showOnlineStatus = true;
    statuses.push_back(status4);
    
    FriendListSorter::sortFriendsByStatus(names, statuses);
    
    // Online friends first
    REQUIRE((names[0] == "Online1" || names[0] == "Online2"));
    REQUIRE((names[1] == "Online1" || names[1] == "Online2"));
    // Then offline
    REQUIRE((names[2] == "Offline1" || names[2] == "Offline2"));
    REQUIRE((names[3] == "Offline1" || names[3] == "Offline2"));
}

TEST_CASE("FriendListSorter - SortFriendsAlphabeticallyObjects", "[Core][FriendListSorter]") {
    std::vector<Friend> friends;
    friends.push_back(Friend("Charlie", "Charlie"));
    friends.push_back(Friend("Alice", "Alice"));
    friends.push_back(Friend("Bob", "Bob"));
    
    FriendListSorter::sortFriendsAlphabetically(friends);
    
    REQUIRE(friends[0].name == "Alice");  // Names preserve original case
    REQUIRE(friends[1].name == "Bob");
    REQUIRE(friends[2].name == "Charlie");
}

