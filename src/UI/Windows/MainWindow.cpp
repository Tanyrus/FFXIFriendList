// Unified main window combining Friends List and Options with XIUI-style sidebar navigation

#include "UI/Windows/MainWindow.h"
#include "UI/Widgets/Controls.h"
#include "UI/Widgets/Inputs.h"
#include "UI/Widgets/Indicators.h"
#include "UI/Widgets/Layout.h"
#include "UI/Widgets/Tables.h"
#include "UI/Windows/UiCloseCoordinator.h"
#include "UI/UiConstants.h"
#include "Protocol/MessageTypes.h"
#include "Protocol/JsonUtils.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "UI/Interfaces/IUiRenderer.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include "UI/Helpers/TooltipHelper.h"
#include "Platform/Ashita/IconManager.h"
#include "UI/Notifications/ToastManager.h"
#include "App/Notifications/Toast.h"
#include "App/NotificationConstants.h"
#include "Core/MemoryStats.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>
#include <chrono>
#include <windows.h>  // For VK_ESCAPE, VK_BACK, ShellExecuteA, etc.
#include <shellapi.h>  // For ShellExecuteA, ShellExecuteEx
#include <iomanip>  // For std::put_time
#include <ctime>  // For std::time_t, std::localtime
#include <thread>  // For std::thread
#include <string>  // For std::string

namespace XIFriendList {
namespace UI {

namespace {
    std::string capitalizeWords(const std::string& s) {
        std::string out = s;
        if (!out.empty()) {
            out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
            for (size_t i = 1; i < out.length(); ++i) {
                if (out[i - 1] == ' ') {
                    out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
                } else {
                    out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
                }
            }
        }
        return out;
    }
    
    // Safe wrapper for opening URLs that prevents crashes
    // Defers execution to a separate thread to avoid calling ShellExecute during ImGui rendering
    void safeOpenUrl(const char* url) {
        if (!url || !*url) {
            return;
        }
        
        // Copy URL to string for thread safety
        std::string urlStr(url);
        
        // Spawn a detached thread to open the URL after a small delay
        // This prevents crashes from calling ShellExecute during ImGui rendering
        std::thread([urlStr]() {
            // Small delay to ensure ImGui frame is complete
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            __try {
                // Use ShellExecuteEx for better error handling and reliability
                SHELLEXECUTEINFOA sei = {0};
                sei.cbSize = sizeof(SHELLEXECUTEINFOA);
                sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
                sei.lpVerb = "open";
                sei.lpFile = urlStr.c_str();
                sei.nShow = SW_SHOWNORMAL;
                
                BOOL result = ShellExecuteExA(&sei);
                
                // Close process handle if opened (prevents handle leaks)
                if (sei.hProcess) {
                    CloseHandle(sei.hProcess);
                    sei.hProcess = nullptr;
                }
                
                // Check result - ShellExecuteEx returns FALSE on failure
                if (!result) {
                    // GetLastError() would provide error code, but we don't log to avoid noise
                    // Common errors: ERROR_NO_ASSOCIATION (1155), ERROR_FILE_NOT_FOUND (2), etc.
                    DWORD error = GetLastError();
                    (void)error; // Suppress unused variable warning
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                // Catch any structured exceptions (access violations, etc.) to prevent crashes
                // Opening a URL is not critical functionality, so we silently fail
            }
        }).detach();
    }
}

MainWindow::MainWindow()
    : friendListViewModel_(nullptr)
    , optionsViewModel_(nullptr)
    , altVisibilityViewModel_(nullptr)
    , themesViewModel_(nullptr)
    , pluginName_("XI FriendList")
    , pluginAuthor_("Carrott")
    , pluginVersion_("0.9.0")
    , commandHandler_(nullptr)
    , iconManager_(nullptr)
    , visible_(false)
    , windowId_("MainWindow")
    , locked_(false)
    , selectedTab_(Tab::Friends)
    , newFriendInput_()
    , newFriendNoteInput_()
    , pendingRequestsSectionExpanded_(true)
    , shareFriendsAcrossAlts_(true)
    , privacySectionExpanded_(true)
    , altVisibilitySectionExpanded_(false)
    , requestExpandAltVisibility_(false)
    , altVisibilityDataLoaded_(false)
    , altVisibilityFilterText_()
    , notificationsSectionExpanded_(false)
    , controlsSectionExpanded_(false)
    , debugSectionExpanded_(false)
    , themeSettingsSectionExpanded_(true)
    , currentPresetIndex_(0)
    , currentCloseKeyIndex_(0)
    , colorSectionWindowCollapsed_(false)
    , colorSectionFrameCollapsed_(false)
    , colorSectionTitleCollapsed_(false)
    , colorSectionButtonCollapsed_(false)
    , colorSectionSeparatorCollapsed_(false)
    , colorSectionScrollbarCollapsed_(false)
    , colorSectionCheckSliderCollapsed_(false)
    , colorSectionHeaderCollapsed_(false)
    , colorSectionTextCollapsed_(false)
    , cachedWindowTitle_()
    , cachedCharacterName_()
{
    pendingRequestsSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "pendingRequests");
    friendViewSettingsSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "friendViewSettings");
    privacySectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "privacy");
    altVisibilitySectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "altVisibility");
    controlsSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "controls");
    notificationsSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "notifications");
    debugSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "debug");
    themeSettingsSectionExpanded_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadCollapsibleSectionState(
        windowId_, "themeSettings");
    
    // TODO: Add proper int preference storage to AshitaPreferencesStore if needed
    
    FriendTableWidgetSpec spec;
    spec.tableId = "friend_list_table";
    spec.toggleRowId = "friend_list_column_visibility_row";
    spec.sectionHeaderId = "friends_header";
    spec.sectionHeaderLabel = "Your Friends";
    spec.showSectionHeader = true;
    spec.showColumnToggles = false;
    spec.commandScope = "FriendList";
    friendTable_.setSpec(spec);
    
    initializeColorBuffer(windowBgColor_);
    initializeColorBuffer(childBgColor_);
    initializeColorBuffer(frameBgColor_);
    initializeColorBuffer(frameBgHovered_);
    initializeColorBuffer(frameBgActive_);
    initializeColorBuffer(titleBg_);
    initializeColorBuffer(titleBgActive_);
    initializeColorBuffer(titleBgCollapsed_);
    initializeColorBuffer(buttonColor_);
    initializeColorBuffer(buttonHoverColor_);
    initializeColorBuffer(buttonActiveColor_);
    initializeColorBuffer(separatorColor_);
    initializeColorBuffer(separatorHovered_);
    initializeColorBuffer(separatorActive_);
    initializeColorBuffer(scrollbarBg_);
    initializeColorBuffer(scrollbarGrab_);
    initializeColorBuffer(scrollbarGrabHovered_);
    initializeColorBuffer(scrollbarGrabActive_);
    initializeColorBuffer(checkMark_);
    initializeColorBuffer(sliderGrab_);
    initializeColorBuffer(sliderGrabActive_);
    initializeColorBuffer(header_);
    initializeColorBuffer(headerHovered_);
    initializeColorBuffer(headerActive_);
    initializeColorBuffer(textColor_);
    initializeColorBuffer(textDisabled_);
    initializeColorBuffer(tableBgColor_);
    
    initializeColorBuffer(quickOnlineWindowBgColor_);
    initializeColorBuffer(quickOnlineChildBgColor_);
    initializeColorBuffer(quickOnlineFrameBgColor_);
    initializeColorBuffer(quickOnlineFrameBgHovered_);
    initializeColorBuffer(quickOnlineFrameBgActive_);
    initializeColorBuffer(quickOnlineTitleBg_);
    initializeColorBuffer(quickOnlineTitleBgActive_);
    initializeColorBuffer(quickOnlineTitleBgCollapsed_);
    initializeColorBuffer(quickOnlineButtonColor_);
    initializeColorBuffer(quickOnlineButtonHoverColor_);
    initializeColorBuffer(quickOnlineButtonActiveColor_);
    initializeColorBuffer(quickOnlineSeparatorColor_);
    initializeColorBuffer(quickOnlineSeparatorHovered_);
    initializeColorBuffer(quickOnlineSeparatorActive_);
    initializeColorBuffer(quickOnlineScrollbarBg_);
    initializeColorBuffer(quickOnlineScrollbarGrab_);
    initializeColorBuffer(quickOnlineScrollbarGrabHovered_);
    initializeColorBuffer(quickOnlineScrollbarGrabActive_);
    initializeColorBuffer(quickOnlineCheckMark_);
    initializeColorBuffer(quickOnlineSliderGrab_);
    initializeColorBuffer(quickOnlineSliderGrabActive_);
    initializeColorBuffer(quickOnlineHeader_);
    initializeColorBuffer(quickOnlineHeaderHovered_);
    initializeColorBuffer(quickOnlineHeaderActive_);
    initializeColorBuffer(quickOnlineTextColor_);
    initializeColorBuffer(quickOnlineTextDisabled_);
    initializeColorBuffer(quickOnlineTableBgColor_);
    
    initializeColorBuffer(notificationWindowBgColor_);
    initializeColorBuffer(notificationChildBgColor_);
    initializeColorBuffer(notificationFrameBgColor_);
    initializeColorBuffer(notificationFrameBgHovered_);
    initializeColorBuffer(notificationFrameBgActive_);
    initializeColorBuffer(notificationTitleBg_);
    initializeColorBuffer(notificationTitleBgActive_);
    initializeColorBuffer(notificationTitleBgCollapsed_);
    initializeColorBuffer(notificationButtonColor_);
    initializeColorBuffer(notificationButtonHoverColor_);
    initializeColorBuffer(notificationButtonActiveColor_);
    initializeColorBuffer(notificationSeparatorColor_);
    initializeColorBuffer(notificationSeparatorHovered_);
    initializeColorBuffer(notificationSeparatorActive_);
    initializeColorBuffer(notificationScrollbarBg_);
    initializeColorBuffer(notificationScrollbarGrab_);
    initializeColorBuffer(notificationScrollbarGrabHovered_);
    initializeColorBuffer(notificationScrollbarGrabActive_);
    initializeColorBuffer(notificationCheckMark_);
    initializeColorBuffer(notificationSliderGrab_);
    initializeColorBuffer(notificationSliderGrabActive_);
    initializeColorBuffer(notificationHeader_);
    initializeColorBuffer(notificationHeaderHovered_);
    initializeColorBuffer(notificationHeaderActive_);
    initializeColorBuffer(notificationTextColor_);
    initializeColorBuffer(notificationTextDisabled_);
    initializeColorBuffer(notificationTableBgColor_);
    
    // Initialize test window toggles
    showQuickOnlineForTesting_ = false;
    showNotificationsForTesting_ = false;
    notificationPreviewEnabled_ = false;
}

// Main render function for the unified window. Handles window setup, theme application,
// Flow:
//   1. Early exit if window not visible
//   2. Load preferences and refresh themes on first render (one-time initialization)
//   3. Handle pending close (waits for UI menu to be clean)
//   4. Set window size once (ImGuiCond_Once)
//   5. Apply theme guard for per-window theme scoping
//   6. Load window lock state from preferences
//   7. Begin window with appropriate flags (locked = no move/resize)
//   8. Handle tab selection and auto-expand requests
//   9. Render top bar, sidebar, and content area
// Invariants:
//   - Must be called on main/render thread (ImGui requirement)
//   - Window size set only once (first render)
//   - Preferences/themes loaded only once (static flags)
//   - Theme guard ensures per-window theme isolation
// Edge cases:
//   - Pending close waits for UI menu to be clean (prevents closing during context menus)
//   - Alt visibility section auto-expands if requested (from command)
//   - Preferences/themes refresh when window reopens (flags reset on close)
void MainWindow::render() {
    if (!visible_) {
        return;
    }
    
    static bool preferencesLoaded = false;
    if (!preferencesLoaded && optionsViewModel_ && !optionsViewModel_->isLoaded()) {
        emitCommand(WindowCommandType::LoadPreferences, "");
        preferencesLoaded = true;
    }
    
    static bool themesRefreshed = false;
    if (!themesRefreshed && commandHandler_) {
        // Always refresh themes when window opens, even if ViewModel not set yet
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
        preferencesLoaded = false;  // Reset so preferences reload when reopened
        themesRefreshed = false;  // Reset so themes refresh when reopened
        return;
    }
    
    static bool sizeSet = false;
    if (!sizeSet) {
        ImVec2 windowSize(900.0f, 700.0f);
        renderer->SetNextWindowSize(windowSize, 0x00000002);  // ImGuiCond_Once
        sizeSet = true;
    }
    
    std::optional<XIFriendList::Platform::Ashita::ScopedThemeGuard> themeGuard;
    if (commandHandler_) {
        auto themeTokens = commandHandler_->getCurrentThemeTokens();
        if (themeTokens.has_value()) {
            themeGuard.emplace(themeTokens.value());
        }
    }
    
    locked_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadWindowLockState(windowId_);
    
    int windowFlags = 0;
    if (locked_) {
        windowFlags = 0x0004 | 0x0002;  // ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
    }
    
    std::string windowTitle = getWindowTitle();
    
    // Begin window
    bool windowOpen = visible_;
    bool beginResult = renderer->Begin(windowTitle.c_str(), &windowOpen, windowFlags);
    
    if (!beginResult) {
        renderer->End();
        applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
        return;
    }
    
    visible_ = windowOpen;
    applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
    if (!visible_) {
        preferencesLoaded = false;  // Reset so preferences reload when reopened
        themesRefreshed = false;  // Reset so themes refresh when reopened
        renderer->End();
        return;
    }
    
    if (requestExpandAltVisibility_ && !altVisibilitySectionExpanded_) {
        altVisibilitySectionExpanded_ = true;
        XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveCollapsibleSectionState(
            windowId_, "altVisibility", true);
        requestExpandAltVisibility_ = false;
    }
    
    // Auto-refresh alt visibility data when window first opens if section is already expanded
    if (altVisibilitySectionExpanded_ && !altVisibilityDataLoaded_) {
        emitCommand(WindowCommandType::RefreshAltVisibility);
        altVisibilityDataLoaded_ = true;
    }
    
    renderTopBar();
    
    // Use Dummy() directly for proper spacing (Spacing() doesn't work with default 0.0f)
#ifndef BUILDING_TESTS
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        ::ImVec2 spacing(0.0f, 12.0f);  // 12px vertical spacing
        guiManager->Dummy(spacing);
    }
#endif
    
    // Main layout: sidebar + content area
    // Left sidebar with navigation buttons (includes lock button at bottom)
    renderSidebar();
    
    renderer->SameLine();
    
    // Right content area - full height, no reserve needed since lock button is in sidebar
    UI::ImVec2 contentAvail = renderer->GetContentRegionAvail();
    renderer->BeginChild("ContentArea", UI::ImVec2(0.0f, contentAvail.y), false, WINDOW_BODY_CHILD_FLAGS);
    renderContentArea();
    renderer->EndChild();
    
    // End window
    renderer->End();
}

void MainWindow::renderTopBar() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || friendListViewModel_ == nullptr) {
        return;
    }
    
    // Refresh button (moved from Connection section)
    ButtonSpec refreshButtonSpec;
    refreshButtonSpec.label = std::string(Constants::BUTTON_REFRESH);
    refreshButtonSpec.id = "refresh_button";
    refreshButtonSpec.enabled = friendListViewModel_->isConnected();
    refreshButtonSpec.visible = true;
    
    UI::ImVec2 refreshTextSize = renderer->CalcTextSize(Constants::BUTTON_REFRESH.data());
    refreshButtonSpec.width = refreshTextSize.x + 20.0f;
    refreshButtonSpec.height = refreshTextSize.y + 10.0f;
    
    refreshButtonSpec.onClick = [this]() {
        emitCommand(WindowCommandType::RefreshStatus);
    };
    
    createButton(refreshButtonSpec);
    
    // Tooltip
    if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
        auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
        if (guiManager) {
            guiManager->SetTooltip(std::string(Constants::TOOLTIP_REFRESH).c_str());
        }
