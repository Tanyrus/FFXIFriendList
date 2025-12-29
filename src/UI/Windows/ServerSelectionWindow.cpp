
#include "UI/Windows/ServerSelectionWindow.h"
#include "UI/Widgets/Controls.h"
#include "UI/Widgets/Inputs.h"
#include "UI/Widgets/Indicators.h"
#include "UI/Widgets/Layout.h"
#include "UI/UiConstants.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Protocol/JsonUtils.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Core/MemoryStats.h"
#include "UI/Windows/UiCloseCoordinator.h"
#include "UI/Helpers/WindowHelper.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <algorithm>
#include <map>

namespace XIFriendList {
namespace UI {

ServerSelectionWindow::ServerSelectionWindow()
    : commandHandler_(nullptr)
    , visible_(false)
    , windowId_("ServerSelection")
    , locked_(false)
    , pendingClose_(false)
{
}

void ServerSelectionWindow::setServerList(const XIFriendList::Core::ServerList& serverList) {
    serverList_ = serverList;
    rebuildCombinedServerList();
    
    // Initialize draft with detected server if available and not already set
    if (draftSelectedServerId_.empty() && detectedServerId_.has_value()) {
        // Check if detected server is in the list
        for (const auto& server : combinedServerList_) {
            if (server.id == detectedServerId_.value()) {
                draftSelectedServerId_ = detectedServerId_.value();
                break;
            }
        }
    }
}

void ServerSelectionWindow::setServerSelectionState(const XIFriendList::App::State::ServerSelectionState& state) {
    state_ = state;
}

void ServerSelectionWindow::setDetectedServerSuggestion(const std::string& serverId, const std::string& serverName) {
    detectedServerId_ = serverId;
    detectedServerName_ = serverName;
    
    // Initialize draft with detected server if available and not already set
    if (draftSelectedServerId_.empty()) {
        // Check if detected server is in the list
        for (const auto& server : combinedServerList_) {
            if (server.id == serverId) {
                draftSelectedServerId_ = serverId;
                break;
            }
        }
    }
}

void ServerSelectionWindow::clearDetectedServerSuggestion() {
    detectedServerId_ = std::nullopt;
    detectedServerName_ = std::nullopt;
}

std::string ServerSelectionWindow::getDraftSelectedServerId() const {
    return draftSelectedServerId_;
}

void ServerSelectionWindow::setDraftSelectedServerId(const std::string& serverId) {
    draftSelectedServerId_ = serverId;
}

void ServerSelectionWindow::rebuildCombinedServerList() {
    combinedServerList_.clear();
    
    for (const auto& server : serverList_.servers) {
        if (!server.id.empty()) {
            combinedServerList_.push_back(server);
        }
    }
    
    std::sort(combinedServerList_.begin(), combinedServerList_.end(), 
              [](const XIFriendList::Core::ServerInfo& a, const XIFriendList::Core::ServerInfo& b) {
                  return a.name < b.name;
              });
}

void ServerSelectionWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_) {
        UI::WindowCommand cmd(type, data);
        commandHandler_->handleCommand(cmd);
    }
}

void ServerSelectionWindow::render() {
    if (!visible_) {
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
        ImVec2 windowSize(350.0f, 280.0f);
        renderer->SetNextWindowSize(windowSize, 0x00000002);
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
    
    int windowFlags = 0x0001 | 0x0008;
    if (locked_) {
        windowFlags |= 0x0004 | 0x0002;
    }
    
    std::string windowTitle = "Select Server##" + windowId_;
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
        renderer->End();
        return;
    }
    
    renderServerSelection();
    
    renderer->End();
}

