// ThemeUseCaseTest.cpp
// Unit tests for ThemeUseCase (theme management, Quick Online theme, Notification theme)

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/ThemingUseCases.h"
#include "App/State/ThemeState.h"
#include "Core/ModelsCore.h"
#include <memory>

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;
using namespace XIFriendList::App::State;
using namespace XIFriendList::Core;

TEST_CASE("ThemeUseCase - Initial state", "[App][Theme]") {
    ThemeState state;  // Defaults to -2 (default theme), which matches expected initial state
    ThemeUseCase useCase(state);
    
    // After loadThemes() is called in constructor, values come from store
    // ThemeState defaults to themeIndex = 0 (Warm Brown)
    REQUIRE(useCase.getCurrentThemeIndex() == 0);  // Default theme index (Warm Brown)
    REQUIRE(useCase.getCurrentPresetName() == "");  // Built-in themes have empty preset
    REQUIRE_FALSE(useCase.isDefaultTheme());  // Warm Brown is not the default theme
    REQUIRE(useCase.getCurrentCustomThemeName().empty());
    REQUIRE(useCase.getCustomThemes().empty());
    REQUIRE(useCase.getBackgroundAlpha() == 0.95f);
    REQUIRE(useCase.getTextAlpha() == 1.0f);
}

TEST_CASE("ThemeUseCase - Set built-in theme", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    ThemeResult result = useCase.setTheme(0);  // FFXI Classic
    REQUIRE(result.success);
    REQUIRE(useCase.getCurrentThemeIndex() == 0);
    REQUIRE_FALSE(useCase.isDefaultTheme());
}

TEST_CASE("ThemeUseCase - Set invalid theme index", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    ThemeResult result = useCase.setTheme(5);  // Invalid
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("ThemeUseCase - Set custom theme", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Add a custom theme to the state
    CustomTheme theme1;
    theme1.name = "MyTheme";
    theme1.windowBgColor = Color(0.1f, 0.2f, 0.3f, 1.0f);
    state.customThemes = { theme1 };
    useCase.loadThemes();
    
    ThemeResult result = useCase.setCustomTheme("MyTheme");
    REQUIRE(result.success);
    REQUIRE(useCase.getCurrentThemeIndex() == -1);
    REQUIRE(useCase.getCurrentCustomThemeName() == "MyTheme");
    REQUIRE_FALSE(useCase.isDefaultTheme());
}

TEST_CASE("ThemeUseCase - Set non-existent custom theme", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    ThemeResult result = useCase.setCustomTheme("NonExistent");
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("ThemeUseCase - Save custom theme", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    CustomTheme newTheme;
    newTheme.name = "NewTheme";
    newTheme.windowBgColor = Color(0.5f, 0.6f, 0.7f, 1.0f);
    newTheme.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    
    ThemeResult result = useCase.saveCustomTheme("NewTheme", newTheme);
    REQUIRE(result.success);
    
    auto themes = useCase.getCustomThemes();
    REQUIRE(themes.size() == 1);
    REQUIRE(themes[0].name == "NewTheme");
}

TEST_CASE("ThemeUseCase - Delete custom theme", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Add a custom theme
    CustomTheme theme1;
    theme1.name = "ThemeToDelete";
    state.customThemes = { theme1 };
    useCase.loadThemes();
    
    ThemeResult result = useCase.deleteCustomTheme("ThemeToDelete");
    REQUIRE(result.success);
    
    auto remainingThemes = useCase.getCustomThemes();
    REQUIRE(remainingThemes.empty());
}

TEST_CASE("ThemeUseCase - Update current theme colors", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Set to a built-in theme first
    useCase.setTheme(0);  // FFXI Classic
    
    CustomTheme updatedColors;
    updatedColors.windowBgColor = Color(0.9f, 0.8f, 0.7f, 1.0f);
    updatedColors.textColor = Color(0.1f, 0.2f, 0.3f, 1.0f);
    
    ThemeResult result = useCase.updateCurrentThemeColors(updatedColors);
    REQUIRE(result.success);
    
    CustomTheme current = useCase.getCurrentCustomTheme();
    REQUIRE(current.windowBgColor.r == 0.9f);
    REQUIRE(current.textColor.r == 0.1f);
}

TEST_CASE("ThemeUseCase - Update Quick Online theme colors", "[App][Theme][QuickOnline]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    CustomTheme quickOnlineColors;
    quickOnlineColors.windowBgColor = Color(0.2f, 0.3f, 0.4f, 1.0f);
    quickOnlineColors.textColor = Color(0.9f, 0.9f, 0.9f, 1.0f);
    quickOnlineColors.tableBgColor = Color(0.1f, 0.1f, 0.1f, 1.0f);
    
    ThemeResult result = useCase.updateQuickOnlineThemeColors(quickOnlineColors);
    REQUIRE(result.success);
    
    CustomTheme retrieved = useCase.getQuickOnlineTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.2f);
    REQUIRE(retrieved.windowBgColor.g == 0.3f);
    REQUIRE(retrieved.windowBgColor.b == 0.4f);
    REQUIRE(retrieved.textColor.r == 0.9f);
    REQUIRE(retrieved.tableBgColor.r == 0.1f);
}

TEST_CASE("ThemeUseCase - Quick Online theme persists across updates", "[App][Theme][QuickOnline]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Set initial colors
    CustomTheme initialColors;
    initialColors.windowBgColor = Color(0.1f, 0.2f, 0.3f, 1.0f);
    useCase.updateQuickOnlineThemeColors(initialColors);
    
    // Update with new colors
    CustomTheme newColors;
    newColors.windowBgColor = Color(0.5f, 0.6f, 0.7f, 1.0f);
    newColors.textColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    useCase.updateQuickOnlineThemeColors(newColors);
    
    CustomTheme retrieved = useCase.getQuickOnlineTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.5f);
    REQUIRE(retrieved.windowBgColor.g == 0.6f);
    REQUIRE(retrieved.windowBgColor.b == 0.7f);
    REQUIRE(retrieved.textColor.r == 1.0f);
}