#endif
    }
    
    // Lock button next to Refresh button
    renderer->SameLine(0.0f, 8.0f);
    float lockIconSize = 24.0f;
#ifndef BUILDING_TESTS
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager && iconManager_) {
        XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
        void* lockIcon = locked_ ? iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Lock) : iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Unlock);
        if (lockIcon) {
            ::ImVec2 btnSize(lockIconSize, lockIconSize);
            guiManager->Image(lockIcon, btnSize);
            if (guiManager->IsItemHovered()) {
                std::string tooltip = locked_ ? "Window locked" : "Lock window";
                guiManager->SetTooltip(tooltip.c_str());
            }
            if (guiManager->IsItemClicked(0)) {
                bool newLocked = !locked_;
                std::ostringstream json;
                json << "{\"windowId\":\"" << XIFriendList::Protocol::JsonUtils::escapeString(windowId_)
                     << "\",\"locked\":" << (newLocked ? "true" : "false") << "}";
                emitCommand(WindowCommandType::UpdateWindowLock, json.str());
                locked_ = newLocked;
                // Save lock state immediately
                XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveWindowLockState(windowId_, locked_);
            }
        }
    }
#endif
    
    // Icons on the right side
    renderer->SameLine(0.0f, 0.0f);
    float iconSpacing = 8.0f;
    float iconSize = 24.0f;
    
    // Push icons to the right using Dummy spacing
    float availableWidth = renderer->GetContentRegionAvail().x;
    float iconsWidth = (iconSize + iconSpacing) * 3 - iconSpacing;  // 3 social icons (lock moved to title bar)
    float rightPadding = 10.0f;
    float spacerWidth = availableWidth - iconsWidth - rightPadding;
    
    if (spacerWidth > 0) {
#ifndef BUILDING_TESTS
        if (guiManager) {
            ::ImVec2 spacer(spacerWidth, iconSize);
            guiManager->Dummy(spacer);
            guiManager->SameLine(0.0f, 0.0f);
        }
#endif
    }
    
    // Discord icon
    void* discordIcon = nullptr;
    if (iconManager_) {
        XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
        discordIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Discord);
    }
    if (discordIcon) {
        UI::ImVec2 iconSizeVec(iconSize, iconSize);
        renderer->Image(discordIcon, iconSizeVec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("Discord");
            }
#endif
            if (renderer->IsItemClicked(0)) {
                safeOpenUrl(std::string(Constants::URL_DISCORD).c_str());
            }
        }
    } else {
        ButtonSpec discordButtonSpec;
        discordButtonSpec.label = "D";
        discordButtonSpec.id = "discord_icon";
        discordButtonSpec.width = iconSize + 4.0f;
        discordButtonSpec.height = iconSize + 4.0f;
        discordButtonSpec.enabled = true;
        discordButtonSpec.visible = true;
        discordButtonSpec.onClick = []() {
            safeOpenUrl(std::string(Constants::URL_DISCORD).c_str());
        };
        createButton(discordButtonSpec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("Discord");
            }
#endif
        }
    }
    
    renderer->SameLine(0.0f, iconSpacing);
    
    // GitHub icon
    void* githubIcon = nullptr;
    if (iconManager_) {
        XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
        githubIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::GitHub);
    }
    if (githubIcon) {
        UI::ImVec2 iconSizeVec(iconSize, iconSize);
        renderer->Image(githubIcon, iconSizeVec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("GitHub");
            }
#endif
            if (renderer->IsItemClicked(0)) {
                safeOpenUrl(std::string(Constants::URL_GITHUB).c_str());
            }
        }
    } else {
        ButtonSpec githubButtonSpec;
        githubButtonSpec.label = "G";
        githubButtonSpec.id = "github_icon";
        githubButtonSpec.width = iconSize + 4.0f;
        githubButtonSpec.height = iconSize + 4.0f;
        githubButtonSpec.enabled = true;
        githubButtonSpec.visible = true;
        githubButtonSpec.onClick = []() {
            safeOpenUrl(std::string(Constants::URL_GITHUB).c_str());
        };
        createButton(githubButtonSpec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("GitHub");
            }
#endif
        }
    }
    
    renderer->SameLine(0.0f, iconSpacing);
    
    // Heart icon (About/Thanks)
    void* heartIcon = nullptr;
    if (iconManager_) {
        XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
        heartIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Heart);
    }
    if (heartIcon) {
        UI::ImVec2 iconSizeVec(iconSize, iconSize);
        renderer->Image(heartIcon, iconSizeVec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("About / Special Thanks");
            }
#endif
            if (renderer->IsItemClicked(0)) {
                if (!showAboutPopup_) {
                    showAboutPopup_ = true;
                    aboutPopupJustOpened_ = true;
                }
            }
        }
    } else {
        ButtonSpec heartButtonSpec;
        heartButtonSpec.label = "♥";
        heartButtonSpec.id = "heart_icon";
        heartButtonSpec.width = iconSize + 4.0f;
        heartButtonSpec.height = iconSize + 4.0f;
        heartButtonSpec.enabled = true;
        heartButtonSpec.visible = true;
        heartButtonSpec.onClick = [this]() {
            if (!showAboutPopup_) {
                showAboutPopup_ = true;
                aboutPopupJustOpened_ = true;
            }
        };
        createButton(heartButtonSpec);
        if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
            if (guiManager) {
                guiManager->SetTooltip("About / Special Thanks");
            }
#endif
        }
    }
    
    
    // About/Thanks popup
    // MANUAL E2E TEST REQUIRED: Verify icons appear far right of Refresh button,
    // tooltips display correctly on hover, Discord/GitHub links open correctly,
    // and Heart icon opens popup with plugin name, version, and Special Thanks
    // (Ashita and XIUI credits).
    if (aboutPopupJustOpened_) {
        renderer->OpenPopup("##about_popup");
        aboutPopupJustOpened_ = false;
    }
    
    if (renderer->BeginPopup("##about_popup")) {
        renderer->TextUnformatted((pluginName_ + " " + pluginVersion_).c_str());
        
        std::string buildInfo = "Compiled On: " + std::string(__DATE__) + " " + std::string(__TIME__);
        renderer->TextUnformatted(buildInfo.c_str());
        
        renderer->Spacing(4.0f);
        renderer->Separator();
        renderer->Spacing(4.0f);
        
        renderer->TextUnformatted("Special Thanks:");
        renderer->TextUnformatted("  - Ashita (Atom0s and Thorny)");
        renderer->TextUnformatted("  - XIUI (for inspiration)");
        renderer->EndPopup();
    } else {
        if (showAboutPopup_) {
            showAboutPopup_ = false;
        }
    }
}

void MainWindow::renderSidebar() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    const float sidebarWidth = 180.0f;
    
    renderer->BeginChild("Sidebar", UI::ImVec2(sidebarWidth, 0.0f), false, 0);
    
#ifndef BUILDING_TESTS
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        // Push style for buttons
        ::ImVec2 framePadding(10.0f, 8.0f);
        guiManager->PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
        
        struct TabInfo {
            Tab tab;
            std::string_view label;
        };
        
        TabInfo tabs[] = {
            { Tab::Friends, Constants::TAB_FRIENDS },
            { Tab::Privacy, Constants::TAB_PRIVACY },
            { Tab::Notifications, Constants::TAB_NOTIFICATIONS },
            { Tab::Controls, Constants::TAB_CONTROLS },
            { Tab::Themes, Constants::TAB_THEMES }
        };
        
        for (const auto& tabInfo : tabs) {
            bool isSelected = (selectedTab_ == tabInfo.tab);
            
            if (isSelected) {
                ::ImVec4 selectedColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray for selected
                guiManager->PushStyleColor(ImGuiCol_Button, selectedColor);
                guiManager->PushStyleColor(ImGuiCol_ButtonHovered, selectedColor);
                guiManager->PushStyleColor(ImGuiCol_ButtonActive, selectedColor);
            } else {
                ::ImVec4 transparent(0.0f, 0.0f, 0.0f, 0.0f);
                ::ImVec4 hoverColor(0.3f, 0.3f, 0.3f, 1.0f);
                ::ImVec4 activeColor(0.4f, 0.4f, 0.4f, 1.0f);
                guiManager->PushStyleColor(ImGuiCol_Button, transparent);
                guiManager->PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
                guiManager->PushStyleColor(ImGuiCol_ButtonActive, activeColor);
            }
            
            ::ImVec2 btnPos = guiManager->GetCursorScreenPos();
            
            std::string label = std::string(tabInfo.label);
            ::ImVec2 buttonSize(sidebarWidth - 16.0f, 32.0f);
            if (guiManager->Button(label.c_str(), buttonSize)) {
                selectedTab_ = tabInfo.tab;
            }
            
            guiManager->PopStyleColor(3);
        }
        
        guiManager->PopStyleVar();
    } else {
        for (int i = 0; i < 5; ++i) {
            Tab tab = static_cast<Tab>(i);
            std::string label;
            switch (tab) {
                case Tab::Friends: label = std::string(Constants::TAB_FRIENDS); break;
                case Tab::Privacy: label = std::string(Constants::TAB_PRIVACY); break;
                case Tab::Notifications: label = std::string(Constants::TAB_NOTIFICATIONS); break;
                case Tab::Controls: label = std::string(Constants::TAB_CONTROLS); break;
                case Tab::Themes: label = std::string(Constants::TAB_THEMES); break;
            }
            
            ButtonSpec buttonSpec;
            buttonSpec.label = label;
            buttonSpec.id = "sidebar_tab_" + std::to_string(i);
            buttonSpec.enabled = true;
            buttonSpec.visible = true;
            if (createButton(buttonSpec)) {
                selectedTab_ = tab;
                // TODO: Persist selected tab when proper int preference storage is available
            }
            renderer->NewLine();
        }
    }
#else
    ButtonSpec friendsButton;
    friendsButton.label = std::string(Constants::TAB_FRIENDS);
    friendsButton.id = "sidebar_friends";
    if (createButton(friendsButton)) {
        selectedTab_ = Tab::Friends;
    }
    renderer->NewLine();
    
    ButtonSpec privacyButton;
    privacyButton.label = std::string(Constants::TAB_PRIVACY);
    privacyButton.id = "sidebar_privacy";
    if (createButton(privacyButton)) {
        selectedTab_ = Tab::Privacy;
    }
    renderer->NewLine();
    
    ButtonSpec notificationsButton;
    notificationsButton.label = std::string(Constants::TAB_NOTIFICATIONS);
    notificationsButton.id = "sidebar_notifications";
    if (createButton(notificationsButton)) {
        selectedTab_ = Tab::Notifications;
    }
    renderer->NewLine();
    
    ButtonSpec controlsButton;
    controlsButton.label = std::string(Constants::TAB_CONTROLS);
    controlsButton.id = "sidebar_controls";
    if (createButton(controlsButton)) {
        selectedTab_ = Tab::Controls;
    }
    renderer->NewLine();
    
    ButtonSpec themesButton;
    themesButton.label = std::string(Constants::TAB_THEMES);
    themesButton.id = "sidebar_themes";
    if (createButton(themesButton)) {
        selectedTab_ = Tab::Themes;
    }
#endif
    
    renderer->EndChild();
}

void MainWindow::renderContentArea() {
    switch (selectedTab_) {
        case Tab::Friends:
            renderFriendsTab();
            break;
        case Tab::Privacy:
            renderPrivacyTab();
            break;
        case Tab::Notifications:
            renderNotificationsTab();
            break;
        case Tab::Controls:
            renderControlsTab();
            break;

        case Tab::Themes:
            renderThemesTab();
            break;

    }
}

void MainWindow::renderFriendsTab() {
    if (friendListViewModel_ == nullptr) {
        return;
    }
    
    renderAddFriendSection();
    
    auto* renderer = GetUiRenderer();
    if (renderer) {
        renderer->NewLine();
        renderer->Spacing(5.0f);
    }
    
    renderPendingRequestsSection();
    
    if (renderer) {
        renderer->NewLine();
        renderer->Spacing(5.0f);
    }

    if (renderer->BeginChild("##friends_table_child", UI::ImVec2(0.0f, 0.0f), false, WINDOW_BODY_CHILD_FLAGS)) {
        friendTable_.render();
        renderer->EndChild();
    }
    
    if (!selectedFriendForDetails_.empty()) {
        renderFriendDetailsPopup();
    }
}

void MainWindow::renderPrivacyTab() {
    renderFriendViewSettingsSection();
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer) {
        renderer->Spacing(5.0f);
    }
    
    renderPrivacyControlsSection();
    
    if (renderer) {
        renderer->Spacing(5.0f);
    }
    
    // Alt visibility section
    renderAltVisibilitySection();
}

void MainWindow::renderNotificationsTab() {
    renderNotificationsSection();
}

void MainWindow::renderControlsTab() {
    // Controls section
    renderControlsSection();
    
    // Debug section (only in Debug builds)
#ifndef NDEBUG
    renderDebugSettingsSection();
#endif
}

void MainWindow::renderThemesTab() {
    renderThemeSettingsSection();
}

void MainWindow::renderAddFriendSection() {
    if (friendListViewModel_ == nullptr) {
        return;
    }
    
    // Two inputs side-by-side: Friend Name and Note, with Add Friend button
    
    auto* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    auto sendFriendRequest = [this]() {
        if (newFriendInput_.empty() || !friendListViewModel_ || !friendListViewModel_->isConnected()) {
            return;
        }
        
        std::string commandData = newFriendInput_;
        if (!newFriendNoteInput_.empty()) {
            commandData += "|" + newFriendNoteInput_;
        }
        
        emitCommand(WindowCommandType::SendFriendRequest, commandData);
        
        newFriendInput_.clear();
        newFriendNoteInput_.clear();
    };
    
    // Layout: Friend Name label -> Friend Name input -> Note label -> Note input -> Add Friend button
    // Temporarily set ItemSpacing to 0 to remove gaps between elements, then restore
#ifndef BUILDING_TESTS
    IGuiManager* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        // Push ItemSpacing to 0 to remove gaps between elements
        ::ImVec2 zeroSpacing(0.0f, 0.0f);
        guiManager->PushStyleVar(ImGuiStyleVar_ItemSpacing, zeroSpacing);
    }
#endif
    
    const float labelSpacing = 5.0f;  // Small spacing between label and input
    const float nameInputWidth = 150.0f;  // Width for friend name input
    const float noteInputWidth = 250.0f;  // Width for note input
    
    renderer->TextUnformatted("Friend Name:");
    renderer->SameLine(0.0f, labelSpacing);
#ifndef BUILDING_TESTS
    if (guiManager) {
        guiManager->PushItemWidth(nameInputWidth);
    }
#endif
    InputTextSpec nameInputSpec;
    nameInputSpec.label = "##friend_name";
    nameInputSpec.id = "new_friend_input";
    nameInputSpec.buffer = &newFriendInput_;
    nameInputSpec.bufferSize = 256;
    nameInputSpec.enabled = friendListViewModel_->isConnected();
    nameInputSpec.visible = true;
    nameInputSpec.onEnter = [sendFriendRequest](const std::string& text) {
        if (!text.empty()) {
            sendFriendRequest();
        }
    };
    createInputText(nameInputSpec);
#ifndef BUILDING_TESTS
    if (guiManager) {
        guiManager->PopItemWidth();
    }
#endif
    
    renderer->SameLine(0.0f, labelSpacing);
    renderer->TextUnformatted("Note:");
    renderer->SameLine(0.0f, labelSpacing);
    
#ifndef BUILDING_TESTS
    if (guiManager) {
        guiManager->PushItemWidth(noteInputWidth);
    }
#endif
    InputTextSpec noteInputSpec;
    noteInputSpec.label = "##friend_note";
    noteInputSpec.id = "new_friend_note_input";
    noteInputSpec.buffer = &newFriendNoteInput_;
    noteInputSpec.bufferSize = 512;  // Larger buffer for notes
    noteInputSpec.enabled = friendListViewModel_->isConnected();
    noteInputSpec.visible = true;
    noteInputSpec.onEnter = [sendFriendRequest](const std::string& text) {
        // Enter in note field sends the request
        sendFriendRequest();
    };
    createInputText(noteInputSpec);
