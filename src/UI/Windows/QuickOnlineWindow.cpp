
#include "UI/Windows/QuickOnlineWindow.h"

#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "UI/Widgets/Layout.h"
#include "UI/Widgets/Controls.h"
#include "UI/Widgets/Indicators.h"
#include "UI/UiConstants.h"
#include "Protocol/JsonUtils.h"
#include "Core/MemoryStats.h"
#include "Platform/Ashita/IconManager.h"
#include "UI/Windows/UiCloseCoordinator.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <sstream>

namespace XIFriendList {
namespace UI {

QuickOnlineWindow::QuickOnlineWindow() {
    FriendTableWidgetSpec spec;
    spec.tableId = "quick_online_table";
    spec.toggleRowId = "quick_online_column_visibility_row";
    spec.sectionHeaderId = "quick_online_header";
    spec.sectionHeaderLabel = "";
    spec.showSectionHeader = false;
    spec.showColumnToggles = false;
    spec.commandScope = "QuickOnline";
    friendTable_.setSpec(spec);
}

void QuickOnlineWindow::render() {
    if (!visible_ || viewModel_ == nullptr) {
        return;
    }

    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }

    if (pendingClose_ && isUiMenuCleanForClose(renderer)) {
        pendingClose_ = false;
        visible_ = false;
        return;
    }

    static bool sizeSet = false;
    if (!sizeSet) {
        ImVec2 windowSize(420.0f, 320.0f);
        renderer->SetNextWindowSize(windowSize, 0x00000002); // ImGuiCond_Once
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
        windowFlags = 0x0004 | 0x0002; // NoMove | NoResize
    }

    bool windowOpen = visible_;
    if (!renderer->Begin(title_.c_str(), &windowOpen, windowFlags)) {
        renderer->End();
        applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
        return;
    }

    visible_ = windowOpen;
    applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
    
    // Top bar with Refresh and Lock buttons
    renderTopBar();
    
    UI::ImVec2 contentAvail = renderer->GetContentRegionAvail();
    renderer->BeginChild("##quick_online_body", UI::ImVec2(0.0f, contentAvail.y), false, WINDOW_BODY_CHILD_FLAGS);
    friendTable_.render();
    renderer->EndChild();

    renderer->End();
    
    // Render friend details popup if a friend is selected
    if (!selectedFriendForDetails_.empty()) {
        renderFriendDetailsPopup();
    }
}

void QuickOnlineWindow::renderTopBar() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr || viewModel_ == nullptr) {
        return;
    }
    
    // Refresh button
    ButtonSpec refreshButtonSpec;
    refreshButtonSpec.label = std::string(Constants::BUTTON_REFRESH);
    refreshButtonSpec.id = "quick_online_refresh_button";
    refreshButtonSpec.enabled = viewModel_->isConnected();
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
}

void QuickOnlineWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
    WindowCommand command(type, data);
    commandHandler_->handleCommand(command);
    }
}

::XIFriendList::Core::MemoryStats QuickOnlineWindow::getMemoryStats() const {
    size_t bytes = sizeof(QuickOnlineWindow);
    
    bytes += title_.capacity();
    bytes += windowId_.capacity();
    bytes += friendTable_.getMemoryStats().estimatedBytes;
    
    return ::XIFriendList::Core::MemoryStats(1, bytes, "QuickOnline Window");
}

void QuickOnlineWindow::renderFriendDetailsPopup() {
    if (selectedFriendForDetails_.empty() || !viewModel_) {
        return;
    }
    
    auto detailsOpt = viewModel_->getFriendDetails(selectedFriendForDetails_);
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
    
    auto capitalizeWords = [](const std::string& s) -> std::string {
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
    };
    
    renderer->SetNextWindowSize(UI::ImVec2(400.0f, 0.0f), 0);
    bool open = true;
    if (renderer->Begin("Friend Details##quick_online_friend_details_popup", &open, 0x00000040)) {
        if (!open) {
            selectedFriendForDetails_.clear();
            renderer->End();
            return;
        }
        
        renderer->Spacing(5.0f);
        
        TextSpec nameSpec;
        nameSpec.text = capitalizeWords(row.name);
        nameSpec.id = "friend_details_name";
        nameSpec.visible = true;
        createText(nameSpec);
        renderer->NewLine();
        renderer->Separator();
        renderer->Spacing(5.0f);
        
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


