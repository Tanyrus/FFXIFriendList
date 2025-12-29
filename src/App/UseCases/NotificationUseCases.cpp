#include "App/UseCases/NotificationUseCases.h"
#include "App/Notifications/Toast.h"
#include "App/NotificationConstants.h"

namespace XIFriendList {
namespace App {
namespace UseCases {

NotificationUseCase::NotificationUseCase() {
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createFriendOnlineToast(const std::string& friendName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::FriendOnline;
    toast.title = "Friend Online";
    toast.message = friendName + " is now online";
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::FriendOnline);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createFriendOfflineToast(const std::string& friendName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::FriendOffline;
    toast.title = "Friend Offline";
    toast.message = friendName + " is now offline";
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::FriendOffline);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createFriendRequestReceivedToast(const std::string& requesterName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::FriendRequestReceived;
    toast.title = "Friend Request";
    toast.message = requesterName + " wants to be your friend";
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::FriendRequestReceived);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createFriendRequestAcceptedToast(const std::string& friendName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::FriendRequestAccepted;
    toast.title = "Friend Request Accepted";
    toast.message = friendName + " is now your friend";
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::FriendRequestAccepted);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createFriendRequestRejectedToast(const std::string& requesterName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::FriendRequestRejected;
    toast.title = "Friend Request Rejected";
    toast.message = requesterName + " rejected your friend request";
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::FriendRequestRejected);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createMailReceivedToast(const std::string& senderName, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::MailReceived;
    toast.title = "New Mail";
    toast.message = "You have mail from " + senderName;
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::MailReceived);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createErrorToast(const std::string& message, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::Error;
    toast.title = std::string(XIFriendList::App::Notifications::Constants::TITLE_ERROR);
    toast.message = message;
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::Error);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createInfoToast(const std::string& title, const std::string& message, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::Info;
    toast.title = title;
    toast.message = message;
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::Info);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createWarningToast(const std::string& title, const std::string& message, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::Warning;
    toast.title = title;
    toast.message = message;
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::Warning);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

XIFriendList::App::Notifications::Toast NotificationUseCase::createSuccessToast(const std::string& title, const std::string& message, int64_t currentTime) {
    XIFriendList::App::Notifications::Toast toast;
    toast.type = XIFriendList::App::Notifications::ToastType::Success;
    toast.title = title;
    toast.message = message;
    toast.createdAt = currentTime;
    toast.duration = getDefaultDuration(XIFriendList::App::Notifications::ToastType::Success);
    toast.state = XIFriendList::App::Notifications::ToastState::ENTERING;
    return toast;
}

int64_t NotificationUseCase::getDefaultDuration(XIFriendList::App::Notifications::ToastType type) const {
    switch (type) {
        case XIFriendList::App::Notifications::ToastType::FriendOnline:
            return DEFAULT_DURATION_FRIEND_ONLINE;
        case XIFriendList::App::Notifications::ToastType::FriendOffline:
            return DEFAULT_DURATION_FRIEND_OFFLINE;
        case XIFriendList::App::Notifications::ToastType::FriendRequestReceived:
        case XIFriendList::App::Notifications::ToastType::FriendRequestAccepted:
        case XIFriendList::App::Notifications::ToastType::FriendRequestRejected:
            return DEFAULT_DURATION_FRIEND_REQUEST;
        case XIFriendList::App::Notifications::ToastType::MailReceived:
            return DEFAULT_DURATION_MAIL;
        case XIFriendList::App::Notifications::ToastType::Error:
            return DEFAULT_DURATION_ERROR;
        case XIFriendList::App::Notifications::ToastType::Info:
            return DEFAULT_DURATION_INFO;
        case XIFriendList::App::Notifications::ToastType::Warning:
            return DEFAULT_DURATION_WARNING;
        case XIFriendList::App::Notifications::ToastType::Success:
            return DEFAULT_DURATION_SUCCESS;
        default:
            return DEFAULT_DURATION_INFO;
    }
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

