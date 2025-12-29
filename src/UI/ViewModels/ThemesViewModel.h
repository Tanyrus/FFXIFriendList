// ViewModel for Themes window (UI layer)

#ifndef UI_THEMES_VIEW_MODEL_H
#define UI_THEMES_VIEW_MODEL_H

#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <vector>
#include <mutex>

namespace XIFriendList {
namespace UI {

// ViewModel for Themes window
// Holds UI state and provides formatted strings/flags for rendering
class ThemesViewModel {
public:
    ThemesViewModel();
    ~ThemesViewModel() = default;
    
    int getCurrentThemeIndex() const { return currentThemeIndex_; }
    void setCurrentThemeIndex(int index) { currentThemeIndex_ = index; }
    
    std::string getCurrentThemeName() const;
    bool isDefaultTheme() const { return currentThemeIndex_ == -2; }
    bool isCustomTheme() const { return currentThemeIndex_ == -1; }
    
    std::vector<std::string> getBuiltInThemeNames() const;
    int getBuiltInThemeCount() const { return 5; }  // Default + 4 built-in themes
    
    const std::vector<XIFriendList::Core::CustomTheme>& getCustomThemes() const { return customThemes_; }
    void setCustomThemes(const std::vector<XIFriendList::Core::CustomTheme>& themes) { customThemes_ = themes; }
    
    // Current custom theme name (when using custom theme)
    const std::string& getCurrentCustomThemeName() const { return currentCustomThemeName_; }
    void setCurrentCustomThemeName(const std::string& name) { currentCustomThemeName_ = name; }
    
    XIFriendList::Core::CustomTheme& getCurrentThemeColors() { return currentThemeColors_; }
    const XIFriendList::Core::CustomTheme& getCurrentThemeColors() const { return currentThemeColors_; }
    void setCurrentThemeColors(const XIFriendList::Core::CustomTheme& colors) { currentThemeColors_ = colors; }
    
    // Transparency
    float getBackgroundAlpha() const { return backgroundAlpha_; }
    float& getBackgroundAlpha() { return backgroundAlpha_; }
    void setBackgroundAlpha(float alpha) { backgroundAlpha_ = alpha; }
    
    float getTextAlpha() const { return textAlpha_; }
    float& getTextAlpha() { return textAlpha_; }
    void setTextAlpha(float alpha) { textAlpha_ = alpha; }
    
    const std::string& getNewThemeName() const { return newThemeName_; }
    std::string& getNewThemeName() { return newThemeName_; }
    void setNewThemeName(const std::string& name) { newThemeName_ = name; }
    
    bool canSaveTheme() const { return !newThemeName_.empty(); }
    bool canDeleteTheme() const { return isCustomTheme() && !customThemes_.empty(); }
    
    // Navigation
    bool canGoPrevious() const;
    bool canGoNext() const;
    
    // Color editing state
    bool isEditingColors() const { return editingColors_; }
    void setEditingColors(bool editing) { editingColors_ = editing; }
    
    std::string getCurrentPresetName() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return currentPresetName_; 
    }
    void setCurrentPresetName(const std::string& presetName) { 
        std::lock_guard<std::mutex> lock(mutex_);
        currentPresetName_ = presetName; 
    }
    std::vector<std::string> getAvailablePresets() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return availablePresets_; 
    }
    void setAvailablePresets(const std::vector<std::string>& presets) { 
        std::lock_guard<std::mutex> lock(mutex_);
        availablePresets_ = presets; 
    }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    mutable std::mutex mutex_;  // Protects preset-related state (currentPresetName_, availablePresets_)
    int currentThemeIndex_;  // -2 = default/no theme, -1 = custom, 0-3 = built-in
    std::vector<XIFriendList::Core::CustomTheme> customThemes_;
    std::string currentCustomThemeName_;  // Name of current custom theme (if currentThemeIndex_ == -1)
    XIFriendList::Core::CustomTheme currentThemeColors_;  // Current theme colors (for editing)
    float backgroundAlpha_;
    float textAlpha_;
    std::string newThemeName_;  // Name for saving new custom theme
    bool editingColors_;
    std::string currentPresetName_;  // Current theme preset name (e.g., "XIUI Default", "Classic")
    std::vector<std::string> availablePresets_;  // Available preset names
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_THEMES_VIEW_MODEL_H

