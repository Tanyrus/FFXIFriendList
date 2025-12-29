// Centralized UI text constants (buttons, labels, tooltips, help notes, section headers)
// Single source of truth for all user-facing text

#ifndef UI_UI_CONSTANTS_H
#define UI_UI_CONSTANTS_H

#include <string_view>

namespace XIFriendList {
namespace UI {
namespace Constants {

// Lock button positioning constants
inline constexpr float LOCK_BUTTON_RESERVE_HEIGHT = 25.0f;
inline constexpr float LOCK_BUTTON_ICON_SIZE = 18.0f;
inline constexpr float LOCK_BUTTON_BOTTOM_SPACING = 6.0f;

// Connection & Sync
inline constexpr std::string_view BUTTON_CONNECT_TO_SERVER = "Connect to Server";
inline constexpr std::string_view BUTTON_RECONNECT_TO_SERVER = "Reconnect to Server";
inline constexpr std::string_view BUTTON_REFRESH_FRIEND_LIST = "Refresh Friend List";
inline constexpr std::string_view BUTTON_REFRESH = "Refresh";
inline constexpr std::string_view TOOLTIP_CONNECT = "Connect to the friend list server";
inline constexpr std::string_view TOOLTIP_REFRESH = "Manually refresh your friend list from the server";

inline constexpr std::string_view BUTTON_SEND_FRIEND_REQUEST = "Send Friend Request";
inline constexpr std::string_view BUTTON_ADD_FRIEND = "Add Friend";
inline constexpr std::string_view BUTTON_ACCEPT_REQUEST = "Accept";
inline constexpr std::string_view BUTTON_REJECT_REQUEST = "Reject";
inline constexpr std::string_view BUTTON_CANCEL_REQUEST = "Cancel";
inline constexpr std::string_view TOOLTIP_ADD_FRIEND = "Send a friend request to this player";

inline constexpr std::string_view BUTTON_REMOVE_FRIEND = "Remove Friend";
inline constexpr std::string_view BUTTON_HIDE_FROM_FRIEND = "Hide from Friend";
inline constexpr std::string_view MENU_ITEM_EDIT_NOTE = "Edit Note";
inline constexpr std::string_view MENU_ITEM_REMOVE_FRIEND = "Remove Friend";
inline constexpr std::string_view TOOLTIP_REMOVE_FRIEND = "Remove this friend from your friend list";

inline constexpr std::string_view LABEL_SHARE_ONLINE_STATUS = "Share Online Status";
inline constexpr std::string_view LABEL_SHOW_ONLINE_STATUS = "Show Online Status";
inline constexpr std::string_view LABEL_SHARE_CHARACTER_DATA = "Share Character Data";
inline constexpr std::string_view LABEL_SHARE_JOB_NATION_RANK_ANONYMOUS = "Share Job, Nation, and Rank when Anonymous";
inline constexpr std::string_view LABEL_SHARE_LOCATION = "Share Location";
inline constexpr std::string_view LABEL_SHARE_VISIBILITY_OF_ALTS = "Share Visibility of Alt's to all Friends";
inline constexpr std::string_view TOOLTIP_SHARE_ONLINE_STATUS = "Allow friends to see when you are online";
inline constexpr std::string_view TOOLTIP_SHARE_CHARACTER_DATA = "Share your job, rank, and nation with friends";

inline constexpr std::string_view BUTTON_COMPOSE_MAIL = "Compose";
inline constexpr std::string_view BUTTON_SEND_MAIL = "Send";
inline constexpr std::string_view BUTTON_REPLY_MAIL = "Reply";
inline constexpr std::string_view BUTTON_MARK_AS_READ = "Mark as Read";
inline constexpr std::string_view BUTTON_DELETE_MAIL = "Delete";
inline constexpr std::string_view BUTTON_CLEAR_CACHE = "Clear Cache";
inline constexpr std::string_view LABEL_INBOX = "Inbox";
inline constexpr std::string_view LABEL_SENT = "Sent";
inline constexpr std::string_view TOOLTIP_COMPOSE_MAIL = "Compose and send a new mail message";

// Notes
inline constexpr std::string_view BUTTON_SAVE_NOTE = "Save Note";
inline constexpr std::string_view BUTTON_DELETE_NOTE = "Delete Note";
inline constexpr std::string_view BUTTON_UPLOAD_NOTES = "Upload";
inline constexpr std::string_view BUTTON_DOWNLOAD_NOTES = "Download";
inline constexpr std::string_view LABEL_USE_SERVER_NOTES = "Use Server Notes";
inline constexpr std::string_view LABEL_OVERWRITE_NOTES_ON_UPLOAD = "Overwrite Notes On Upload";
inline constexpr std::string_view LABEL_OVERWRITE_NOTES_ON_DOWNLOAD = "Overwrite Notes On Download";
inline constexpr std::string_view TOOLTIP_SAVE_NOTE = "Save your note about this friend";

inline constexpr std::string_view BUTTON_SAVE_CUSTOM_THEME = "Save Theme";
inline constexpr std::string_view BUTTON_DELETE_CUSTOM_THEME = "Delete Current Theme";
inline constexpr std::string_view BUTTON_APPLY_THEME = "Apply Theme";
inline constexpr std::string_view LABEL_THEME = "Theme:";
inline constexpr std::string_view LABEL_PRESET = "Preset:";
inline constexpr std::string_view LABEL_FONT_FAMILY = "Font Family:";
inline constexpr std::string_view LABEL_FONT_WEIGHT = "Font Weight:";
inline constexpr std::string_view LABEL_FONT_OUTLINE_WIDTH = "Font Outline Width:";
inline constexpr std::string_view LABEL_BACKGROUND_TRANSPARENCY = "Background Transparency:";
inline constexpr std::string_view LABEL_TEXT_TRANSPARENCY = "Text Transparency:";
inline constexpr std::string_view LABEL_SAVE_CURRENT_COLORS_AS_THEME = "Save Current Colors as Theme:";
inline constexpr std::string_view LABEL_THEME_NAME = "Theme Name:";
inline constexpr std::string_view TOOLTIP_SAVE_THEME = "Save your current color scheme as a custom theme";
inline constexpr std::string_view TOOLTIP_SELECT_THEME = "Select a theme (built-in themes or custom themes)";

// Options & Settings
inline constexpr std::string_view BUTTON_SAVE_SETTINGS = "Save Settings";
inline constexpr std::string_view BUTTON_RESET_TO_DEFAULTS = "Reset to Defaults";
inline constexpr std::string_view BUTTON_OPTIONS = "Options";
inline constexpr std::string_view BUTTON_SUPPORT = "Support";
inline constexpr std::string_view BUTTON_DEBUG = "Debug";
inline constexpr std::string_view BUTTON_CLOSE = "Close";
inline constexpr std::string_view LABEL_SEARCH = "Search";
inline constexpr std::string_view LABEL_ENABLE_NOTIFICATION_SOUNDS = "Enable Notification Sounds";
inline constexpr std::string_view LABEL_PLAY_SOUND_ON_FRIEND_ONLINE = "Play sound on Friend Online";
inline constexpr std::string_view LABEL_PLAY_SOUND_ON_FRIEND_REQUEST = "Play sound on Friend Request";
inline constexpr std::string_view LABEL_NOTIFICATION_SOUND_VOLUME = "Notification Sound Volume";
inline constexpr std::string_view LABEL_NOTIFICATION_DURATION_SECONDS = "Notification Duration (seconds)";
inline constexpr std::string_view TOOLTIP_FILTER_FRIENDS = "Filter friends by name, job, or zone";

// Window Controls
inline constexpr std::string_view BUTTON_LOCK_WINDOW = "Lock Window";
inline constexpr std::string_view BUTTON_UNLOCK_WINDOW = "Unlock Window";
inline constexpr std::string_view BUTTON_CLOSE_WINDOW = "Close Window";
inline constexpr std::string_view TOOLTIP_LOCK_WINDOW = "Lock window position and size";

// Section Headers
inline constexpr std::string_view HEADER_FRIENDS_LIST = "Friends List";
inline constexpr std::string_view HEADER_PENDING_REQUESTS = "Pending Friend Requests";
inline constexpr std::string_view HEADER_CONNECTION = "Connection";
inline constexpr std::string_view HEADER_ADD_FRIEND = "Add New Friend";
inline constexpr std::string_view HEADER_NOTES_SETTINGS = "Notes Settings";
inline constexpr std::string_view HEADER_THEME_SETTINGS = "Theme Settings";
inline constexpr std::string_view HEADER_PRIVACY_VISIBILITY_CONTROLS = "Privacy / Visibility Controls";
inline constexpr std::string_view HEADER_ALT_ONLINE_VISIBILITY = "Alt Online Visibility";
inline constexpr std::string_view HEADER_DEBUG_ADVANCED = "Debug / Advanced";
inline constexpr std::string_view HEADER_CONTROLS = "Controls";
inline constexpr std::string_view HEADER_NOTIFICATIONS = "Notifications";
inline constexpr std::string_view HEADER_THEME_PRESET = "Theme Preset";
inline constexpr std::string_view HEADER_THEME_SELECTION = "Theme Selection";
inline constexpr std::string_view HEADER_CUSTOM_COLORS = "Custom Themes";
inline constexpr std::string_view HEADER_THEME_MANAGEMENT = "Theme Management";

inline constexpr std::string_view TAB_GENERAL = "General";
inline constexpr std::string_view TAB_PRIVACY = "General";
inline constexpr std::string_view TAB_NOTIFICATIONS = "Notifications";
inline constexpr std::string_view TAB_COLORS = "Colors";
inline constexpr std::string_view TAB_FRIENDS = "Friends";
inline constexpr std::string_view TAB_CONTROLS = "Controls";
inline constexpr std::string_view TAB_THEMES = "Themes";
inline constexpr std::string_view TAB_ALTS = "Alts";
inline constexpr std::string_view TAB_ABOUT = "About";

// Messages & Help Text
inline constexpr std::string_view MESSAGE_NO_PENDING_REQUESTS = "No pending friend requests";
inline constexpr std::string_view MESSAGE_INCOMING_REQUESTS = "Incoming";
inline constexpr std::string_view MESSAGE_SENT_REQUESTS = "Sent";
inline constexpr std::string_view MESSAGE_SUBMITTING = "Submitting...";
inline constexpr std::string_view MESSAGE_REPORT_ISSUE = "Report Issue";
inline constexpr std::string_view MESSAGE_SEND_FEEDBACK = "Send Feedback";
inline constexpr std::string_view HELP_FRIEND_REQUESTS = "Pending friend requests will appear here";
inline constexpr std::string_view HELP_CONNECTION_STATUS = "Connection status and server information";

// Controller Buttons
inline constexpr std::string_view CONTROLLER_BUTTON_B_DEFAULT = "B (Default)";
inline constexpr std::string_view CONTROLLER_BUTTON_A = "A";
inline constexpr std::string_view CONTROLLER_BUTTON_X = "X";
inline constexpr std::string_view CONTROLLER_BUTTON_Y = "Y";
inline constexpr std::string_view CONTROLLER_BUTTON_BACK = "Back";
inline constexpr std::string_view CONTROLLER_BUTTON_DISABLED = "Disabled";
inline constexpr std::string_view LABEL_CONTROLLER_BUTTON = "Controller Button:";
inline constexpr std::string_view LABEL_CLOSE_KEY = "Close Key:";
inline constexpr std::string_view MESSAGE_CLOSE_KEY_DEFAULT = "ESC (Default)";
inline constexpr std::string_view MESSAGE_CONFIGURE_CLOSE_KEY = "(Configure in ffxifriendlist.ini: CloseKey=<VK_CODE>)";

// Input Labels
inline constexpr std::string_view LABEL_TO = "To";
inline constexpr std::string_view LABEL_SUBJECT = "Subject";
inline constexpr std::string_view LABEL_MESSAGE = "Message";
inline constexpr std::string_view LABEL_NOTE = "Note";

// Additional Buttons
inline constexpr std::string_view BUTTON_CLEAR = "Clear";

} // namespace Constants
} // namespace UI
} // namespace XIFriendList

#endif // UI_UI_CONSTANTS_H

