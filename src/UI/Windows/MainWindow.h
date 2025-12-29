// Unified main window combining Friends List and Options with XIUI-style sidebar navigation

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include "UI/ViewModels/FriendListViewModel.h"
#include "UI/ViewModels/OptionsViewModel.h"
#include "UI/ViewModels/AltVisibilityViewModel.h"
#include "UI/ViewModels/ThemesViewModel.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/Widgets/WidgetSpecs.h"
#include "UI/Widgets/Tables.h"
#include "Core/MemoryStats.h"
#include <string>
#include <unordered_map>

namespace XIFriendList {
namespace UI {
    // Forward declarations
    class IUiRenderer;

// Main window
// Combines FriendListWindow and OptionsWindow functionality
// Uses XIUI-style sidebar navigation with tabs: Friends, Privacy, Notifications, Controls, Themes, Alts
class MainWindow {
public:
    enum class Tab {
        Friends = 0,
        Privacy = 1,
        Notifications = 2,
        Controls = 3,
        Themes = 4
    };

    MainWindow();
    ~MainWindow() = default;
    
    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
        friendTable_.setCommandHandler(handler);
    }
    
    void setFriendListViewModel(FriendListViewModel* viewModel) {
        friendListViewModel_ = viewModel;
        friendTable_.setViewModel(viewModel);
    }
    
    void setOptionsViewModel(OptionsViewModel* viewModel) {
        optionsViewModel_ = viewModel;
    }
    
    void setAltVisibilityViewModel(AltVisibilityViewModel* viewModel) {
        altVisibilityViewModel_ = viewModel;
    }
    
    void setThemesViewModel(ThemesViewModel* viewModel) {
        themesViewModel_ = viewModel;
    }
    
    
    void setPluginInfo(const std::string& name, const std::string& author, const std::string& version) {
        pluginName_ = name;
        pluginAuthor_ = author;
        pluginVersion_ = version;
    }
    
    void setIconManager(void* iconManager) {
        iconManager_ = iconManager;
        friendTable_.setIconManager(iconManager);
    }
    
    // Render the window
    void render();
    
    void setVisible(bool visible) {
        visible_ = visible;
    }
    
    bool isVisible() const {
        return visible_;
    }
    
    void setShareFriendsAcrossAlts(bool enabled) {
        shareFriendsAcrossAlts_ = enabled;
        friendTable_.setShareFriendsAcrossAlts(enabled);
    }
    
    bool getShareFriendsAcrossAlts() const {
        return shareFriendsAcrossAlts_;
    }
    
    void setFriendViewSettings(const XIFriendList::Core::FriendViewSettings& settings) {
        friendTable_.setViewSettings(settings);
    }
    
    void setSelectedFriendForDetails(const std::string& friendName) {
        selectedFriendForDetails_ = friendName;
    }
    
    const std::string& getSelectedFriendForDetails() const {
        return selectedFriendForDetails_;
    }
    
    void clearSelectedFriendForDetails() {
        selectedFriendForDetails_.clear();
    }
    
    // Request to expand Alt Visibility section when opening window
    void requestExpandAltVisibilitySection() {
        requestExpandAltVisibility_ = true;
    }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    // Emit command to App layer
    void emitCommand(WindowCommandType type, const std::string& data = "");
    void emitPreferenceUpdate(const std::string& field, bool value);
    void emitPreferenceUpdate(const std::string& field, const std::string& value);
    void emitPreferenceUpdate(const std::string& field, float value);
    void emitPreferenceUpdate(const std::string& field, int value);
    
    // Top bar rendering
    void renderTopBar();
    
    // Sidebar rendering
    void renderSidebar();
    
    // Content area rendering
    void renderContentArea();
    
    void renderFriendsTab();
    void renderPrivacyTab();
    void renderNotificationsTab();
    void renderControlsTab();
    void renderThemesTab();
    
