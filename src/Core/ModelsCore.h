
#ifndef CORE_MODELS_CORE_H
#define CORE_MODELS_CORE_H

#include "App/NotificationConstants.h"
#include <string>
#include <cstdint>

namespace XIFriendList {
namespace Core {

struct Presence {
    std::string characterName;
    std::string job;
    std::string rank;
    int nation;
    std::string zone;
    bool isAnonymous;
    uint64_t timestamp;
    
    Presence()
        : nation(0)
        , isAnonymous(false)
        , timestamp(0)
    {}
    
    bool hasChanged(const Presence& other) const;
    bool isValid() const;
};

struct FriendViewSettings {
    bool showJob;           // Show Job column (default: true)
    bool showZone;          // Show Zone column (default: false)
    bool showNationRank;    // Show Nation/Rank combined column (default: false)
    bool showLastSeen;      // Show Last Seen column (default: false)
    
    FriendViewSettings()
        : showJob(true)
        , showZone(false)
        , showNationRank(false)
        , showLastSeen(false)
    {}
    
    bool operator==(const FriendViewSettings& other) const {
        return showJob == other.showJob &&
               showZone == other.showZone &&
               showNationRank == other.showNationRank &&
               showLastSeen == other.showLastSeen;
    }
    
    bool operator!=(const FriendViewSettings& other) const {
        return !(*this == other);
    }
};

struct Preferences {
    bool useServerNotes;
    bool shareFriendsAcrossAlts;

    FriendViewSettings mainFriendView;        // Main window friend view settings
    FriendViewSettings quickOnlineFriendView; // Quick Online window friend view settings
    
    bool debugMode;                   // Enable debug logging (local-only)
    
    bool mailCacheEnabled;            // Enable local mail caching (default: true)
    int maxCachedMessagesPerMailbox;  // Maximum messages to cache per mailbox (default: 5000)
    
    bool overwriteNotesOnUpload;      // Local-only: Overwrite existing notes when uploading
    bool overwriteNotesOnDownload;    // Local-only: Overwrite existing notes when downloading
    bool shareJobWhenAnonymous;       // Local-only: Share job/nation/rank with friends when anonymous
    bool showOnlineStatus;            // Local-only: Show online status to friends (false = invisible)
    bool shareLocation;               // Local-only: Share zone/location with friends
    float notificationDuration;       // Local-only: Notification display duration in seconds (default: 8.0)
    float notificationPositionX;      // Local-only: Toast notification X position in pixels (default: calculated from TopRight)
    float notificationPositionY;      // Local-only: Toast notification Y position in pixels (default: calculated from TopRight)
    int customCloseKeyCode;           // Local-only: Custom virtual key code for closing windows (0 = use default ESC, default: 0 = ESC)
    int controllerCloseButton;        // Local-only: XInput button code for closing windows (0x2000 = B button default, 0 = disabled)
    bool windowsLocked;               // Local-only: Lock windows from being closed via ESC/controller (default: false)
    
    bool notificationSoundsEnabled;   // Local-only: Master toggle for notification sounds (default: true)
    bool soundOnFriendOnline;        // Local-only: Play sound when friend comes online (default: true)
    bool soundOnFriendRequest;        // Local-only: Play sound when friend request received (default: true)
    float notificationSoundVolume;    // Local-only: Notification sound volume 0.0-1.0 (default: 0.6)
    
    
    Preferences()
        : useServerNotes(false)
        , shareFriendsAcrossAlts(true)
        , mainFriendView()  // Default: showJob=true, others=false
        , quickOnlineFriendView()  // Default: showJob=true, others=false
        , debugMode(false)
        , mailCacheEnabled(true)
        , maxCachedMessagesPerMailbox(5000)
        , overwriteNotesOnUpload(false)
        , overwriteNotesOnDownload(false)
        , shareJobWhenAnonymous(false)
        , showOnlineStatus(true)
        , shareLocation(true)
        , notificationDuration(8.0f)
        , notificationPositionX(App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X)
        , notificationPositionY(App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y)
        , customCloseKeyCode(0)  // 0 = default to ESC (VK_ESCAPE = 27)
        , controllerCloseButton(0x2000)  // 0x2000 = XINPUT_GAMEPAD_B (default)
        , windowsLocked(false)  // Windows can be closed by default
        , notificationSoundsEnabled(true)  // Sounds enabled by default
        , soundOnFriendOnline(true)  // Play sound on friend online by default
        , soundOnFriendRequest(true)  // Play sound on friend request by default
        , notificationSoundVolume(0.6f)  // Default volume 60%
    {}
    
