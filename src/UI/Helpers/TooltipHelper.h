// Helper functions for tooltips and help markers

#ifndef UI_HELPERS_TOOLTIP_HELPER_H
#define UI_HELPERS_TOOLTIP_HELPER_H

namespace XIFriendList {
namespace UI {

// Render a help marker: a small "(?)" icon that shows a tooltip on hover
// Usage: UI::HelpMarker("This setting controls...");
// Renders the (?) icon on the same line, shows tooltip on hover
void HelpMarker(const char* desc);

} // namespace UI
} // namespace XIFriendList

#endif // UI_HELPERS_TOOLTIP_HELPER_H

