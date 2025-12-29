// FriendListFilterTest.cpp
// Unit tests for Core::FriendListFilter

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("FriendListFilter - FilterByName", "[Core][FriendListFilter]") {
    std::vector<Friend> friends;
    friends.push_back(Friend("Alice", "Alice"));
    friends.push_back(Friend("Bob", "Bob"));
    friends.push_back(Friend("Charlie", "Charlie"));
    
    auto filtered = FriendListFilter::filterByName(friends, "al");
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].name == "Alice");  // Names preserve original case
    
    filtered = FriendListFilter::filterByName(friends, "B");
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].name == "Bob");
    
    filtered = FriendListFilter::filterByName(friends, "");
    REQUIRE(filtered.size() == 3);
}

TEST_CASE("FriendListFilter - FilterByOnlineStatus", "[Core][FriendListFilter]") {
    std::vector<std::string> names = {"Online1", "Offline1", "Online2"};
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "Online1";
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "Offline1";
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "Online2";
    status3.isOnline = true;
    status3.showOnlineStatus = true;
    statuses.push_back(status3);
    
    auto online = FriendListFilter::filterByOnlineStatus(names, statuses, true);
    REQUIRE(online.size() == 2);
    
    auto offline = FriendListFilter::filterByOnlineStatus(names, statuses, false);
    REQUIRE(offline.size() == 1);
}

TEST_CASE("FriendListFilter - FilterOnline", "[Core][FriendListFilter]") {
    std::vector<std::string> names = {"Online1", "Offline1"};
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "Online1";
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "Offline1";
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses.push_back(status2);
    
    auto online = FriendListFilter::filterOnline(names, statuses);
    REQUIRE(online.size() == 1);
    REQUIRE(online[0] == "Online1");
}

TEST_CASE("FriendListFilter - FilterOnline respects hidden online status", "[Core][FriendListFilter]") {
    std::vector<std::string> names = {"HiddenOnline", "VisibleOnline"};

    std::vector<FriendStatus> statuses;
    FriendStatus hidden;
    hidden.characterName = "HiddenOnline";
    hidden.isOnline = true;
    hidden.showOnlineStatus = false; // privacy: should not appear as online
    statuses.push_back(hidden);

    FriendStatus visible;
    visible.characterName = "VisibleOnline";
    visible.isOnline = true;
    visible.showOnlineStatus = true;
    statuses.push_back(visible);

    auto online = FriendListFilter::filterOnline(names, statuses);
    REQUIRE(online.size() == 1);
    REQUIRE(online[0] == "VisibleOnline");
}

TEST_CASE("FriendListFilter - FilterWithPredicate", "[Core][FriendListFilter]") {
    std::vector<Friend> friends;
    friends.push_back(Friend("Alice", "Alice"));
    friends.push_back(Friend("Bob", "Bob"));
    friends.push_back(Friend("Charlie", "Charlie"));
    
    auto filtered = FriendListFilter::filter(friends,
        [](const Friend& f) { return f.name.length() > 3; });
    
    REQUIRE(filtered.size() == 2);
}

