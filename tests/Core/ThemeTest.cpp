// ThemeTest.cpp
// Unit tests for Core::Theme structures

#include <catch2/catch_test_macros.hpp>
#include "Core/ModelsCore.h"

using namespace XIFriendList::Core;

TEST_CASE("Color - Construction", "[Core][Theme]") {
    Color c1;
    REQUIRE(c1.r == 0.0f);
    REQUIRE(c1.g == 0.0f);
    REQUIRE(c1.b == 0.0f);
    REQUIRE(c1.a == 1.0f);
    
    Color c2(0.5f, 0.6f, 0.7f, 0.8f);
    REQUIRE(c2.r == 0.5f);
    REQUIRE(c2.g == 0.6f);
    REQUIRE(c2.b == 0.7f);
    REQUIRE(c2.a == 0.8f);
    
    Color c3(1.0f, 0.0f, 0.5f);  // Default alpha
    REQUIRE(c3.a == 1.0f);
}

TEST_CASE("Color - Equality", "[Core][Theme]") {
    Color c1(0.5f, 0.6f, 0.7f, 0.8f);
    Color c2(0.5f, 0.6f, 0.7f, 0.8f);
    Color c3(0.5f, 0.6f, 0.7f, 0.9f);  // Different alpha
    
    REQUIRE(c1 == c2);
    REQUIRE_FALSE(c1 == c3);
}

TEST_CASE("CustomTheme - Construction", "[Core][Theme]") {
    CustomTheme theme;
    REQUIRE(theme.name.empty());
    REQUIRE(theme.windowBgColor.r == 0.0f);
    REQUIRE(theme.windowBgColor.a == 1.0f);
    REQUIRE(theme.textColor.r == 0.0f);
}

TEST_CASE("CustomTheme - Name assignment", "[Core][Theme]") {
    CustomTheme theme;
    theme.name = "TestTheme";
    REQUIRE(theme.name == "TestTheme");
}

TEST_CASE("BuiltInTheme - Enum values", "[Core][Theme]") {
    REQUIRE(static_cast<int>(BuiltInTheme::Default) == -2);
    REQUIRE(static_cast<int>(BuiltInTheme::FFXIClassic) == 0);
    REQUIRE(static_cast<int>(BuiltInTheme::ModernDark) == 1);
    REQUIRE(static_cast<int>(BuiltInTheme::GreenNature) == 2);
    REQUIRE(static_cast<int>(BuiltInTheme::PurpleMystic) == 3);
}

TEST_CASE("getBuiltInThemeName - All themes", "[Core][Theme]") {
    REQUIRE(getBuiltInThemeName(BuiltInTheme::Default) == "Default (No Theme)");
    REQUIRE(getBuiltInThemeName(BuiltInTheme::FFXIClassic) == "Warm Brown");
    REQUIRE(getBuiltInThemeName(BuiltInTheme::ModernDark) == "Modern Dark");
    REQUIRE(getBuiltInThemeName(BuiltInTheme::GreenNature) == "Green Nature");
    REQUIRE(getBuiltInThemeName(BuiltInTheme::PurpleMystic) == "Purple Mystic");
}

