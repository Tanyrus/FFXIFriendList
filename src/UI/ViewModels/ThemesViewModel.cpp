// ViewModel for Themes window implementation

#include "UI/ViewModels/ThemesViewModel.h"
#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"

namespace XIFriendList {
namespace UI {

ThemesViewModel::ThemesViewModel()
    : currentThemeIndex_(-2)  // Default to "No Theme" (ImGui defaults)
    , backgroundAlpha_(0.95f)
    , textAlpha_(1.0f)
    , editingColors_(false)
{
}

std::string ThemesViewModel::getCurrentThemeName() const {
    if (currentThemeIndex_ == -2) {
        return XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::Default);
    } else if (currentThemeIndex_ == -1) {
        if (!currentCustomThemeName_.empty()) {
            return currentCustomThemeName_;
        }
        for (const auto& theme : customThemes_) {
            if (!theme.name.empty()) {
                return theme.name;
            }
        }
        return "Custom";
    } else {
        return XIFriendList::Core::getBuiltInThemeName(static_cast<XIFriendList::Core::BuiltInTheme>(currentThemeIndex_));
    }
}

std::vector<std::string> ThemesViewModel::getBuiltInThemeNames() const {
    return {
        XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::Default),
        XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::FFXIClassic),
        XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::ModernDark),
        XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::GreenNature),
        XIFriendList::Core::getBuiltInThemeName(XIFriendList::Core::BuiltInTheme::PurpleMystic)
    };
}

bool ThemesViewModel::canGoPrevious() const {
    if (currentThemeIndex_ == -1) {
        return true;
    }
    // Can go previous if not already at Default (-2)
    return currentThemeIndex_ > -2;
}

bool ThemesViewModel::canGoNext() const {
    if (currentThemeIndex_ == -2) {
        return true;
    }
    if (currentThemeIndex_ == -1) {
        return false;
    }
    if (currentThemeIndex_ == 3) {
        // Last built-in - can go to custom if custom themes exist
        return !customThemes_.empty();
    }
    return true;
}

XIFriendList::Core::MemoryStats ThemesViewModel::getMemoryStats() const {
    size_t bytes = sizeof(ThemesViewModel);
    
    for (const auto& theme : customThemes_) {
        bytes += sizeof(XIFriendList::Core::CustomTheme);
        bytes += theme.name.capacity();
    }
    bytes += customThemes_.capacity() * sizeof(XIFriendList::Core::CustomTheme);
    
    bytes += currentCustomThemeName_.capacity();
    bytes += newThemeName_.capacity();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bytes += currentPresetName_.capacity();
        for (const auto& preset : availablePresets_) {
            bytes += preset.capacity();
        }
        bytes += availablePresets_.capacity() * sizeof(std::string);
    }
    
    bytes += sizeof(XIFriendList::Core::CustomTheme);
    
    size_t count = customThemes_.size();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        count += availablePresets_.size();
    }
    
    return XIFriendList::Core::MemoryStats(count, bytes, "Themes ViewModel");
}

} // namespace UI
} // namespace XIFriendList

