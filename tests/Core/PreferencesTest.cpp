// PreferencesTest.cpp
// Unit tests for Core::Preferences

#include <catch2/catch_test_macros.hpp>
#include "Core/ModelsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("Preferences - Construction and equality", "[Core][Preferences]") {
    SECTION("Default construction") {
        Preferences prefs;
        
        REQUIRE_FALSE(prefs.useServerNotes);
        REQUIRE(prefs.mainFriendView.showJob);
        REQUIRE_FALSE(prefs.mainFriendView.showZone);
        REQUIRE_FALSE(prefs.mainFriendView.showNationRank);
        REQUIRE_FALSE(prefs.mainFriendView.showLastSeen);
        REQUIRE(prefs.quickOnlineFriendView.showJob);
        REQUIRE_FALSE(prefs.quickOnlineFriendView.showZone);
        REQUIRE_FALSE(prefs.quickOnlineFriendView.showNationRank);
        REQUIRE_FALSE(prefs.quickOnlineFriendView.showLastSeen);
        REQUIRE_FALSE(prefs.debugMode);
        REQUIRE_FALSE(prefs.overwriteNotesOnUpload);
        REQUIRE_FALSE(prefs.overwriteNotesOnDownload);
        REQUIRE_FALSE(prefs.shareJobWhenAnonymous);
        REQUIRE(prefs.showOnlineStatus);
        REQUIRE(prefs.shareLocation);
    }
    
    SECTION("Equality with default values") {
        Preferences prefs1;
        Preferences prefs2;
        
        REQUIRE(prefs1 == prefs2);
        REQUIRE_FALSE(prefs1 != prefs2);
    }
    
    SECTION("Inequality when fields differ") {
        Preferences prefs1;
        Preferences prefs2;
        
        prefs1.useServerNotes = true;
        REQUIRE_FALSE(prefs1 == prefs2);
        REQUIRE(prefs1 != prefs2);
        
        prefs2.useServerNotes = true;
        REQUIRE(prefs1 == prefs2);
    }
    
    SECTION("Comprehensive equality with all fields") {
        Preferences prefs1;
        Preferences prefs2;
        
        prefs1.useServerNotes = true;
        prefs1.mainFriendView.showJob = false;
        prefs1.mainFriendView.showZone = false;
        prefs1.mainFriendView.showNationRank = false;
        prefs1.mainFriendView.showLastSeen = false;
        prefs1.quickOnlineFriendView.showJob = true;
        prefs1.quickOnlineFriendView.showZone = true;
        prefs1.quickOnlineFriendView.showNationRank = true;
        prefs1.quickOnlineFriendView.showLastSeen = true;
        prefs1.debugMode = true;
        prefs1.overwriteNotesOnUpload = true;
        prefs1.overwriteNotesOnDownload = true;
        prefs1.shareJobWhenAnonymous = true;
        prefs1.showOnlineStatus = false;
        prefs1.shareLocation = false;
        
        prefs2.useServerNotes = true;
        prefs2.mainFriendView.showJob = false;
        prefs2.mainFriendView.showZone = false;
        prefs2.mainFriendView.showNationRank = false;
        prefs2.mainFriendView.showLastSeen = false;
        prefs2.quickOnlineFriendView.showJob = true;
        prefs2.quickOnlineFriendView.showZone = true;
        prefs2.quickOnlineFriendView.showNationRank = true;
        prefs2.quickOnlineFriendView.showLastSeen = true;
        prefs2.debugMode = true;
        prefs2.overwriteNotesOnUpload = true;
        prefs2.overwriteNotesOnDownload = true;
        prefs2.shareJobWhenAnonymous = true;
        prefs2.showOnlineStatus = false;
        prefs2.shareLocation = false;
        
        REQUIRE(prefs1 == prefs2);
        
        prefs2.debugMode = false;
        REQUIRE_FALSE(prefs1 == prefs2);
    }
}