#ifndef BUILDING_TESTS
    if (guiManager) {
        guiManager->PopItemWidth();
    }
#endif
    
    renderer->SameLine(0.0f, labelSpacing);
    ButtonSpec addButtonSpec;
    addButtonSpec.label = std::string(Constants::BUTTON_ADD_FRIEND);
    addButtonSpec.id = "add_friend_button";
    addButtonSpec.enabled = friendListViewModel_->isConnected() && !newFriendInput_.empty();
    addButtonSpec.visible = true;
    addButtonSpec.onClick = sendFriendRequest;
    createButton(addButtonSpec);
    
#ifndef BUILDING_TESTS
    // Restore ItemSpacing
    if (guiManager) {
        guiManager->PopStyleVar(1);
    }
#endif
}

void MainWindow::renderPendingRequestsSection() {
    if (friendListViewModel_ == nullptr) {
        return;
    }
    
    const auto& incoming = friendListViewModel_->getIncomingRequests();
    const auto& outgoing = friendListViewModel_->getOutgoingRequests();
    
    // Always show section with count (even if 0) - only count incoming requests
    int incomingCount = static_cast<int>(incoming.size());
    // Use direct ImGui calls for collapsible section with persistence
    std::string sectionLabel = std::string(Constants::HEADER_PENDING_REQUESTS);
    sectionLabel += " (" + std::to_string(incomingCount) + ")";  // Always show count, including 0
    
#ifndef BUILDING_TESTS
    IGuiManager* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        // Render collapsible header (direct IGuiManager call)
        bool openNow = guiManager->CollapsingHeader(sectionLabel.c_str(), nullptr);
        
        // Detect state change and save (only save when state actually changes)
        if (openNow != pendingRequestsSectionExpanded_) {
            pendingRequestsSectionExpanded_ = openNow;
            XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveCollapsibleSectionState(
                windowId_, "pendingRequests", openNow);
        }
    } else {
        // Fallback to abstraction if guiManager not available
        SectionHeaderSpec headerSpec;
        headerSpec.label = sectionLabel;
        headerSpec.id = "pending_requests_header";
        headerSpec.visible = true;
        headerSpec.collapsible = true;
        bool collapsed = !pendingRequestsSectionExpanded_;
        headerSpec.collapsed = &collapsed;
        createSectionHeader(headerSpec);
        pendingRequestsSectionExpanded_ = !collapsed;
    }
#else
    // Test build - use abstraction
    SectionHeaderSpec headerSpec;
    headerSpec.label = sectionLabel;
    headerSpec.id = "pending_requests_header";
    headerSpec.visible = true;
    headerSpec.collapsible = true;
    bool collapsed = !pendingRequestsSectionExpanded_;
    headerSpec.collapsed = &collapsed;
    createSectionHeader(headerSpec);
    pendingRequestsSectionExpanded_ = !collapsed;
#endif
    
    if (!pendingRequestsSectionExpanded_) {
        return;
    }
    
    // Show incoming requests
    if (!incoming.empty()) {
        TextSpec incomingLabelSpec;
        incomingLabelSpec.text = "Incoming (" + std::to_string(incoming.size()) + "):";
        incomingLabelSpec.id = "incoming_label";
        incomingLabelSpec.visible = true;
        createText(incomingLabelSpec);
        
        for (const auto& request : incoming) {
            IUiRenderer* renderer = GetUiRenderer();
            if (renderer) {
                // Display icon + name + buttons all on one line
                void* iconHandle = nullptr;
                if (iconManager_) {
                    XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
                    iconHandle = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Pending);
                }
                
                // Render icon if available (full opacity for pending)
                const float iconSize = 13.0f;
                if (iconHandle) {
                    UI::ImVec2 iconSizeVec(iconSize, iconSize);
                    UI::ImVec4 tintCol(1.0f, 1.0f, 1.0f, 1.0f);  // Full opacity for pending
                    renderer->Image(iconHandle, iconSizeVec, UI::ImVec2(0, 0), UI::ImVec2(1, 1), tintCol);
                    renderer->SameLine(0, 4.0f);  // Small spacing after icon
                }
                
                // Display request from user (capitalize name)
                std::string displayName = request.fromCharacterName;
                if (!displayName.empty()) {
                    displayName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(displayName[0])));
                    for (size_t i = 1; i < displayName.length(); ++i) {
                        if (displayName[i-1] == ' ') {
                            displayName[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(displayName[i])));
                        } else {
                            displayName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(displayName[i])));
                        }
                    }
                }
                TextSpec requestTextSpec;
                requestTextSpec.text = displayName;
                requestTextSpec.id = "incoming_request_" + request.requestId;
                requestTextSpec.visible = true;
                createText(requestTextSpec);
                
                // Accept button (on same line)
                renderer->SameLine(0, 8.0f);  // Spacing before buttons
                ButtonSpec acceptButtonSpec;
                acceptButtonSpec.label = std::string(Constants::BUTTON_ACCEPT_REQUEST);
                acceptButtonSpec.id = "accept_" + request.requestId;
                acceptButtonSpec.enabled = friendListViewModel_->isConnected();
                acceptButtonSpec.visible = true;
                acceptButtonSpec.onClick = [this, request]() {
                    emitCommand(WindowCommandType::AcceptFriendRequest, request.requestId);
                };
                createButton(acceptButtonSpec);
                
                // Reject button (on same line)
                renderer->SameLine(0, 4.0f);  // Spacing between buttons
                ButtonSpec rejectButtonSpec;
                rejectButtonSpec.label = std::string(Constants::BUTTON_REJECT_REQUEST);
                rejectButtonSpec.id = "reject_" + request.requestId;
                rejectButtonSpec.enabled = friendListViewModel_->isConnected();
                rejectButtonSpec.visible = true;
                rejectButtonSpec.onClick = [this, request]() {
                    emitCommand(WindowCommandType::RejectFriendRequest, request.requestId);
                };
                createButton(rejectButtonSpec);
            }
        }
    }
    
    // Show message when empty
    if (incoming.empty() && outgoing.empty()) {
        TextSpec emptySpec;
        emptySpec.text = std::string(Constants::MESSAGE_NO_PENDING_REQUESTS);
        emptySpec.id = "pending_requests_empty";
        emptySpec.visible = true;
        createText(emptySpec);
    }
}

void MainWindow::renderFriendViewSettingsSection() {
    if (!optionsViewModel_) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    bool isOpen = createCollapsibleSection("Friend View Settings", "friend_view_settings", [this]() {
        this->renderFriendViewSettings();
    });
    
    if (isOpen != friendViewSettingsSectionExpanded_) {
        friendViewSettingsSectionExpanded_ = isOpen;
        XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveCollapsibleSectionState(windowId_, "friendViewSettings", friendViewSettingsSectionExpanded_);
    }
}

void MainWindow::renderFriendViewSettings() {
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer || !optionsViewModel_) {
        return;
    }
        
    // Main Window View
    TextSpec mainHeaderSpec;
    mainHeaderSpec.text = "Main Window View";
    mainHeaderSpec.id = "main_window_view_header";
    mainHeaderSpec.visible = true;
    createText(mainHeaderSpec);
    renderer->NewLine();
    renderer->Separator();  

    Core::FriendViewSettings mainView = optionsViewModel_->getMainFriendView();
    
    bool showJob = mainView.showJob;
    ToggleSpec jobSpec;
    jobSpec.label = "Show Job";
    jobSpec.id = "main_show_job";
    jobSpec.value = &showJob;
    jobSpec.enabled = true;
    jobSpec.visible = true;
    jobSpec.onChange = [this, &showJob]() {
        Core::FriendViewSettings settings = optionsViewModel_->getMainFriendView();
        settings.showJob = showJob;
        optionsViewModel_->setMainFriendView(settings);
        friendTable_.setViewSettings(settings);
        emitPreferenceUpdate("mainFriendView.showJob", showJob);
    };
    createToggle(jobSpec);
    renderer->NewLine();
    
    bool showZone = mainView.showZone;
    ToggleSpec zoneSpec;
    zoneSpec.label = "Show Zone";
    zoneSpec.id = "main_show_zone";
    zoneSpec.value = &showZone;
    zoneSpec.enabled = true;
    zoneSpec.visible = true;
    zoneSpec.onChange = [this, &showZone]() {
        Core::FriendViewSettings settings = optionsViewModel_->getMainFriendView();
        settings.showZone = showZone;
        optionsViewModel_->setMainFriendView(settings);
        friendTable_.setViewSettings(settings);
        emitPreferenceUpdate("mainFriendView.showZone", showZone);
    };
    createToggle(zoneSpec);
    renderer->NewLine();
    
    bool showNationRank = mainView.showNationRank;
    ToggleSpec nationRankSpec;
    nationRankSpec.label = "Show Nation/Rank";
    nationRankSpec.id = "main_show_nation_rank";
    nationRankSpec.value = &showNationRank;
    nationRankSpec.enabled = true;
    nationRankSpec.visible = true;
    nationRankSpec.onChange = [this, &showNationRank]() {
        Core::FriendViewSettings settings = optionsViewModel_->getMainFriendView();
        settings.showNationRank = showNationRank;
        optionsViewModel_->setMainFriendView(settings);
        friendTable_.setViewSettings(settings);
        emitPreferenceUpdate("mainFriendView.showNationRank", showNationRank);
    };
    createToggle(nationRankSpec);
    renderer->NewLine();
    
    bool showLastSeen = mainView.showLastSeen;
    ToggleSpec lastSeenSpec;
    lastSeenSpec.label = "Show Last Seen";
    lastSeenSpec.id = "main_show_last_seen";
    lastSeenSpec.value = &showLastSeen;
    lastSeenSpec.enabled = true;
    lastSeenSpec.visible = true;
    lastSeenSpec.onChange = [this, &showLastSeen]() {
        Core::FriendViewSettings settings = optionsViewModel_->getMainFriendView();
        settings.showLastSeen = showLastSeen;
        optionsViewModel_->setMainFriendView(settings);
        friendTable_.setViewSettings(settings);
        emitPreferenceUpdate("mainFriendView.showLastSeen", showLastSeen);
    };
    createToggle(lastSeenSpec);
    renderer->NewLine();

    renderer->Spacing(2.0f);

    // Quick Online View
    TextSpec quickHeaderSpec;
    quickHeaderSpec.text = "Quick Online View";
    quickHeaderSpec.id = "quick_online_view_header";
    quickHeaderSpec.visible = true;
    createText(quickHeaderSpec);
    renderer->NewLine();
    renderer->Separator();  

    Core::FriendViewSettings quickView = optionsViewModel_->getQuickOnlineFriendView();
    
    bool quickShowJob = quickView.showJob;
    ToggleSpec quickJobSpec;
    quickJobSpec.label = "Show Job";
    quickJobSpec.id = "quick_show_job";
    quickJobSpec.value = &quickShowJob;
    quickJobSpec.enabled = true;
    quickJobSpec.visible = true;
    quickJobSpec.onChange = [this, &quickShowJob]() {
        Core::FriendViewSettings settings = optionsViewModel_->getQuickOnlineFriendView();
        settings.showJob = quickShowJob;
        optionsViewModel_->setQuickOnlineFriendView(settings);
        emitPreferenceUpdate("quickOnlineFriendView.showJob", quickShowJob);
    };
    createToggle(quickJobSpec);
    renderer->NewLine();
    
    bool quickShowZone = quickView.showZone;
    ToggleSpec quickZoneSpec;
    quickZoneSpec.label = "Show Zone";
    quickZoneSpec.id = "quick_show_zone";
    quickZoneSpec.value = &quickShowZone;
    quickZoneSpec.enabled = true;
    quickZoneSpec.visible = true;
    quickZoneSpec.onChange = [this, &quickShowZone]() {
        Core::FriendViewSettings settings = optionsViewModel_->getQuickOnlineFriendView();
        settings.showZone = quickShowZone;
        optionsViewModel_->setQuickOnlineFriendView(settings);
        emitPreferenceUpdate("quickOnlineFriendView.showZone", quickShowZone);
    };
    createToggle(quickZoneSpec);
    renderer->NewLine();
    
    bool quickShowNationRank = quickView.showNationRank;
    ToggleSpec quickNationRankSpec;
    quickNationRankSpec.label = "Show Nation/Rank";
    quickNationRankSpec.id = "quick_show_nation_rank";
    quickNationRankSpec.value = &quickShowNationRank;
    quickNationRankSpec.enabled = true;
    quickNationRankSpec.visible = true;
    quickNationRankSpec.onChange = [this, &quickShowNationRank]() {
        Core::FriendViewSettings settings = optionsViewModel_->getQuickOnlineFriendView();
        settings.showNationRank = quickShowNationRank;
        optionsViewModel_->setQuickOnlineFriendView(settings);
        emitPreferenceUpdate("quickOnlineFriendView.showNationRank", quickShowNationRank);
    };
    createToggle(quickNationRankSpec);
    renderer->NewLine();
    
    bool quickShowLastSeen = quickView.showLastSeen;
    ToggleSpec quickLastSeenSpec;
    quickLastSeenSpec.label = "Show Last Seen";
    quickLastSeenSpec.id = "quick_show_last_seen";
    quickLastSeenSpec.value = &quickShowLastSeen;
    quickLastSeenSpec.enabled = true;
    quickLastSeenSpec.visible = true;
    quickLastSeenSpec.onChange = [this, &quickShowLastSeen]() {
        Core::FriendViewSettings settings = optionsViewModel_->getQuickOnlineFriendView();
        settings.showLastSeen = quickShowLastSeen;
        optionsViewModel_->setQuickOnlineFriendView(settings);
        emitPreferenceUpdate("quickOnlineFriendView.showLastSeen", quickShowLastSeen);
    };
    createToggle(quickLastSeenSpec);
    renderer->NewLine();
}

void MainWindow::renderPrivacyControlsSection() {
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    bool isOpen = createCollapsibleSection("Privacy", "privacy_controls", [this]() {
        this->renderPrivacyControls();
    });
    
    if (isOpen != privacySectionExpanded_) {
        privacySectionExpanded_ = isOpen;
        XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveCollapsibleSectionState(windowId_, "privacy", privacySectionExpanded_);
    }
}

void MainWindow::renderPrivacyControls() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || optionsViewModel_ == nullptr) {
        return;
    }
    
    // Share Job, Nation, and Rank when Anonymous
    bool shareJobWhenAnonymous = optionsViewModel_->getShareJobWhenAnonymous();
    ToggleSpec toggleSpec;
    toggleSpec.label = std::string(Constants::LABEL_SHARE_JOB_NATION_RANK_ANONYMOUS);
    toggleSpec.id = "share_job_anonymous_toggle";
    toggleSpec.value = &shareJobWhenAnonymous;
    toggleSpec.enabled = true;
    toggleSpec.visible = true;
    toggleSpec.onChange = [this, &shareJobWhenAnonymous]() {
        optionsViewModel_->setShareJobWhenAnonymous(shareJobWhenAnonymous);
        emitPreferenceUpdate("shareJobWhenAnonymous", shareJobWhenAnonymous);
    };
    createToggle(toggleSpec);
    HelpMarker("When enabled, your job, nation, and rank are shared even when you're set to anonymous mode.");
    renderer->NewLine();
    
    // Show Online Status
    bool showOnlineStatus = optionsViewModel_->getShowOnlineStatus();
    ToggleSpec statusSpec;
    statusSpec.label = std::string(Constants::LABEL_SHOW_ONLINE_STATUS);
    statusSpec.id = "show_online_status_toggle";
    statusSpec.value = &showOnlineStatus;
    statusSpec.enabled = true;
    statusSpec.visible = true;
    statusSpec.onChange = [this, &showOnlineStatus]() {
        optionsViewModel_->setShowOnlineStatus(showOnlineStatus);
        emitPreferenceUpdate("showOnlineStatus", showOnlineStatus);
    };
    createToggle(statusSpec);
    HelpMarker("Controls whether your online status is visible to friends.");
    renderer->NewLine();
    
    // Share Location
    bool shareLocation = optionsViewModel_->getShareLocation();
    ToggleSpec locationSpec;
    locationSpec.label = std::string(Constants::LABEL_SHARE_LOCATION);
    locationSpec.id = "share_location_toggle";
    locationSpec.value = &shareLocation;
    locationSpec.enabled = true;
    locationSpec.visible = true;
    locationSpec.onChange = [this, &shareLocation]() {
        optionsViewModel_->setShareLocation(shareLocation);
        emitPreferenceUpdate("shareLocation", shareLocation);
    };
    createToggle(locationSpec);
    HelpMarker("When enabled, your current zone and position are shared with friends.");
    renderer->NewLine();
}

