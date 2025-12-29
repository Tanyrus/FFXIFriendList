#include <catch2/catch_test_macros.hpp>
#include "Core/FriendsCore.h"
#include "Core/MemoryStats.h"
#include "App/State/NotesState.h"
#include "App/State/ThemeState.h"

using namespace XIFriendList;
using namespace XIFriendList::App::State;

TEST_CASE("FriendList memory stats", "[memory]") {
    Core::FriendList friendList;
    
    friendList.addFriend("TestFriend", "TestFriend");
    friendList.addFriend("AnotherFriend", "AnotherFriend");
    
    Core::FriendStatus status;
    status.characterName = "TestFriend";
    status.displayName = "TestFriend";
    status.isOnline = true;
    friendList.updateFriendStatus(status);
    
    auto stats = friendList.getMemoryStats();
    
    REQUIRE(stats.category == "Friends");
    REQUIRE(stats.entryCount >= 3);
    REQUIRE(stats.estimatedBytes > 0);
}

TEST_CASE("NotesState memory stats", "[memory]") {
    NotesState notesState;
    
    Core::Note note1("Friend1", "Note 1", 1000);
    Core::Note note2("Friend2", "Note 2 with longer text", 2000);
    
    notesState.notes["friend1"] = note1;
    notesState.notes["friend2"] = note2;
    
    auto stats = notesState.getMemoryStats();
    
    REQUIRE(stats.category == "Notes");
    REQUIRE(stats.entryCount == 2);
    REQUIRE(stats.estimatedBytes > 0);
}

TEST_CASE("ThemeState memory stats", "[memory]") {
    ThemeState themeState;
    
    Core::CustomTheme theme1;
    theme1.name = "TestTheme1";
    themeState.customThemes.push_back(theme1);
    
    Core::CustomTheme theme2;
    theme2.name = "TestTheme2";
    themeState.customThemes.push_back(theme2);
    
    auto stats = themeState.getMemoryStats();
    
    REQUIRE(stats.category == "Themes");
    REQUIRE(stats.entryCount == 2);
    REQUIRE(stats.estimatedBytes > 0);
}

TEST_CASE("Memory stats aggregation", "[memory]") {
    Core::FriendList friendList;
    friendList.addFriend("Test", "Test");
    
    NotesState notesState;
    Core::Note note("Test", "Note", 1000);
    notesState.notes["test"] = note;
    
    ThemeState themeState;
    
    auto friendStats = friendList.getMemoryStats();
    auto notesStats = notesState.getMemoryStats();
    auto themeStats = themeState.getMemoryStats();
    
    size_t totalBytes = friendStats.estimatedBytes + notesStats.estimatedBytes + themeStats.estimatedBytes;
    
    REQUIRE(totalBytes > 0);
    REQUIRE(friendStats.estimatedBytes > 0);
    REQUIRE(notesStats.estimatedBytes > 0);
}

