// Helper functions for common window operations

#include "WindowHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Platform/Ashita/IconManager.h"
#include "Protocol/JsonUtils.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "UI/Widgets/Controls.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <sstream>

namespace XIFriendList {
namespace UI {

float calculateLockButtonReserve() {
    float buttonHeight = 20.0f;
    float padding = 8.0f;  // Increased padding to ensure button is fully visible
    return buttonHeight + padding;
}

void renderLockButton(IUiRenderer* renderer, const std::string& windowId, bool& locked, void* iconManager, IWindowCommandHandler* commandHandler) {
    if (!renderer || !commandHandler || windowId.empty()) {
        return;
    }
    
    float buttonHeight = 20.0f;
    
#ifndef BUILDING_TESTS
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        void* lockIcon = nullptr;
        if (iconManager) {
            XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager);
            lockIcon = locked ? iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Lock) : iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Unlock);
        }
        
        ::ImVec2 btnSize(buttonHeight, buttonHeight);
        if (lockIcon) {
            guiManager->Image(lockIcon, btnSize);
            if (guiManager->IsItemHovered()) {
                std::string tooltip = locked ? "Window locked" : "Lock window";
                guiManager->SetTooltip(tooltip.c_str());
            }
            if (guiManager->IsItemClicked(0)) {
                bool newLocked = !locked;
                std::ostringstream json;
                json << "{\"windowId\":\"" << XIFriendList::Protocol::JsonUtils::escapeString(windowId)
                     << "\",\"locked\":" << (newLocked ? "true" : "false") << "}";
                WindowCommand cmd(WindowCommandType::UpdateWindowLock, json.str());
                commandHandler->handleCommand(cmd);
                locked = newLocked;
            }
        } else {
            // Fallback to text button if icon not available
            std::string lockLabel = locked ? "🔒" : "🔓";
            if (guiManager->Button(lockLabel.c_str(), btnSize)) {
                bool newLocked = !locked;
                std::ostringstream json;
                json << "{\"windowId\":\"" << XIFriendList::Protocol::JsonUtils::escapeString(windowId)
                     << "\",\"locked\":" << (newLocked ? "true" : "false") << "}";
                WindowCommand cmd(WindowCommandType::UpdateWindowLock, json.str());
                commandHandler->handleCommand(cmd);
                locked = newLocked;
            }
            if (guiManager->IsItemHovered()) {
                std::string tooltip = locked ? "Window locked" : "Lock window";
                guiManager->SetTooltip(tooltip.c_str());
            }
        }
    } else {
        // Fallback: use the old renderLockButtonInline approach
        renderLockButtonInline(windowId, locked, commandHandler, 30.0f, buttonHeight);
    }
#else
    // Test build: use the old renderLockButtonInline approach
    renderLockButtonInline(windowId, locked, commandHandler, 30.0f, buttonHeight);
#endif
}

} // namespace UI
} // namespace XIFriendList

