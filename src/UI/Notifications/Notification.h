// Notification/toast data structures

#ifndef UI_NOTIFICATION_H
#define UI_NOTIFICATION_H

#include <string>
#include <cstdint>

namespace XIFriendList {
namespace UI {

// Notification type enum
enum class NotificationType {
    Success,
    Info,
    Warning,
    Error
};

// Notification data structure
struct Notification {
    NotificationType type;
    std::string message;
    std::string title;  // Optional title
    void* iconHandle;   // Optional icon handle (ImTextureID) for rendering icons
    uint64_t createdAt;  // Timestamp in milliseconds
    float remainingTime;  // Time remaining before auto-dismiss (seconds)
    bool isVisible;  // Whether notification is still visible
    bool isDismissed;  // Whether user manually dismissed
    
    Notification()
        : type(NotificationType::Info)
        , iconHandle(nullptr)
        , createdAt(0)
        , remainingTime(8.0f)  // Default 8 seconds
        , isVisible(true)
        , isDismissed(false)
    {}
    
    Notification(NotificationType t, const std::string& msg, const std::string& title = "", void* icon = nullptr)
        : type(t)
        , message(msg)
        , title(title)
        , iconHandle(icon)
        , createdAt(0)
        , remainingTime(8.0f)
        , isVisible(true)
        , isDismissed(false)
    {}
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_NOTIFICATION_H

