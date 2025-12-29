
#ifndef APP_NOTIFICATION_CONSTANTS_H
#define APP_NOTIFICATION_CONSTANTS_H

#include <string_view>

namespace XIFriendList {
namespace App {
namespace Notifications {
namespace Constants {

inline constexpr std::string_view TITLE_ERROR = "Error";
inline constexpr std::string_view TITLE_FRIEND_REQUEST = "Friend Request";
inline constexpr std::string_view TITLE_FRIEND_REMOVED = "Friend Removed";
inline constexpr std::string_view TITLE_FRIEND_VISIBILITY = "Friend Visibility";
inline constexpr std::string_view TITLE_MAIL_SENT = "Mail Sent";
inline constexpr std::string_view TITLE_MAIL_CACHE = "Mail Cache";
inline constexpr std::string_view TITLE_FEEDBACK_SUBMITTED = "Feedback Submitted";
inline constexpr std::string_view TITLE_ISSUE_REPORTED = "Issue Reported";
inline constexpr std::string_view TITLE_NEW_MAIL = "New Mail";
inline constexpr std::string_view TITLE_KEY_CAPTURE = "Key Capture";

inline constexpr std::string_view MESSAGE_FRIEND_REQUEST_REJECTED = "Friend request rejected";
inline constexpr std::string_view MESSAGE_FRIEND_REQUEST_CANCELED = "Friend request canceled";

inline constexpr std::string_view MESSAGE_FRIEND_REMOVED = "Friend {0} removed";
inline constexpr std::string_view MESSAGE_FRIEND_REMOVED_FROM_VIEW = "Friend {0} removed from this character's view";

inline constexpr std::string_view MESSAGE_VISIBILITY_GRANTED = "Visibility granted for {0}";
inline constexpr std::string_view MESSAGE_VISIBILITY_REQUEST_SENT = "Visibility request sent to {0}";
inline constexpr std::string_view MESSAGE_VISIBILITY_UPDATED = "Visibility updated for {0}";
inline constexpr std::string_view MESSAGE_VISIBILITY_REMOVED = "Visibility removed for {0}";
inline constexpr std::string_view MESSAGE_VISIBILITY_GRANTED_WITH_PERIOD = "Visibility granted for {0}.";

inline constexpr std::string_view MESSAGE_MAIL_SENT = "Mail sent to {0}";
inline constexpr std::string_view MESSAGE_MAIL_CACHE_CLEARED = "Mail cache cleared";
inline constexpr std::string_view MESSAGE_NEW_MAIL_FROM = "New mail from {0}";
inline constexpr std::string_view MESSAGE_NEW_MAIL_FROM_WITH_SUBJECT = "New mail from {0}: {1}";

inline constexpr std::string_view MESSAGE_FAILED_TO_SEND_REQUEST = "Failed to send request: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_SEND_FRIEND_REQUEST = "Failed to send friend request to {0}: {1}";
inline constexpr std::string_view MESSAGE_FAILED_TO_ACCEPT_REQUEST = "Failed to accept request: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_REJECT_REQUEST = "Failed to reject request: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_CANCEL_REQUEST = "Failed to cancel request: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_REMOVE_FRIEND = "Failed to remove friend: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_REMOVE_FRIEND_VISIBILITY = "Failed to remove friend visibility: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_UPDATE_VISIBILITY = "Failed to update visibility: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_SEND_MAIL = "Failed to send mail: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_SUBMIT_FEEDBACK = "Failed to submit feedback: {0}";
inline constexpr std::string_view MESSAGE_FAILED_TO_REPORT_ISSUE = "Failed to report issue: {0}";
inline constexpr std::string_view MESSAGE_CHARACTER_NAME_REQUIRED_FEEDBACK = "Character name required to submit feedback";
inline constexpr std::string_view MESSAGE_CHARACTER_NAME_REQUIRED_ISSUE = "Character name required to report issue";
inline constexpr std::string_view MESSAGE_INVALID_FEEDBACK_DATA = "Invalid feedback data format";
inline constexpr std::string_view MESSAGE_INVALID_ISSUE_DATA = "Invalid issue data format";
inline constexpr std::string_view MESSAGE_FAILED_TO_PARSE_PREFERENCE = "Failed to parse preference update";

inline constexpr std::string_view MESSAGE_FEEDBACK_SUBMITTED = "Feedback submitted! ID: {0}";
inline constexpr std::string_view MESSAGE_ISSUE_REPORTED = "Issue reported! ID: {0}";

inline constexpr std::string_view MESSAGE_PRESS_ANY_KEY = "Press any key to set custom close key...";

// Default notification position (in pixels from top-left corner)
inline constexpr float DEFAULT_NOTIFICATION_POSITION_X = 10.0f;
inline constexpr float DEFAULT_NOTIFICATION_POSITION_Y = 15.0f;

} // namespace Constants
} // namespace Notifications
} // namespace App
} // namespace XIFriendList

#endif // APP_NOTIFICATION_CONSTANTS_H

