
#include "UI/Windows/ThemesWindow.h"
#include "UI/Widgets/Controls.h"
#include "UI/Widgets/Inputs.h"
#include "UI/Widgets/Indicators.h"
#include "UI/Widgets/Layout.h"
#include "UI/Windows/UiCloseCoordinator.h"
#include "UI/UiConstants.h"
#include "Core/ModelsCore.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Protocol/JsonUtils.h"
#include "UI/Helpers/WindowHelper.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include "UI/Interfaces/IUiRenderer.h"
#include <sstream>

namespace XIFriendList {
namespace UI {

ThemesWindow::ThemesWindow()
    : commandHandler_(nullptr)
    , viewModel_(nullptr)
    , visible_(false)
    , title_("Themes")
    , windowId_("Themes")
    , locked_(false)
{
    // Initialize color buffers
    syncColorsToBuffers();
}

void ThemesWindow::syncColorsToBuffers() {
    if (!viewModel_) {
        return;
    }
    
    if (viewModel_->isDefaultTheme()) {
        return;
    }
    
    const auto& colors = viewModel_->getCurrentThemeColors();
    
    windowBgColor_[0] = colors.windowBgColor.r;
    windowBgColor_[1] = colors.windowBgColor.g;
    windowBgColor_[2] = colors.windowBgColor.b;
    windowBgColor_[3] = colors.windowBgColor.a;
    
    childBgColor_[0] = colors.childBgColor.r;
    childBgColor_[1] = colors.childBgColor.g;
    childBgColor_[2] = colors.childBgColor.b;
    childBgColor_[3] = colors.childBgColor.a;
    
    frameBgColor_[0] = colors.frameBgColor.r;
    frameBgColor_[1] = colors.frameBgColor.g;
    frameBgColor_[2] = colors.frameBgColor.b;
    frameBgColor_[3] = colors.frameBgColor.a;
    
    frameBgHovered_[0] = colors.frameBgHovered.r;
    frameBgHovered_[1] = colors.frameBgHovered.g;
    frameBgHovered_[2] = colors.frameBgHovered.b;
    frameBgHovered_[3] = colors.frameBgHovered.a;
    
    frameBgActive_[0] = colors.frameBgActive.r;
    frameBgActive_[1] = colors.frameBgActive.g;
    frameBgActive_[2] = colors.frameBgActive.b;
    frameBgActive_[3] = colors.frameBgActive.a;
    
    titleBg_[0] = colors.titleBg.r;
    titleBg_[1] = colors.titleBg.g;
    titleBg_[2] = colors.titleBg.b;
    titleBg_[3] = colors.titleBg.a;
    
    titleBgActive_[0] = colors.titleBgActive.r;
    titleBgActive_[1] = colors.titleBgActive.g;
    titleBgActive_[2] = colors.titleBgActive.b;
    titleBgActive_[3] = colors.titleBgActive.a;
    
    titleBgCollapsed_[0] = colors.titleBgCollapsed.r;
    titleBgCollapsed_[1] = colors.titleBgCollapsed.g;
    titleBgCollapsed_[2] = colors.titleBgCollapsed.b;
    titleBgCollapsed_[3] = colors.titleBgCollapsed.a;
    
    buttonColor_[0] = colors.buttonColor.r;
    buttonColor_[1] = colors.buttonColor.g;
    buttonColor_[2] = colors.buttonColor.b;
    buttonColor_[3] = colors.buttonColor.a;
    
    buttonHoverColor_[0] = colors.buttonHoverColor.r;
    buttonHoverColor_[1] = colors.buttonHoverColor.g;
    buttonHoverColor_[2] = colors.buttonHoverColor.b;
    buttonHoverColor_[3] = colors.buttonHoverColor.a;
    
    buttonActiveColor_[0] = colors.buttonActiveColor.r;
    buttonActiveColor_[1] = colors.buttonActiveColor.g;
    buttonActiveColor_[2] = colors.buttonActiveColor.b;
    buttonActiveColor_[3] = colors.buttonActiveColor.a;
    
    separatorColor_[0] = colors.separatorColor.r;
    separatorColor_[1] = colors.separatorColor.g;
    separatorColor_[2] = colors.separatorColor.b;
    separatorColor_[3] = colors.separatorColor.a;
    
    separatorHovered_[0] = colors.separatorHovered.r;
    separatorHovered_[1] = colors.separatorHovered.g;
    separatorHovered_[2] = colors.separatorHovered.b;
    separatorHovered_[3] = colors.separatorHovered.a;
    
    separatorActive_[0] = colors.separatorActive.r;
    separatorActive_[1] = colors.separatorActive.g;
    separatorActive_[2] = colors.separatorActive.b;
    separatorActive_[3] = colors.separatorActive.a;
    
    scrollbarBg_[0] = colors.scrollbarBg.r;
    scrollbarBg_[1] = colors.scrollbarBg.g;
    scrollbarBg_[2] = colors.scrollbarBg.b;
    scrollbarBg_[3] = colors.scrollbarBg.a;
    
    scrollbarGrab_[0] = colors.scrollbarGrab.r;
    scrollbarGrab_[1] = colors.scrollbarGrab.g;
    scrollbarGrab_[2] = colors.scrollbarGrab.b;
    scrollbarGrab_[3] = colors.scrollbarGrab.a;
    
    scrollbarGrabHovered_[0] = colors.scrollbarGrabHovered.r;
    scrollbarGrabHovered_[1] = colors.scrollbarGrabHovered.g;
    scrollbarGrabHovered_[2] = colors.scrollbarGrabHovered.b;
    scrollbarGrabHovered_[3] = colors.scrollbarGrabHovered.a;
    
    scrollbarGrabActive_[0] = colors.scrollbarGrabActive.r;
    scrollbarGrabActive_[1] = colors.scrollbarGrabActive.g;
    scrollbarGrabActive_[2] = colors.scrollbarGrabActive.b;
    scrollbarGrabActive_[3] = colors.scrollbarGrabActive.a;
    
    checkMark_[0] = colors.checkMark.r;
    checkMark_[1] = colors.checkMark.g;
    checkMark_[2] = colors.checkMark.b;
    checkMark_[3] = colors.checkMark.a;
    
    sliderGrab_[0] = colors.sliderGrab.r;
    sliderGrab_[1] = colors.sliderGrab.g;
    sliderGrab_[2] = colors.sliderGrab.b;
    sliderGrab_[3] = colors.sliderGrab.a;
    
    sliderGrabActive_[0] = colors.sliderGrabActive.r;
    sliderGrabActive_[1] = colors.sliderGrabActive.g;
    sliderGrabActive_[2] = colors.sliderGrabActive.b;
    sliderGrabActive_[3] = colors.sliderGrabActive.a;
    
    header_[0] = colors.header.r;
    header_[1] = colors.header.g;
    header_[2] = colors.header.b;
    header_[3] = colors.header.a;
    
    headerHovered_[0] = colors.headerHovered.r;
    headerHovered_[1] = colors.headerHovered.g;
    headerHovered_[2] = colors.headerHovered.b;
    headerHovered_[3] = colors.headerHovered.a;
    
    headerActive_[0] = colors.headerActive.r;
    headerActive_[1] = colors.headerActive.g;
    headerActive_[2] = colors.headerActive.b;
    headerActive_[3] = colors.headerActive.a;
    
    textColor_[0] = colors.textColor.r;
    textColor_[1] = colors.textColor.g;
    textColor_[2] = colors.textColor.b;
    textColor_[3] = colors.textColor.a;
    
    textDisabled_[0] = colors.textDisabled.r;
    textDisabled_[1] = colors.textDisabled.g;
    textDisabled_[2] = colors.textDisabled.b;
    textDisabled_[3] = colors.textDisabled.a;
    
}

void ThemesWindow::syncBuffersToColors() {
    if (!viewModel_) {
        return;
    }
    
    auto& colors = viewModel_->getCurrentThemeColors();
    
    colors.windowBgColor = XIFriendList::Core::Color(windowBgColor_[0], windowBgColor_[1], windowBgColor_[2], windowBgColor_[3]);
    colors.childBgColor = XIFriendList::Core::Color(childBgColor_[0], childBgColor_[1], childBgColor_[2], childBgColor_[3]);
    colors.frameBgColor = XIFriendList::Core::Color(frameBgColor_[0], frameBgColor_[1], frameBgColor_[2], frameBgColor_[3]);
    colors.frameBgHovered = XIFriendList::Core::Color(frameBgHovered_[0], frameBgHovered_[1], frameBgHovered_[2], frameBgHovered_[3]);
    colors.frameBgActive = XIFriendList::Core::Color(frameBgActive_[0], frameBgActive_[1], frameBgActive_[2], frameBgActive_[3]);
    colors.titleBg = XIFriendList::Core::Color(titleBg_[0], titleBg_[1], titleBg_[2], titleBg_[3]);
    colors.titleBgActive = XIFriendList::Core::Color(titleBgActive_[0], titleBgActive_[1], titleBgActive_[2], titleBgActive_[3]);
    colors.titleBgCollapsed = XIFriendList::Core::Color(titleBgCollapsed_[0], titleBgCollapsed_[1], titleBgCollapsed_[2], titleBgCollapsed_[3]);
    colors.buttonColor = XIFriendList::Core::Color(buttonColor_[0], buttonColor_[1], buttonColor_[2], buttonColor_[3]);
    colors.buttonHoverColor = XIFriendList::Core::Color(buttonHoverColor_[0], buttonHoverColor_[1], buttonHoverColor_[2], buttonHoverColor_[3]);
    colors.buttonActiveColor = XIFriendList::Core::Color(buttonActiveColor_[0], buttonActiveColor_[1], buttonActiveColor_[2], buttonActiveColor_[3]);
    colors.separatorColor = XIFriendList::Core::Color(separatorColor_[0], separatorColor_[1], separatorColor_[2], separatorColor_[3]);
    colors.separatorHovered = XIFriendList::Core::Color(separatorHovered_[0], separatorHovered_[1], separatorHovered_[2], separatorHovered_[3]);
    colors.separatorActive = XIFriendList::Core::Color(separatorActive_[0], separatorActive_[1], separatorActive_[2], separatorActive_[3]);
    colors.scrollbarBg = XIFriendList::Core::Color(scrollbarBg_[0], scrollbarBg_[1], scrollbarBg_[2], scrollbarBg_[3]);
    colors.scrollbarGrab = XIFriendList::Core::Color(scrollbarGrab_[0], scrollbarGrab_[1], scrollbarGrab_[2], scrollbarGrab_[3]);
    colors.scrollbarGrabHovered = XIFriendList::Core::Color(scrollbarGrabHovered_[0], scrollbarGrabHovered_[1], scrollbarGrabHovered_[2], scrollbarGrabHovered_[3]);
    colors.scrollbarGrabActive = XIFriendList::Core::Color(scrollbarGrabActive_[0], scrollbarGrabActive_[1], scrollbarGrabActive_[2], scrollbarGrabActive_[3]);
    colors.checkMark = XIFriendList::Core::Color(checkMark_[0], checkMark_[1], checkMark_[2], checkMark_[3]);
    colors.sliderGrab = XIFriendList::Core::Color(sliderGrab_[0], sliderGrab_[1], sliderGrab_[2], sliderGrab_[3]);
    colors.sliderGrabActive = XIFriendList::Core::Color(sliderGrabActive_[0], sliderGrabActive_[1], sliderGrabActive_[2], sliderGrabActive_[3]);
    colors.header = XIFriendList::Core::Color(header_[0], header_[1], header_[2], header_[3]);
    colors.headerHovered = XIFriendList::Core::Color(headerHovered_[0], headerHovered_[1], headerHovered_[2], headerHovered_[3]);
    colors.headerActive = XIFriendList::Core::Color(headerActive_[0], headerActive_[1], headerActive_[2], headerActive_[3]);
    colors.textColor = XIFriendList::Core::Color(textColor_[0], textColor_[1], textColor_[2], textColor_[3]);
    colors.textDisabled = XIFriendList::Core::Color(textDisabled_[0], textDisabled_[1], textDisabled_[2], textDisabled_[3]);
}

void ThemesWindow::render() {
    if (!visible_ || viewModel_ == nullptr) {
        return;
    }
    
    static bool themesRefreshed = false;
    if (!themesRefreshed) {
        emitCommand(WindowCommandType::RefreshThemesList, "");
        themesRefreshed = true;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }

    if (pendingClose_ && isUiMenuCleanForClose(renderer)) {
        pendingClose_ = false;
        visible_ = false;
        themesRefreshed = false;  // Reset so themes refresh when reopened
        return;
    }
    
    // Apply theme to this window using ScopedThemeGuard (per-window theme scoping)
    std::optional<XIFriendList::Platform::Ashita::ScopedThemeGuard> themeGuard;
    if (commandHandler_) {
        auto themeTokens = commandHandler_->getCurrentThemeTokens();
        if (themeTokens.has_value()) {
            themeGuard.emplace(themeTokens.value());
        }
    }
    
    static bool sizeSet = false;
    if (!sizeSet) {
        ImVec2 windowSize(600.0f, 700.0f);
        renderer->SetNextWindowSize(windowSize, 0x00000002);  // ImGuiCond_Once
        sizeSet = true;
    }
    
    locked_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadWindowLockState(windowId_);
    
    // G2: Set window flags to prevent move/resize when locked
    int windowFlags = 0;
    if (locked_) {
        windowFlags = 0x0004 | 0x0002;  // ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
    }
    
    bool windowOpen = visible_;
    if (!renderer->Begin(title_.c_str(), &windowOpen, windowFlags)) {
        renderer->End();
        applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
        if (!visible_) {
            themesRefreshed = false;  // Reset so themes refresh when reopened
        }
        return;
    }
    
    visible_ = windowOpen;
    applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
    if (!visible_) {
        themesRefreshed = false;
        renderer->End();
        return;
    }
    
    UI::ImVec2 contentAvail = renderer->GetContentRegionAvail();
#ifndef BUILDING_TESTS
    float reserve = calculateLockButtonReserve();
    UI::ImVec2 childSize(0.0f, contentAvail.y - reserve);
    if (childSize.y < 0.0f) {
        childSize.y = 0.0f;
    }
#else
    UI::ImVec2 childSize(0.0f, contentAvail.y);
#endif
    renderer->BeginChild("##themes_body", childSize, false, WINDOW_BODY_CHILD_FLAGS);
    // Sync colors from viewModel to buffers when theme changes or ViewModel is updated
    static int lastThemeIndex = -999;
    static bool lastWindowBgColorInitialized = false;
    static XIFriendList::Core::Color lastWindowBgColor;
    int currentIndex = viewModel_->getCurrentThemeIndex();
    
    bool themeIndexChanged = (lastThemeIndex != currentIndex);
    bool colorsChanged = false;
    if (!viewModel_->isDefaultTheme()) {
        const auto& currentColors = viewModel_->getCurrentThemeColors();
        // On first render or when theme changes, always sync
        if (!lastWindowBgColorInitialized || themeIndexChanged) {
            colorsChanged = true;
            lastWindowBgColorInitialized = true;
        } else {
            colorsChanged = !(currentColors.windowBgColor == lastWindowBgColor);
        }
        if (colorsChanged) {
            lastWindowBgColor = currentColors.windowBgColor;
        }
    } else {
        lastWindowBgColorInitialized = false;
    }
    
    if (themeIndexChanged || colorsChanged) {
        if (!viewModel_->isDefaultTheme()) {
            syncColorsToBuffers();
        }
        lastThemeIndex = currentIndex;
    }
    
    renderThemePresetSelector();
    renderThemeSelection();
    renderThemeManagement();
    renderer->EndChild();
    
    renderLockButton(renderer, windowId_, locked_, nullptr, commandHandler_);
    
    renderer->End();
}

void ThemesWindow::renderThemePresetSelector() {
    if (!viewModel_) {
        return;
    }
    
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_THEME_PRESET);
    headerSpec.id = "theme_preset_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    const auto& availablePresets = viewModel_->getAvailablePresets();
    const std::string& currentPresetName = viewModel_->getCurrentPresetName();
    