    bool operator==(const Preferences& other) const {
        return useServerNotes == other.useServerNotes &&
               shareFriendsAcrossAlts == other.shareFriendsAcrossAlts &&
               mainFriendView == other.mainFriendView &&
               quickOnlineFriendView == other.quickOnlineFriendView &&
               debugMode == other.debugMode &&
               mailCacheEnabled == other.mailCacheEnabled &&
               maxCachedMessagesPerMailbox == other.maxCachedMessagesPerMailbox &&
               overwriteNotesOnUpload == other.overwriteNotesOnUpload &&
               overwriteNotesOnDownload == other.overwriteNotesOnDownload &&
               shareJobWhenAnonymous == other.shareJobWhenAnonymous &&
               showOnlineStatus == other.showOnlineStatus &&
               shareLocation == other.shareLocation &&
               notificationDuration == other.notificationDuration &&
               notificationPositionX == other.notificationPositionX &&
               notificationPositionY == other.notificationPositionY &&
               customCloseKeyCode == other.customCloseKeyCode &&
               controllerCloseButton == other.controllerCloseButton &&
               windowsLocked == other.windowsLocked &&
               notificationSoundsEnabled == other.notificationSoundsEnabled &&
               soundOnFriendOnline == other.soundOnFriendOnline &&
               soundOnFriendRequest == other.soundOnFriendRequest &&
               notificationSoundVolume == other.notificationSoundVolume;
    }
    
    bool operator!=(const Preferences& other) const {
        return !(*this == other);
    }
};


struct Color {
    float r;
    float g;
    float b;
    float a;
    
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

enum class BuiltInTheme {
    Default = -2,     // No theme - use ImGui default styling (no overrides)
    FFXIClassic = 0,  // Warm browns, golds, and parchment tones
    ModernDark = 1,   // Dark blue/cyan theme
    GreenNature = 2,  // Forest/green tones
    PurpleMystic = 3  // Purple/violet tones
};

struct CustomTheme {
    std::string name;
    
    Color windowBgColor;
    Color childBgColor;
    Color frameBgColor;
    Color frameBgHovered;
    Color frameBgActive;
    
    Color titleBg;
    Color titleBgActive;
    Color titleBgCollapsed;
    
    Color buttonColor;
    Color buttonHoverColor;
    Color buttonActiveColor;
    
    Color separatorColor;
    Color separatorHovered;
    Color separatorActive;
    
    Color scrollbarBg;
    Color scrollbarGrab;
    Color scrollbarGrabHovered;
    Color scrollbarGrabActive;
    
    Color checkMark;
    Color sliderGrab;
    Color sliderGrabActive;
    
    Color header;
    Color headerHovered;
    Color headerActive;
    
    Color textColor;
    Color textDisabled;
    
    Color tableBgColor;
    
    CustomTheme() {
        windowBgColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        childBgColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        frameBgColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        frameBgHovered = Color(0.0f, 0.0f, 0.0f, 1.0f);
        frameBgActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        titleBg = Color(0.0f, 0.0f, 0.0f, 1.0f);
        titleBgActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        titleBgCollapsed = Color(0.0f, 0.0f, 0.0f, 1.0f);
        buttonColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        buttonHoverColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        buttonActiveColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        separatorColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        separatorHovered = Color(0.0f, 0.0f, 0.0f, 1.0f);
        separatorActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        scrollbarBg = Color(0.0f, 0.0f, 0.0f, 1.0f);
        scrollbarGrab = Color(0.0f, 0.0f, 0.0f, 1.0f);
        scrollbarGrabHovered = Color(0.0f, 0.0f, 0.0f, 1.0f);
        scrollbarGrabActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        checkMark = Color(0.0f, 0.0f, 0.0f, 1.0f);
        sliderGrab = Color(0.0f, 0.0f, 0.0f, 1.0f);
        sliderGrabActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        header = Color(0.0f, 0.0f, 0.0f, 1.0f);
        headerHovered = Color(0.0f, 0.0f, 0.0f, 1.0f);
        headerActive = Color(0.0f, 0.0f, 0.0f, 1.0f);
        textColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        textDisabled = Color(0.0f, 0.0f, 0.0f, 1.0f);
        tableBgColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    }
};

inline std::string getBuiltInThemeName(BuiltInTheme theme) {
    switch (theme) {
        case BuiltInTheme::Default: return "Default (No Theme)";
        case BuiltInTheme::FFXIClassic: return "Warm Brown";
        case BuiltInTheme::ModernDark: return "Modern Dark";
        case BuiltInTheme::GreenNature: return "Green Nature";
        case BuiltInTheme::PurpleMystic: return "Purple Mystic";
        default: return "Unknown";
    }
}


enum class MailFolder {
    Inbox,
    Sent
};

struct MailMessage {
    std::string messageId;
    std::string fromUserId;      // Sender's character name (normalized)
    std::string toUserId;        // Recipient's character name (normalized)
    std::string subject;         // Message subject (1-100 chars)
    std::string body;            // Message body (1-2000 chars)
    uint64_t createdAt;         // Timestamp when message was created
    uint64_t readAt;             // Timestamp when message was read (0 if unread)
    bool isRead;                 // Whether message has been read
    
    MailMessage()
        : createdAt(0)
        , readAt(0)
        , isRead(false)
    {}
    
    bool isUnread() const {
        return !isRead;
    }
    
    bool operator==(const MailMessage& other) const {
        return messageId == other.messageId;
    }
    
    bool operator!=(const MailMessage& other) const {
        return !(*this == other);
    }
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_MODELS_CORE_H

