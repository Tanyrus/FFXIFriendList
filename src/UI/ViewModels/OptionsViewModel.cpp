// ViewModel for Options window implementation

#include "UI/ViewModels/OptionsViewModel.h"
#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include "App/NotificationConstants.h"
#include <sstream>
#include <iomanip>

namespace XIFriendList {
namespace UI {

OptionsViewModel::OptionsViewModel()
    : useServerNotes_(false)
    , shareFriendsAcrossAlts_(true)
    , overwriteNotesOnUpload_(false)
    , overwriteNotesOnDownload_(false)
    , shareJobWhenAnonymous_(false)
    , showOnlineStatus_(true)
    , shareLocation_(true)
    , showFriendedAsColumn_(true)
    , showJobColumn_(true)
    , showRankColumn_(true)
    , showNationColumn_(true)
    , showZoneColumn_(true)
    , showLastSeenColumn_(true)
    , mainFriendView_()  // Default: showJob=true, others=false
    , quickOnlineFriendView_()  // Default: showJob=true, others=false
    , debugMode_(false)
    , notificationDuration_(8.0f)
    , notificationPositionX_(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X)
    , notificationPositionY_(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y)
    , customCloseKeyCode_(0)  // 0 = default to ESC
    , controllerCloseButton_(0x2000)  // 0x2000 = XINPUT_GAMEPAD_B (default)
    , windowsLocked_(false)  // Windows can be closed by default
    , notificationSoundsEnabled_(true)  // Sounds enabled by default
    , soundOnFriendOnline_(true)  // Play sound on friend online by default
    , soundOnFriendRequest_(true)  // Play sound on friend request by default
    , notificationSoundVolume_(0.6f)  // Default volume 60%
    , loaded_(false)
{
}

void OptionsViewModel::markDirty(const std::string& field) {
    if (field == "useServerNotes") dirtyFlags_.useServerNotes = true;
    else if (field == "shareFriendsAcrossAlts") dirtyFlags_.shareFriendsAcrossAlts = true;
    else if (field == "overwriteNotesOnUpload") dirtyFlags_.overwriteNotesOnUpload = true;
    else if (field == "overwriteNotesOnDownload") dirtyFlags_.overwriteNotesOnDownload = true;
    else if (field == "shareJobWhenAnonymous") dirtyFlags_.shareJobWhenAnonymous = true;
    else if (field == "showOnlineStatus") dirtyFlags_.showOnlineStatus = true;
    else if (field == "shareLocation") dirtyFlags_.shareLocation = true;
    else if (field == "showFriendedAsColumn") dirtyFlags_.showFriendedAsColumn = true;
    else if (field == "showJobColumn") dirtyFlags_.showJobColumn = true;
    else if (field == "showRankColumn") dirtyFlags_.showRankColumn = true;
    else if (field == "showNationColumn") dirtyFlags_.showNationColumn = true;
    else if (field == "showZoneColumn") dirtyFlags_.showZoneColumn = true;
    else if (field == "showLastSeenColumn") dirtyFlags_.showLastSeenColumn = true;
    else if (field == "mainFriendView") dirtyFlags_.mainFriendView = true;
    else if (field == "quickOnlineFriendView") dirtyFlags_.quickOnlineFriendView = true;
    else if (field == "debugMode") dirtyFlags_.debugMode = true;
    else if (field == "notificationDuration") dirtyFlags_.notificationDuration = true;
    else if (field == "notificationPositionX") dirtyFlags_.notificationPositionX = true;
    else if (field == "notificationPositionY") dirtyFlags_.notificationPositionY = true;
    else if (field == "customCloseKeyCode") dirtyFlags_.customCloseKeyCode = true;
    else if (field == "controllerCloseButton") dirtyFlags_.controllerCloseButton = true;
    else if (field == "windowsLocked") dirtyFlags_.windowsLocked = true;
    else if (field == "notificationSoundsEnabled") dirtyFlags_.notificationSoundsEnabled = true;
    else if (field == "soundOnFriendOnline") dirtyFlags_.soundOnFriendOnline = true;
    else if (field == "soundOnFriendRequest") dirtyFlags_.soundOnFriendRequest = true;
    else if (field == "notificationSoundVolume") dirtyFlags_.notificationSoundVolume = true;
}

bool OptionsViewModel::hasDirtyFields() const {
    return dirtyFlags_.useServerNotes || dirtyFlags_.shareFriendsAcrossAlts || dirtyFlags_.overwriteNotesOnUpload ||
           dirtyFlags_.overwriteNotesOnDownload || dirtyFlags_.shareJobWhenAnonymous ||
           dirtyFlags_.showOnlineStatus || dirtyFlags_.shareLocation ||
           dirtyFlags_.showFriendedAsColumn ||
           dirtyFlags_.showJobColumn || dirtyFlags_.showRankColumn ||
           dirtyFlags_.showNationColumn || dirtyFlags_.showZoneColumn ||
           dirtyFlags_.showLastSeenColumn || dirtyFlags_.mainFriendView || dirtyFlags_.quickOnlineFriendView || dirtyFlags_.debugMode ||
           dirtyFlags_.notificationDuration || dirtyFlags_.notificationPositionX || dirtyFlags_.notificationPositionY || dirtyFlags_.customCloseKeyCode ||
           dirtyFlags_.controllerCloseButton || dirtyFlags_.windowsLocked ||
           dirtyFlags_.notificationSoundsEnabled || dirtyFlags_.soundOnFriendOnline ||
           dirtyFlags_.soundOnFriendRequest || dirtyFlags_.notificationSoundVolume;
}

void OptionsViewModel::clearDirtyFlags() {
    dirtyFlags_ = DirtyFlags();
}

void OptionsViewModel::updateFromPreferences(const XIFriendList::Core::Preferences& prefs) {
    useServerNotes_ = prefs.useServerNotes;
    shareFriendsAcrossAlts_ = prefs.shareFriendsAcrossAlts;
    overwriteNotesOnUpload_ = prefs.overwriteNotesOnUpload;
    overwriteNotesOnDownload_ = prefs.overwriteNotesOnDownload;
    shareJobWhenAnonymous_ = prefs.shareJobWhenAnonymous;
    showOnlineStatus_ = prefs.showOnlineStatus;
    shareLocation_ = prefs.shareLocation;
    // Old fields removed - now using mainFriendView and quickOnlineFriendView
    mainFriendView_ = prefs.mainFriendView;
    quickOnlineFriendView_ = prefs.quickOnlineFriendView;
    debugMode_ = prefs.debugMode;
    notificationDuration_ = prefs.notificationDuration;
    // Convert -1 (old default) to default position
    notificationPositionX_ = (prefs.notificationPositionX < 0.0f) ? XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : prefs.notificationPositionX;
    notificationPositionY_ = (prefs.notificationPositionY < 0.0f) ? XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : prefs.notificationPositionY;
    customCloseKeyCode_ = prefs.customCloseKeyCode;
    controllerCloseButton_ = prefs.controllerCloseButton;
    windowsLocked_ = prefs.windowsLocked;
    notificationSoundsEnabled_ = prefs.notificationSoundsEnabled;
    soundOnFriendOnline_ = prefs.soundOnFriendOnline;
    soundOnFriendRequest_ = prefs.soundOnFriendRequest;
    notificationSoundVolume_ = prefs.notificationSoundVolume;
    
    clearDirtyFlags();
    loaded_ = true;
}

XIFriendList::Core::Preferences OptionsViewModel::toPreferences() const {
    XIFriendList::Core::Preferences prefs;
    prefs.useServerNotes = useServerNotes_;
    prefs.shareFriendsAcrossAlts = shareFriendsAcrossAlts_;
    prefs.overwriteNotesOnUpload = overwriteNotesOnUpload_;
    prefs.overwriteNotesOnDownload = overwriteNotesOnDownload_;
    prefs.shareJobWhenAnonymous = shareJobWhenAnonymous_;
    prefs.showOnlineStatus = showOnlineStatus_;
    prefs.shareLocation = shareLocation_;
    // Old fields removed - now using mainFriendView and quickOnlineFriendView
    prefs.mainFriendView = mainFriendView_;
    prefs.quickOnlineFriendView = quickOnlineFriendView_;
    prefs.debugMode = debugMode_;
    prefs.notificationDuration = notificationDuration_;
    prefs.customCloseKeyCode = customCloseKeyCode_;
    prefs.controllerCloseButton = controllerCloseButton_;
    prefs.windowsLocked = windowsLocked_;
    prefs.notificationSoundsEnabled = notificationSoundsEnabled_;
    prefs.soundOnFriendOnline = soundOnFriendOnline_;
    prefs.soundOnFriendRequest = soundOnFriendRequest_;
    prefs.notificationSoundVolume = notificationSoundVolume_;
    return prefs;
}

std::string OptionsViewModel::getCustomCloseKeyName() const {
    if (customCloseKeyCode_ <= 0 || customCloseKeyCode_ >= 256) {
        return "ESC (Default)";
    }
    
    // Convert virtual key code to readable name
    if (customCloseKeyCode_ >= 'A' && customCloseKeyCode_ <= 'Z') {
        return std::string(1, static_cast<char>(customCloseKeyCode_));
    }
    if (customCloseKeyCode_ >= '0' && customCloseKeyCode_ <= '9') {
        return std::string(1, static_cast<char>(customCloseKeyCode_));
    }
    
    // Common virtual key codes
    switch (customCloseKeyCode_) {
        case VK_ESCAPE: return "ESC";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_TAB: return "Tab";
        case VK_BACK: return "Backspace";
        case VK_DELETE: return "Delete";
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_UP: return "Up Arrow";
        case VK_DOWN: return "Down Arrow";
        case VK_LEFT: return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default:
            return "VK_" + std::to_string(customCloseKeyCode_);
    }
}

std::string OptionsViewModel::getControllerCloseButtonName() const {
    if (controllerCloseButton_ == 0) {
        return "Disabled";
    }
    
    // XInput button codes
    switch (controllerCloseButton_) {
        case 0x0001: return "D-Pad Up";
        case 0x0002: return "D-Pad Down";
        case 0x0004: return "D-Pad Left";
        case 0x0008: return "D-Pad Right";
        case 0x0010: return "Start";
        case 0x0020: return "Back";
        case 0x0040: return "Left Thumb";
        case 0x0080: return "Right Thumb";
        case 0x0100: return "Left Shoulder";
        case 0x0200: return "Right Shoulder";
        case 0x1000: return "A";
        case 0x2000: return "B (Default)";
        case 0x4000: return "X";
        case 0x8000: return "Y";
        default: {
            std::ostringstream oss;
            oss << "Button 0x" << std::hex << controllerCloseButton_;
            return oss.str();
        }
    }
}

XIFriendList::Core::MemoryStats OptionsViewModel::getMemoryStats() const {
    size_t bytes = sizeof(OptionsViewModel);
    bytes += error_.capacity();
    
    return XIFriendList::Core::MemoryStats(1, bytes, "Options ViewModel");
}

} // namespace UI
} // namespace XIFriendList