    if (availablePresets.empty()) {
        TextSpec emptySpec;
        emptySpec.text = "No theme presets available";
        emptySpec.id = "theme_preset_empty";
        emptySpec.visible = true;
        createText(emptySpec);
        return;
    }
    
    // Find current preset index
    int currentPresetIndex = 0;
    for (size_t i = 0; i < availablePresets.size(); ++i) {
        if (availablePresets[i] == currentPresetName) {
            currentPresetIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Store preset names in a static variable to ensure they remain valid for the lambda
    static std::vector<std::string> cachedPresets;
    cachedPresets = availablePresets;
    
    ComboSpec presetComboSpec;
    presetComboSpec.label = std::string(Constants::LABEL_PRESET);
    presetComboSpec.id = "theme_preset_combo";
    presetComboSpec.currentItem = &currentPresetIndex;
    presetComboSpec.items = availablePresets;
    presetComboSpec.enabled = true;
    presetComboSpec.visible = true;
    presetComboSpec.onChange = [this](int newIndex) {
        if (viewModel_) {
            const auto& presets = viewModel_->getAvailablePresets();
            if (newIndex >= 0 && newIndex < static_cast<int>(presets.size())) {
                emitCommand(WindowCommandType::SetThemePreset, presets[newIndex]);
            }
        }
    };
    
    createCombo(presetComboSpec);
}

void ThemesWindow::renderThemeSelection() {
    if (!viewModel_) {
        return;
    }
    
    // Hide old theme selection if using presets (XIUI Default, Classic, etc.)
    const std::string& currentPresetName = viewModel_->getCurrentPresetName();
    if (!currentPresetName.empty() && currentPresetName != "Classic") {
        // Using preset system - hide old theme selection
        return;
    }
    
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_THEME_SELECTION);
    headerSpec.id = "theme_selection_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    std::vector<std::string> allThemeNames;
    std::vector<int> themeIndices;  // Parallel array: index for each theme name
    
    // Add built-in themes
    const auto& builtInNames = viewModel_->getBuiltInThemeNames();
    for (size_t i = 0; i < builtInNames.size(); ++i) {
        allThemeNames.push_back(builtInNames[i]);
        // Map: 0 -> -2 (Default), 1-4 -> 0-3 (built-in themes)
        int themeIndex = (i == 0) ? -2 : static_cast<int>(i - 1);
        themeIndices.push_back(themeIndex);
    }
    
    // Add custom themes
    const auto& customThemes = viewModel_->getCustomThemes();
    for (const auto& customTheme : customThemes) {
        allThemeNames.push_back(customTheme.name);
        themeIndices.push_back(-1);  // -1 indicates custom theme
    }
    
    if (allThemeNames.empty()) {
        TextSpec emptySpec;
        emptySpec.text = "No themes available";
        emptySpec.id = "theme_selection_empty";
        emptySpec.visible = true;
        createText(emptySpec);
        return;
    }
    
    // Find current theme index in the list
    int currentComboIndex = 0;
    int currentThemeIndex = viewModel_->getCurrentThemeIndex();
    std::string currentThemeName = viewModel_->getCurrentThemeName();
    
    for (size_t i = 0; i < allThemeNames.size(); ++i) {
        if (themeIndices[i] == currentThemeIndex) {
            // For custom themes, also check the name matches
            if (currentThemeIndex == -1) {
                if (allThemeNames[i] == currentThemeName) {
                    currentComboIndex = static_cast<int>(i);
                    break;
                }
            } else {
                currentComboIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    ComboSpec themeComboSpec;
    themeComboSpec.label = std::string(Constants::LABEL_THEME);
    themeComboSpec.id = "theme_selection_combo";
    themeComboSpec.currentItem = &currentComboIndex;
    themeComboSpec.items = allThemeNames;
    themeComboSpec.enabled = true;
    themeComboSpec.visible = true;
    themeComboSpec.onChange = [this, themeIndices, allThemeNames](int newIndex) {
        if (newIndex < 0 || newIndex >= static_cast<int>(themeIndices.size())) {
            return;
        }
        
        int selectedThemeIndex = themeIndices[newIndex];
        
        if (selectedThemeIndex == -1) {
            std::string themeName = allThemeNames[newIndex];
            emitCommand(WindowCommandType::SetCustomTheme, themeName);
        } else {
            emitCommand(WindowCommandType::ApplyTheme, std::to_string(selectedThemeIndex));
        }
    };
    
    createCombo(themeComboSpec);
    
    // Transparency sliders (moved here from renderCustomColors)
    if (!viewModel_->isDefaultTheme()) {
        TextSpec bgAlphaLabelSpec;
        bgAlphaLabelSpec.text = "Background Transparency:";
        bgAlphaLabelSpec.id = "bg_alpha_label";
        bgAlphaLabelSpec.visible = true;
        createText(bgAlphaLabelSpec);
        
        SliderSpec bgAlphaSliderSpec;
        bgAlphaSliderSpec.label = "##bgAlpha";
        bgAlphaSliderSpec.id = "bg_alpha_slider";
        bgAlphaSliderSpec.value = &viewModel_->getBackgroundAlpha();
        bgAlphaSliderSpec.min = 0.0f;
        bgAlphaSliderSpec.max = 1.0f;
        bgAlphaSliderSpec.format = "%.2f";
        bgAlphaSliderSpec.enabled = true;
        bgAlphaSliderSpec.visible = true;
        bgAlphaSliderSpec.onChange = [this](float value) {
            viewModel_->setBackgroundAlpha(value);
            emitCommand(WindowCommandType::SetBackgroundAlpha, std::to_string(value));
        };
        bgAlphaSliderSpec.onDeactivated = [this](float value) {
            emitCommand(WindowCommandType::SaveThemeAlpha, std::to_string(value));
        };
        createSlider(bgAlphaSliderSpec);
        
        TextSpec textAlphaLabelSpec;
        textAlphaLabelSpec.text = std::string(Constants::LABEL_TEXT_TRANSPARENCY);
        textAlphaLabelSpec.id = "text_alpha_label";
        textAlphaLabelSpec.visible = true;
        createText(textAlphaLabelSpec);
        
        SliderSpec textAlphaSliderSpec;
        textAlphaSliderSpec.label = "##textAlpha";
        textAlphaSliderSpec.id = "text_alpha_slider";
        textAlphaSliderSpec.value = &viewModel_->getTextAlpha();
        textAlphaSliderSpec.min = 0.0f;
        textAlphaSliderSpec.max = 1.0f;
        textAlphaSliderSpec.format = "%.2f";
        textAlphaSliderSpec.enabled = true;
        textAlphaSliderSpec.visible = true;
        textAlphaSliderSpec.onChange = [this](float value) {
            viewModel_->setTextAlpha(value);
            emitCommand(WindowCommandType::SetTextAlpha, std::to_string(value));
        };
        textAlphaSliderSpec.onDeactivated = [this](float value) {
            emitCommand(WindowCommandType::SaveThemeAlpha, "");
        };
        createSlider(textAlphaSliderSpec);
        
        // Custom Colors section (appears directly after Text Transparency)
        renderCustomColors();
    }
}

void ThemesWindow::renderCustomColors() {
    if (!viewModel_) {
        return;
    }
    
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_CUSTOM_COLORS);
    headerSpec.id = "custom_colors_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    // Helper lambda to sync buffers and apply theme immediately
    auto applyColorChange = [this]() {
        syncBuffersToColors();
        // Emit UpdateThemeColors command for immediate per-color updates
        emitCommand(WindowCommandType::UpdateThemeColors, "");
    };
    
    // Color pickers - Window colors
    ColorPickerSpec windowBgSpec("Window Background", "window_bg", windowBgColor_);
    windowBgSpec.showAlpha = true;
    windowBgSpec.onChange = applyColorChange;
    createColorPicker(windowBgSpec);
    
    ColorPickerSpec childBgSpec("Child Background", "child_bg", childBgColor_);
    childBgSpec.showAlpha = true;
    childBgSpec.onChange = applyColorChange;
    createColorPicker(childBgSpec);
    
    ColorPickerSpec frameBgSpec("Frame Background", "frame_bg", frameBgColor_);
    frameBgSpec.showAlpha = true;
    frameBgSpec.onChange = applyColorChange;
    createColorPicker(frameBgSpec);
    
    ColorPickerSpec frameBgHoveredSpec("Frame Hovered", "frame_bg_hovered", frameBgHovered_);
    frameBgHoveredSpec.showAlpha = true;
    frameBgHoveredSpec.onChange = applyColorChange;
    createColorPicker(frameBgHoveredSpec);
    
    ColorPickerSpec frameBgActiveSpec("Frame Active", "frame_bg_active", frameBgActive_);
    frameBgActiveSpec.showAlpha = true;
    frameBgActiveSpec.onChange = applyColorChange;
    createColorPicker(frameBgActiveSpec);
    
    ColorPickerSpec titleBgSpec("Title Background", "title_bg", titleBg_);
    titleBgSpec.showAlpha = true;
    titleBgSpec.onChange = applyColorChange;
    createColorPicker(titleBgSpec);
    
    ColorPickerSpec titleBgActiveSpec("Title Active", "title_bg_active", titleBgActive_);
    titleBgActiveSpec.showAlpha = true;
    titleBgActiveSpec.onChange = applyColorChange;
    createColorPicker(titleBgActiveSpec);
    
    ColorPickerSpec titleBgCollapsedSpec("Title Collapsed", "title_bg_collapsed", titleBgCollapsed_);
    titleBgCollapsedSpec.showAlpha = true;
    titleBgCollapsedSpec.onChange = applyColorChange;
    createColorPicker(titleBgCollapsedSpec);
    
    ColorPickerSpec buttonSpec("Button", "button", buttonColor_);
    buttonSpec.showAlpha = true;
    buttonSpec.onChange = applyColorChange;
    createColorPicker(buttonSpec);
    
    ColorPickerSpec buttonHoverSpec("Button Hovered", "button_hovered", buttonHoverColor_);
    buttonHoverSpec.showAlpha = true;
    buttonHoverSpec.onChange = applyColorChange;
    createColorPicker(buttonHoverSpec);
    
    ColorPickerSpec buttonActiveSpec("Button Active", "button_active", buttonActiveColor_);
    buttonActiveSpec.showAlpha = true;
    buttonActiveSpec.onChange = applyColorChange;
    createColorPicker(buttonActiveSpec);
    
    ColorPickerSpec separatorSpec("Separator", "separator", separatorColor_);
    separatorSpec.showAlpha = true;
    separatorSpec.onChange = applyColorChange;
    createColorPicker(separatorSpec);
    
    ColorPickerSpec separatorHoveredSpec("Separator Hovered", "separator_hovered", separatorHovered_);
    separatorHoveredSpec.showAlpha = true;
    separatorHoveredSpec.onChange = applyColorChange;
    createColorPicker(separatorHoveredSpec);
    
    ColorPickerSpec separatorActiveSpec("Separator Active", "separator_active", separatorActive_);
    separatorActiveSpec.showAlpha = true;
    separatorActiveSpec.onChange = applyColorChange;
    createColorPicker(separatorActiveSpec);
    
    ColorPickerSpec scrollbarBgSpec("Scrollbar Bg", "scrollbar_bg", scrollbarBg_);
    scrollbarBgSpec.showAlpha = true;
    scrollbarBgSpec.onChange = applyColorChange;
    createColorPicker(scrollbarBgSpec);
    
    ColorPickerSpec scrollbarGrabSpec("Scrollbar Grab", "scrollbar_grab", scrollbarGrab_);
    scrollbarGrabSpec.showAlpha = true;
    scrollbarGrabSpec.onChange = applyColorChange;
    createColorPicker(scrollbarGrabSpec);
    
    ColorPickerSpec scrollbarGrabHoveredSpec("Scrollbar Grab Hovered", "scrollbar_grab_hovered", scrollbarGrabHovered_);
    scrollbarGrabHoveredSpec.showAlpha = true;
    scrollbarGrabHoveredSpec.onChange = applyColorChange;
    createColorPicker(scrollbarGrabHoveredSpec);
    
    ColorPickerSpec scrollbarGrabActiveSpec("Scrollbar Grab Active", "scrollbar_grab_active", scrollbarGrabActive_);
    scrollbarGrabActiveSpec.showAlpha = true;
    scrollbarGrabActiveSpec.onChange = applyColorChange;
    createColorPicker(scrollbarGrabActiveSpec);
    
    ColorPickerSpec checkMarkSpec("Check Mark", "check_mark", checkMark_);
    checkMarkSpec.showAlpha = true;
    checkMarkSpec.onChange = applyColorChange;
    createColorPicker(checkMarkSpec);
    
    ColorPickerSpec sliderGrabSpec("Slider Grab", "slider_grab", sliderGrab_);
    sliderGrabSpec.showAlpha = true;
    sliderGrabSpec.onChange = applyColorChange;
    createColorPicker(sliderGrabSpec);
    
    ColorPickerSpec sliderGrabActiveSpec("Slider Grab Active", "slider_grab_active", sliderGrabActive_);
    sliderGrabActiveSpec.showAlpha = true;
    sliderGrabActiveSpec.onChange = applyColorChange;
    createColorPicker(sliderGrabActiveSpec);
    
    ColorPickerSpec headerColorSpec("Header", "header", header_);
    headerColorSpec.showAlpha = true;
    headerColorSpec.onChange = applyColorChange;
    createColorPicker(headerColorSpec);
    
    ColorPickerSpec headerHoveredSpec("Header Hovered", "header_hovered", headerHovered_);
    headerHoveredSpec.showAlpha = true;
    headerHoveredSpec.onChange = applyColorChange;
    createColorPicker(headerHoveredSpec);
    
    ColorPickerSpec headerActiveSpec("Header Active", "header_active", headerActive_);
    headerActiveSpec.showAlpha = true;
    headerActiveSpec.onChange = applyColorChange;
    createColorPicker(headerActiveSpec);
    
    ColorPickerSpec textSpec("Text", "text", textColor_);
    textSpec.showAlpha = true;
    textSpec.onChange = applyColorChange;
    createColorPicker(textSpec);
    
    ColorPickerSpec textDisabledSpec("Text Disabled", "text_disabled", textDisabled_);
    textDisabledSpec.showAlpha = true;
    textDisabledSpec.onChange = applyColorChange;
    createColorPicker(textDisabledSpec);
}

void ThemesWindow::renderThemeManagement() {
    if (!viewModel_) {
        return;
    }
    
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_THEME_MANAGEMENT);
    headerSpec.id = "theme_management_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    TextSpec saveLabelSpec;
    saveLabelSpec.text = std::string(Constants::LABEL_SAVE_CURRENT_COLORS_AS_THEME);
    saveLabelSpec.id = "save_theme_label";
    saveLabelSpec.visible = true;
    createText(saveLabelSpec);
    
    TextSpec themeNameLabelSpec;
    themeNameLabelSpec.text = std::string(Constants::LABEL_THEME_NAME);
    themeNameLabelSpec.id = "theme_name_label";
    themeNameLabelSpec.visible = true;
    createText(themeNameLabelSpec);
    
    InputTextSpec themeNameInputSpec;
    themeNameInputSpec.label = "##saveThemeName";
    themeNameInputSpec.id = "save_theme_name_input";
    themeNameInputSpec.buffer = &viewModel_->getNewThemeName();
    themeNameInputSpec.bufferSize = 256;
    themeNameInputSpec.enabled = true;
    themeNameInputSpec.visible = true;
    themeNameInputSpec.readOnly = false;
    themeNameInputSpec.onChange = [this](const std::string& value) {
        viewModel_->setNewThemeName(value);
    };
    themeNameInputSpec.onEnter = [this](const std::string& value) {
        viewModel_->setNewThemeName(value);
    };
    createInputText(themeNameInputSpec);
    
    ButtonSpec saveButtonSpec;
    saveButtonSpec.label = std::string(Constants::BUTTON_SAVE_CUSTOM_THEME);
    saveButtonSpec.id = "save_theme_button";
    saveButtonSpec.enabled = viewModel_->canSaveTheme();
    saveButtonSpec.visible = true;
    saveButtonSpec.onClick = [this]() {
        syncBuffersToColors();
        emitCommand(WindowCommandType::SaveCustomTheme, viewModel_->getNewThemeName());
    };
    createButton(saveButtonSpec);
    
    // Delete current custom theme
    if (viewModel_->canDeleteTheme()) {
        ButtonSpec deleteButtonSpec;
        deleteButtonSpec.label = std::string(Constants::BUTTON_DELETE_CUSTOM_THEME);
        deleteButtonSpec.id = "delete_theme_button";
        deleteButtonSpec.enabled = true;
        deleteButtonSpec.visible = true;
        deleteButtonSpec.onClick = [this]() {
            emitCommand(WindowCommandType::DeleteCustomTheme, viewModel_->getCurrentThemeName());
        };
        createButton(deleteButtonSpec);
    }
}

void ThemesWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
        WindowCommand command(type, data);
        commandHandler_->handleCommand(command);
    }
}

} // namespace UI
} // namespace XIFriendList
