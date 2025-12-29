
#ifndef APP_NOTIFICATIONS_TOAST_H
#define APP_NOTIFICATIONS_TOAST_H

#include <string>
#include <cstdint>

namespace XIFriendList {
namespace App {
namespace Notifications {

enum class ToastType {
    FriendOnline,
    FriendOffline,
    FriendRequestReceived,
    FriendRequestAccepted,
    FriendRequestRejected,
    MailReceived,
    Error,
    Info,
    Warning,
    Success
};

enum class ToastState {
    ENTERING,
    VISIBLE,
    EXITING,
    COMPLETE
};

struct Toast {
    ToastType type;
    std::string title;
    std::string message;
    int64_t createdAt;
    int64_t duration;
    ToastState state;
    float alpha;
    float offsetX;
    bool dismissed;
    
    Toast()
        : type(ToastType::Info)
        , createdAt(0)
        , duration(5000)
        , state(ToastState::ENTERING)
        , alpha(0.0f)
        , offsetX(0.0f)
        , dismissed(false)
    {}
};

} // namespace Notifications
} // namespace App
} // namespace XIFriendList

#endif // APP_NOTIFICATIONS_TOAST_H