void MainWindow::renderAltVisibilitySection() {
    if (!altVisibilityViewModel_) {
        return;
    }
    
    // Auto-refresh alt visibility data when window first opens
    if (!altVisibilityDataLoaded_) {
        emitCommand(WindowCommandType::RefreshAltVisibility);
        altVisibilityDataLoaded_ = true;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    bool isOpen = createCollapsibleSection("Alt Online Visibility", "alt_visibility", [this]() {
        this->renderAltVisibilityContent();
    });
    
    if (isOpen != altVisibilitySectionExpanded_) {
        altVisibilitySectionExpanded_ = isOpen;
        XIFriendList::Platform::Ashita::AshitaPreferencesStore::saveCollapsibleSectionState(windowId_, "altVisibility", altVisibilitySectionExpanded_);
    }
}

void MainWindow::renderAltVisibilityContent() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || optionsViewModel_ == nullptr) {
        return;
    }
    // Share Visibility of Alt's to all Friends (Account-level preference)
    // This toggle controls whether alt visibility is shared across all friends
    bool shareFriendsAcrossAlts = optionsViewModel_->getShareFriendsAcrossAlts();
    ToggleSpec shareFriendsSpec;
    shareFriendsSpec.label = std::string(Constants::LABEL_SHARE_VISIBILITY_OF_ALTS);
    shareFriendsSpec.id = "share_friends_across_alts_toggle";
    shareFriendsSpec.value = &shareFriendsAcrossAlts;
    shareFriendsSpec.enabled = true;
    shareFriendsSpec.visible = true;
    shareFriendsSpec.onChange = [this, &shareFriendsAcrossAlts]() {
        optionsViewModel_->setShareFriendsAcrossAlts(shareFriendsAcrossAlts);
        emitPreferenceUpdate("shareFriendsAcrossAlts", shareFriendsAcrossAlts);
        // Automatically refresh alt visibility data when toggle changes
        emitCommand(WindowCommandType::RefreshAltVisibility);
    };
    createToggle(shareFriendsSpec);
    HelpMarker("When enabled, alt visibility settings are shared across all friends. When disabled, you can set visibility per friend per character. Alt visibility is shared across all friends when enabled. Disable sharing to manage per-character visibility.");
    renderer->NewLine();
    
    // Show explanatory text when sharing is enabled
    if (shareFriendsAcrossAlts) {
        TextSpec infoSpec;
        infoSpec.text = "A table will appear below if you disable sharing to manage visibility per friend per character.";
        infoSpec.id = "alt_visibility_sharing_info";
        infoSpec.visible = true;
        createText(infoSpec);
        renderer->NewLine();
    } else {
        // Search/filter input - show after toggle, before table (only when not sharing)
        if (altVisibilityViewModel_) {
            renderer->Spacing(5.0f);
            InputTextSpec filterSpec;
            filterSpec.label = std::string(Constants::LABEL_SEARCH);
            filterSpec.id = "alt_visibility_filter";
            filterSpec.buffer = &altVisibilityFilterText_;
            filterSpec.bufferSize = 256;
            filterSpec.visible = true;
            filterSpec.enabled = true;
            filterSpec.onEnter = [this](const std::string&) {
                // Allow Enter key to work normally (no special action needed)
            };
            createInputText(filterSpec);
            renderer->NewLine();
        }
        
        // Show the table only when not sharing
        renderAltVisibilityFriendTable();
    }
}

void MainWindow::renderAltVisibilityFriendTable() {
    if (!altVisibilityViewModel_) {
        return;
    }
    
    
    const auto& allRows = altVisibilityViewModel_->getRows();
    auto rows = altVisibilityViewModel_->getFilteredRows(altVisibilityFilterText_);
    const auto& characters = altVisibilityViewModel_->getCharacters();
    
    if (rows.empty()) {
        TextSpec emptySpec;
        emptySpec.text = allRows.empty() ? "No friends found" : "No friends match filter";
        emptySpec.id = "alt_visibility_empty";
        emptySpec.visible = true;
        createText(emptySpec);
        return;
    }
    
    if (characters.empty()) {
        TextSpec emptySpec;
        emptySpec.text = "No characters found";
        emptySpec.id = "alt_visibility_no_chars";
        emptySpec.visible = true;
        createText(emptySpec);
        return;
    }
    
    TableSpec tableSpec;
    tableSpec.id = "alt_visibility_table";
    tableSpec.visible = true;
    tableSpec.rowCount = rows.size();
    
    // Setup columns: Friend Name + Shown/Hidden + one column per character
    TableColumnSpec nameCol;
    nameCol.header = "Friend";
    nameCol.id = "friend_name_col";
    nameCol.width = 200.0f;
    tableSpec.columns.push_back(nameCol);
    
    TableColumnSpec statusCol;
    statusCol.header = "Shown/Hidden";
    statusCol.id = "visibility_status_col";
    statusCol.width = 120.0f;
    tableSpec.columns.push_back(statusCol);
    
    for (const auto& charInfo : characters) {
        TableColumnSpec charCol;
        charCol.header = capitalizeWords(charInfo.characterName);
        charCol.id = "char_col_" + std::to_string(charInfo.characterId);
        charCol.width = 120.0f;
        tableSpec.columns.push_back(charCol);
    }
    
    tableSpec.rowRenderer = [this, &rows, &characters](size_t rowIndex) -> std::vector<std::string> {
        if (rowIndex >= rows.size()) {
            return {};
        }
        const auto& row = rows[rowIndex];
        
        std::vector<std::string> cells;
        std::string friendName = row.friendedAsName;
        if (!friendName.empty()) {
            friendName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(friendName[0])));
            for (size_t i = 1; i < friendName.length(); ++i) {
                if (friendName[i - 1] == ' ') {
                    friendName[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(friendName[i])));
                } else {
                    friendName[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(friendName[i])));
                }
            }
        }
        cells.push_back(friendName);
        
        // Shown/Hidden status based on visibilityMode
        std::string statusText = (row.visibilityMode == "ALL") ? "Shown" : "Hidden";
        cells.push_back(statusText);
        
        // Add placeholder text for each character column (will be replaced by checkboxes in cellRenderer)
        for (size_t i = 0; i < characters.size(); i++) {
            cells.push_back("");  // Empty string - checkbox will be rendered in cellRenderer
        }
        
        return cells;
    };
    
    tableSpec.cellRenderer = [this, &rows, &characters](size_t rowIndex, size_t colIndex, const std::string& colId) -> bool {
        if (rowIndex >= rows.size()) {
            return false;
        }
        
        // Column 0 = Friend name, Column 1 = Shown/Hidden status - use default rendering
        if (colIndex == 0 || colIndex == 1) {
            return false;
        }
        
        // Character columns start at index 2 (after Friend and Shown/Hidden)
        size_t charIndex = colIndex - 2;
        if (charIndex >= characters.size()) {
            return false;
        }
        
        const auto& row = rows[rowIndex];
        const auto& charInfo = characters[charIndex];
        
        // Find character visibility data for this character
        const CharacterVisibilityData* charVis = nullptr;
        for (const auto& cv : row.characterVisibility) {
            if (cv.characterId == charInfo.characterId) {
                charVis = &cv;
                break;
            }
        }
        
        if (!charVis) {
            return false;  // Character not found in visibility data
        }
        
        int checkboxKey = row.friendAccountId * 1000000 + charInfo.characterId;  // Simple composite key
        bool& checkboxValue = altVisibilityCheckboxValues_[checkboxKey];
        
        // 1. If busy, always preserve (user's intent during request)
        // 2. If state is PendingRequest and checkbox is true, preserve (user toggled ON, request pending)
        // 3. If checkbox is true but state is NotVisible, preserve (user just toggled ON, server hasn't confirmed yet)
        //    This handles the case where visibility is requested for a different character (alt vs main)
        // 4. Otherwise, sync from ViewModel (server truth)
        bool viewModelChecked = charVis->checkboxChecked();
        bool shouldPreserve = charVis->isBusy || 
                             (charVis->visibilityState == AltVisibilityState::PendingRequest && checkboxValue) ||
                             (checkboxValue && charVis->visibilityState == AltVisibilityState::NotVisible && !viewModelChecked);
        
        if (!shouldPreserve) {
            checkboxValue = viewModelChecked;  // Sync with ViewModel state
        }
        
        // Check if sharing is enabled (checkboxes should be disabled when sharing)
        bool sharingEnabled = optionsViewModel_ && optionsViewModel_->getShareFriendsAcrossAlts();
        
        // Render checkbox (always render, but may be disabled)
        ToggleSpec toggleSpec;
        toggleSpec.label = "";  // No label, checkbox only
        toggleSpec.id = "visibility_checkbox_" + std::to_string(row.friendAccountId) + "_" + std::to_string(charInfo.characterId) + "_" + std::to_string(rowIndex);
        toggleSpec.value = &checkboxValue;
        toggleSpec.enabled = !sharingEnabled && charVis->checkboxEnabled();
        toggleSpec.visible = true;
        toggleSpec.onChange = [this, row, charInfo, &checkboxValue]() {
            // Toggle visibility for this specific character
            bool desiredVisible = checkboxValue;
            emitCommand(WindowCommandType::ToggleFriendVisibility, 
                       std::to_string(row.friendAccountId) + "|" + std::to_string(charInfo.characterId) + "|" + row.friendedAsName + "|" + (desiredVisible ? "true" : "false"));
        };
        
        createToggle(toggleSpec);
        
        if (!charVis->checkboxEnabled()) {
            std::string statusText;
            if (charVis->visibilityState == AltVisibilityState::PendingRequest) {
                statusText = " (Pending)";
            } else if (charVis->visibilityState == AltVisibilityState::Unknown) {
                statusText = " (Unknown)";
            }
            if (!statusText.empty()) {
                TextSpec statusTextSpec;
                statusTextSpec.text = statusText;
                statusTextSpec.id = "checkbox_status_" + std::to_string(row.friendAccountId) + "_" + std::to_string(charInfo.characterId);
                statusTextSpec.visible = true;
                createText(statusTextSpec);
            }
        }
        
        return true;  // Handled custom rendering
    };
    
    // Wrap alt visibility table in scrollable child window
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer && renderer->BeginChild("##alt_visibility_table_child", UI::ImVec2(0.0f, 0.0f), false, WINDOW_BODY_CHILD_FLAGS)) {
        createTable(tableSpec);
        renderer->EndChild();
    } else {
        createTable(tableSpec);
    }
}

std::string MainWindow::getVisibilityStateText(AltVisibilityState state) const {
    switch (state) {
        case AltVisibilityState::Visible:
            return "Visible";
        case AltVisibilityState::NotVisible:
            return "Not Visible";
        case AltVisibilityState::PendingRequest:
            return "Pending Request";
        default:
            return "Unknown";
    }
}

void MainWindow::renderNotificationsSection() {
    // Show section header (non-collapsible - content always visible)
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_NOTIFICATIONS);
    headerSpec.id = "notifications_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    // Render notifications content
    renderNotifications();
}

