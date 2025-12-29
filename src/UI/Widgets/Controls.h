// Consolidated control widgets: Button, Toggle, CheckboxRow, LockButton
//
// File Organization:
// - Section headers use: // ============================================================================
// - Widgets are grouped by type
// - Forward declarations used when possible
// - Includes: WidgetSpecs, Commands, Interfaces, Standard Library

#ifndef UI_CONTROLS_H
#define UI_CONTROLS_H

#include "UI/Widgets/WidgetSpecs.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/Interfaces/IUiRenderer.h"
#include <vector>

namespace XIFriendList {
namespace UI {

// Forward declaration
class OptionsViewModel;

// Button widget

// Returns true if button was clicked
bool createButton(const ButtonSpec& spec);

// Toggle widget

// Returns true if toggle state changed
bool createToggle(const ToggleSpec& spec);


struct CheckboxItemSpec {
    std::string label;
    std::string id;
    bool* value;  // Pointer to boolean value (must remain valid)
    bool enabled = true;
    bool visible = true;
    std::function<void()> onChange;  // Callback when toggled
    
    CheckboxItemSpec() : value(nullptr) {}
    CheckboxItemSpec(const std::string& label, const std::string& id, bool* value)
        : label(label), id(id), value(value) {}
};

struct CheckboxRowSpec {
    std::string id;
    std::vector<CheckboxItemSpec> checkboxes;
    float spacing = 10.0f;  // Spacing between checkboxes
    bool visible = true;
    
    CheckboxRowSpec() = default;
    CheckboxRowSpec(const std::string& id, const std::vector<CheckboxItemSpec>& checkboxes)
        : id(id), checkboxes(checkboxes) {}
};

void createCheckboxRow(const CheckboxRowSpec& spec);

// LockButton widget

// Render a small lock/unlock button at the bottom left of the current window (legacy placement).
// windowId: unique identifier for this window (e.g., "FriendList", "Mail", "Options")
// lockState: current lock state (true = locked, false = unlocked)
// commandHandler: command handler to emit toggle command
void renderLockButton(const std::string& windowId, bool lockState, IWindowCommandHandler* commandHandler);

// Render a small lock/unlock button inline at the current cursor position.
// Intended for use by shared layout helpers like the window footer.
void renderLockButtonInline(const std::string& windowId,
                            bool lockState,
                            IWindowCommandHandler* commandHandler,
                            float widthPx = 30.0f,
                            float heightPx = 20.0f);

} // namespace UI
} // namespace XIFriendList

#endif // UI_CONTROLS_H

