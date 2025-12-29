#ifndef APP_NOTIFICATION_USE_CASES_H
#define APP_NOTIFICATION_USE_CASES_H

#include "App/Notifications/Toast.h"
#include <string>
#include <cstdint>

namespace XIFriendList {
namespace App {
namespace UseCases {

class NotificationUseCase {
public:
    NotificationUseCase();
    ~NotificationUseCase() = default;
    
    XIFriendList::App::Notifications::Toast createFriendOnlineToast(const std::string& friendName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createFriendOfflineToast(const std::string& friendName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createFriendRequestReceivedToast(const std::string& requesterName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createFriendRequestAcceptedToast(const std::string& friendName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createFriendRequestRejectedToast(const std::string& requesterName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createMailReceivedToast(const std::string& senderName, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createErrorToast(const std::string& message, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createInfoToast(const std::string& title, const std::string& message, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createWarningToast(const std::string& title, const std::string& message, int64_t currentTime);
    XIFriendList::App::Notifications::Toast createSuccessToast(const std::string& title, const std::string& message, int64_t currentTime);
    
    int64_t getDefaultDuration(XIFriendList::App::Notifications::ToastType type) const;
    
private:
    static constexpr int64_t DEFAULT_DURATION_FRIEND_ONLINE = 5000;
    static constexpr int64_t DEFAULT_DURATION_FRIEND_OFFLINE = 5000;
    static constexpr int64_t DEFAULT_DURATION_FRIEND_REQUEST = 8000;
    static constexpr int64_t DEFAULT_DURATION_MAIL = 6000;
    static constexpr int64_t DEFAULT_DURATION_ERROR = 10000;
    static constexpr int64_t DEFAULT_DURATION_INFO = 5000;
    static constexpr int64_t DEFAULT_DURATION_WARNING = 7000;
    static constexpr int64_t DEFAULT_DURATION_SUCCESS = 5000;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_NOTIFICATION_USE_CASES_H