void MainWindow::renderNotifications() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || optionsViewModel_ == nullptr) {
        return;
    }
    
    // Notification Duration
    // Use static variable to persist slider value across frames
    // This prevents the slider from resetting when the value changes
    static float notificationDuration = 8.0f;
    static float lastViewModelValue = -1.0f;  // Track last known viewModel value
    
    float currentViewModelValue = optionsViewModel_->getNotificationDuration();
    
    // Initialize or update from viewModel if:
    // 1. We haven't initialized yet (lastViewModelValue is -1)
    // 2. The viewModel value changed significantly (more than 0.1 seconds) and we're not currently dragging
    // This handles plugin reloads and preference updates
    if (lastViewModelValue < 0.0f || 
        (std::abs(notificationDuration - currentViewModelValue) > 0.1f && 
         std::abs(lastViewModelValue - currentViewModelValue) < 0.01f)) {
        // ViewModel value changed (likely from preferences being loaded), update our static variable
        notificationDuration = currentViewModelValue;
        lastViewModelValue = currentViewModelValue;
    } else if (std::abs(lastViewModelValue - currentViewModelValue) > 0.01f) {
        // ViewModel was updated (likely from user changing the slider), update our tracking
        lastViewModelValue = currentViewModelValue;
    }
    
    // Master toggle
    bool soundsEnabled = optionsViewModel_->getNotificationSoundsEnabled();
    ToggleSpec masterSoundSpec;
    masterSoundSpec.label = std::string(Constants::LABEL_ENABLE_NOTIFICATION_SOUNDS);
    masterSoundSpec.id = "notification_sounds_enabled_toggle";
    masterSoundSpec.value = &soundsEnabled;
    masterSoundSpec.enabled = true;
    masterSoundSpec.visible = true;
    masterSoundSpec.onChange = [this, &soundsEnabled]() {
        optionsViewModel_->setNotificationSoundsEnabled(soundsEnabled);
        emitPreferenceUpdate("notificationSoundsEnabled", soundsEnabled);
    };
    createToggle(masterSoundSpec);
    renderer->NewLine();
    
    // Individual sound toggles (only enabled if master is on)
    bool soundOnOnline = optionsViewModel_->getSoundOnFriendOnline();
    ToggleSpec onlineSoundSpec;
    onlineSoundSpec.label = std::string(Constants::LABEL_PLAY_SOUND_ON_FRIEND_ONLINE);
    onlineSoundSpec.id = "sound_on_friend_online_toggle";
    onlineSoundSpec.value = &soundOnOnline;
    onlineSoundSpec.enabled = soundsEnabled;
    onlineSoundSpec.visible = true;
    onlineSoundSpec.onChange = [this, &soundOnOnline]() {
        optionsViewModel_->setSoundOnFriendOnline(soundOnOnline);
        emitPreferenceUpdate("soundOnFriendOnline", soundOnOnline);
    };
    createToggle(onlineSoundSpec);
    renderer->NewLine();
    
    bool soundOnRequest = optionsViewModel_->getSoundOnFriendRequest();
    ToggleSpec requestSoundSpec;
    requestSoundSpec.label = std::string(Constants::LABEL_PLAY_SOUND_ON_FRIEND_REQUEST);
    requestSoundSpec.id = "sound_on_friend_request_toggle";
    requestSoundSpec.value = &soundOnRequest;
    requestSoundSpec.enabled = soundsEnabled;
    requestSoundSpec.visible = true;
    requestSoundSpec.onChange = [this, &soundOnRequest]() {
        optionsViewModel_->setSoundOnFriendRequest(soundOnRequest);
        emitPreferenceUpdate("soundOnFriendRequest", soundOnRequest);
    };
    createToggle(requestSoundSpec);
    renderer->NewLine();
    
    // Volume slider (only enabled if master is on)
    // Use 0-100 range for display, convert to 0.0-1.0 for storage
    static float soundVolumeDisplay = 60.0f;  // Display value (0-100)
    static float lastSoundVolumeValue = -1.0f;
    float currentSoundVolumeValue = optionsViewModel_->getNotificationSoundVolume();
    float currentSoundVolumeDisplay = currentSoundVolumeValue * 100.0f;  // Convert to 0-100 for display
    
    if (lastSoundVolumeValue < 0.0f || 
        (std::abs(soundVolumeDisplay - currentSoundVolumeDisplay) > 5.0f && 
         std::abs(lastSoundVolumeValue - currentSoundVolumeValue) < 0.01f)) {
        soundVolumeDisplay = currentSoundVolumeDisplay;
        lastSoundVolumeValue = currentSoundVolumeValue;
    } else if (std::abs(lastSoundVolumeValue - currentSoundVolumeValue) > 0.01f) {
        lastSoundVolumeValue = currentSoundVolumeValue;
    }
    
    SliderSpec volumeSpec;
    volumeSpec.label = std::string(Constants::LABEL_NOTIFICATION_SOUND_VOLUME);
    volumeSpec.id = "notification_sound_volume_slider";
    volumeSpec.value = &soundVolumeDisplay;
    volumeSpec.min = 0.0f;
    volumeSpec.max = 100.0f;
    volumeSpec.format = "%.0f%%";
    volumeSpec.enabled = soundsEnabled;
    volumeSpec.visible = true;
    volumeSpec.onChange = [this](float value) {
        // Convert from 0-100 display range to 0.0-1.0 storage range
        float normalizedValue = value / 100.0f;
        optionsViewModel_->setNotificationSoundVolume(normalizedValue);
        // Don't save here - save only on deactivation (debounced)
    };
    volumeSpec.onDeactivated = [this](float value) {
        float normalizedValue = value / 100.0f;
        emitPreferenceUpdate("notificationSoundVolume", normalizedValue);
    };
    createSlider(volumeSpec);
    renderer->NewLine();
    
    SliderSpec durationSpec;
    durationSpec.label = std::string(Constants::LABEL_NOTIFICATION_DURATION_SECONDS);
    durationSpec.id = "notification_duration_slider";
    durationSpec.value = &notificationDuration;
    durationSpec.min = 1.0f;
    durationSpec.max = 30.0f;
    durationSpec.format = "%.1f";
    durationSpec.enabled = true;
    durationSpec.visible = true;
    durationSpec.onChange = [this](float value) {
        optionsViewModel_->setNotificationDuration(value);
        // Don't save here - save only on deactivation (debounced)
    };
    durationSpec.onDeactivated = [this](float value) {
        emitPreferenceUpdate("notificationDuration", value);
    };
    createSlider(durationSpec);
    renderer->NewLine();
    
    // Notification Position (X, Y coordinates)
    renderer->TextUnformatted("Position (X, Y pixels):");
    renderer->NewLine();
    
    // Static buffers for position inputs (persist across frames)
    static std::string notificationPosXBuffer;
    static std::string notificationPosYBuffer;
    static float lastPosX = -1.0f;
    static float lastPosY = -1.0f;
    static bool wasXInputActive = false;
    static bool wasYInputActive = false;
    
    // Initialize buffers from ViewModel if they changed
    float currentPosX = optionsViewModel_->getNotificationPositionX();
    float currentPosY = optionsViewModel_->getNotificationPositionY();
    if (lastPosX < -0.5f || std::abs(lastPosX - currentPosX) > 0.01f) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << currentPosX;
        notificationPosXBuffer = oss.str();
        lastPosX = currentPosX;
    }
    if (lastPosY < -0.5f || std::abs(lastPosY - currentPosY) > 0.01f) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << currentPosY;
        notificationPosYBuffer = oss.str();
        lastPosY = currentPosY;
    }
    
    // X position input (limited width using child window)
    renderer->TextUnformatted("X:");
    renderer->SameLine(0.0f, 5.0f);
    if (renderer->BeginChild("##x_pos_child", UI::ImVec2(100.0f, 0.0f), false, 0)) {
        InputTextSpec xPosSpec;
        xPosSpec.id = "notification_position_x";
        xPosSpec.label = "##x_pos";
        xPosSpec.buffer = &notificationPosXBuffer;
        xPosSpec.bufferSize = 64;
        xPosSpec.enabled = true;
        xPosSpec.visible = true;
        xPosSpec.onChange = [this](const std::string& value) {
            try {
                float x = std::stof(value);
                optionsViewModel_->setNotificationPositionX(x);
                // Update ToastManager immediately for real-time feedback
                UI::Notifications::ToastManager::getInstance().setPosition(x, optionsViewModel_->getNotificationPositionY());
                emitPreferenceUpdate("notificationPositionX", x);
            } catch (...) {
                // Invalid input, ignore
            }
        };
        createInputText(xPosSpec);
        
        // Save value when input loses focus after editing
        bool isXInputActive = renderer->IsItemActive();
        if (wasXInputActive && !isXInputActive && renderer->IsItemDeactivatedAfterEdit()) {
            try {
                float x = std::stof(notificationPosXBuffer);
                optionsViewModel_->setNotificationPositionX(x);
                // Update ToastManager immediately for real-time feedback
                UI::Notifications::ToastManager::getInstance().setPosition(x, optionsViewModel_->getNotificationPositionY());
                emitPreferenceUpdate("notificationPositionX", x);
            } catch (...) {
                // Invalid input, ignore
            }
        }
        wasXInputActive = isXInputActive;
        
        renderer->EndChild();
    }
    
    renderer->SameLine(0.0f, 10.0f);
    
    // Y position input (limited width using child window)
    renderer->TextUnformatted("Y:");
    renderer->SameLine(0.0f, 5.0f);
    if (renderer->BeginChild("##y_pos_child", UI::ImVec2(100.0f, 0.0f), false, 0)) {
        InputTextSpec yPosSpec;
        yPosSpec.id = "notification_position_y";
        yPosSpec.label = "##y_pos";
        yPosSpec.buffer = &notificationPosYBuffer;
        yPosSpec.bufferSize = 64;
        yPosSpec.enabled = true;
        yPosSpec.visible = true;
        yPosSpec.onChange = [this](const std::string& value) {
            try {
                float y = std::stof(value);
                optionsViewModel_->setNotificationPositionY(y);
                // Update ToastManager immediately for real-time feedback
                UI::Notifications::ToastManager::getInstance().setPosition(optionsViewModel_->getNotificationPositionX(), y);
                emitPreferenceUpdate("notificationPositionY", y);
            } catch (...) {
                // Invalid input, ignore
            }
        };
        createInputText(yPosSpec);
        
        // Save value when input loses focus after editing
        bool isYInputActive = renderer->IsItemActive();
        if (wasYInputActive && !isYInputActive && renderer->IsItemDeactivatedAfterEdit()) {
            try {
                float y = std::stof(notificationPosYBuffer);
                optionsViewModel_->setNotificationPositionY(y);
                // Update ToastManager immediately for real-time feedback
                UI::Notifications::ToastManager::getInstance().setPosition(optionsViewModel_->getNotificationPositionX(), y);
                emitPreferenceUpdate("notificationPositionY", y);
            } catch (...) {
                // Invalid input, ignore
            }
        }
        wasYInputActive = isYInputActive;
        
        renderer->EndChild();
    }
    
    renderer->SameLine(0.0f, 10.0f);
    
    // Reset to default button
    ButtonSpec resetButtonSpec;
    resetButtonSpec.id = "notification_position_reset";
    resetButtonSpec.label = "Reset to Default";
    resetButtonSpec.enabled = true;
    resetButtonSpec.visible = true;
    resetButtonSpec.onClick = [this]() {
        // Reset to default position
        float defaultX = XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X;
        float defaultY = XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y;
        optionsViewModel_->setNotificationPositionX(defaultX);
        optionsViewModel_->setNotificationPositionY(defaultY);
        // Update ToastManager immediately
        UI::Notifications::ToastManager::getInstance().setPosition(defaultX, defaultY);
        emitPreferenceUpdate("notificationPositionX", defaultX);
        emitPreferenceUpdate("notificationPositionY", defaultY);
    };
    createButton(resetButtonSpec);
    
    renderer->SameLine(0.0f, 10.0f);
    
    // Preview toggle - shows/hides a test toast at the current position
    static bool lastPreviewState = false;
    static bool previewToastAdded = false;
    ToggleSpec previewToggleSpec;
    previewToggleSpec.id = "notification_position_preview";
    previewToggleSpec.label = "Preview";
    previewToggleSpec.value = &notificationPreviewEnabled_;
    previewToggleSpec.enabled = true;
    previewToggleSpec.visible = true;
    previewToggleSpec.onChange = [this]() {
        if (notificationPreviewEnabled_) {
            // Toggle ON: Add preview toast immediately
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            float posX = optionsViewModel_->getNotificationPositionX();
            float posY = optionsViewModel_->getNotificationPositionY();
            UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
            
            XIFriendList::App::Notifications::Toast previewToast;
            previewToast.type = XIFriendList::App::Notifications::ToastType::Info;
            previewToast.title = "Notification Preview";
            previewToast.message = "This is a preview of how notifications will appear at this position.";
            previewToast.createdAt = static_cast<int64_t>(ms);
            previewToast.duration = 0;  // No auto-dismiss (0 means never dismiss)
            previewToast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
            previewToast.alpha = 0.0f;
            previewToast.offsetX = 0.0f;
            previewToast.dismissed = false;
            UI::Notifications::ToastManager::getInstance().addToast(previewToast);
            previewToastAdded = true;
        } else {
            // Toggle OFF: Clear all toasts (including preview)
            UI::Notifications::ToastManager::getInstance().clear();
            previewToastAdded = false;
        }
    };
    createToggle(previewToggleSpec);
    
    // If preview is enabled, ensure it's always visible and update position
    static float lastPreviewPosX = -999.0f;
    static float lastPreviewPosY = -999.0f;
    
    if (notificationPreviewEnabled_) {
        float posX = optionsViewModel_->getNotificationPositionX();
        float posY = optionsViewModel_->getNotificationPositionY();
        
        // Update position if it changed
        if (std::abs(posX - lastPreviewPosX) > 0.01f || std::abs(posY - lastPreviewPosY) > 0.01f) {
            UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
            lastPreviewPosX = posX;
            lastPreviewPosY = posY;
        }
        
        // If no toasts exist (maybe it was cleared or dismissed), add a new one
        if (UI::Notifications::ToastManager::getInstance().getToastCount() == 0 && previewToastAdded) {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            XIFriendList::App::Notifications::Toast previewToast;
            previewToast.type = XIFriendList::App::Notifications::ToastType::Info;
            previewToast.title = "Notification Preview";
            previewToast.message = "This is a preview of how notifications will appear at this position.";
            previewToast.createdAt = static_cast<int64_t>(ms);
            previewToast.duration = 0;  // No auto-dismiss
            previewToast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
            previewToast.alpha = 0.0f;
            previewToast.offsetX = 0.0f;
            previewToast.dismissed = false;
            UI::Notifications::ToastManager::getInstance().addToast(previewToast);
        }
    } else {
        // Reset state when preview is disabled
        lastPreviewPosX = -999.0f;
        lastPreviewPosY = -999.0f;
        previewToastAdded = false;
    }
    
    lastPreviewState = notificationPreviewEnabled_;
    
    renderer->NewLine();
    std::ostringstream helpText;
    helpText << "(Default is " << static_cast<int>(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X) 
             << "," << static_cast<int>(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y) 
             << " for top-left corner. Use positive X,Y to move it)";
    renderer->TextUnformatted(helpText.str().c_str());
    renderer->NewLine();
}

void MainWindow::renderControlsSection() {
    // Show section header (non-collapsible - content always visible)
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_CONTROLS);
    headerSpec.id = "controls_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    // Render controls content
    renderControls();
}

void MainWindow::renderControls() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || optionsViewModel_ == nullptr) {
        return;
    }
    
    // Close Key - combo box for selection
    struct KeyOption {
        const char* name;
        int vkCode;
    };
    
    const KeyOption keyOptions[] = {
        {"ESC", VK_ESCAPE},
        {"Space", VK_SPACE},
        {"Enter", VK_RETURN},
        {"Tab", VK_TAB},
        {"Backspace", VK_BACK},
        {"Delete", VK_DELETE},
        {"Insert", VK_INSERT},
        {"Home", VK_HOME},
        {"End", VK_END},
        {"Page Up", VK_PRIOR},
        {"Page Down", VK_NEXT},
        {"Up Arrow", VK_UP},
        {"Down Arrow", VK_DOWN},
        {"Left Arrow", VK_LEFT},
        {"Right Arrow", VK_RIGHT},
        {"F1", VK_F1},
        {"F2", VK_F2},
        {"F3", VK_F3},
        {"F4", VK_F4},
        {"F5", VK_F5},
        {"F6", VK_F6},
        {"F7", VK_F7},
        {"F8", VK_F8},
        {"F9", VK_F9},
        {"F10", VK_F10},
        {"F11", VK_F11},
        {"F12", VK_F12},
        {"A", 'A'},
        {"B", 'B'},
        {"C", 'C'},
        {"D", 'D'},
        {"E", 'E'},
        {"F", 'F'},
        {"G", 'G'},
        {"H", 'H'},
        {"I", 'I'},
        {"J", 'J'},
        {"K", 'K'},
        {"L", 'L'},
        {"M", 'M'},
        {"N", 'N'},
        {"O", 'O'},
        {"P", 'P'},
        {"Q", 'Q'},
        {"R", 'R'},
        {"S", 'S'},
        {"T", 'T'},
        {"U", 'U'},
        {"V", 'V'},
        {"W", 'W'},
        {"X", 'X'},
        {"Y", 'Y'},
        {"Z", 'Z'},
        {"0", '0'},
        {"1", '1'},
        {"2", '2'},
        {"3", '3'},
        {"4", '4'},
        {"5", '5'},
        {"6", '6'},
        {"7", '7'},
        {"8", '8'},
        {"9", '9'},
    };
    
    int currentKeyCode = optionsViewModel_->getCustomCloseKeyCode();
    if (currentKeyCode == 0) {
        currentKeyCode = VK_ESCAPE;  // Default to ESC
    }
    
    // Find current key code in the list
    int foundIndex = 0;
    for (size_t i = 0; i < sizeof(keyOptions) / sizeof(keyOptions[0]); ++i) {
        if (keyOptions[i].vkCode == currentKeyCode) {
            foundIndex = static_cast<int>(i);
            break;
        }
    }
    currentCloseKeyIndex_ = foundIndex;
    
    // Build items list for combo box
    std::vector<std::string> keyNames;
    for (const auto& option : keyOptions) {
        keyNames.push_back(option.name);
    }
    
    // Create combo box
    ComboSpec keyComboSpec;
    keyComboSpec.label = std::string(Constants::LABEL_CLOSE_KEY);
    keyComboSpec.id = "close_key_combo";
    keyComboSpec.currentItem = &currentCloseKeyIndex_;
    keyComboSpec.items = keyNames;
    keyComboSpec.enabled = true;
    keyComboSpec.visible = true;
    keyComboSpec.onChange = [this, keyOptions](int newIndex) {
        if (newIndex >= 0 && newIndex < static_cast<int>(sizeof(keyOptions) / sizeof(keyOptions[0]))) {
            int newKeyCode = keyOptions[newIndex].vkCode;
            optionsViewModel_->setCustomCloseKeyCode(newKeyCode);
            emitPreferenceUpdate("customCloseKeyCode", static_cast<float>(newKeyCode));
        }
    };
    createCombo(keyComboSpec);
    renderer->NewLine();
    
    // Controller close button
    // Common controller button options
    struct ControllerButtonOption {
        std::string label;
        int buttonCode;
    };
    const ControllerButtonOption buttonOptions[] = {
        {std::string(Constants::CONTROLLER_BUTTON_B_DEFAULT), 0x2000},
        {std::string(Constants::CONTROLLER_BUTTON_A), 0x1000},
        {std::string(Constants::CONTROLLER_BUTTON_X), 0x4000},
        {std::string(Constants::CONTROLLER_BUTTON_Y), 0x8000},
        {std::string(Constants::CONTROLLER_BUTTON_BACK), 0x0020},
        {std::string(Constants::CONTROLLER_BUTTON_DISABLED), 0}
    };
    
    for (const auto& option : buttonOptions) {
        ButtonSpec buttonSpec;
        buttonSpec.label = option.label;
        buttonSpec.id = "controller_button_" + std::to_string(option.buttonCode);
        buttonSpec.enabled = true;
        buttonSpec.visible = true;
        
        UI::ImVec2 textSize = renderer->CalcTextSize(option.label.c_str());
        buttonSpec.width = textSize.x + 16.0f;
        buttonSpec.height = textSize.y + 8.0f;
        
        buttonSpec.onClick = [this, option]() {
            optionsViewModel_->setControllerCloseButton(option.buttonCode);
            emitPreferenceUpdate("controllerCloseButton", static_cast<float>(option.buttonCode));
        };
        createButton(buttonSpec);
        renderer->SameLine(0.0f, 4.0f);
    }
    renderer->NewLine();
    
    // Controller button label (below buttons)
    std::string controllerButtonName = optionsViewModel_->getControllerCloseButtonName();
    TextSpec controllerLabelSpec;
    controllerLabelSpec.text = std::string(Constants::LABEL_CONTROLLER_BUTTON) + " " + controllerButtonName;
    controllerLabelSpec.id = "controller_button_label";
    controllerLabelSpec.visible = true;
    createText(controllerLabelSpec);
}

