// RealmDetectorTest.cpp
// Tests for IRealmDetector behavior and realm detection contract

#include <catch2/catch_test_macros.hpp>
#include "FakeRealmDetector.h"
#include <set>
#include <string>

using namespace XIFriendList::App;

TEST_CASE("RealmDetector - Default realm is horizon", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    // With no sentinel files, should default to horizon
    REQUIRE(detector.detectRealm() == "horizon");
    REQUIRE(detector.getRealmId() == "horizon");
}

TEST_CASE("RealmDetector - Sentinel file priority", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    SECTION("Nasomi has highest priority") {
        detector.setSentinelFiles({"Nasomi", "Eden", "Horizon"});
        REQUIRE(detector.detectRealm() == "nasomi");
    }
    
    SECTION("Eden has second priority") {
        detector.setSentinelFiles({"Eden", "Catseye", "Horizon"});
        REQUIRE(detector.detectRealm() == "eden");
    }
    
    SECTION("Catseye has third priority") {
        detector.setSentinelFiles({"Catseye", "Horizon", "Gaia"});
        REQUIRE(detector.detectRealm() == "catseye");
    }
    
    SECTION("Horizon before Gaia") {
        detector.setSentinelFiles({"Horizon", "Gaia", "LevelDown99"});
        REQUIRE(detector.detectRealm() == "horizon");
    }
    
    SECTION("Gaia before LevelDown99") {
        detector.setSentinelFiles({"Gaia", "LevelDown99"});
        REQUIRE(detector.detectRealm() == "gaia");
    }
    
    SECTION("LevelDown99 when only present") {
        detector.setSentinelFiles({"LevelDown99"});
        REQUIRE(detector.detectRealm() == "leveldown99");
    }
}

TEST_CASE("RealmDetector - Individual realm detection", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    SECTION("Nasomi") {
        detector.setSentinelFiles({"Nasomi"});
        REQUIRE(detector.detectRealm() == "nasomi");
        REQUIRE(detector.getRealmId() == "nasomi");
    }
    
    SECTION("Eden") {
        detector.setSentinelFiles({"Eden"});
        REQUIRE(detector.detectRealm() == "eden");
        REQUIRE(detector.getRealmId() == "eden");
    }
    
    SECTION("Catseye") {
        detector.setSentinelFiles({"Catseye"});
        REQUIRE(detector.detectRealm() == "catseye");
        REQUIRE(detector.getRealmId() == "catseye");
    }
    
    SECTION("Horizon") {
        detector.setSentinelFiles({"Horizon"});
        REQUIRE(detector.detectRealm() == "horizon");
        REQUIRE(detector.getRealmId() == "horizon");
    }
    
    SECTION("Gaia") {
        detector.setSentinelFiles({"Gaia"});
        REQUIRE(detector.detectRealm() == "gaia");
        REQUIRE(detector.getRealmId() == "gaia");
    }
    
    SECTION("LevelDown99") {
        detector.setSentinelFiles({"LevelDown99"});
        REQUIRE(detector.detectRealm() == "leveldown99");
        REQUIRE(detector.getRealmId() == "leveldown99");
    }
}

TEST_CASE("RealmDetector - Caching behavior", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    // Initial state - default horizon
    REQUIRE(detector.getRealmId() == "horizon");
    
    // Set sentinel files - should update cached value
    detector.setSentinelFiles({"Eden"});
    REQUIRE(detector.getRealmId() == "eden");
    
    // Change sentinel files - should update cached value
    detector.setSentinelFiles({"Nasomi"});
    REQUIRE(detector.getRealmId() == "nasomi");
    
    // Clear sentinel files - should revert to default
    detector.clearSentinelFiles();
    REQUIRE(detector.getRealmId() == "horizon");
}

TEST_CASE("RealmDetector - Unknown sentinel files ignored", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    // Unknown sentinel files should be ignored, fall back to horizon
    detector.setSentinelFiles({"SomeUnknownRealm", "AnotherRealm"});
    REQUIRE(detector.detectRealm() == "horizon");
    
    // Known sentinel among unknown ones should be detected
    detector.setSentinelFiles({"Unknown", "Eden", "AlsoUnknown"});
    REQUIRE(detector.detectRealm() == "eden");
}

TEST_CASE("RealmDetector - Realm ID format", "[App][RealmDetector]") {
    FakeRealmDetector detector;
    
    // All realm IDs should be lowercase
    detector.setSentinelFiles({"Nasomi"});
    REQUIRE(detector.getRealmId() == "nasomi");
    
    detector.setSentinelFiles({"Eden"});
    REQUIRE(detector.getRealmId() == "eden");
    
    detector.setSentinelFiles({"Catseye"});
    REQUIRE(detector.getRealmId() == "catseye");
    
    detector.setSentinelFiles({"Horizon"});
    REQUIRE(detector.getRealmId() == "horizon");
    
    detector.setSentinelFiles({"Gaia"});
    REQUIRE(detector.getRealmId() == "gaia");
    
    detector.setSentinelFiles({"LevelDown99"});
    REQUIRE(detector.getRealmId() == "leveldown99");
}