TEST_CASE("ThemeUseCase - Save Quick Online theme", "[App][Theme][QuickOnline]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    CustomTheme colors;
    colors.windowBgColor = Color(0.3f, 0.4f, 0.5f, 1.0f);
    useCase.updateQuickOnlineThemeColors(colors);
    
    ThemeResult result = useCase.saveQuickOnlineTheme();
    REQUIRE(result.success);
    
    // Theme should still be retrievable after save
    CustomTheme retrieved = useCase.getQuickOnlineTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.3f);
}

TEST_CASE("ThemeUseCase - Update Notification theme colors", "[App][Theme][Notification]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    CustomTheme notificationColors;
    notificationColors.windowBgColor = Color(0.4f, 0.5f, 0.6f, 1.0f);
    notificationColors.textColor = Color(1.0f, 1.0f, 0.9f, 1.0f);
    notificationColors.tableBgColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    
    ThemeResult result = useCase.updateNotificationThemeColors(notificationColors);
    REQUIRE(result.success);
    
    CustomTheme retrieved = useCase.getNotificationTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.4f);
    REQUIRE(retrieved.windowBgColor.g == 0.5f);
    REQUIRE(retrieved.windowBgColor.b == 0.6f);
    REQUIRE(retrieved.textColor.r == 1.0f);
    REQUIRE(retrieved.textColor.g == 1.0f);
    REQUIRE(retrieved.textColor.b == 0.9f);
    REQUIRE(retrieved.tableBgColor.r == 0.2f);
}

TEST_CASE("ThemeUseCase - Notification theme persists across updates", "[App][Theme][Notification]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Set initial colors
    CustomTheme initialColors;
    initialColors.windowBgColor = Color(0.2f, 0.3f, 0.4f, 1.0f);
    useCase.updateNotificationThemeColors(initialColors);
    
    // Update with new colors
    CustomTheme newColors;
    newColors.windowBgColor = Color(0.6f, 0.7f, 0.8f, 1.0f);
    newColors.textColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    useCase.updateNotificationThemeColors(newColors);
    
    CustomTheme retrieved = useCase.getNotificationTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.6f);
    REQUIRE(retrieved.windowBgColor.g == 0.7f);
    REQUIRE(retrieved.windowBgColor.b == 0.8f);
    REQUIRE(retrieved.textColor.r == 0.0f);
}

TEST_CASE("ThemeUseCase - Save Notification theme", "[App][Theme][Notification]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    CustomTheme colors;
    colors.windowBgColor = Color(0.5f, 0.6f, 0.7f, 1.0f);
    useCase.updateNotificationThemeColors(colors);
    
    ThemeResult result = useCase.saveNotificationTheme();
    REQUIRE(result.success);
    
    // Theme should still be retrievable after save
    CustomTheme retrieved = useCase.getNotificationTheme();
    REQUIRE(retrieved.windowBgColor.r == 0.5f);
}

TEST_CASE("ThemeUseCase - Quick Online and Notification themes are independent", "[App][Theme][QuickOnline][Notification]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    // Set Quick Online theme
    CustomTheme quickOnlineColors;
    quickOnlineColors.windowBgColor = Color(0.1f, 0.1f, 0.1f, 1.0f);
    useCase.updateQuickOnlineThemeColors(quickOnlineColors);
    
    // Set Notification theme with different colors
    CustomTheme notificationColors;
    notificationColors.windowBgColor = Color(0.9f, 0.9f, 0.9f, 1.0f);
    useCase.updateNotificationThemeColors(notificationColors);
    
    // Verify they are independent
    CustomTheme quickOnline = useCase.getQuickOnlineTheme();
    CustomTheme notification = useCase.getNotificationTheme();
    
    REQUIRE(quickOnline.windowBgColor.r == 0.1f);
    REQUIRE(notification.windowBgColor.r == 0.9f);
    REQUIRE(quickOnline.windowBgColor.r != notification.windowBgColor.r);
}

TEST_CASE("ThemeUseCase - Background and text alpha", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    ThemeResult result1 = useCase.setBackgroundAlpha(0.5f);
    REQUIRE(result1.success);
    REQUIRE(useCase.getBackgroundAlpha() == 0.5f);
    
    ThemeResult result2 = useCase.setTextAlpha(0.75f);
    REQUIRE(result2.success);
    REQUIRE(useCase.getTextAlpha() == 0.75f);
}

TEST_CASE("ThemeUseCase - Invalid alpha values", "[App][Theme]") {
    ThemeState state;
    ThemeUseCase useCase(state);
    
    ThemeResult result1 = useCase.setBackgroundAlpha(1.5f);  // > 1.0
    REQUIRE_FALSE(result1.success);
    
    ThemeResult result2 = useCase.setBackgroundAlpha(-0.1f);  // < 0.0
    REQUIRE_FALSE(result2.success);
    
    ThemeResult result3 = useCase.setTextAlpha(2.0f);  // > 1.0
    REQUIRE_FALSE(result3.success);
}