void MainWindow::renderDebugSettingsSection() {
    // Show section header (non-collapsible - content always visible)
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_DEBUG_ADVANCED);
    headerSpec.id = "debug_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    // Render debug settings content
    renderDebugSettings();
}

void MainWindow::renderDebugSettings() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
#ifdef _DEBUG
    renderer->TextUnformatted("Memory Usage:");
    renderer->Spacing(4.0f);
    
    auto toastStats = UI::Notifications::ToastManager::getInstance().getMemoryStats();
    size_t toastKb = (toastStats.estimatedBytes + 512) / 1024;
    std::string toastText = "  Notifications: " + std::to_string(toastStats.entryCount) + 
                           " active (~" + std::to_string(toastKb) + " KB)";
    renderer->TextUnformatted(toastText.c_str());
    
    if (iconManager_) {
        auto* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
        auto iconStats = iconMgr->getMemoryStats();
        size_t iconKb = (iconStats.estimatedBytes + 512) / 1024;
        std::string iconText = "  Icons/Textures: ~" + std::to_string(iconKb) + " KB";
        renderer->TextUnformatted(iconText.c_str());
    }
    
    renderer->Spacing(8.0f);
    renderer->TextUnformatted("Use /fl stats for full memory report");
#else
    renderer->TextUnformatted("Debug information available in debug builds only");
    renderer->Spacing(4.0f);
    renderer->TextUnformatted("Use /fl stats for memory usage");
#endif
}

void MainWindow::renderThemeSettingsSection() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Show section header (non-collapsible - content always visible)
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_THEME_SETTINGS);
    headerSpec.id = "theme_settings_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    renderThemeSettings();
}

void MainWindow::renderThemeSettings() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Always show something, even if ViewModel is not set
    if (!themesViewModel_) {
        TextSpec emptySpec;
        emptySpec.text = "Theme settings not available (ViewModel not initialized)";
        emptySpec.id = "theme_settings_unavailable";
        emptySpec.visible = true;
        createText(emptySpec);
        renderer->NewLine();
        return;
    }
    
    // Combined Theme Selector: built-in themes + custom themes
    std::vector<std::string> allThemeNames;
    std::vector<int> themeTypes;  // Parallel array: 0-3 = built-in theme index, -1 = custom theme
    std::vector<std::string> presetNames;  // Parallel array: actual preset name for display items (empty for non-presets)
    
    // Add built-in themes first (Warm Brown is default)
    allThemeNames.push_back("Warm Brown");
    themeTypes.push_back(0);  // BuiltInTheme::FFXIClassic
    presetNames.push_back("");  // Not a preset
    allThemeNames.push_back("Modern Dark");
    themeTypes.push_back(1);  // BuiltInTheme::ModernDark
    presetNames.push_back("");  // Not a preset
    allThemeNames.push_back("Green Nature");
    themeTypes.push_back(2);  // BuiltInTheme::GreenNature
    presetNames.push_back("");  // Not a preset
    allThemeNames.push_back("Purple Mystic");
    themeTypes.push_back(3);  // BuiltInTheme::PurpleMystic
    presetNames.push_back("");  // Not a preset
    
    // Add all custom themes
    const auto& customThemes = themesViewModel_->getCustomThemes();
    for (const auto& customTheme : customThemes) {
        allThemeNames.push_back(customTheme.name);
        themeTypes.push_back(-1);  // Custom theme marker
        presetNames.push_back("");  // Not a preset
    }
    
    std::string currentPresetName = themesViewModel_->getCurrentPresetName();
    std::string currentCustomThemeName = themesViewModel_->getCurrentCustomThemeName();
    std::string currentThemeName = themesViewModel_->getCurrentThemeName();
    int currentThemeIndex = themesViewModel_->getCurrentThemeIndex();
    
    currentPresetIndex_ = 0;  
    bool foundMatch = false;
    
    for (size_t i = 0; i < allThemeNames.size(); ++i) {
        if (themeTypes[i] == -1) {
            if (currentThemeIndex == -1 && !currentCustomThemeName.empty() && 
                allThemeNames[i] == currentCustomThemeName) {
                currentPresetIndex_ = static_cast<int>(i);
                foundMatch = true;
                break;
            }
        } else {
            bool nameMatches = (allThemeNames[i] == currentThemeName);
            bool indexMatches = (currentPresetName.empty() && themeTypes[i] == currentThemeIndex);
            
            if (nameMatches || indexMatches) {
                currentPresetIndex_ = static_cast<int>(i);
                foundMatch = true;
                break;
            }
        }
    }
    
    if (!allThemeNames.empty() && currentPresetIndex_ >= 0 && 
        currentPresetIndex_ < static_cast<int>(allThemeNames.size())) {
        ComboSpec themeComboSpec;
        themeComboSpec.label = std::string(Constants::LABEL_THEME);
        themeComboSpec.id = "theme_selection_combo_options";
        themeComboSpec.currentItem = &currentPresetIndex_;
        themeComboSpec.items = allThemeNames;
        themeComboSpec.enabled = true;
        themeComboSpec.visible = true;
        themeComboSpec.onChange = [this, themeTypes, allThemeNames](int newIndex) {
            if (newIndex >= 0 && newIndex < static_cast<int>(allThemeNames.size())) {
                currentPresetIndex_ = newIndex;
                
                std::string selectedName = allThemeNames[newIndex];
                int themeType = themeTypes[newIndex];
                
                if (themeType == -1) {
                    emitCommand(WindowCommandType::SetCustomTheme, selectedName);
                } else {
                    emitCommand(WindowCommandType::ApplyTheme, std::to_string(themeType));
                }
            }
        };
        
        createCombo(themeComboSpec);
        
        renderer->SameLine(0.0f, 4.0f);
        HelpMarker("Select a theme (built-in themes or custom themes)");
        renderer->NewLine();
        
        ButtonSpec resetButtonSpec;
        resetButtonSpec.label = "Reset to Default";
        resetButtonSpec.id = "reset_theme_button";
        resetButtonSpec.enabled = true;
        resetButtonSpec.visible = true;
        resetButtonSpec.onClick = [this]() {
            emitCommand(WindowCommandType::ApplyTheme, "0");
        };
        createButton(resetButtonSpec);
        renderer->NewLine();
        renderer->NewLine();
        
        #ifndef BUILDING_TESTS
        auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
        if (guiManager) {
            ::ImVec2 spacing(0.0f, 8.0f); 
            guiManager->Dummy(spacing);
        }
        #endif
    }
    
    if (themesViewModel_) {
        syncColorsToBuffers();
        
        renderCustomColors();
        renderer->NewLine();
        
        renderThemeManagement();
        
        renderer->NewLine();
        renderer->NewLine();
        
        renderQuickOnlineThemeSection();
        renderer->NewLine();
        renderNotificationThemeSection();
    }
}

void MainWindow::syncColorsToBuffers() {
    if (!themesViewModel_) {
        return;
    }
    
    const auto& colors = themesViewModel_->getCurrentThemeColors();
    
    syncColorToBuffer(colors.windowBgColor, windowBgColor_);
    syncColorToBuffer(colors.childBgColor, childBgColor_);
    syncColorToBuffer(colors.frameBgColor, frameBgColor_);
    syncColorToBuffer(colors.frameBgHovered, frameBgHovered_);
    syncColorToBuffer(colors.frameBgActive, frameBgActive_);
    syncColorToBuffer(colors.titleBg, titleBg_);
    syncColorToBuffer(colors.titleBgActive, titleBgActive_);
    syncColorToBuffer(colors.titleBgCollapsed, titleBgCollapsed_);
    syncColorToBuffer(colors.buttonColor, buttonColor_);
    syncColorToBuffer(colors.buttonHoverColor, buttonHoverColor_);
    syncColorToBuffer(colors.buttonActiveColor, buttonActiveColor_);
    syncColorToBuffer(colors.separatorColor, separatorColor_);
    syncColorToBuffer(colors.separatorHovered, separatorHovered_);
    syncColorToBuffer(colors.separatorActive, separatorActive_);
    syncColorToBuffer(colors.scrollbarBg, scrollbarBg_);
    syncColorToBuffer(colors.scrollbarGrab, scrollbarGrab_);
    syncColorToBuffer(colors.scrollbarGrabHovered, scrollbarGrabHovered_);
    syncColorToBuffer(colors.scrollbarGrabActive, scrollbarGrabActive_);
    syncColorToBuffer(colors.checkMark, checkMark_);
    syncColorToBuffer(colors.sliderGrab, sliderGrab_);
    syncColorToBuffer(colors.sliderGrabActive, sliderGrabActive_);
    syncColorToBuffer(colors.header, header_);
    syncColorToBuffer(colors.headerHovered, headerHovered_);
    syncColorToBuffer(colors.headerActive, headerActive_);
    syncColorToBuffer(colors.textColor, textColor_);
    syncColorToBuffer(colors.textDisabled, textDisabled_);
}

void MainWindow::syncBuffersToColors() {
    if (!themesViewModel_) {
        return;
    }
    
    auto& colors = themesViewModel_->getCurrentThemeColors();
    
    syncBufferToColor(windowBgColor_, colors.windowBgColor);
    syncBufferToColor(childBgColor_, colors.childBgColor);
    syncBufferToColor(frameBgColor_, colors.frameBgColor);
    syncBufferToColor(frameBgHovered_, colors.frameBgHovered);
    syncBufferToColor(frameBgActive_, colors.frameBgActive);
    syncBufferToColor(titleBg_, colors.titleBg);
    syncBufferToColor(titleBgActive_, colors.titleBgActive);
    syncBufferToColor(titleBgCollapsed_, colors.titleBgCollapsed);
    syncBufferToColor(buttonColor_, colors.buttonColor);
    syncBufferToColor(buttonHoverColor_, colors.buttonHoverColor);
    syncBufferToColor(buttonActiveColor_, colors.buttonActiveColor);
    syncBufferToColor(separatorColor_, colors.separatorColor);
    syncBufferToColor(separatorHovered_, colors.separatorHovered);
    syncBufferToColor(separatorActive_, colors.separatorActive);
    syncBufferToColor(scrollbarBg_, colors.scrollbarBg);
    syncBufferToColor(scrollbarGrab_, colors.scrollbarGrab);
    syncBufferToColor(scrollbarGrabHovered_, colors.scrollbarGrabHovered);
    syncBufferToColor(scrollbarGrabActive_, colors.scrollbarGrabActive);
    syncBufferToColor(checkMark_, colors.checkMark);
    syncBufferToColor(sliderGrab_, colors.sliderGrab);
    syncBufferToColor(sliderGrabActive_, colors.sliderGrabActive);
    syncBufferToColor(header_, colors.header);
    syncBufferToColor(headerHovered_, colors.headerHovered);
    syncBufferToColor(headerActive_, colors.headerActive);
    syncBufferToColor(textColor_, colors.textColor);
    syncBufferToColor(textDisabled_, colors.textDisabled);
}

void MainWindow::initializeColorBuffer(float buffer[4], float r, float g, float b, float a) {
    buffer[0] = r;
    buffer[1] = g;
    buffer[2] = b;
    buffer[3] = a;
}

void MainWindow::syncColorToBuffer(const Core::Color& color, float buffer[4]) {
    buffer[0] = color.r;
    buffer[1] = color.g;
    buffer[2] = color.b;
    buffer[3] = color.a;
}

void MainWindow::syncBufferToColor(const float buffer[4], Core::Color& color) {
    color = Core::Color(buffer[0], buffer[1], buffer[2], buffer[3]);
}

void MainWindow::syncQuickOnlineColorsToBuffers() {
    if (!commandHandler_) {
        return;
    }
    
    const auto& colors = commandHandler_->getQuickOnlineTheme();
    
    syncColorToBuffer(colors.windowBgColor, quickOnlineWindowBgColor_);
    syncColorToBuffer(colors.childBgColor, quickOnlineChildBgColor_);
    syncColorToBuffer(colors.frameBgColor, quickOnlineFrameBgColor_);
    syncColorToBuffer(colors.frameBgHovered, quickOnlineFrameBgHovered_);
    syncColorToBuffer(colors.frameBgActive, quickOnlineFrameBgActive_);
    syncColorToBuffer(colors.titleBg, quickOnlineTitleBg_);
    syncColorToBuffer(colors.titleBgActive, quickOnlineTitleBgActive_);
    syncColorToBuffer(colors.titleBgCollapsed, quickOnlineTitleBgCollapsed_);
    syncColorToBuffer(colors.buttonColor, quickOnlineButtonColor_);
    syncColorToBuffer(colors.buttonHoverColor, quickOnlineButtonHoverColor_);
    syncColorToBuffer(colors.buttonActiveColor, quickOnlineButtonActiveColor_);
    syncColorToBuffer(colors.separatorColor, quickOnlineSeparatorColor_);
    syncColorToBuffer(colors.separatorHovered, quickOnlineSeparatorHovered_);
    syncColorToBuffer(colors.separatorActive, quickOnlineSeparatorActive_);
    syncColorToBuffer(colors.scrollbarBg, quickOnlineScrollbarBg_);
    syncColorToBuffer(colors.scrollbarGrab, quickOnlineScrollbarGrab_);
    syncColorToBuffer(colors.scrollbarGrabHovered, quickOnlineScrollbarGrabHovered_);
    syncColorToBuffer(colors.scrollbarGrabActive, quickOnlineScrollbarGrabActive_);
    syncColorToBuffer(colors.checkMark, quickOnlineCheckMark_);
    syncColorToBuffer(colors.sliderGrab, quickOnlineSliderGrab_);
    syncColorToBuffer(colors.sliderGrabActive, quickOnlineSliderGrabActive_);
    syncColorToBuffer(colors.header, quickOnlineHeader_);
    syncColorToBuffer(colors.headerHovered, quickOnlineHeaderHovered_);
    syncColorToBuffer(colors.headerActive, quickOnlineHeaderActive_);
    syncColorToBuffer(colors.textColor, quickOnlineTextColor_);
    syncColorToBuffer(colors.textDisabled, quickOnlineTextDisabled_);
    syncColorToBuffer(colors.tableBgColor, quickOnlineTableBgColor_);
}

