#include "App/State/ThemeState.h"

namespace XIFriendList {
namespace App {
namespace State {

XIFriendList::Core::MemoryStats ThemeState::getMemoryStats() const {
    size_t themeBytes = sizeof(ThemeState);
    themeBytes += presetName.capacity();
    themeBytes += customThemeName.capacity();
    
    for (const auto& theme : customThemes) {
        themeBytes += sizeof(XIFriendList::Core::CustomTheme);
        themeBytes += theme.name.capacity();
    }
    themeBytes += customThemes.capacity() * sizeof(XIFriendList::Core::CustomTheme);
    
    return XIFriendList::Core::MemoryStats(customThemes.size(), themeBytes, "Themes");
}

} // namespace State
} // namespace App
} // namespace XIFriendList

