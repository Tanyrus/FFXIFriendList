// Consolidated layout widgets implementation

#include "UI/Widgets/Layout.h"
#include "UI/Widgets/Controls.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Protocol/JsonUtils.h"
#ifndef BUILDING_TESTS
#include "Platform/Ashita/ImGuiBridge.h"
#include <Ashita.h>
#endif
#include <sstream>
#include <functional>

namespace XIFriendList {
namespace UI {

// SectionHeader widget implementation

bool createSectionHeader(const SectionHeaderSpec& spec) {
    if (!spec.visible) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    bool result = false;
    if (spec.collapsible && spec.collapsed != nullptr) {
        // Use CollapsingHeader for collapsible sections
        result = renderer->CollapsingHeader(spec.label.c_str(), spec.collapsed);
    } else {
        // Use Separator + Text for non-collapsible section headers
        renderer->Text("%s", spec.label.c_str());
        renderer->Separator();
        result = false;  // Non-collapsible headers don't return click state
    }
    
    renderer->PopID();
    
    return result;
}

// CollapsibleSection helper implementation

bool createCollapsibleSection(const std::string& label, const std::string& id, std::function<void()> renderContent) {
    if (label.empty() || id.empty() || !renderContent) {
        return false;
    }
    
    #ifndef BUILDING_TESTS
    // Use IGuiManager directly for better state management (same pattern as pending requests)
    IGuiManager* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (guiManager) {
        // Use PushID for stable widget IDs
        guiManager->PushID(id.c_str());
        
        // Render collapsible header (nullptr = ImGui manages state internally)
        bool isOpen = guiManager->CollapsingHeader(label.c_str(), nullptr);
        
        // Render content if section is open
        if (isOpen) {
            renderContent();
        }
        
        guiManager->PopID();
        return isOpen;
    }
    #endif
    
    // Fallback to abstraction layer (for tests or when IGuiManager unavailable)
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    renderer->PushID(id.c_str());
    
    // Use a local collapsed state for the abstraction layer
    bool collapsed = false;
    bool isOpen = renderer->CollapsingHeader(label.c_str(), &collapsed);
    
    if (isOpen) {
        renderContent();
    }
    
    renderer->PopID();
    return isOpen;
}

// ToolbarRow widget implementation

void createToolbarRow(const ToolbarRowSpec& spec) {
    if (!spec.visible || spec.buttons.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Render buttons horizontally
    bool firstButton = true;
    for (const auto& buttonSpec : spec.buttons) {
        if (!buttonSpec.visible) {
            continue;
        }
        
        if (!firstButton) {
            renderer->SameLine(0.0f, spec.spacing);
        }
        firstButton = false;
        
        // Convert ToolbarButtonSpec to ButtonSpec
        ButtonSpec btnSpec;
        btnSpec.label = buttonSpec.label;
        btnSpec.id = buttonSpec.id;
        btnSpec.enabled = buttonSpec.enabled;
        btnSpec.visible = buttonSpec.visible;
        btnSpec.onClick = buttonSpec.onClick;
        
        createButton(btnSpec);
    }
    
    renderer->PopID();
}


bool createTabBar(const TabBarSpec& spec) {
    if (!spec.visible || spec.tabs.empty() || spec.activeTabIndex == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    bool tabChanged = false;
    int previousActiveTab = *spec.activeTabIndex;
    
    // Render tabs horizontally
    for (size_t i = 0; i < spec.tabs.size(); ++i) {
        const auto& tab = spec.tabs[i];
        if (!tab.visible) {
            continue;
        }
        
        if (i > 0) {
            renderer->SameLine();
        }
        
        // Convert TabSpec to ButtonSpec
        ButtonSpec btnSpec;
        btnSpec.label = tab.label;
        btnSpec.id = tab.id;
        btnSpec.enabled = tab.enabled;
        btnSpec.visible = tab.visible;
        btnSpec.onClick = [&tab, &spec, i, &tabChanged]() {
            if (spec.activeTabIndex && *spec.activeTabIndex != static_cast<int>(i)) {
                *spec.activeTabIndex = static_cast<int>(i);
                tabChanged = true;
            }
            if (tab.onClick) {
                tab.onClick();
            }
        };
        
        createButton(btnSpec);
    }
    
    tabChanged = (*spec.activeTabIndex != previousActiveTab);
    
    renderer->PopID();
    
    return tabChanged;
}


} // namespace UI
} // namespace XIFriendList