void MainWindow::syncQuickOnlineBuffersToColors() {
    if (!commandHandler_) {
        return;
    }
    
    Core::CustomTheme colors;
    syncBufferToColor(quickOnlineWindowBgColor_, colors.windowBgColor);
    syncBufferToColor(quickOnlineChildBgColor_, colors.childBgColor);
    syncBufferToColor(quickOnlineFrameBgColor_, colors.frameBgColor);
    syncBufferToColor(quickOnlineFrameBgHovered_, colors.frameBgHovered);
    syncBufferToColor(quickOnlineFrameBgActive_, colors.frameBgActive);
    syncBufferToColor(quickOnlineTitleBg_, colors.titleBg);
    syncBufferToColor(quickOnlineTitleBgActive_, colors.titleBgActive);
    syncBufferToColor(quickOnlineTitleBgCollapsed_, colors.titleBgCollapsed);
    syncBufferToColor(quickOnlineButtonColor_, colors.buttonColor);
    syncBufferToColor(quickOnlineButtonHoverColor_, colors.buttonHoverColor);
    syncBufferToColor(quickOnlineButtonActiveColor_, colors.buttonActiveColor);
    syncBufferToColor(quickOnlineSeparatorColor_, colors.separatorColor);
    syncBufferToColor(quickOnlineSeparatorHovered_, colors.separatorHovered);
    syncBufferToColor(quickOnlineSeparatorActive_, colors.separatorActive);
    syncBufferToColor(quickOnlineScrollbarBg_, colors.scrollbarBg);
    syncBufferToColor(quickOnlineScrollbarGrab_, colors.scrollbarGrab);
    syncBufferToColor(quickOnlineScrollbarGrabHovered_, colors.scrollbarGrabHovered);
    syncBufferToColor(quickOnlineScrollbarGrabActive_, colors.scrollbarGrabActive);
    syncBufferToColor(quickOnlineCheckMark_, colors.checkMark);
    syncBufferToColor(quickOnlineSliderGrab_, colors.sliderGrab);
    syncBufferToColor(quickOnlineSliderGrabActive_, colors.sliderGrabActive);
    syncBufferToColor(quickOnlineHeader_, colors.header);
    syncBufferToColor(quickOnlineHeaderHovered_, colors.headerHovered);
    syncBufferToColor(quickOnlineHeaderActive_, colors.headerActive);
    syncBufferToColor(quickOnlineTextColor_, colors.textColor);
    syncBufferToColor(quickOnlineTextDisabled_, colors.textDisabled);
    syncBufferToColor(quickOnlineTableBgColor_, colors.tableBgColor);
    
    commandHandler_->updateQuickOnlineThemeColors(colors);
    emitCommand(WindowCommandType::UpdateQuickOnlineThemeColors, "");
}

void MainWindow::syncNotificationColorsToBuffers() {
    if (!commandHandler_) {
        return;
    }
    
    const auto& colors = commandHandler_->getNotificationTheme();
    
    syncColorToBuffer(colors.windowBgColor, notificationWindowBgColor_);
    syncColorToBuffer(colors.childBgColor, notificationChildBgColor_);
    syncColorToBuffer(colors.frameBgColor, notificationFrameBgColor_);
    syncColorToBuffer(colors.frameBgHovered, notificationFrameBgHovered_);
    syncColorToBuffer(colors.frameBgActive, notificationFrameBgActive_);
    syncColorToBuffer(colors.titleBg, notificationTitleBg_);
    syncColorToBuffer(colors.titleBgActive, notificationTitleBgActive_);
    syncColorToBuffer(colors.titleBgCollapsed, notificationTitleBgCollapsed_);
    syncColorToBuffer(colors.buttonColor, notificationButtonColor_);
    syncColorToBuffer(colors.buttonHoverColor, notificationButtonHoverColor_);
    syncColorToBuffer(colors.buttonActiveColor, notificationButtonActiveColor_);
    syncColorToBuffer(colors.separatorColor, notificationSeparatorColor_);
    syncColorToBuffer(colors.separatorHovered, notificationSeparatorHovered_);
    syncColorToBuffer(colors.separatorActive, notificationSeparatorActive_);
    syncColorToBuffer(colors.scrollbarBg, notificationScrollbarBg_);
    syncColorToBuffer(colors.scrollbarGrab, notificationScrollbarGrab_);
    syncColorToBuffer(colors.scrollbarGrabHovered, notificationScrollbarGrabHovered_);
    syncColorToBuffer(colors.scrollbarGrabActive, notificationScrollbarGrabActive_);
    syncColorToBuffer(colors.checkMark, notificationCheckMark_);
    syncColorToBuffer(colors.sliderGrab, notificationSliderGrab_);
    syncColorToBuffer(colors.sliderGrabActive, notificationSliderGrabActive_);
    syncColorToBuffer(colors.header, notificationHeader_);
    syncColorToBuffer(colors.headerHovered, notificationHeaderHovered_);
    syncColorToBuffer(colors.headerActive, notificationHeaderActive_);
    syncColorToBuffer(colors.textColor, notificationTextColor_);
    syncColorToBuffer(colors.textDisabled, notificationTextDisabled_);
    syncColorToBuffer(colors.tableBgColor, notificationTableBgColor_);
}

void MainWindow::syncNotificationBuffersToColors() {
    if (!commandHandler_) {
        return;
    }
    
    Core::CustomTheme colors;
    syncBufferToColor(notificationWindowBgColor_, colors.windowBgColor);
    syncBufferToColor(notificationChildBgColor_, colors.childBgColor);
    syncBufferToColor(notificationFrameBgColor_, colors.frameBgColor);
    syncBufferToColor(notificationFrameBgHovered_, colors.frameBgHovered);
    syncBufferToColor(notificationFrameBgActive_, colors.frameBgActive);
    syncBufferToColor(notificationTitleBg_, colors.titleBg);
    syncBufferToColor(notificationTitleBgActive_, colors.titleBgActive);
    syncBufferToColor(notificationTitleBgCollapsed_, colors.titleBgCollapsed);
    syncBufferToColor(notificationButtonColor_, colors.buttonColor);
    syncBufferToColor(notificationButtonHoverColor_, colors.buttonHoverColor);
    syncBufferToColor(notificationButtonActiveColor_, colors.buttonActiveColor);
    syncBufferToColor(notificationSeparatorColor_, colors.separatorColor);
    syncBufferToColor(notificationSeparatorHovered_, colors.separatorHovered);
    syncBufferToColor(notificationSeparatorActive_, colors.separatorActive);
    syncBufferToColor(notificationScrollbarBg_, colors.scrollbarBg);
    syncBufferToColor(notificationScrollbarGrab_, colors.scrollbarGrab);
    syncBufferToColor(notificationScrollbarGrabHovered_, colors.scrollbarGrabHovered);
    syncBufferToColor(notificationScrollbarGrabActive_, colors.scrollbarGrabActive);
    syncBufferToColor(notificationCheckMark_, colors.checkMark);
    syncBufferToColor(notificationSliderGrab_, colors.sliderGrab);
    syncBufferToColor(notificationSliderGrabActive_, colors.sliderGrabActive);
    syncBufferToColor(notificationHeader_, colors.header);
    syncBufferToColor(notificationHeaderHovered_, colors.headerHovered);
    syncBufferToColor(notificationHeaderActive_, colors.headerActive);
    syncBufferToColor(notificationTextColor_, colors.textColor);
    syncBufferToColor(notificationTextDisabled_, colors.textDisabled);
    syncBufferToColor(notificationTableBgColor_, colors.tableBgColor);
    
    commandHandler_->updateNotificationThemeColors(colors);
    emitCommand(WindowCommandType::UpdateNotificationThemeColors, "");
}

// Renders the custom color editor UI with collapsible sections for each color category.
// Provides color pickers for all theme colors, theme name input, and save button.
// Updates theme colors in real-time as user adjusts pickers.
// Flow:
//   1. Validate ViewModel exists
//   2. Render section header and theme name input with save button
//   3. Render collapsible color sections (Window, Frame, Title, Button, Separator, Scrollbar, Check/Slider, Header, Text, Table)
//   4. Each section contains color pickers that trigger immediate theme updates
//   5. Color changes sync to ViewModel and emit UpdateThemeColors command
// Invariants:
//   - themesViewModel_ must be valid (checked at start)
//   - Color buffers synced from ViewModel before rendering
//   - Changes immediately applied via syncBuffersToColors() and command emission
//   - All color pickers use float[4] buffers for ImGui compatibility
// Edge cases:
//   - Save button enabled only when theme name is valid and not empty
//   - Color sections remember collapsed/expanded state (persisted)
//   - Real-time updates prevent need for explicit "apply" button
void MainWindow::renderCustomColors() {
    if (!themesViewModel_) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Show custom themes editor for all themes
    SectionHeaderSpec headerSpec;
    headerSpec.label = std::string(Constants::HEADER_CUSTOM_COLORS);
    headerSpec.id = "custom_colors_header";
    headerSpec.visible = true;
    createSectionHeader(headerSpec);
    
    TextSpec themeNameLabelSpec;
    themeNameLabelSpec.text = std::string(Constants::LABEL_THEME_NAME);
    themeNameLabelSpec.id = "theme_name_label";
    themeNameLabelSpec.visible = true;
    createText(themeNameLabelSpec);
    
    // Use ViewModel's newThemeName buffer
    std::string& newThemeNameBuffer = themesViewModel_->getNewThemeName();
    
    InputTextSpec themeNameInputSpec;
    themeNameInputSpec.label = "##saveThemeName";
    themeNameInputSpec.id = "save_theme_name_input";
    themeNameInputSpec.buffer = &newThemeNameBuffer;
    themeNameInputSpec.bufferSize = 256;
    themeNameInputSpec.enabled = true;
    themeNameInputSpec.visible = true;
    themeNameInputSpec.readOnly = false;
    themeNameInputSpec.onChange = [this](const std::string& value) {
        themesViewModel_->setNewThemeName(value);
    };
    themeNameInputSpec.onEnter = [this](const std::string& value) {
        themesViewModel_->setNewThemeName(value);
    };
    createInputText(themeNameInputSpec);
    
    ButtonSpec saveButtonSpec;
    saveButtonSpec.label = std::string(Constants::BUTTON_SAVE_CUSTOM_THEME);
    saveButtonSpec.id = "save_theme_button";
    saveButtonSpec.enabled = themesViewModel_->canSaveTheme();
    saveButtonSpec.visible = true;
    saveButtonSpec.onClick = [this]() {
        syncBuffersToColors();
        std::string themeName = themesViewModel_->getNewThemeName();
        emitCommand(WindowCommandType::SaveCustomTheme, themeName);
        themesViewModel_->setNewThemeName("");
    };
    renderer->SameLine();
    createButton(saveButtonSpec);
    renderer->NewLine();
    renderer->NewLine();
    
    auto applyColorChange = [this]() {
        syncBuffersToColors();
        // Emit UpdateThemeColors command for immediate per-color updates
        emitCommand(WindowCommandType::UpdateThemeColors, "");
    };
    
    auto renderColorSection = [this, &applyColorChange](const std::string& sectionName, const std::string& sectionId, bool* collapsed, std::function<void()> renderColors) {
        bool isOpen = createCollapsibleSection(sectionName, sectionId, renderColors);
        *collapsed = !isOpen;
    };
    
    renderColorSection("Window", "color_section_window", &colorSectionWindowCollapsed_, [this, &applyColorChange]() {
        ColorPickerSpec windowBgSpec("Window Background", "window_bg", windowBgColor_);
        windowBgSpec.showAlpha = true;
        windowBgSpec.onChange = applyColorChange;
        createColorPicker(windowBgSpec);
        
        ColorPickerSpec childBgSpec("Child Background", "child_bg", childBgColor_);
        childBgSpec.showAlpha = true;
        childBgSpec.onChange = applyColorChange;
        createColorPicker(childBgSpec);
    });
    
    // Frame colors section
    renderColorSection("Frame", "color_section_frame", &colorSectionFrameCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Title", "color_section_title", &colorSectionTitleCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Button", "color_section_button", &colorSectionButtonCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Separator", "color_section_separator", &colorSectionSeparatorCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Scrollbar", "color_section_scrollbar", &colorSectionScrollbarCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Check & Slider", "color_section_check_slider", &colorSectionCheckSliderCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Header", "color_section_header", &colorSectionHeaderCollapsed_, [this, &applyColorChange]() {
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
    });
    
    renderColorSection("Text", "color_section_text", &colorSectionTextCollapsed_, [this, &applyColorChange]() {
        ColorPickerSpec textSpec("Text", "text", textColor_);
        textSpec.showAlpha = true;
        textSpec.onChange = applyColorChange;
        createColorPicker(textSpec);
        
        ColorPickerSpec textDisabledSpec("Text Disabled", "text_disabled", textDisabled_);
        textDisabledSpec.showAlpha = true;
        textDisabledSpec.onChange = applyColorChange;
        createColorPicker(textDisabledSpec);
        
    });
}

void MainWindow::renderThemeManagement() {
    if (!themesViewModel_) {
        return;
    }
    
    // Delete current custom theme (save button is now in renderCustomColors)
    if (themesViewModel_->canDeleteTheme()) {
        ButtonSpec deleteButtonSpec;
        deleteButtonSpec.label = std::string(Constants::BUTTON_DELETE_CUSTOM_THEME);
        deleteButtonSpec.id = "delete_theme_button";
        deleteButtonSpec.enabled = true;
        deleteButtonSpec.visible = true;
        deleteButtonSpec.onClick = [this]() {
            emitCommand(WindowCommandType::DeleteCustomTheme, themesViewModel_->getCurrentThemeName());
        };
        createButton(deleteButtonSpec);
    }
}

// Renders the Quick Online window theme customization section. Provides color pickers
// for all theme colors specific to the Quick Online window. Updates theme in real-time.
// Flow:
//   1. Validate command handler exists
//   2. Sync colors from ThemeUseCase to UI buffers (if not already synced)
//   3. Render collapsible section with color pickers for all theme colors
//   4. On color change: sync buffers to colors, update ThemeUseCase, emit command
// Invariants:
//   - commandHandler_ must be valid (checked at start)
//   - Colors synced from ThemeUseCase before first render
//   - Changes immediately applied via syncQuickOnlineBuffersToColors() and command emission
// Edge cases:
//   - Section collapsed state persisted across sessions
//   - Real-time updates apply immediately (no save button needed)
//   - Separate theme from main window theme (independent customization)
void MainWindow::renderQuickOnlineThemeSection() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || !commandHandler_) {
        return;
    }
    
    static bool quickOnlineThemeBuffersSynced = false;
    if (!quickOnlineThemeBuffersSynced) {
        syncQuickOnlineColorsToBuffers();
        quickOnlineThemeBuffersSynced = true;
    }
    
    auto applyQuickOnlineColorChange = [this]() {
        syncQuickOnlineBuffersToColors();
    };
    
    auto renderQuickOnlineColorSection = [this, &applyQuickOnlineColorChange](const std::string& sectionName, const std::string& sectionId, bool* collapsed, std::function<void()> renderColors) {
        bool isOpen = createCollapsibleSection(sectionName, sectionId, renderColors);
        *collapsed = !isOpen;
    };
    
    if (UI::createCollapsibleSection("Quick Online Window Theme", "quick_online_theme_section", [this, &applyQuickOnlineColorChange, &renderQuickOnlineColorSection]() {
        IUiRenderer* renderer = GetUiRenderer();
        if (renderer == nullptr) {
            return;
        }
        
        renderQuickOnlineColorSection("Window", "quick_online_color_section_window", &colorSectionWindowCollapsed_, [this, &applyQuickOnlineColorChange]() {
            ColorPickerSpec windowBgSpec("Window Background", "quick_online_window_bg", quickOnlineWindowBgColor_);
            windowBgSpec.showAlpha = true;
            windowBgSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(windowBgSpec);
            
            ColorPickerSpec childBgSpec("Child Background", "quick_online_child_bg", quickOnlineChildBgColor_);
            childBgSpec.showAlpha = true;
            childBgSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(childBgSpec);
        });
        
        renderQuickOnlineColorSection("Frame", "quick_online_color_section_frame", &colorSectionFrameCollapsed_, [this, &applyQuickOnlineColorChange]() {
            ColorPickerSpec frameBgSpec("Frame Background", "quick_online_frame_bg", quickOnlineFrameBgColor_);
            frameBgSpec.showAlpha = true;
            frameBgSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(frameBgSpec);
            
            ColorPickerSpec frameBgHoveredSpec("Frame Hovered", "quick_online_frame_bg_hovered", quickOnlineFrameBgHovered_);
            frameBgHoveredSpec.showAlpha = true;
            frameBgHoveredSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(frameBgHoveredSpec);
            
            ColorPickerSpec frameBgActiveSpec("Frame Active", "quick_online_frame_bg_active", quickOnlineFrameBgActive_);
            frameBgActiveSpec.showAlpha = true;
            frameBgActiveSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(frameBgActiveSpec);
        });
        
        renderQuickOnlineColorSection("Title", "quick_online_color_section_title", &colorSectionTitleCollapsed_, [this, &applyQuickOnlineColorChange]() {
            ColorPickerSpec titleBgSpec("Title Background", "quick_online_title_bg", quickOnlineTitleBg_);
            titleBgSpec.showAlpha = true;
            titleBgSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(titleBgSpec);
            
            ColorPickerSpec titleBgActiveSpec("Title Active", "quick_online_title_bg_active", quickOnlineTitleBgActive_);
            titleBgActiveSpec.showAlpha = true;
            titleBgActiveSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(titleBgActiveSpec);
            
            ColorPickerSpec titleBgCollapsedSpec("Title Collapsed", "quick_online_title_bg_collapsed", quickOnlineTitleBgCollapsed_);
            titleBgCollapsedSpec.showAlpha = true;
            titleBgCollapsedSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(titleBgCollapsedSpec);
        });
        
        renderQuickOnlineColorSection("Button", "quick_online_color_section_button", &colorSectionButtonCollapsed_, [this, &applyQuickOnlineColorChange]() {
            ColorPickerSpec buttonSpec("Button", "quick_online_button", quickOnlineButtonColor_);
            buttonSpec.showAlpha = true;
            buttonSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(buttonSpec);
            
            ColorPickerSpec buttonHoverSpec("Button Hovered", "quick_online_button_hovered", quickOnlineButtonHoverColor_);
            buttonHoverSpec.showAlpha = true;
            buttonHoverSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(buttonHoverSpec);
            
            ColorPickerSpec buttonActiveSpec("Button Active", "quick_online_button_active", quickOnlineButtonActiveColor_);
            buttonActiveSpec.showAlpha = true;
            buttonActiveSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(buttonActiveSpec);
        });
        
        renderQuickOnlineColorSection("Text", "quick_online_color_section_text", &colorSectionTextCollapsed_, [this, &applyQuickOnlineColorChange]() {
            ColorPickerSpec textSpec("Text", "quick_online_text", quickOnlineTextColor_);
            textSpec.showAlpha = true;
            textSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(textSpec);
            
            ColorPickerSpec textDisabledSpec("Text Disabled", "quick_online_text_disabled", quickOnlineTextDisabled_);
            textDisabledSpec.showAlpha = true;
            textDisabledSpec.onChange = applyQuickOnlineColorChange;
            createColorPicker(textDisabledSpec);
        });
    })) {
    }
}

