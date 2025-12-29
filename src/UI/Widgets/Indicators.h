// Consolidated indicator widgets: Text, StatusBadge, ActionBanner, NotificationBanner

#ifndef UI_INDICATORS_H
#define UI_INDICATORS_H

#include <string>

namespace XIFriendList {
namespace UI {

// Text widget

// Text display specification
struct TextSpec {
    std::string text;      // Text to display (should be cached, not allocated per-frame)
    std::string id;        // Unique identifier
    bool visible = true;
    
    TextSpec() = default;
    TextSpec(const std::string& text, const std::string& id)
        : text(text), id(id) {}
};

void createText(const TextSpec& spec);


struct StatusBadgeSpec {
    std::string text;
    std::string id;
    bool visible = true;
    
    StatusBadgeSpec() = default;
    StatusBadgeSpec(const std::string& text, const std::string& id)
        : text(text), id(id) {}
};

// This should be called after the main content, then SameLine with spacing to push it right
void createStatusBadge(const StatusBadgeSpec& spec);

// ActionBanner widget

// Action banner specification
struct ActionBannerSpec {
    bool visible;
    bool success;  // true for success, false for error
    std::string message;
    std::string id;
    
    ActionBannerSpec()
        : visible(false)
        , success(false)
    {}
    
    ActionBannerSpec(bool visible, bool success, const std::string& message, const std::string& id)
        : visible(visible)
        , success(success)
        , message(message)
        , id(id)
    {}
};

// Renders as a colored banner line with the message
void createActionBanner(const ActionBannerSpec& spec);

// NotificationBanner widget


} // namespace UI
} // namespace XIFriendList

#endif // UI_INDICATORS_H