void ServerSelectionWindow::renderServerSelection() {
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    // Enable text wrapping
    ImVec2 avail = renderer->GetContentRegionAvail();
    float wrapPos = avail.x > 0 ? avail.x : 300.0f;
    renderer->PushTextWrapPos(wrapPos);
    
    renderer->PushID("explanation_text");
    renderer->TextUnformatted("The plugin will not connect to the server until you select and save a server.");
    renderer->PopID();
    
    renderer->Separator();
    
    renderer->PushID("warning_text");
    renderer->TextUnformatted("Warning: If you select the wrong server, you may not be able to find your friends.");
    renderer->PopID();
    
    renderer->PopTextWrapPos();
    
    renderer->Separator();
    
    if (!serverList_.loaded && serverList_.error.empty()) {
        TextSpec loadingSpec;
        loadingSpec.text = "Loading server list...";
        loadingSpec.id = "loading_text";
        loadingSpec.visible = true;
        createText(loadingSpec);
        
        ButtonSpec retrySpec;
        retrySpec.label = "Retry";
        retrySpec.id = "retry_button";
        retrySpec.enabled = true;
        retrySpec.visible = true;
        retrySpec.onClick = [this]() {
            emitCommand(WindowCommandType::RefreshServerList, "");
        };
        createButton(retrySpec);
    } else if (!serverList_.error.empty()) {
        // Enable text wrapping for error messages
        ImVec2 errorAvail = renderer->GetContentRegionAvail();
        float errorWrapPos = errorAvail.x > 0 ? errorAvail.x : 300.0f;
        renderer->PushTextWrapPos(errorWrapPos);
        
        renderer->PushID("error_text");
        std::string errorText = "Error: " + serverList_.error;
        renderer->TextUnformatted(errorText.c_str());
        renderer->PopID();
        
        renderer->Separator();
        
        renderer->PushID("retry_hint_text");
        renderer->TextUnformatted("Please retry to load the server list.");
        renderer->PopID();
        
        renderer->PopTextWrapPos();
        
        renderer->Separator();
        
        ButtonSpec retrySpec;
        retrySpec.label = "Retry";
        retrySpec.id = "retry_button";
        retrySpec.enabled = true;
        retrySpec.visible = true;
        retrySpec.onClick = [this]() {
            emitCommand(WindowCommandType::RefreshServerList, "");
        };
        createButton(retrySpec);
    }
    
    renderer->Separator();
    
    rebuildCombinedServerList();
    
    if (combinedServerList_.empty()) {
        TextSpec emptySpec;
        emptySpec.text = "No servers available.";
        emptySpec.id = "empty_text";
        emptySpec.visible = true;
        createText(emptySpec);
    } else {
        std::vector<std::string> serverNames;
        serverNames.push_back("NONE");
        int currentIndex = 0;
        
        for (size_t i = 0; i < combinedServerList_.size(); ++i) {
            serverNames.push_back(combinedServerList_[i].name);
            
            if (combinedServerList_[i].id == draftSelectedServerId_) {
                currentIndex = static_cast<int>(i + 1);
            }
        }
        
        ComboSpec serverComboSpec;
        serverComboSpec.label = "Server";
        serverComboSpec.id = "server_combo";
        serverComboSpec.currentItem = &currentIndex;
        serverComboSpec.items = serverNames;
        serverComboSpec.enabled = true;
        serverComboSpec.visible = true;
        serverComboSpec.onChange = [this](int newIndex) {
            if (newIndex == 0) {
                draftSelectedServerId_.clear();
            } else {
                size_t serverIndex = static_cast<size_t>(newIndex - 1);
                if (serverIndex < combinedServerList_.size()) {
                    draftSelectedServerId_ = combinedServerList_[serverIndex].id;
                }
            }
        };
        
        bool changed = createCombo(serverComboSpec);
        if (changed && currentIndex > 0) {
            size_t serverIndex = static_cast<size_t>(currentIndex - 1);
            if (serverIndex < combinedServerList_.size()) {
                draftSelectedServerId_ = combinedServerList_[serverIndex].id;
            }
        }
    }
    
    renderer->Separator();
    
    ButtonSpec saveSpec;
    saveSpec.label = "Save";
    saveSpec.id = "save_button";
    saveSpec.enabled = !draftSelectedServerId_.empty();
    saveSpec.visible = true;
    saveSpec.onClick = [this]() {
        if (!draftSelectedServerId_.empty()) {
            emitCommand(WindowCommandType::SaveServerSelection, draftSelectedServerId_);
            pendingClose_ = true;
        }
    };
    createButton(saveSpec);
    
    renderer->SameLine();
    
    ButtonSpec closeSpec;
    closeSpec.label = "Close";
    closeSpec.id = "close_button";
    closeSpec.enabled = true;
    closeSpec.visible = true;
    closeSpec.onClick = [this]() {
        pendingClose_ = true;
    };
    createButton(closeSpec);
}

::XIFriendList::Core::MemoryStats ServerSelectionWindow::getMemoryStats() const {
    size_t bytes = sizeof(ServerSelectionWindow);
    
    bytes += windowId_.capacity();
    bytes += draftSelectedServerId_.capacity();
    if (detectedServerId_.has_value()) {
        bytes += detectedServerId_->capacity();
    }
    if (detectedServerName_.has_value()) {
        bytes += detectedServerName_->capacity();
    }
    for (const auto& server : combinedServerList_) {
        bytes += server.id.capacity() + server.name.capacity() + 
                 server.baseUrl.capacity() + server.realmId.capacity();
    }
    bytes += serverList_.servers.size() * sizeof(XIFriendList::Core::ServerInfo);
    
    return ::XIFriendList::Core::MemoryStats(1, bytes, "ServerSelection Window");
}

} // namespace UI
} // namespace XIFriendList

