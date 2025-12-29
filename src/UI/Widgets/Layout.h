// Consolidated layout widgets: SectionHeader, ToolbarRow, TabBar, WindowFooter

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "WidgetSpecs.h"
#include "UI/Commands/WindowCommands.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace UI {

// SectionHeader widget

// Returns true if header was clicked (for collapsible headers)
bool createSectionHeader(const SectionHeaderSpec& spec);

// CollapsibleSection helper

// Render a collapsible section with automatic state management
// Uses ImGui's internal state management (no need to track collapsed state manually)
// Parameters:
//   - label: The header text to display
//   - id: Unique identifier for the section (used for stable widget IDs)
//   - renderContent: Callback function that renders the content when section is open
// Returns: true if the section is currently open/expanded
bool createCollapsibleSection(const std::string& label, const std::string& id, std::function<void()> renderContent);

// ToolbarRow widget

// Toolbar button specification
struct ToolbarButtonSpec {
    std::string label;
    std::string id;
    bool enabled = true;
    bool visible = true;
    std::function<void()> onClick;
    
    ToolbarButtonSpec() = default;
    ToolbarButtonSpec(const std::string& label, const std::string& id)
        : label(label), id(id) {}
};

// Toolbar row specification
struct ToolbarRowSpec {
    std::string id;
    std::vector<ToolbarButtonSpec> buttons;
    float spacing = 10.0f;  // Spacing between buttons
    bool visible = true;
    
    ToolbarRowSpec() = default;
    ToolbarRowSpec(const std::string& id, const std::vector<ToolbarButtonSpec>& buttons)
        : id(id), buttons(buttons) {}
};

void createToolbarRow(const ToolbarRowSpec& spec);


struct TabSpec {
    std::string label;
    std::string id;
    bool enabled = true;
    bool visible = true;
    std::function<void()> onClick;  // Callback when tab is clicked
    
    TabSpec() = default;
    TabSpec(const std::string& label, const std::string& id)
        : label(label), id(id) {}
};

struct TabBarSpec {
    std::string id;
    std::vector<TabSpec> tabs;
    int* activeTabIndex;  // Pointer to active tab index (must remain valid)
    bool visible = true;
    
    TabBarSpec() : activeTabIndex(nullptr) {}
    TabBarSpec(const std::string& id, std::vector<TabSpec> tabs, int* activeTabIndex)
        : id(id), tabs(tabs), activeTabIndex(activeTabIndex) {}
};

// Returns true if active tab changed
bool createTabBar(const TabBarSpec& spec);

// Window Body Child Flags

// Flags used for the "body" child region in windows.
// We disable the child background so the body region uses the same background as the parent
// window (prevents the lighter gray panel effect).
//
// Note: We use the numeric value to keep UI layer platform-agnostic:
// ImGuiWindowFlags_NoBackground = 1 << 7.
constexpr int WINDOW_BODY_CHILD_FLAGS = (1 << 7);

} // namespace UI
} // namespace XIFriendList

#endif // UI_LAYOUT_H

