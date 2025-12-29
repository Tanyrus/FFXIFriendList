// ProtocolVersionTest.cpp
// Tests for protocol version handling

#include <catch2/catch_test_macros.hpp>
#include "Protocol/ProtocolVersion.h"

using namespace XIFriendList::Protocol;

TEST_CASE("Version parsing", "[protocol][version]") {
    Version v;
    
    SECTION("Valid version string") {
        REQUIRE(Version::parse("1.0.0", v));
        REQUIRE(v.major == 1);
        REQUIRE(v.minor == 0);
        REQUIRE(v.patch == 0);
    }
    
    SECTION("Valid version with non-zero components") {
        REQUIRE(Version::parse("2.5.10", v));
        REQUIRE(v.major == 2);
        REQUIRE(v.minor == 5);
        REQUIRE(v.patch == 10);
    }
    
    SECTION("Invalid version - empty string") {
        REQUIRE_FALSE(Version::parse("", v));
    }
    
    SECTION("Invalid version - missing components") {
        REQUIRE_FALSE(Version::parse("1.0", v));
        REQUIRE_FALSE(Version::parse("1", v));
    }
    
    SECTION("Invalid version - non-numeric") {
        REQUIRE_FALSE(Version::parse("a.b.c", v));
        REQUIRE_FALSE(Version::parse("1.0.a", v));
    }
    
    SECTION("Invalid version - extra components") {
        REQUIRE_FALSE(Version::parse("1.0.0.0", v));
    }
}

TEST_CASE("Version comparison", "[protocol][version]") {
    Version v1(1, 0, 0);
    Version v2(1, 0, 1);
    Version v3(1, 1, 0);
    Version v4(2, 0, 0);
    
    SECTION("Equality") {
        REQUIRE(v1 == v1);
        REQUIRE_FALSE(v1 == v2);
    }
    
    SECTION("Inequality") {
        REQUIRE(v1 != v2);
        REQUIRE_FALSE(v1 != v1);
    }
    
    SECTION("Less than") {
        REQUIRE(v1 < v2);
        REQUIRE(v1 < v3);
        REQUIRE(v1 < v4);
        REQUIRE_FALSE(v2 < v1);
    }
    
    SECTION("Greater than") {
        REQUIRE(v2 > v1);
        REQUIRE(v3 > v1);
        REQUIRE(v4 > v1);
        REQUIRE_FALSE(v1 > v2);
    }
    
    SECTION("Less than or equal") {
        REQUIRE(v1 <= v1);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v2 <= v1);
    }
    
    SECTION("Greater than or equal") {
        REQUIRE(v1 >= v1);
        REQUIRE(v2 >= v1);
        REQUIRE_FALSE(v1 >= v2);
    }
}

TEST_CASE("Version compatibility", "[protocol][version]") {
    Version v1(1, 0, 0);
    Version v2(1, 0, 1);
    Version v3(1, 1, 0);
    Version v4(2, 0, 0);
    
    SECTION("Same major version is compatible") {
        REQUIRE(v1.isCompatibleWith(v2));
        REQUIRE(v1.isCompatibleWith(v3));
        REQUIRE(v2.isCompatibleWith(v1));
    }
    
    SECTION("Different major version is not compatible") {
        REQUIRE_FALSE(v1.isCompatibleWith(v4));
        REQUIRE_FALSE(v4.isCompatibleWith(v1));
    }
}

TEST_CASE("Version to string", "[protocol][version]") {
    Version v(1, 2, 3);
    REQUIRE(v.toString() == "1.2.3");
    
    Version v2(10, 20, 30);
    REQUIRE(v2.toString() == "10.20.30");
}

TEST_CASE("Current version", "[protocol][version]") {
    Version current = getCurrentVersion();
    REQUIRE(current.major == 2);
    REQUIRE(current.minor == 0);
    REQUIRE(current.patch == 0);
}

TEST_CASE("Version validation", "[protocol][version]") {
    REQUIRE(isValidVersion("1.0.0"));
    REQUIRE(isValidVersion("2.5.10"));
    REQUIRE_FALSE(isValidVersion(""));
    REQUIRE_FALSE(isValidVersion("1.0"));
    REQUIRE_FALSE(isValidVersion("invalid"));
}

