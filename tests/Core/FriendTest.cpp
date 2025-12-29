// FriendTest.cpp
// Unit tests for Core::Friend

#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("Friend - Construction", "[Core][Friend]") {
    Friend friend1;
    REQUIRE(friend1.name.empty());
    REQUIRE(friend1.friendedAs.empty());
    REQUIRE(friend1.linkedCharacters.empty());
    
    Friend friend2("TestName", "FriendedAs");
    REQUIRE(friend2.name == "TestName");
    REQUIRE(friend2.friendedAs == "FriendedAs");
}

TEST_CASE("Friend - Equality", "[Core][Friend]") {
    Friend friend1("TestName", "FriendedAs");
    Friend friend2("testname", "friendedas");  // Case-insensitive
    Friend friend3("OtherName", "FriendedAs");
    
    REQUIRE(friend1 == friend2);
    REQUIRE(friend1 != friend3);
}

TEST_CASE("Friend - MatchesCharacter", "[Core][Friend]") {
    Friend f("TestName", "FriendedAs");
    f.linkedCharacters.push_back("AltChar1");
    f.linkedCharacters.push_back("AltChar2");
    
    REQUIRE(f.matchesCharacter("TestName"));
    REQUIRE(f.matchesCharacter("testname"));  // Case-insensitive
    REQUIRE(f.matchesCharacter("AltChar1"));
    REQUIRE(f.matchesCharacter("altchar1"));  // Case-insensitive
    REQUIRE(f.matchesCharacter("AltChar2"));
    REQUIRE_FALSE(f.matchesCharacter("OtherName"));
}

TEST_CASE("Friend - HasLinkedCharacters", "[Core][Friend]") {
    Friend friend1("TestName", "FriendedAs");
    REQUIRE_FALSE(friend1.hasLinkedCharacters());
    
    Friend friend2("TestName", "FriendedAs");
    friend2.linkedCharacters.push_back("AltChar");
    REQUIRE(friend2.hasLinkedCharacters());
}

