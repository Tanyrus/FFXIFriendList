// Helper functions for tooltips and help markers

#include "TooltipHelper.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Platform/Ashita/ImGuiBridge.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif
#include <imgui.h>

namespace XIFriendList {
namespace UI {

void HelpMarker(const char* desc) {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Stay on the same line as the previous widget
    renderer->SameLine(0.0f, 4.0f);
    
    // Render "(?)" text in a disabled color
    renderer->TextDisabled("(?)");
    
    // Show tooltip on hover using IGuiManager
    if (renderer->IsItemHovered()) {
#ifndef BUILDING_TESTS
        // Use IGuiManager to set tooltip (platform-specific)
        auto* guiManager = Platform::Ashita::ImGuiBridge::getGuiManager();
        if (guiManager) {
            guiManager->SetTooltip(desc);
        }
#else
        // Test build - tooltips not available
        (void)desc;
#endif
    }
}

} // namespace UI
} // namespace XIFriendList

