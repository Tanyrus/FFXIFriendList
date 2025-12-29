
#ifndef APP_THEME_STATE_H
#define APP_THEME_STATE_H

#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace App {
namespace State {

struct ThemeState {
    int themeIndex = 0;  // Default to Warm Brown
    std::string presetName = "";
    std::string customThemeName = "";
    std::vector<XIFriendList::Core::CustomTheme> customThemes;
    float backgroundAlpha = 0.95f;
    float textAlpha = 1.0f;
    
    ThemeState() = default;
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;
};

} // namespace State
} // namespace App
} // namespace XIFriendList

#endif // APP_THEME_STATE_H