// Renders the Notification window theme customization section. Provides color pickers
// for all theme colors specific to the Notification window. Updates theme in real-time.
// Flow:
//   1. Validate command handler exists
//   2. Sync colors from ThemeUseCase to UI buffers (if not already synced)
//   3. Render collapsible section with color pickers for all theme colors
//   4. On color change: sync buffers to colors, update ThemeUseCase, emit command
// Invariants:
//   - commandHandler_ must be valid (checked at start)
//   - Colors synced from ThemeUseCase before first render
//   - Changes immediately applied via syncNotificationBuffersToColors() and command emission
// Edge cases:
//   - Section collapsed state persisted across sessions
//   - Real-time updates apply immediately (no save button needed)
//   - Separate theme from main window theme (independent customization)
void MainWindow::renderNotificationThemeSection() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || !commandHandler_) {
        return;
    }
    
    static bool notificationThemeBuffersSynced = false;
    if (!notificationThemeBuffersSynced) {
        syncNotificationColorsToBuffers();
        notificationThemeBuffersSynced = true;
    }
    
    auto applyNotificationColorChange = [this]() {
        syncNotificationBuffersToColors();
    };
    
    auto renderNotificationColorSection = [this, &applyNotificationColorChange](const std::string& sectionName, const std::string& sectionId, bool* collapsed, std::function<void()> renderColors) {
        bool isOpen = createCollapsibleSection(sectionName, sectionId, renderColors);
        *collapsed = !isOpen;
    };
    
    if (UI::createCollapsibleSection("Notification Theme", "notification_theme_section", [this, &applyNotificationColorChange, &renderNotificationColorSection]() {
        IUiRenderer* renderer = GetUiRenderer();
        if (renderer == nullptr) {
            return;
        }
        
        renderNotificationColorSection("Window", "notification_color_section_window", &colorSectionWindowCollapsed_, [this, &applyNotificationColorChange]() {
            ColorPickerSpec windowBgSpec("Window Background", "notification_window_bg", notificationWindowBgColor_);
            windowBgSpec.showAlpha = true;
            windowBgSpec.onChange = applyNotificationColorChange;
            createColorPicker(windowBgSpec);
            
            ColorPickerSpec childBgSpec("Child Background", "notification_child_bg", notificationChildBgColor_);
            childBgSpec.showAlpha = true;
            childBgSpec.onChange = applyNotificationColorChange;
            createColorPicker(childBgSpec);
        });
        
        renderNotificationColorSection("Frame", "notification_color_section_frame", &colorSectionFrameCollapsed_, [this, &applyNotificationColorChange]() {
            ColorPickerSpec frameBgSpec("Frame Background", "notification_frame_bg", notificationFrameBgColor_);
            frameBgSpec.showAlpha = true;
            frameBgSpec.onChange = applyNotificationColorChange;
            createColorPicker(frameBgSpec);
            
            ColorPickerSpec frameBgHoveredSpec("Frame Hovered", "notification_frame_bg_hovered", notificationFrameBgHovered_);
            frameBgHoveredSpec.showAlpha = true;
            frameBgHoveredSpec.onChange = applyNotificationColorChange;
            createColorPicker(frameBgHoveredSpec);
            
            ColorPickerSpec frameBgActiveSpec("Frame Active", "notification_frame_bg_active", notificationFrameBgActive_);
            frameBgActiveSpec.showAlpha = true;
            frameBgActiveSpec.onChange = applyNotificationColorChange;
            createColorPicker(frameBgActiveSpec);
        });
        
        renderNotificationColorSection("Title", "notification_color_section_title", &colorSectionTitleCollapsed_, [this, &applyNotificationColorChange]() {
            ColorPickerSpec titleBgSpec("Title Background", "notification_title_bg", notificationTitleBg_);
            titleBgSpec.showAlpha = true;
            titleBgSpec.onChange = applyNotificationColorChange;
            createColorPicker(titleBgSpec);
            
            ColorPickerSpec titleBgActiveSpec("Title Active", "notification_title_bg_active", notificationTitleBgActive_);
            titleBgActiveSpec.showAlpha = true;
            titleBgActiveSpec.onChange = applyNotificationColorChange;
            createColorPicker(titleBgActiveSpec);
            
            ColorPickerSpec titleBgCollapsedSpec("Title Collapsed", "notification_title_bg_collapsed", notificationTitleBgCollapsed_);
            titleBgCollapsedSpec.showAlpha = true;
            titleBgCollapsedSpec.onChange = applyNotificationColorChange;
            createColorPicker(titleBgCollapsedSpec);
        });
        
        renderNotificationColorSection("Button", "notification_color_section_button", &colorSectionButtonCollapsed_, [this, &applyNotificationColorChange]() {
            ColorPickerSpec buttonSpec("Button", "notification_button", notificationButtonColor_);
            buttonSpec.showAlpha = true;
            buttonSpec.onChange = applyNotificationColorChange;
            createColorPicker(buttonSpec);
            
            ColorPickerSpec buttonHoverSpec("Button Hovered", "notification_button_hovered", notificationButtonHoverColor_);
            buttonHoverSpec.showAlpha = true;
            buttonHoverSpec.onChange = applyNotificationColorChange;
            createColorPicker(buttonHoverSpec);
            
            ColorPickerSpec buttonActiveSpec("Button Active", "notification_button_active", notificationButtonActiveColor_);
            buttonActiveSpec.showAlpha = true;
            buttonActiveSpec.onChange = applyNotificationColorChange;
            createColorPicker(buttonActiveSpec);
        });
        
        renderNotificationColorSection("Text", "notification_color_section_text", &colorSectionTextCollapsed_, [this, &applyNotificationColorChange]() {
            ColorPickerSpec textSpec("Text", "notification_text", notificationTextColor_);
            textSpec.showAlpha = true;
            textSpec.onChange = applyNotificationColorChange;
            createColorPicker(textSpec);
            
            ColorPickerSpec textDisabledSpec("Text Disabled", "notification_text_disabled", notificationTextDisabled_);
            textDisabledSpec.showAlpha = true;
            textDisabledSpec.onChange = applyNotificationColorChange;
            createColorPicker(textDisabledSpec);
        });
    })) {
    }
}


std::string MainWindow::getWindowTitle() const {
    std::string title = "XIFriendList";
    
    if (friendListViewModel_ != nullptr) {
        std::string characterName = friendListViewModel_->getCurrentCharacterName();
        if (!characterName.empty()) {
            std::string characterNameUpper = characterName;
            if (!characterNameUpper.empty()) {
                characterNameUpper[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(characterNameUpper[0])));
            }
            title += " - " + characterNameUpper;
        }
    }
    
    return title;
}


void MainWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
        WindowCommand command(type, data);
        commandHandler_->handleCommand(command);
    }
}

void MainWindow::emitPreferenceUpdate(const std::string& field, bool value) {
    std::ostringstream json;
    json << "{\"field\":\"" << field << "\",\"value\":" << (value ? "true" : "false") << "}";
    emitCommand(WindowCommandType::UpdatePreference, json.str());
}

void MainWindow::emitPreferenceUpdate(const std::string& field, float value) {
    std::ostringstream json;
    json << "{\"field\":\"" << field << "\",\"value\":" << value << "}";
    emitCommand(WindowCommandType::UpdatePreference, json.str());
}

void MainWindow::emitPreferenceUpdate(const std::string& field, int value) {
    std::ostringstream json;
    json << "{\"field\":\"" << field << "\",\"value\":" << value << "}";
    emitCommand(WindowCommandType::UpdatePreference, json.str());
}

void MainWindow::emitPreferenceUpdate(const std::string& field, const std::string& value) {
    std::ostringstream json;
    json << "{\"field\":\"" << field << "\",\"value\":\"" << XIFriendList::Protocol::JsonUtils::escapeString(value) << "\"}";
    emitCommand(WindowCommandType::UpdatePreference, json.str());
}

Core::MemoryStats MainWindow::getMemoryStats() const {
    size_t bytes = sizeof(MainWindow);
    
    bytes += newFriendInput_.capacity();
    bytes += newFriendNoteInput_.capacity();
    bytes += altVisibilityFilterText_.capacity();
    bytes += cachedWindowTitle_.capacity();
    bytes += cachedCharacterName_.capacity();
    
    bytes += altVisibilityCheckboxValues_.size() * (sizeof(int) + sizeof(bool));
    
    constexpr size_t COLOR_BUFFER_SIZE = 4 * sizeof(float);
    bytes += 28 * COLOR_BUFFER_SIZE;
    bytes += 28 * COLOR_BUFFER_SIZE;
    bytes += 28 * COLOR_BUFFER_SIZE;
    
    bytes += friendTable_.getMemoryStats().estimatedBytes;
    
    return Core::MemoryStats(1, bytes, "MainWindow State");
}

void MainWindow::renderFriendDetailsPopup() {
    if (selectedFriendForDetails_.empty() || !friendListViewModel_) {
        return;
    }
    
    auto detailsOpt = friendListViewModel_->getFriendDetails(selectedFriendForDetails_);
    if (!detailsOpt.has_value()) {
        selectedFriendForDetails_.clear();
        return;
    }
    
    const auto& details = detailsOpt.value();
    const auto& row = details.rowData;
    
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    renderer->SetNextWindowSize(UI::ImVec2(400.0f, 0.0f), 0);
    bool open = true;
    if (renderer->Begin("Friend Details##friend_details_popup", &open, 0x00000040)) {
        if (!open) {
            selectedFriendForDetails_.clear();
            renderer->End();
            return;
        }
        
        renderer->Spacing(5.0f);
        
        // Friend name
        TextSpec nameSpec;
        nameSpec.text = capitalizeWords(row.name);
        nameSpec.id = "friend_details_name";
        nameSpec.visible = true;
        createText(nameSpec);
        renderer->NewLine();
        renderer->Separator();
        renderer->Spacing(5.0f);
        
        // Online status
        void* statusIcon = nullptr;
        if (iconManager_) {
            XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
            if (row.isPending) {
                statusIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Pending);
            } else if (row.isOnline) {
                statusIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Online);
            } else {
                statusIcon = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Offline);
            }
        }
        
        renderer->TextUnformatted("Status: ");
        renderer->SameLine(0.0f, 5.0f);
        if (statusIcon) {
            ImVec2 iconSize(12.0f, 12.0f);
            ImVec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
            if (!row.isOnline && !row.isPending) {
                tint = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
            }
            renderer->Image(statusIcon, iconSize, ImVec2(0, 0), ImVec2(1, 1), tint);
            renderer->SameLine(0.0f, 6.0f);
        }
        std::string statusText = row.isPending ? "Pending" : (row.isOnline ? "Online" : "Offline");
        renderer->TextUnformatted(statusText.c_str());
        renderer->NewLine();
        
        // Friended As
        if (!row.friendedAs.empty() && row.friendedAs != row.name) {
            renderer->TextUnformatted(("Friended As: " + capitalizeWords(row.friendedAs)).c_str());
            renderer->NewLine();
        }
        
        // Job
        if (!row.jobText.empty()) {
            renderer->TextUnformatted(("Job: " + row.jobText).c_str());
            renderer->NewLine();
        }
        
        // Zone
        if (!row.zoneText.empty()) {
            renderer->TextUnformatted(("Zone: " + row.zoneText).c_str());
            renderer->NewLine();
        }
        
        // Nation/Rank
        if (row.nation >= 0 && row.nation <= 3) {
            renderer->TextUnformatted("Nation/Rank: ");
            renderer->SameLine(0.0f, 5.0f);
            if (iconManager_) {
                XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
                void* nationIcon = nullptr;
                XIFriendList::Platform::Ashita::IconType iconType;
                switch (row.nation) {
                    case 0: iconType = XIFriendList::Platform::Ashita::IconType::NationSandy; break;
                    case 1: iconType = XIFriendList::Platform::Ashita::IconType::NationBastok; break;
                    case 2: iconType = XIFriendList::Platform::Ashita::IconType::NationWindurst; break;
                    case 3: iconType = XIFriendList::Platform::Ashita::IconType::NationJeuno; break;
                    default: iconType = XIFriendList::Platform::Ashita::IconType::NationSandy; break;
                }
                nationIcon = iconMgr->getIcon(iconType);
                if (nationIcon && row.nation != 3) {
                    ImVec2 iconSize(13.0f, 13.0f);
                    renderer->Image(nationIcon, iconSize);
                    renderer->SameLine(0.0f, 4.0f);
                }
            }
            std::string rankText = row.rankText.empty() ? "Hidden" : row.rankText;
            renderer->TextUnformatted(rankText.c_str());
            renderer->NewLine();
        }
        
        // Last seen
        if (!row.lastSeenText.empty()) {
            renderer->TextUnformatted(("Last Seen: " + row.lastSeenText).c_str());
            renderer->NewLine();
        }
        
        // Visible alts
        if (!details.linkedCharacters.empty()) {
            renderer->Spacing(5.0f);
            renderer->Separator();
            renderer->Spacing(5.0f);
            renderer->TextUnformatted("Visible Alts:");
            renderer->NewLine();
            for (const auto& altName : details.linkedCharacters) {
                renderer->TextUnformatted(("  - " + capitalizeWords(altName)).c_str());
                renderer->NewLine();
            }
        }
        
        renderer->Spacing(10.0f);
        renderer->Separator();
        renderer->Spacing(5.0f);
        
        // Close button
        ButtonSpec closeButtonSpec;
        closeButtonSpec.label = "Close";
        closeButtonSpec.id = "friend_details_close";
        closeButtonSpec.visible = true;
        closeButtonSpec.enabled = true;
        closeButtonSpec.width = 100.0f;
        closeButtonSpec.onClick = [this]() {
            selectedFriendForDetails_.clear();
        };
        if (createButton(closeButtonSpec)) {
            selectedFriendForDetails_.clear();
        }
    }
    renderer->End();
}

} // namespace UI
} // namespace XIFriendList


