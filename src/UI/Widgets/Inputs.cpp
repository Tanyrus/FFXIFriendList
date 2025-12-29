// Consolidated input widgets implementation

#include "UI/Widgets/Inputs.h"
#include "UI/Interfaces/IUiRenderer.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace XIFriendList {
namespace UI {

// InputText widget implementation

// Track previous active state per input ID for global active tracking
static std::map<std::string, bool> g_inputActiveState;

// Global flag to track if any input is currently active
// Used to prevent game from processing Enter key when typing in input boxes
static bool g_anyInputActive = false;

// Function to check if any input is active (used by command handler)
bool IsAnyInputActive() {
    return g_anyInputActive;
}

// ImGui flag constants
constexpr int kImGuiInputTextFlags_ReadOnly = 0x00000080;        // ImGuiInputTextFlags_ReadOnly
constexpr int kImGuiInputTextFlags_EnterReturnsTrue = 0x00000020; // ImGuiInputTextFlags_EnterReturnsTrue

bool createInputText(const InputTextSpec& spec) {
    if (!spec.visible || spec.buffer == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Ensure buffer has enough capacity
    if (spec.buffer->capacity() < spec.bufferSize) {
        spec.buffer->reserve(spec.bufferSize);
    }
    
    // ImGui InputText requires a char array, so we need to work with the buffer
    // We'll use a temporary buffer and copy back
    std::string tempBuffer = *spec.buffer;
    tempBuffer.resize(spec.bufferSize, '\0');
    
    // Store old value to detect changes
    std::string oldValue = *spec.buffer;
    
    // - ReadOnly: prevent editing when disabled
    // - EnterReturnsTrue: InputText returns true ONLY when Enter is pressed (not on blur or Tab)
    //   This prevents accidental submissions when clicking away from the input
    int flags = 0;
    if (spec.readOnly) {
        flags |= kImGuiInputTextFlags_ReadOnly;
    }
    if (spec.onEnter) {
        flags |= kImGuiInputTextFlags_EnterReturnsTrue;
    }
    
    bool inputResult = renderer->InputText(spec.label.c_str(), 
                                           const_cast<char*>(tempBuffer.c_str()), 
                                           spec.bufferSize,
                                           flags);
    
    // - inputResult is true ONLY when Enter is pressed
    // - inputResult is false on blur, Tab, or any other deactivation
    // - inputResult is true whenever the text changes
    
    // Always update buffer from tempBuffer (text may have changed even if Enter wasn't pressed)
    size_t len = strlen(tempBuffer.c_str());
    std::string newValue(tempBuffer.c_str(), len);
    
    if (spec.enabled && !spec.readOnly && newValue != oldValue) {
        *spec.buffer = newValue;
        
        // Call onChange callback for text changes
        if (spec.onChange) {
            spec.onChange(*spec.buffer);
        }
    }
    
    // Track active state for global input tracking
    bool isCurrentlyActive = renderer->IsItemActive();
    g_inputActiveState[spec.id] = isCurrentlyActive;
    
    // Placeholder text support deferred
    // Cannot implement overlay placeholder due to Windows API SetCursorPos macro conflict
    // SDK doesn't expose InputTextWithHint, and overlay approach requires SetCursorPos
    // This feature is deferred until SDK exposes InputTextWithHint or workaround found
    
    g_anyInputActive = false;
    for (const auto& pair : g_inputActiveState) {
        if (pair.second) {
            g_anyInputActive = true;
            break;
        }
    }
    
    // Trigger onEnter callback when Enter is pressed (inputResult is true with EnterReturnsTrue flag)
    if (spec.enabled && !spec.readOnly && spec.onEnter && inputResult && !spec.buffer->empty()) {
        spec.onEnter(*spec.buffer);
    }
    
    renderer->PopID();
    
    return inputResult && spec.enabled && !spec.readOnly;
}

// InputTextMultiline widget implementation

bool createInputTextMultiline(const InputTextMultilineSpec& spec) {
    if (!spec.visible || spec.buffer == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Ensure buffer has enough capacity
    if (spec.buffer->capacity() < spec.bufferSize) {
        spec.buffer->reserve(spec.bufferSize);
    }
    
    // ImGui InputTextMultiline requires a char array, so we need to work with the buffer
    // We'll use a temporary buffer and copy back
    std::string tempBuffer = *spec.buffer;
    tempBuffer.resize(spec.bufferSize, '\0');
    
    // Store old value to detect changes
    std::string oldValue = *spec.buffer;
    
    ImVec2 size(spec.width, spec.height);
    
    // Render multiline input text (use flags for read-only state)
    // Note: ImGuiInputTextFlags_ReadOnly is typically 0x00000080
    int flags = spec.readOnly ? 0x00000080 : 0;  // ImGuiInputTextFlags_ReadOnly
    bool changed = renderer->InputTextMultiline(spec.label.c_str(), 
                                                const_cast<char*>(tempBuffer.c_str()), 
                                                spec.bufferSize,
                                                size,
                                                flags);
    
    if (changed && spec.enabled && !spec.readOnly) {
        // Copy back to buffer (trim null terminator)
        size_t len = strlen(tempBuffer.c_str());
        *spec.buffer = std::string(tempBuffer.c_str(), len);
        
        // Call onChange callback
        if (spec.onChange && *spec.buffer != oldValue) {
            spec.onChange(*spec.buffer);
        }
    }
    
    renderer->PopID();
    
    return changed && spec.enabled && !spec.readOnly;
}

// Combo widget implementation

bool createCombo(const ComboSpec& spec) {
    if (!spec.visible || spec.currentItem == nullptr || spec.items.empty()) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Clamp current item to valid range
    if (*spec.currentItem < 0 || *spec.currentItem >= static_cast<int>(spec.items.size())) {
        *spec.currentItem = 0;
    }
    
    const char* preview = (*spec.currentItem >= 0 && *spec.currentItem < static_cast<int>(spec.items.size()))
        ? spec.items[*spec.currentItem].c_str()
        : "";
    
    bool changed = false;
    if (spec.enabled) {
        // Begin combo - this should always render the button, even if it returns false
        // (false just means the popup isn't open)
        bool comboOpen = renderer->BeginCombo(spec.label.c_str(), preview, 0);
        
        // Always render items if combo is open
        if (comboOpen) {
            // Render items
            for (size_t i = 0; i < spec.items.size(); ++i) {
                bool isSelected = (*spec.currentItem == static_cast<int>(i));
                if (renderer->Selectable(spec.items[i].c_str(), isSelected)) {
                    *spec.currentItem = static_cast<int>(i);
                    changed = true;
                }
            }
            renderer->EndCombo();
        }
        // Note: Even if BeginCombo returns false, the combo button should still be rendered
    } else {
        renderer->Text("%s: %s", spec.label.c_str(), preview);
    }
    
    // Call onChange callback if selection changed
    if (changed && spec.onChange) {
        spec.onChange(*spec.currentItem);
    }
    
    renderer->PopID();
    
    return changed;
}

// Slider widget implementation

bool createSlider(const SliderSpec& spec) {
    if (!spec.visible || spec.value == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    renderer->PushID(spec.id.c_str());
    
    bool changed = false;
    if (spec.enabled) {
        changed = renderer->SliderFloat(spec.label.c_str(), spec.value, spec.min, spec.max, spec.format);
    } else {
        renderer->Text("%s: %.2f", spec.label.c_str(), *spec.value);
    }
    
    if (changed && spec.onChange) {
        spec.onChange(*spec.value);
    }
    
    if (spec.enabled && renderer->IsItemDeactivatedAfterEdit() && spec.onDeactivated) {
        spec.onDeactivated(*spec.value);
    }
    
    renderer->PopID();
    return changed;
}

// ColorPicker widget implementation

bool createColorPicker(const ColorPickerSpec& spec) {
    if (!spec.visible || spec.color == nullptr) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    renderer->PushID(spec.id.c_str());
    
    bool changed = false;
    if (spec.enabled) {
        // ColorEdit4 with alpha bar flag
        int flags = 0;
        if (spec.showAlpha) {
            flags = 0x00000002;  // ImGuiColorEditFlags_AlphaBar
        }
        changed = renderer->ColorEdit4(spec.label.c_str(), spec.color, flags);
    } else {
        renderer->Text("%s: (%.2f, %.2f, %.2f, %.2f)", 
            spec.label.c_str(), 
            spec.color[0], spec.color[1], spec.color[2], 
            spec.showAlpha ? spec.color[3] : 1.0f);
    }
    
    if (changed && spec.onChange) {
        spec.onChange();
    }
    
    renderer->PopID();
    return changed;
}

} // namespace UI
} // namespace XIFriendList

