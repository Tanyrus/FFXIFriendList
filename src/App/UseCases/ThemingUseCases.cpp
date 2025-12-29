#include "App/UseCases/ThemingUseCases.h"
#include "App/Theming/ThemeTokens.h"
#include <algorithm>

namespace XIFriendList {
namespace App {
namespace UseCases {

ThemeUseCase::ThemeUseCase(XIFriendList::App::State::ThemeState& state)
    : state_(state)
    , currentThemeIndex_(-2)
    , currentPresetName_("")
    , isEditingBuiltInTheme_(false)
    , backgroundAlpha_(0.95f)
    , textAlpha_(1.0f)
{
    loadThemes();
}

ThemeResult ThemeUseCase::setTheme(int themeIndex) {
    if (themeIndex < -2 || themeIndex > 3) {
        return ThemeResult(false, "Invalid theme index");
    }
    
    if (currentThemeIndex_ != themeIndex) {
        isEditingBuiltInTheme_ = false;
    }
    
    currentThemeIndex_ = themeIndex;
    if (themeIndex == -1) {
        if (currentCustomThemeName_.empty()) {
            return ThemeResult(false, "No custom theme selected");
        }
    }
    
    if (themeIndex >= 0 && themeIndex <= 3) {
        currentPresetName_ = "";
    }
    
    saveThemes();
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::setCustomTheme(const std::string& themeName) {
    auto it = std::find_if(customThemes_.begin(), customThemes_.end(),
        [&themeName](const XIFriendList::Core::CustomTheme& theme) {
            return theme.name == themeName;
        });
    
    if (it == customThemes_.end()) {
        return ThemeResult(false, "Custom theme not found: " + themeName);
    }
    
    currentThemeIndex_ = -1;
    currentCustomThemeName_ = themeName;
    currentCustomTheme_ = *it;
    isEditingBuiltInTheme_ = false;
    currentPresetName_ = "";
    
    saveThemes();
    return ThemeResult(true);
}

XIFriendList::Core::CustomTheme ThemeUseCase::getCurrentCustomTheme() const {
    if (currentPresetName_ == "XIUI Default" && currentThemeIndex_ == -2) {
        if (!(currentCustomTheme_.windowBgColor.r == 0.0f && currentCustomTheme_.windowBgColor.g == 0.0f && 
              currentCustomTheme_.windowBgColor.b == 0.0f && currentCustomTheme_.windowBgColor.a == 1.0f)) {
            XIFriendList::Core::CustomTheme theme = currentCustomTheme_;
            theme.name = "XIUI Default";
            return theme;
        }
        
        XIFriendList::App::Theming::ThemeTokens tokens = createXIUIDefaultTheme();
        XIFriendList::Core::CustomTheme theme;
        theme.name = "XIUI Default";
        theme.windowBgColor = tokens.windowBgColor;
        theme.childBgColor = tokens.childBgColor;
        theme.frameBgColor = tokens.frameBgColor;
        theme.frameBgHovered = tokens.frameBgHovered;
        theme.frameBgActive = tokens.frameBgActive;
        theme.titleBg = tokens.titleBg;
        theme.titleBgActive = tokens.titleBgActive;
        theme.titleBgCollapsed = tokens.titleBgCollapsed;
        theme.buttonColor = tokens.buttonColor;
        theme.buttonHoverColor = tokens.buttonHoverColor;
        theme.buttonActiveColor = tokens.buttonActiveColor;
        theme.separatorColor = tokens.separatorColor;
        theme.separatorHovered = tokens.separatorHovered;
        theme.separatorActive = tokens.separatorActive;
        theme.scrollbarBg = tokens.scrollbarBg;
        theme.scrollbarGrab = tokens.scrollbarGrab;
        theme.scrollbarGrabHovered = tokens.scrollbarGrabHovered;
        theme.scrollbarGrabActive = tokens.scrollbarGrabActive;
        theme.checkMark = tokens.checkMark;
        theme.sliderGrab = tokens.sliderGrab;
        theme.sliderGrabActive = tokens.sliderGrabActive;
        theme.header = tokens.header;
        theme.headerHovered = tokens.headerHovered;
        theme.headerActive = tokens.headerActive;
        theme.textColor = tokens.textColor;
        theme.textDisabled = tokens.textDisabled;
        theme.tableBgColor = XIFriendList::Core::Color(
            tokens.childBgColor.r * 1.1f,
            tokens.childBgColor.g * 1.1f,
            tokens.childBgColor.b * 1.1f,
            tokens.childBgColor.a
        );
        return theme;
    }
    
    if (currentThemeIndex_ == -2) {
        return XIFriendList::Core::CustomTheme();
    } else if (currentThemeIndex_ == -1) {
        return currentCustomTheme_;
    } else {
        if (isEditingBuiltInTheme_) {
            return currentCustomTheme_;
        } else {
            return getBuiltInTheme(static_cast<XIFriendList::Core::BuiltInTheme>(currentThemeIndex_));
        }
    }
}

XIFriendList::Core::CustomTheme ThemeUseCase::getBuiltInTheme(XIFriendList::Core::BuiltInTheme theme) const {
    XIFriendList::Core::CustomTheme customTheme;
    initializeBuiltInTheme(theme, customTheme);
    return customTheme;
}

XIFriendList::App::Theming::ThemeTokens ThemeUseCase::getCurrentThemeTokens() const {
    if (currentPresetName_ == "XIUI Default") {
        XIFriendList::Core::CustomTheme theme = getCurrentCustomTheme();
        
        if (!(theme.windowBgColor.r == 0.0f && theme.windowBgColor.g == 0.0f && 
              theme.windowBgColor.b == 0.0f && theme.windowBgColor.a == 1.0f)) {
            XIFriendList::App::Theming::ThemeTokens tokens;
            tokens.windowBgColor = theme.windowBgColor;
            tokens.childBgColor = theme.childBgColor;
            tokens.frameBgColor = theme.frameBgColor;
            tokens.frameBgHovered = theme.frameBgHovered;
            tokens.frameBgActive = theme.frameBgActive;
            tokens.titleBg = theme.titleBg;
            tokens.titleBgActive = theme.titleBgActive;
            tokens.titleBgCollapsed = theme.titleBgCollapsed;
            tokens.buttonColor = theme.buttonColor;
            tokens.buttonHoverColor = theme.buttonHoverColor;
            tokens.buttonActiveColor = theme.buttonActiveColor;
            tokens.separatorColor = theme.separatorColor;
            tokens.separatorHovered = theme.separatorHovered;
            tokens.separatorActive = theme.separatorActive;
            tokens.scrollbarBg = theme.scrollbarBg;
            tokens.scrollbarGrab = theme.scrollbarGrab;
            tokens.scrollbarGrabHovered = theme.scrollbarGrabHovered;
            tokens.scrollbarGrabActive = theme.scrollbarGrabActive;
            tokens.checkMark = theme.checkMark;
            tokens.sliderGrab = theme.sliderGrab;
            tokens.sliderGrabActive = theme.sliderGrabActive;
            tokens.header = theme.header;
            tokens.headerHovered = theme.headerHovered;
            tokens.headerActive = theme.headerActive;
            tokens.textColor = theme.textColor;
            tokens.textDisabled = theme.textDisabled;
            tokens.backgroundAlpha = backgroundAlpha_;
            tokens.textAlpha = textAlpha_;
            tokens.presetName = "XIUI Default";
            return tokens;
        }
        
        XIFriendList::App::Theming::ThemeTokens tokens = createXIUIDefaultTheme();
        tokens.backgroundAlpha = backgroundAlpha_;
        tokens.textAlpha = textAlpha_;
        return tokens;
    }
    
    XIFriendList::App::Theming::ThemeTokens tokens;
    
    XIFriendList::Core::CustomTheme theme = getCurrentCustomTheme();
    
    if (currentPresetName_ == "Classic" && currentThemeIndex_ == -2) {
        theme = getBuiltInTheme(XIFriendList::Core::BuiltInTheme::FFXIClassic);
    }
    
    tokens.windowBgColor = theme.windowBgColor;
    tokens.childBgColor = theme.childBgColor;
    tokens.frameBgColor = theme.frameBgColor;
    tokens.frameBgHovered = theme.frameBgHovered;
    tokens.frameBgActive = theme.frameBgActive;
    tokens.titleBg = theme.titleBg;
    tokens.titleBgActive = theme.titleBgActive;
    tokens.titleBgCollapsed = theme.titleBgCollapsed;
    tokens.buttonColor = theme.buttonColor;
    tokens.buttonHoverColor = theme.buttonHoverColor;
    tokens.buttonActiveColor = theme.buttonActiveColor;
    tokens.separatorColor = theme.separatorColor;
    tokens.separatorHovered = theme.separatorHovered;
    tokens.separatorActive = theme.separatorActive;
    tokens.scrollbarBg = theme.scrollbarBg;
    tokens.scrollbarGrab = theme.scrollbarGrab;
    tokens.scrollbarGrabHovered = theme.scrollbarGrabHovered;
    tokens.scrollbarGrabActive = theme.scrollbarGrabActive;
    tokens.checkMark = theme.checkMark;
    tokens.sliderGrab = theme.sliderGrab;
    tokens.sliderGrabActive = theme.sliderGrabActive;
    tokens.header = theme.header;
    tokens.headerHovered = theme.headerHovered;
    tokens.headerActive = theme.headerActive;
    tokens.textColor = theme.textColor;
    tokens.textDisabled = theme.textDisabled;
    tokens.borderColor = theme.separatorColor;
    
    if (currentPresetName_ == "XIUI Default") {
        tokens.windowPadding = XIFriendList::App::Theming::Vec2(10.0f, 10.0f);
        tokens.windowRounding = 6.0f;
        tokens.framePadding = XIFriendList::App::Theming::Vec2(4.0f, 2.0f);
        tokens.frameRounding = 3.0f;
        tokens.itemSpacing = XIFriendList::App::Theming::Vec2(8.0f, 4.0f);
        tokens.itemInnerSpacing = XIFriendList::App::Theming::Vec2(4.0f, 3.0f);
    } else {
        tokens.windowPadding = XIFriendList::App::Theming::Vec2(12.0f, 12.0f);
        tokens.windowRounding = 6.0f;
        tokens.framePadding = XIFriendList::App::Theming::Vec2(6.0f, 3.0f);
        tokens.frameRounding = 3.0f;
        tokens.itemSpacing = XIFriendList::App::Theming::Vec2(6.0f, 4.0f);
        tokens.itemInnerSpacing = XIFriendList::App::Theming::Vec2(4.0f, 3.0f);
    }
    tokens.scrollbarSize = 12.0f;
    tokens.scrollbarRounding = 3.0f;
    tokens.grabRounding = 3.0f;
    
    tokens.backgroundAlpha = backgroundAlpha_;
    tokens.textAlpha = textAlpha_;
    
    
    if (!currentPresetName_.empty()) {
        tokens.presetName = currentPresetName_;
    } else {
        if (currentThemeIndex_ == -2) {
            tokens.presetName = "Default";
        } else if (currentThemeIndex_ == -1) {
            tokens.presetName = currentCustomThemeName_.empty() ? "Custom" : currentCustomThemeName_;
        } else {
            switch (currentThemeIndex_) {
                case 0: tokens.presetName = "Warm Brown"; break;
                case 1: tokens.presetName = "Modern Dark"; break;
                case 2: tokens.presetName = "Green Nature"; break;
                case 3: tokens.presetName = "Purple Mystic"; break;
                default: tokens.presetName = "Unknown"; break;
            }
        }
    }
    
    return tokens;
}

ThemeResult ThemeUseCase::saveCustomTheme(const std::string& themeName, const XIFriendList::Core::CustomTheme& theme) {
    std::vector<std::string> presetNames = {
        "XIUI Default",
        "Classic",
        "Modern Dark",
        "Green Nature",
        "Purple Mystic"
    };
    
    std::string actualThemeName = themeName;
    std::string baseName = themeName;
    
    bool isPresetName = false;
    if (themeName.empty()) {
        if (!currentPresetName_.empty()) {
            baseName = currentPresetName_;
        } else if (currentThemeIndex_ >= 0 && currentThemeIndex_ <= 3) {
            switch (currentThemeIndex_) {
                case 0: baseName = "Warm Brown"; break;
                case 1: baseName = "Modern Dark"; break;
                case 2: baseName = "Green Nature"; break;
                case 3: baseName = "Purple Mystic"; break;
                default: baseName = "Custom Theme"; break;
            }
        } else {
            baseName = "Custom Theme";
        }
        isPresetName = true;
    } else {
        for (const auto& preset : presetNames) {
            if (themeName == preset) {
                baseName = preset;
                isPresetName = true;
                break;
            }
        }
    }
    
    if (isPresetName) {
        int suffix = 1;
        actualThemeName = baseName + " (" + std::to_string(suffix) + ")";
        
        while (std::find_if(customThemes_.begin(), customThemes_.end(),
                [&actualThemeName](const XIFriendList::Core::CustomTheme& t) {
                    return t.name == actualThemeName;
                }) != customThemes_.end()) {
            suffix++;
            actualThemeName = baseName + " (" + std::to_string(suffix) + ")";
        }
    } else {
        for (const auto& preset : presetNames) {
            if (themeName == preset) {
                int suffix = 1;
                actualThemeName = preset + " (" + std::to_string(suffix) + ")";
                while (std::find_if(customThemes_.begin(), customThemes_.end(),
                        [&actualThemeName](const XIFriendList::Core::CustomTheme& t) {
                            return t.name == actualThemeName;
                        }) != customThemes_.end()) {
                    suffix++;
                    actualThemeName = preset + " (" + std::to_string(suffix) + ")";
                }
                break;
            }
        }
    }
    
    auto it = std::find_if(customThemes_.begin(), customThemes_.end(),
        [&actualThemeName](const XIFriendList::Core::CustomTheme& t) {
            return t.name == actualThemeName;
        });
    
    XIFriendList::Core::CustomTheme newTheme = theme;
    newTheme.name = actualThemeName;
    
    if (it != customThemes_.end() && !isPresetName) {
        *it = newTheme;
    } else {
        customThemes_.push_back(newTheme);
    }
    
    if (currentThemeIndex_ == -1 && currentCustomThemeName_ == actualThemeName) {
        currentCustomTheme_ = newTheme;
    }
    
    saveThemes();
    return ThemeResult(true, "", actualThemeName);
}

ThemeResult ThemeUseCase::deleteCustomTheme(const std::string& themeName) {
    auto it = std::find_if(customThemes_.begin(), customThemes_.end(),
        [&themeName](const XIFriendList::Core::CustomTheme& theme) {
            return theme.name == themeName;
        });
    
    if (it == customThemes_.end()) {
        return ThemeResult(false, "Custom theme not found: " + themeName);
    }
    
    if (currentThemeIndex_ == -1 && currentCustomThemeName_ == themeName) {
        currentThemeIndex_ = -2;  // Switch to "No Theme" (ImGui defaults)
        currentCustomThemeName_.clear();
    }
    
    customThemes_.erase(it);
    saveThemes();
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::setBackgroundAlpha(float alpha) {
    if (alpha < 0.0f || alpha > 1.0f) {
        return ThemeResult(false, "Alpha must be between 0.0 and 1.0");
    }
    
    backgroundAlpha_ = alpha;
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::saveBackgroundAlpha() {
    state_.backgroundAlpha = backgroundAlpha_;
    saveThemes();
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::setTextAlpha(float alpha) {
    if (alpha < 0.0f || alpha > 1.0f) {
        return ThemeResult(false, "Alpha must be between 0.0 and 1.0");
    }
    
    textAlpha_ = alpha;
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::saveTextAlpha() {
    state_.textAlpha = textAlpha_;
    saveThemes();
    return ThemeResult(true);
}


ThemeResult ThemeUseCase::updateCurrentThemeColors(const XIFriendList::Core::CustomTheme& colors) {
    bool isXIUIDefault = (currentPresetName_ == "XIUI Default" && currentThemeIndex_ == -2);
    
    if (currentThemeIndex_ == -2 && !isXIUIDefault) {
        return ThemeResult(false, "Cannot update colors for default theme");
    }
    
    currentCustomTheme_ = colors;
    if (currentThemeIndex_ == -1 && !currentCustomThemeName_.empty()) {
        currentCustomTheme_.name = currentCustomThemeName_;
    } else if (isXIUIDefault) {
        currentCustomTheme_.name = "XIUI Default";
    }
    
    if (currentThemeIndex_ >= 0 && currentThemeIndex_ <= 3) {
        isEditingBuiltInTheme_ = true;
    }
    
    return ThemeResult(true);
}

void ThemeUseCase::loadThemes() {
    currentThemeIndex_ = state_.themeIndex;
    if (currentThemeIndex_ < -2 || currentThemeIndex_ > 3) {
        currentThemeIndex_ = 0;  // Default to Warm Brown
    }
    
    currentPresetName_ = state_.presetName;
    
    customThemes_ = state_.customThemes;
    
    customThemes_.erase(
        std::remove_if(customThemes_.begin(), customThemes_.end(),
            [](const XIFriendList::Core::CustomTheme& t) {
                return t.name == "__XIUI_Default_Modified__";
            }),
        customThemes_.end());
    
    if (currentPresetName_.empty()) {
        if (currentThemeIndex_ == -2) {
            currentPresetName_ = "XIUI Default";
            state_.presetName = currentPresetName_;
        } else if (currentThemeIndex_ == -1) {
            currentPresetName_ = "Classic";
            state_.presetName = currentPresetName_;
        }
        // For built-in themes (0-3), keep presetName empty so combo box can match by theme index
    }
    
    if (currentThemeIndex_ == -1 && !customThemes_.empty()) {
        std::string savedThemeName = state_.customThemeName;
        
        if (!savedThemeName.empty()) {
            auto it = std::find_if(customThemes_.begin(), customThemes_.end(),
                [&savedThemeName](const XIFriendList::Core::CustomTheme& theme) {
                    return theme.name == savedThemeName;
                });
            
            if (it != customThemes_.end()) {
                currentCustomThemeName_ = it->name;
                currentCustomTheme_ = *it;
            } else {
                currentCustomThemeName_ = customThemes_[0].name;
                currentCustomTheme_ = customThemes_[0];
            }
        } else {
            currentCustomThemeName_ = customThemes_[0].name;
            currentCustomTheme_ = customThemes_[0];
        }
    }
    
    isEditingBuiltInTheme_ = false;
    
    backgroundAlpha_ = state_.backgroundAlpha;
    if (backgroundAlpha_ < 0.0f || backgroundAlpha_ > 1.0f) {
        backgroundAlpha_ = 0.95f;  // Default
    }
    
    textAlpha_ = state_.textAlpha;
    if (textAlpha_ < 0.0f || textAlpha_ > 1.0f) {
        textAlpha_ = 1.0f;  // Default
    }
    
    XIFriendList::Core::CustomTheme mainTheme = getCurrentCustomTheme();
    quickOnlineTheme_ = mainTheme;
    notificationTheme_ = mainTheme;
}

void ThemeUseCase::saveThemes() {
    state_.themeIndex = currentThemeIndex_;
    
    state_.presetName = currentPresetName_;
    
    if (currentThemeIndex_ == -1 && !currentCustomThemeName_.empty()) {
        state_.customThemeName = currentCustomThemeName_;
    } else {
        state_.customThemeName = "";
    }
    
    state_.customThemes = customThemes_;
    state_.backgroundAlpha = backgroundAlpha_;
    state_.textAlpha = textAlpha_;
}

static void applyThemeFromPalette(
    const XIFriendList::Core::Color& bgDark,
    const XIFriendList::Core::Color& bgMedium,
    const XIFriendList::Core::Color& bgLight,
    const XIFriendList::Core::Color& bgLighter,
    const XIFriendList::Core::Color& accent,
    const XIFriendList::Core::Color& accentDark,
    const XIFriendList::Core::Color& accentDarker,
    const XIFriendList::Core::Color& borderDark,
    const XIFriendList::Core::Color& textLight,
    const XIFriendList::Core::Color& textMuted,
    const XIFriendList::Core::Color& childBg,
    XIFriendList::Core::CustomTheme& outTheme) {
    
    outTheme.windowBgColor = bgDark;
    outTheme.childBgColor = childBg;
    outTheme.titleBg = bgMedium;
    outTheme.titleBgActive = bgLight;
    outTheme.titleBgCollapsed = bgDark;
    outTheme.frameBgColor = bgMedium;
    outTheme.frameBgHovered = bgLight;
    outTheme.frameBgActive = bgLighter;
    outTheme.header = bgLight;
    outTheme.headerHovered = bgLighter;
    outTheme.headerActive = XIFriendList::Core::Color(accent.r, accent.g, accent.b, 0.3f);
    
    outTheme.buttonColor = bgMedium;
    outTheme.buttonHoverColor = bgLight;
    outTheme.buttonActiveColor = bgLighter;
    
    outTheme.textColor = textLight;
    outTheme.textDisabled = textMuted;
    
    outTheme.scrollbarBg = bgMedium;
    outTheme.scrollbarGrab = bgLighter;
    outTheme.scrollbarGrabHovered = borderDark;
    outTheme.scrollbarGrabActive = accentDark;
    
    outTheme.separatorColor = borderDark;
    outTheme.separatorHovered = borderDark;
    outTheme.separatorActive = borderDark;
    
    outTheme.checkMark = accent;
    outTheme.sliderGrab = accentDark;
    outTheme.sliderGrabActive = accent;
    
    outTheme.tableBgColor = XIFriendList::Core::Color(
        childBg.r * 1.1f,
        childBg.g * 1.1f,
        childBg.b * 1.1f,
        childBg.a
    );
}

void ThemeUseCase::initializeBuiltInTheme(XIFriendList::Core::BuiltInTheme theme, XIFriendList::Core::CustomTheme& outTheme) const {
    switch (theme) {
        case XIFriendList::Core::BuiltInTheme::Default: {
            break;
        }
        case XIFriendList::Core::BuiltInTheme::FFXIClassic: {
            XIFriendList::Core::Color bgDark(0.20f, 0.16f, 0.14f, 0.95f);      // Deep brown-black
            XIFriendList::Core::Color bgMedium(0.30f, 0.24f, 0.20f, 1.0f);     // Warm medium brown
            XIFriendList::Core::Color bgLight(0.40f, 0.32f, 0.26f, 1.0f);      // Lighter brown
            XIFriendList::Core::Color bgLighter(0.50f, 0.40f, 0.32f, 1.0f);    // Lightest brown
            XIFriendList::Core::Color accent(0.85f, 0.70f, 0.45f, 1.0f);       // Gold accent
            XIFriendList::Core::Color accentDark(0.70f, 0.58f, 0.38f, 1.0f);   // Darker gold
            XIFriendList::Core::Color accentDarker(0.60f, 0.50f, 0.32f, 1.0f); // Darkest gold
            XIFriendList::Core::Color borderDark(0.50f, 0.40f, 0.30f, 1.0f);   // Brown border
            XIFriendList::Core::Color textLight(0.95f, 0.90f, 0.85f, 1.0f);    // Warm off-white
            XIFriendList::Core::Color textMuted(0.70f, 0.65f, 0.60f, 1.0f);    // Muted brown
            XIFriendList::Core::Color childBg(0.0f, 0.0f, 0.0f, 0.0f);         // Transparent (matches XIUI Default)
            
            applyThemeFromPalette(bgDark, bgMedium, bgLight, bgLighter, accent, accentDark, accentDarker, 
                                 borderDark, textLight, textMuted, childBg, outTheme);
            break;
        }
        case XIFriendList::Core::BuiltInTheme::ModernDark: {
            XIFriendList::Core::Color bgDark(0.08f, 0.08f, 0.12f, 0.95f);      // Deep blue-black
            XIFriendList::Core::Color bgMedium(0.12f, 0.12f, 0.18f, 1.0f);     // Medium dark blue
            XIFriendList::Core::Color bgLight(0.18f, 0.20f, 0.28f, 1.0f);      // Lighter blue
            XIFriendList::Core::Color bgLighter(0.24f, 0.28f, 0.38f, 1.0f);    // Lightest blue
            XIFriendList::Core::Color accent(0.40f, 0.70f, 1.0f, 1.0f);       // Bright cyan accent
            XIFriendList::Core::Color accentDark(0.30f, 0.50f, 0.80f, 1.0f);   // Darker cyan
            XIFriendList::Core::Color accentDarker(0.20f, 0.40f, 0.70f, 1.0f);  // Darkest cyan
            XIFriendList::Core::Color borderDark(0.20f, 0.25f, 0.35f, 1.0f);   // Blue border
            XIFriendList::Core::Color textLight(0.90f, 0.90f, 0.95f, 1.0f);    // Cool off-white
            XIFriendList::Core::Color textMuted(0.60f, 0.65f, 0.70f, 1.0f);    // Muted blue-gray
            XIFriendList::Core::Color childBg(0.0f, 0.0f, 0.0f, 0.0f);         // Transparent (matches XIUI Default)
            
            applyThemeFromPalette(bgDark, bgMedium, bgLight, bgLighter, accent, accentDark, accentDarker, 
                                 borderDark, textLight, textMuted, childBg, outTheme);
            break;
        }
        case XIFriendList::Core::BuiltInTheme::GreenNature: {
            XIFriendList::Core::Color bgDark(0.10f, 0.15f, 0.12f, 0.95f);      // Deep green-black
            XIFriendList::Core::Color bgMedium(0.15f, 0.22f, 0.18f, 1.0f);     // Medium dark green
            XIFriendList::Core::Color bgLight(0.20f, 0.30f, 0.24f, 1.0f);      // Lighter green
            XIFriendList::Core::Color bgLighter(0.25f, 0.38f, 0.30f, 1.0f);    // Lightest green
            XIFriendList::Core::Color accent(0.40f, 0.80f, 0.50f, 1.0f);       // Bright green accent
            XIFriendList::Core::Color accentDark(0.35f, 0.60f, 0.40f, 1.0f);   // Darker green
            XIFriendList::Core::Color accentDarker(0.25f, 0.50f, 0.30f, 1.0f); // Darkest green
            XIFriendList::Core::Color borderDark(0.25f, 0.35f, 0.28f, 1.0f);    // Green border
            XIFriendList::Core::Color textLight(0.85f, 0.95f, 0.88f, 1.0f);     // Cool green-tinted white
            XIFriendList::Core::Color textMuted(0.60f, 0.70f, 0.65f, 1.0f);    // Muted green-gray
            XIFriendList::Core::Color childBg(0.0f, 0.0f, 0.0f, 0.0f);         // Transparent (matches XIUI Default)
            
            applyThemeFromPalette(bgDark, bgMedium, bgLight, bgLighter, accent, accentDark, accentDarker, 
                                 borderDark, textLight, textMuted, childBg, outTheme);
            break;
        }
        case XIFriendList::Core::BuiltInTheme::PurpleMystic: {
            XIFriendList::Core::Color bgDark(0.12f, 0.10f, 0.18f, 0.95f);      // Deep purple-black
            XIFriendList::Core::Color bgMedium(0.18f, 0.15f, 0.25f, 1.0f);     // Medium dark purple
            XIFriendList::Core::Color bgLight(0.24f, 0.20f, 0.32f, 1.0f);      // Lighter purple
            XIFriendList::Core::Color bgLighter(0.30f, 0.25f, 0.40f, 1.0f);    // Lightest purple
            XIFriendList::Core::Color accent(0.80f, 0.60f, 0.95f, 1.0f);       // Bright purple accent
            XIFriendList::Core::Color accentDark(0.60f, 0.45f, 0.75f, 1.0f);   // Darker purple
            XIFriendList::Core::Color accentDarker(0.50f, 0.35f, 0.65f, 1.0f); // Darkest purple
            XIFriendList::Core::Color borderDark(0.30f, 0.25f, 0.38f, 1.0f);   // Purple border
            XIFriendList::Core::Color textLight(0.95f, 0.90f, 0.98f, 1.0f);     // Cool purple-tinted white
            XIFriendList::Core::Color textMuted(0.70f, 0.65f, 0.75f, 1.0f);     // Muted purple-gray
            XIFriendList::Core::Color childBg(0.0f, 0.0f, 0.0f, 0.0f);         // Transparent (matches XIUI Default)
            
            applyThemeFromPalette(bgDark, bgMedium, bgLight, bgLighter, accent, accentDark, accentDarker, 
                                 borderDark, textLight, textMuted, childBg, outTheme);
            break;
        }
    }
}

XIFriendList::App::Theming::ThemeTokens ThemeUseCase::createXIUIDefaultTheme() const {
    XIFriendList::App::Theming::ThemeTokens tokens;
    
    tokens.presetName = "XIUI Default";
    
    
    XIFriendList::Core::Color bgDark(0.051f, 0.051f, 0.051f, 0.95f);
    XIFriendList::Core::Color bgMedium(0.098f, 0.090f, 0.075f, 1.0f);
    XIFriendList::Core::Color bgLight(0.137f, 0.125f, 0.106f, 1.0f);
    XIFriendList::Core::Color bgLighter(0.176f, 0.161f, 0.137f, 1.0f);
    
    XIFriendList::Core::Color gold(0.957f, 0.855f, 0.592f, 1.0f);
    XIFriendList::Core::Color goldDark(0.765f, 0.684f, 0.474f, 1.0f);
    XIFriendList::Core::Color goldDarker(0.573f, 0.512f, 0.355f, 1.0f);
    
    XIFriendList::Core::Color borderDark(0.3f, 0.275f, 0.235f, 1.0f);
    
    XIFriendList::Core::Color textLight(0.878f, 0.855f, 0.812f, 1.0f);
    XIFriendList::Core::Color textMuted(0.6f, 0.58f, 0.54f, 1.0f);
    
    tokens.windowBgColor = bgDark;  // ImGuiCol_WindowBg
    tokens.childBgColor = XIFriendList::Core::Color(0.0f, 0.0f, 0.0f, 0.0f);  // ImGuiCol_ChildBg: {0, 0, 0, 0}
    tokens.titleBg = bgMedium;  // ImGuiCol_TitleBg
    tokens.titleBgActive = bgLight;  // ImGuiCol_TitleBgActive
    tokens.titleBgCollapsed = bgDark;  // ImGuiCol_TitleBgCollapsed
    tokens.frameBgColor = bgMedium;  // ImGuiCol_FrameBg
    tokens.frameBgHovered = bgLight;  // ImGuiCol_FrameBgHovered
    tokens.frameBgActive = bgLighter;  // ImGuiCol_FrameBgActive
    tokens.header = bgLight;  // ImGuiCol_Header
    tokens.headerHovered = bgLighter;  // ImGuiCol_HeaderHovered
    tokens.headerActive = XIFriendList::Core::Color(gold.r, gold.g, gold.b, 0.3f);  // ImGuiCol_HeaderActive: {gold[1], gold[2], gold[3], 0.3}
    
    tokens.buttonColor = bgMedium;  // buttonColor = bgMedium
    tokens.buttonHoverColor = bgLight;  // buttonHoverColor = bgLight
    tokens.buttonActiveColor = bgLighter;  // buttonActiveColor = bgLighter
    
    tokens.textColor = textLight;  // ImGuiCol_Text
    tokens.textDisabled = XIFriendList::Core::Color(76.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f);  // ImGuiCol_TextDisabled: RGB 76, 76, 76
    
    tokens.scrollbarBg = bgMedium;  // ImGuiCol_ScrollbarBg
    tokens.scrollbarGrab = bgLighter;  // ImGuiCol_ScrollbarGrab
    tokens.scrollbarGrabHovered = borderDark;  // ImGuiCol_ScrollbarGrabHovered
    tokens.scrollbarGrabActive = goldDark;  // ImGuiCol_ScrollbarGrabActive
    
    tokens.separatorColor = borderDark;  // ImGuiCol_Separator
    tokens.separatorHovered = borderDark;  // Use same color
    tokens.separatorActive = borderDark;  // Use same color
    
    tokens.checkMark = gold;  // ImGuiCol_CheckMark
    tokens.sliderGrab = goldDark;  // ImGuiCol_SliderGrab
    tokens.sliderGrabActive = gold;  // ImGuiCol_SliderGrabActive
    
    tokens.borderColor = borderDark;  // ImGuiCol_Border
    
    
    tokens.windowPadding = XIFriendList::App::Theming::Vec2(12.0f, 12.0f);  // {12, 12}
    tokens.windowRounding = 6.0f;  // 6.0
    tokens.framePadding = XIFriendList::App::Theming::Vec2(6.0f, 4.0f);  // {6, 4}
    tokens.frameRounding = 4.0f;  // 4.0
    tokens.itemSpacing = XIFriendList::App::Theming::Vec2(8.0f, 6.0f);  // {8, 6}
    tokens.itemInnerSpacing = XIFriendList::App::Theming::Vec2(4.0f, 3.0f);  // Default (not specified in XIUI, using reasonable default)
    tokens.scrollbarSize = 12.0f;  // Default
    tokens.scrollbarRounding = 4.0f;  // 4.0
    tokens.grabRounding = 4.0f;  // 4.0
    
    
    tokens.backgroundAlpha = 0.95f;  // From bgDark alpha (0.95)
    tokens.textAlpha = 1.0f;
    
    return tokens;
}

std::vector<std::string> ThemeUseCase::getAvailablePresets() const {
    std::vector<std::string> presets = {"XIUI Default", "Classic"};
    
    if (presets.empty()) {
        presets = {"XIUI Default", "Classic"};
    }
    
    return presets;
}

ThemeResult ThemeUseCase::setThemePreset(const std::string& presetName) {
    if (presetName.empty()) {
        return ThemeResult(false, "Preset name cannot be empty");
    }
    
    std::vector<std::string> availablePresets = getAvailablePresets();
    bool isValid = false;
    for (const auto& preset : availablePresets) {
        if (preset == presetName) {
            isValid = true;
            break;
        }
    }
    
    if (!isValid) {
        return ThemeResult(false, "Invalid preset name: " + presetName);
    }
    
    currentPresetName_ = presetName;
    
    if (presetName == "XIUI Default") {
        currentThemeIndex_ = -2;
        isEditingBuiltInTheme_ = false;
        currentCustomTheme_ = XIFriendList::Core::CustomTheme();
    } else if (presetName == "Classic") {
        if (currentThemeIndex_ < -2 || currentThemeIndex_ > 3) {
            currentThemeIndex_ = -2;
        }
        isEditingBuiltInTheme_ = false;
    }
    
    state_.presetName = currentPresetName_;
    saveThemes();
    
    return ThemeResult(true);
}


ThemeResult ThemeUseCase::updateQuickOnlineThemeColors(const XIFriendList::Core::CustomTheme& colors) {
    quickOnlineTheme_ = colors;
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::saveQuickOnlineTheme() {
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::updateNotificationThemeColors(const XIFriendList::Core::CustomTheme& colors) {
    notificationTheme_ = colors;
    return ThemeResult(true);
}

ThemeResult ThemeUseCase::saveNotificationTheme() {
    return ThemeResult(true);
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

