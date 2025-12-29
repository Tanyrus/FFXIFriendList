// ThemesViewModelTest.cpp
// Unit tests for ThemesViewModel

#include <catch2/catch_test_macros.hpp>
#include "UI/ViewModels/ThemesViewModel.h"
#include "Core/ModelsCore.h"

using namespace XIFriendList::UI;
using namespace XIFriendList::Core;

TEST_CASE("ThemesViewModel - Initial state", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    REQUIRE(vm.getCurrentThemeIndex() == -2);  // Default theme
    REQUIRE(vm.isDefaultTheme());
    REQUIRE_FALSE(vm.isCustomTheme());
    REQUIRE(vm.getCurrentThemeName() == "Default (No Theme)");
    REQUIRE(vm.getBackgroundAlpha() == 0.95f);
    REQUIRE(vm.getTextAlpha() == 1.0f);
    REQUIRE(vm.getCustomThemes().empty());
}

TEST_CASE("ThemesViewModel - Theme index changes", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    vm.setCurrentThemeIndex(0);
    REQUIRE(vm.getCurrentThemeIndex() == 0);
    REQUIRE_FALSE(vm.isDefaultTheme());
    REQUIRE_FALSE(vm.isCustomTheme());
    REQUIRE(vm.getCurrentThemeName() == "Warm Brown");
    
    vm.setCurrentThemeIndex(-1);
    REQUIRE(vm.isCustomTheme());
    REQUIRE_FALSE(vm.isDefaultTheme());
}

TEST_CASE("ThemesViewModel - Custom theme name", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    vm.setCurrentThemeIndex(-1);
    vm.setCurrentCustomThemeName("MyTheme");
    REQUIRE(vm.getCurrentCustomThemeName() == "MyTheme");
    REQUIRE(vm.getCurrentThemeName() == "MyTheme");
}

TEST_CASE("ThemesViewModel - Built-in theme names", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    auto names = vm.getBuiltInThemeNames();
    REQUIRE(names.size() == 5);
    REQUIRE(names[0] == "Default (No Theme)");
    REQUIRE(names[1] == "Warm Brown");
    REQUIRE(names[2] == "Modern Dark");
    REQUIRE(names[3] == "Green Nature");
    REQUIRE(names[4] == "Purple Mystic");
}

TEST_CASE("ThemesViewModel - Custom themes list", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    CustomTheme theme1;
    theme1.name = "Theme1";
    CustomTheme theme2;
    theme2.name = "Theme2";
    
    std::vector<CustomTheme> themes = { theme1, theme2 };
    vm.setCustomThemes(themes);
    
    REQUIRE(vm.getCustomThemes().size() == 2);
    REQUIRE(vm.getCustomThemes()[0].name == "Theme1");
    REQUIRE(vm.getCustomThemes()[1].name == "Theme2");
}

TEST_CASE("ThemesViewModel - Current theme colors", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    CustomTheme colors;
    colors.windowBgColor = Color(0.5f, 0.6f, 0.7f, 0.8f);
    colors.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    vm.setCurrentThemeColors(colors);
    
    const CustomTheme& current = vm.getCurrentThemeColors();
    REQUIRE(current.windowBgColor.r == 0.5f);
    REQUIRE(current.textColor.r == 1.0f);
}

TEST_CASE("ThemesViewModel - Transparency", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    vm.setBackgroundAlpha(0.5f);
    REQUIRE(vm.getBackgroundAlpha() == 0.5f);
    
    vm.setTextAlpha(0.75f);
    REQUIRE(vm.getTextAlpha() == 0.75f);
}

TEST_CASE("ThemesViewModel - Navigation", "[UI][ViewModel]") {
    ThemesViewModel vm;
    
    // At default (-2), can go next
    REQUIRE(vm.canGoNext());
    REQUIRE_FALSE(vm.canGoPrevious());
    
    // At first built-in (0), can go both ways
    vm.setCurrentThemeIndex(0);
    REQUIRE(vm.canGoNext());
    REQUIRE(vm.canGoPrevious());
    
    // At last built-in (3), can go previous
    vm.setCurrentThemeIndex(3);
    REQUIRE(vm.canGoPrevious());
    
    // At custom (-1), can go previous
    vm.setCurrentThemeIndex(-1);
    REQUIRE(vm.canGoPrevious());
    REQUIRE_FALSE(vm.canGoNext());
}

