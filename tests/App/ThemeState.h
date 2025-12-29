// ThemeState.h
// Simple state struct for theme data

#ifndef APP_THEME_STATE_H
#define APP_THEME_STATE_H

#include "Core/ModelsCore.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace App {

// Simple state struct for theme data
// This is what tests use directly instead of mocking an interface
struct ThemeState {
    int themeIndex = -2;  // Default to "No Theme"
    std::string presetName = "";
    std::string customThemeName = "";
    std::vector<Core::CustomTheme> customThemes;
    float backgroundAlpha = 0.95f;
    float textAlpha = 1.0f;
    
    ThemeState() = default;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_THEME_STATE_H

