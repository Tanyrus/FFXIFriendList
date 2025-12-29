// VersionTest.cpp
// Unit tests for Core::Version (SemVer parsing and comparison)

#include <catch2/catch_test_macros.hpp>
#include "Core/VersionCore.h"
#include <string>

using namespace XIFriendList::Core;

TEST_CASE("Version - Parse standard format", "[Core][Version]") {
    Version v;
    
    REQUIRE(Version::parse("1.2.3", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    REQUIRE(v.prerelease.empty());
    REQUIRE(v.build.empty());
}

TEST_CASE("Version - Parse with v prefix", "[Core][Version]") {
    Version v;
    
    REQUIRE(Version::parse("v1.2.3", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    
    REQUIRE(Version::parse("V1.2.3", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
}

TEST_CASE("Version - Parse with prerelease", "[Core][Version]") {
    Version v;
    
    REQUIRE(Version::parse("1.2.3-beta", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    REQUIRE(v.prerelease == "beta");
    
    REQUIRE(Version::parse("1.2.3-alpha.1", v));
    REQUIRE(v.prerelease == "alpha.1");
    
    REQUIRE(Version::parse("v1.2.3-rc.2", v));
    REQUIRE(v.prerelease == "rc.2");
}

TEST_CASE("Version - Parse with build metadata", "[Core][Version]") {
    Version v;
    
    REQUIRE(Version::parse("1.2.3+dev", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    REQUIRE(v.build == "dev");
    
    REQUIRE(Version::parse("1.2.3+20240101", v));
    REQUIRE(v.build == "20240101");
}

TEST_CASE("Version - Parse with prerelease and build", "[Core][Version]") {
    Version v;
    
    REQUIRE(Version::parse("1.2.3-beta+dev", v));
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    REQUIRE(v.prerelease == "beta");
    REQUIRE(v.build == "dev");
}

TEST_CASE("Version - Parse invalid versions", "[Core][Version]") {
    Version v;
    
    REQUIRE_FALSE(Version::parse("", v));
    REQUIRE_FALSE(Version::parse("dev", v));
    REQUIRE_FALSE(Version::parse("unknown", v));
    REQUIRE_FALSE(Version::parse("0.0.0-dev", v));
    REQUIRE_FALSE(Version::parse("1.2", v));
    REQUIRE_FALSE(Version::parse("1", v));
    REQUIRE_FALSE(Version::parse("abc", v));
}

TEST_CASE("Version - Comparison operators", "[Core][Version]") {
    Version v1_2_3, v1_2_4, v1_3_0, v2_0_0;
    Version::parse("1.2.3", v1_2_3);
    Version::parse("1.2.4", v1_2_4);
    Version::parse("1.3.0", v1_3_0);
    Version::parse("2.0.0", v2_0_0);
    
    // Equality
    REQUIRE(v1_2_3 == v1_2_3);
    REQUIRE_FALSE(v1_2_3 == v1_2_4);
    
    // Less than
    REQUIRE(v1_2_3 < v1_2_4);
    REQUIRE(v1_2_3 < v1_3_0);
    REQUIRE(v1_2_3 < v2_0_0);
    REQUIRE(v1_2_4 < v1_3_0);
    REQUIRE(v1_3_0 < v2_0_0);
    
    // Greater than
    REQUIRE(v1_2_4 > v1_2_3);
    REQUIRE(v2_0_0 > v1_2_3);
    
    // Less than or equal
    REQUIRE(v1_2_3 <= v1_2_3);
    REQUIRE(v1_2_3 <= v1_2_4);
    
    // Greater than or equal
    REQUIRE(v1_2_4 >= v1_2_3);
    REQUIRE(v1_2_3 >= v1_2_3);
}

TEST_CASE("Version - Prerelease comparison", "[Core][Version]") {
    Version v_release, v_beta, v_alpha;
    Version::parse("1.2.3", v_release);
    Version::parse("1.2.3-beta", v_beta);
    Version::parse("1.2.3-alpha", v_alpha);
    
    // Prerelease < release
    REQUIRE(v_beta < v_release);
    REQUIRE(v_alpha < v_release);
    
    // alpha < beta
    REQUIRE(v_alpha < v_beta);
    
    // isOutdated
    REQUIRE(v_beta.isOutdated(v_release));
    REQUIRE(v_alpha.isOutdated(v_beta));
}

TEST_CASE("Version - toString", "[Core][Version]") {
    Version v;
    Version::parse("1.2.3", v);
    REQUIRE(v.toString() == "1.2.3");
    
    Version::parse("1.2.3-beta", v);
    REQUIRE(v.toString() == "1.2.3-beta");
    
    Version::parse("1.2.3+dev", v);
    REQUIRE(v.toString() == "1.2.3+dev");
    
    Version::parse("1.2.3-beta+dev", v);
    REQUIRE(v.toString() == "1.2.3-beta+dev");
}

TEST_CASE("Version - isValid and isDevVersion", "[Core][Version]") {
    Version v;
    Version::parse("1.2.3", v);
    REQUIRE(v.isValid());
    REQUIRE_FALSE(v.isDevVersion());
    
    Version::parse("1.2.3-dev", v);
    REQUIRE(v.isValid());
    REQUIRE(v.isDevVersion());
}

TEST_CASE("Version - parseVersion helper", "[Core][Version]") {
    Version v = parseVersion("1.2.3");
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    
    REQUIRE_THROWS_AS(parseVersion("invalid"), std::invalid_argument);
}

TEST_CASE("Version - isValidVersionString", "[Core][Version]") {
    REQUIRE(isValidVersionString("1.2.3"));
    REQUIRE(isValidVersionString("v1.2.3"));
    REQUIRE(isValidVersionString("1.2.3-beta"));
    REQUIRE_FALSE(isValidVersionString("dev"));
    REQUIRE_FALSE(isValidVersionString(""));
    REQUIRE_FALSE(isValidVersionString("1.2"));
}

