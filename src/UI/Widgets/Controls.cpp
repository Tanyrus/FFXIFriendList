// Consolidated control widgets implementation

#include "UI/Widgets/Controls.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Protocol/JsonUtils.h"
#include <sstream>

namespace XIFriendList {
namespace UI {

// Button widget implementation

bool createButton(const ButtonSpec& spec) {
    if (!spec.visible) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Render button with optional size
    ImVec2 buttonSize(spec.width > 0.0f ? spec.width : 0.0f, 
                      spec.height > 0.0f ? spec.height : 0.0f);
    bool clicked = renderer->Button(spec.label.c_str(), buttonSize);
    
    if (!spec.enabled && renderer->IsItemHovered()) {
        // Could set tooltip or visual feedback here
    }
    
    renderer->PopID();
    
    // Call onClick callback if button was clicked and enabled
    if (clicked && spec.enabled && spec.onClick) {
        spec.onClick();
    }
    
    return clicked && spec.enabled;
}

// Toggle widget implementation

bool createToggle(const ToggleSpec& spec) {
    if (!spec.visible || spec.value == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Store old value to detect changes
    bool oldValue = *spec.value;
    
    // Render checkbox
    bool changed = renderer->Checkbox(spec.label.c_str(), spec.value);
    
    if (!spec.enabled) {
        *spec.value = oldValue;
        changed = false;
    }
    
    renderer->PopID();
    
    // Call onChange callback if value changed and enabled
    if (changed && spec.enabled && spec.onChange) {
        spec.onChange();
    }
    
    return changed && spec.enabled;
}


void createCheckboxRow(const CheckboxRowSpec& spec) {
    if (!spec.visible || spec.checkboxes.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Render checkboxes horizontally
    bool firstCheckbox = true;
    for (const auto& checkboxSpec : spec.checkboxes) {
        if (!checkboxSpec.visible || checkboxSpec.value == nullptr) {
            continue;
        }
        
        if (!firstCheckbox) {
            renderer->SameLine(0.0f, spec.spacing);
        }
        firstCheckbox = false;
        
        // Convert CheckboxItemSpec to ToggleSpec
        ToggleSpec toggleSpec;
        toggleSpec.label = checkboxSpec.label;
        toggleSpec.id = checkboxSpec.id;
        toggleSpec.value = checkboxSpec.value;
        toggleSpec.enabled = checkboxSpec.enabled;
        toggleSpec.visible = checkboxSpec.visible;
        toggleSpec.onChange = checkboxSpec.onChange;
        
        createToggle(toggleSpec);
    }
    
    renderer->PopID();
}

// LockButton widget implementation

void renderLockButtonInline(const std::string& windowId,
                            bool lockState,
                            IWindowCommandHandler* commandHandler,
                            float widthPx,
                            float heightPx) {
    if (!commandHandler || windowId.empty()) {
        return;
    }

    ButtonSpec buttonSpec;
    buttonSpec.label = lockState ? "L" : "U";
    buttonSpec.id = "window_lock_button_" + windowId;
    buttonSpec.width = widthPx;
    buttonSpec.height = heightPx;
    buttonSpec.enabled = true;
    buttonSpec.visible = true;
    buttonSpec.onClick = [commandHandler, windowId, lockState]() {
        bool newLockState = !lockState;
        std::ostringstream json;
        json << "{\"windowId\":\"" << Protocol::JsonUtils::escapeString(windowId)
             << "\",\"locked\":" << (newLockState ? "true" : "false") << "}";
        WindowCommand command(WindowCommandType::UpdateWindowLock, json.str());
        commandHandler->handleCommand(command);
    };
    createButton(buttonSpec);
}

void renderLockButton(const std::string& windowId, bool lockState, IWindowCommandHandler* commandHandler) {
    if (!commandHandler || windowId.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (!renderer) {
        return;
    }
    
    ImVec2 avail = renderer->GetContentRegionAvail();
    
    // Add spacing to push button to bottom (leave small padding)
    float buttonHeight = 20.0f;
    float padding = 5.0f;
    float spacing = avail.y - buttonHeight - padding;
    
    // Add spacing before button to push it to bottom
    if (spacing > 0) {
        // Use separator as visual spacer
        renderer->Separator();
    }
    
    // Button will be positioned at bottom-left by spacing above (legacy behavior).
    renderLockButtonInline(windowId, lockState, commandHandler, 30.0f, buttonHeight);
}

} // namespace UI
} // namespace XIFriendList