    void renderAddFriendSection();
    void renderPendingRequestsSection();
    
    // General tab content (from OptionsWindow)
    void renderFriendViewSettingsSection();
    void renderFriendViewSettings();
    void renderPrivacyControlsSection();
    void renderPrivacyControls();
    void renderAltVisibilitySection();
    void renderAltVisibilityContent();
    void renderAltVisibilityFriendTable();
    std::string getVisibilityStateText(AltVisibilityState state) const;
    
    void renderNotificationsSection();
    void renderNotifications();
    
    // Controls tab content (from OptionsWindow)
    void renderControlsSection();
    void renderControls();
    void renderDebugSettingsSection();
    void renderDebugSettings();
    
    void renderThemeSettingsSection();
    void renderThemeSettings();
    void renderCustomColors();
    void renderThemeManagement();
    void renderQuickOnlineThemeSection();
    void renderNotificationThemeSection();
    
    void syncColorsToBuffers();
    void syncBuffersToColors();
    void syncQuickOnlineColorsToBuffers();
    void syncQuickOnlineBuffersToColors();
    void syncNotificationColorsToBuffers();
    void syncNotificationBuffersToColors();
    
    // Helper functions for color buffer operations
    static void initializeColorBuffer(float buffer[4], float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    static void syncColorToBuffer(const XIFriendList::Core::Color& color, float buffer[4]);
    static void syncBufferToColor(const float buffer[4], XIFriendList::Core::Color& color);
    
    // Helper functions
    std::string getWindowTitle() const;
    
    // ViewModels
    FriendListViewModel* friendListViewModel_;
    OptionsViewModel* optionsViewModel_;
    AltVisibilityViewModel* altVisibilityViewModel_;
    ThemesViewModel* themesViewModel_;
    
    // Plugin info (for About tab)
    std::string pluginName_;
    std::string pluginAuthor_;
    std::string pluginVersion_;
    
    // Command handler
    IWindowCommandHandler* commandHandler_;
    
    // IconManager (void* to avoid Platform dependency)
    void* iconManager_;
    
    // Window state
    bool visible_;
    std::string windowId_;
    bool locked_;
    bool pendingClose_{ false };
    
    // Sidebar state
    Tab selectedTab_;
    
    std::string newFriendInput_;
    std::string newFriendNoteInput_;
    bool pendingRequestsSectionExpanded_;
    bool shareFriendsAcrossAlts_;
    
    // General tab state
    bool friendViewSettingsSectionExpanded_;
    bool privacySectionExpanded_;
    bool altVisibilitySectionExpanded_;
    bool requestExpandAltVisibility_;
    bool altVisibilityDataLoaded_;
    std::string altVisibilityFilterText_;
    std::unordered_map<int, bool> altVisibilityCheckboxValues_;
    
    bool notificationsSectionExpanded_;
    
    // Controls tab state
    bool controlsSectionExpanded_;
    bool debugSectionExpanded_;
    
    bool themeSettingsSectionExpanded_;
    int currentPresetIndex_;
    int currentCloseKeyIndex_;
    
    // Color section collapsed states (for collapsible color categories)
    bool colorSectionWindowCollapsed_;
    bool colorSectionFrameCollapsed_;
    bool colorSectionTitleCollapsed_;
    bool colorSectionButtonCollapsed_;
    bool colorSectionSeparatorCollapsed_;
    bool colorSectionScrollbarCollapsed_;
    bool colorSectionCheckSliderCollapsed_;
    bool colorSectionHeaderCollapsed_;
    bool colorSectionTextCollapsed_;
    
    // Color editing buffers (float[4] arrays for each color)
    float windowBgColor_[4];
    float childBgColor_[4];
    float frameBgColor_[4];
    float frameBgHovered_[4];
    float frameBgActive_[4];
    float titleBg_[4];
    float titleBgActive_[4];
    float titleBgCollapsed_[4];
    float buttonColor_[4];
    float buttonHoverColor_[4];
    float buttonActiveColor_[4];
    float separatorColor_[4];
    float separatorHovered_[4];
    float separatorActive_[4];
    float scrollbarBg_[4];
    float scrollbarGrab_[4];
    float scrollbarGrabHovered_[4];
    float scrollbarGrabActive_[4];
    float checkMark_[4];
    float sliderGrab_[4];
    float sliderGrabActive_[4];
    float header_[4];
    float headerHovered_[4];
    float headerActive_[4];
    float textColor_[4];
    float textDisabled_[4];
    float tableBgColor_[4];
    
    // Quick Online theme editing buffers (separate from main theme)
    float quickOnlineWindowBgColor_[4];
    float quickOnlineChildBgColor_[4];
    float quickOnlineFrameBgColor_[4];
    float quickOnlineFrameBgHovered_[4];
    float quickOnlineFrameBgActive_[4];
    float quickOnlineTitleBg_[4];
    float quickOnlineTitleBgActive_[4];
    float quickOnlineTitleBgCollapsed_[4];
    float quickOnlineButtonColor_[4];
    float quickOnlineButtonHoverColor_[4];
    float quickOnlineButtonActiveColor_[4];
    float quickOnlineSeparatorColor_[4];
    float quickOnlineSeparatorHovered_[4];
    float quickOnlineSeparatorActive_[4];
    float quickOnlineScrollbarBg_[4];
    float quickOnlineScrollbarGrab_[4];
    float quickOnlineScrollbarGrabHovered_[4];
    float quickOnlineScrollbarGrabActive_[4];
    float quickOnlineCheckMark_[4];
    float quickOnlineSliderGrab_[4];
    float quickOnlineSliderGrabActive_[4];
    float quickOnlineHeader_[4];
    float quickOnlineHeaderHovered_[4];
    float quickOnlineHeaderActive_[4];
    float quickOnlineTextColor_[4];
    float quickOnlineTextDisabled_[4];
    float quickOnlineTableBgColor_[4];
    
    // Notification theme editing buffers (separate from main theme)
    float notificationWindowBgColor_[4];
    float notificationChildBgColor_[4];
    float notificationFrameBgColor_[4];
    float notificationFrameBgHovered_[4];
    float notificationFrameBgActive_[4];
    float notificationTitleBg_[4];
    float notificationTitleBgActive_[4];
    float notificationTitleBgCollapsed_[4];
    float notificationButtonColor_[4];
    float notificationButtonHoverColor_[4];
    float notificationButtonActiveColor_[4];
    float notificationSeparatorColor_[4];
    float notificationSeparatorHovered_[4];
    float notificationSeparatorActive_[4];
    float notificationScrollbarBg_[4];
    float notificationScrollbarGrab_[4];
    float notificationScrollbarGrabHovered_[4];
    float notificationScrollbarGrabActive_[4];
    float notificationCheckMark_[4];
    float notificationSliderGrab_[4];
    float notificationSliderGrabActive_[4];
    float notificationHeader_[4];
    float notificationHeaderHovered_[4];
    float notificationHeaderActive_[4];
    float notificationTextColor_[4];
    float notificationTextDisabled_[4];
    float notificationTableBgColor_[4];
    
    // Test window toggles
    bool showQuickOnlineForTesting_;
    bool showNotificationsForTesting_;
    bool notificationPreviewEnabled_;
    
    // Widgets
    FriendTableWidget friendTable_;
    
    // Window title cache (updated when character changes)
    mutable std::string cachedWindowTitle_;
    mutable std::string cachedCharacterName_;
    
    // About/Thanks popup state
    bool showAboutPopup_{ false };
    bool aboutPopupJustOpened_{ false };
    
    // Friend details popup state
    std::string selectedFriendForDetails_;
    
    void renderFriendDetailsPopup();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_MAIN_WINDOW_H

