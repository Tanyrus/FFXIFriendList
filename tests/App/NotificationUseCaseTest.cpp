#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/NotificationUseCases.h"
#include "App/Notifications/Toast.h"

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("NotificationUseCase - Create friend online toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createFriendOnlineToast("TestFriend", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::FriendOnline);
    REQUIRE(toast.title == "Friend Online");
    REQUIRE(toast.message == "TestFriend is now online");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 5000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create friend offline toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createFriendOfflineToast("TestFriend", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::FriendOffline);
    REQUIRE(toast.title == "Friend Offline");
    REQUIRE(toast.message == "TestFriend is now offline");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 5000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create friend request received toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createFriendRequestReceivedToast("Requester", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::FriendRequestReceived);
    REQUIRE(toast.title == "Friend Request");
    REQUIRE(toast.message == "Requester wants to be your friend");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 8000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create friend request accepted toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createFriendRequestAcceptedToast("NewFriend", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::FriendRequestAccepted);
    REQUIRE(toast.title == "Friend Request Accepted");
    REQUIRE(toast.message == "NewFriend is now your friend");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 8000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create friend request rejected toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createFriendRequestRejectedToast("Rejecter", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::FriendRequestRejected);
    REQUIRE(toast.title == "Friend Request Rejected");
    REQUIRE(toast.message == "Rejecter rejected your friend request");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 8000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create mail received toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createMailReceivedToast("Sender", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::MailReceived);
    REQUIRE(toast.title == "New Mail");
    REQUIRE(toast.message == "You have mail from Sender");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 6000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create error toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createErrorToast("Something went wrong", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::Error);
    REQUIRE(toast.message == "Something went wrong");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 10000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create info toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createInfoToast("Info Title", "Info message", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::Info);
    REQUIRE(toast.title == "Info Title");
    REQUIRE(toast.message == "Info message");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 5000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create warning toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createWarningToast("Warning Title", "Warning message", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::Warning);
    REQUIRE(toast.title == "Warning Title");
    REQUIRE(toast.message == "Warning message");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 7000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Create success toast", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    int64_t currentTime = 1234567890;
    
    auto toast = useCase.createSuccessToast("Success Title", "Success message", currentTime);
    
    REQUIRE(toast.type == Notifications::ToastType::Success);
    REQUIRE(toast.title == "Success Title");
    REQUIRE(toast.message == "Success message");
    REQUIRE(toast.createdAt == currentTime);
    REQUIRE(toast.duration == 5000);
    REQUIRE(toast.state == Notifications::ToastState::ENTERING);
}

TEST_CASE("NotificationUseCase - Default durations", "[NotificationUseCase]") {
    NotificationUseCase useCase;
    
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::FriendOnline) == 5000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::FriendOffline) == 5000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::FriendRequestReceived) == 8000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::FriendRequestAccepted) == 8000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::FriendRequestRejected) == 8000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::MailReceived) == 6000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::Error) == 10000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::Info) == 5000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::Warning) == 7000);
    REQUIRE(useCase.getDefaultDuration(Notifications::ToastType::Success) == 5000);
}

} // namespace App
} // namespace XIFriendList

