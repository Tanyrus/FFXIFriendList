
#ifndef APP_THEMING_USE_CASES_H
#define APP_THEMING_USE_CASES_H

#include "App/State/ThemeState.h"
#include "App/Theming/ThemeTokens.h"
#include "Core/ModelsCore.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct ThemeResult {
    bool success;
    std::string error;
    std::string actualName;  // Actual theme name used (may differ from input if suffix was added)
    
    ThemeResult() : success(false) {}
    ThemeResult(bool success, const std::string& error = "", const std::string& actualName = "") 
        : success(success), error(error), actualName(actualName) {}
};

class ThemeUseCase {
public:
    explicit ThemeUseCase(XIFriendList::App::State::ThemeState& state);
    
    int getCurrentThemeIndex() const { return currentThemeIndex_; }
    bool isDefaultTheme() const { 
        if (currentPresetName_ == "XIUI Default") {
            return false;
        }
        return currentThemeIndex_ == -2; 
    }
    ThemeResult setTheme(int themeIndex);
    ThemeResult setCustomTheme(const std::string& themeName);
    
    XIFriendList::Core::CustomTheme getCurrentCustomTheme() const;
    XIFriendList::App::Theming::ThemeTokens getCurrentThemeTokens() const;
    std::string getCurrentCustomThemeName() const { return currentCustomThemeName_; }
    std::vector<XIFriendList::Core::CustomTheme> getCustomThemes() const { return customThemes_; }
    XIFriendList::Core::CustomTheme getBuiltInTheme(XIFriendList::Core::BuiltInTheme theme) const;
    XIFriendList::App::Theming::ThemeTokens createXIUIDefaultTheme() const;
    ThemeResult saveCustomTheme(const std::string& themeName, const XIFriendList::Core::CustomTheme& theme);
    ThemeResult deleteCustomTheme(const std::string& themeName);
    float getBackgroundAlpha() const { return backgroundAlpha_; }
    ThemeResult setBackgroundAlpha(float alpha);
    ThemeResult saveBackgroundAlpha();
    float getTextAlpha() const { return textAlpha_; }
    ThemeResult setTextAlpha(float alpha);
    ThemeResult saveTextAlpha();
    ThemeResult updateCurrentThemeColors(const XIFriendList::Core::CustomTheme& colors);
    void loadThemes();
    void saveThemes();
    
    std::vector<std::string> getAvailablePresets() const;
    std::string getCurrentPresetName() const { return currentPresetName_; }
    ThemeResult setThemePreset(const std::string& presetName);
    
    XIFriendList::Core::CustomTheme getQuickOnlineTheme() const { return quickOnlineTheme_; }
    ThemeResult updateQuickOnlineThemeColors(const XIFriendList::Core::CustomTheme& colors);
    ThemeResult saveQuickOnlineTheme();
    
    XIFriendList::Core::CustomTheme getNotificationTheme() const { return notificationTheme_; }
    ThemeResult updateNotificationThemeColors(const XIFriendList::Core::CustomTheme& colors);
    ThemeResult saveNotificationTheme();

private:
    XIFriendList::App::State::ThemeState& state_;  // Direct reference to state (no interface)
    int currentThemeIndex_;  // -1 = custom, 0-3 = built-in, -2 = default/no theme
    std::string currentCustomThemeName_;  // Name of current custom theme (if currentThemeIndex_ == -1)
    std::string currentPresetName_;  // Current preset name (e.g., "XIUI Default", "Classic")
    std::vector<XIFriendList::Core::CustomTheme> customThemes_;
    XIFriendList::Core::CustomTheme currentCustomTheme_;  // Current custom theme colors (if using custom)
    bool isEditingBuiltInTheme_;  // True if editing a built-in theme (currentCustomTheme_ contains edited colors)
    float backgroundAlpha_;
    float textAlpha_;
    
    XIFriendList::Core::CustomTheme quickOnlineTheme_;
    XIFriendList::Core::CustomTheme notificationTheme_;
    
    void initializeBuiltInTheme(XIFriendList::Core::BuiltInTheme theme, XIFriendList::Core::CustomTheme& outTheme) const;
    
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_THEMING_USE_CASES_H

