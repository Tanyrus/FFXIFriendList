// ViewModel for Options window (UI layer)

#ifndef UI_OPTIONS_VIEW_MODEL_H
#define UI_OPTIONS_VIEW_MODEL_H

#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <windows.h>

namespace XIFriendList {
namespace UI {

// ViewModel for Options window
// Holds all preference fields as UI-ready values, plus dirty flags and error text
class OptionsViewModel {
public:
    OptionsViewModel();
    ~OptionsViewModel() = default;
    
    // Notes Settings
    bool getUseServerNotes() const { return useServerNotes_; }
    void setUseServerNotes(bool value) { useServerNotes_ = value; markDirty("useServerNotes"); }
    bool isUseServerNotesDirty() const { return dirtyFlags_.useServerNotes; }
    
    bool getShareFriendsAcrossAlts() const { return shareFriendsAcrossAlts_; }
    void setShareFriendsAcrossAlts(bool value) { shareFriendsAcrossAlts_ = value; markDirty("shareFriendsAcrossAlts"); }
    bool isShareFriendsAcrossAltsDirty() const { return dirtyFlags_.shareFriendsAcrossAlts; }
    
    bool getOverwriteNotesOnUpload() const { return overwriteNotesOnUpload_; }
    void setOverwriteNotesOnUpload(bool value) { overwriteNotesOnUpload_ = value; markDirty("overwriteNotesOnUpload"); }
    bool isOverwriteNotesOnUploadDirty() const { return dirtyFlags_.overwriteNotesOnUpload; }
    
    bool getOverwriteNotesOnDownload() const { return overwriteNotesOnDownload_; }
    void setOverwriteNotesOnDownload(bool value) { overwriteNotesOnDownload_ = value; markDirty("overwriteNotesOnDownload"); }
    bool isOverwriteNotesOnDownloadDirty() const { return dirtyFlags_.overwriteNotesOnDownload; }
    
    // Privacy / Visibility Controls
    bool getShareJobWhenAnonymous() const { return shareJobWhenAnonymous_; }
    void setShareJobWhenAnonymous(bool value) { shareJobWhenAnonymous_ = value; markDirty("shareJobWhenAnonymous"); }
    bool isShareJobWhenAnonymousDirty() const { return dirtyFlags_.shareJobWhenAnonymous; }
    
    bool getShowOnlineStatus() const { return showOnlineStatus_; }
    void setShowOnlineStatus(bool value) { showOnlineStatus_ = value; markDirty("showOnlineStatus"); }
    bool isShowOnlineStatusDirty() const { return dirtyFlags_.showOnlineStatus; }
    
    bool getShareLocation() const { return shareLocation_; }
    void setShareLocation(bool value) { shareLocation_ = value; markDirty("shareLocation"); }
    bool isShareLocationDirty() const { return dirtyFlags_.shareLocation; }
    
    // UI Behavior / Column Visibility
    bool getShowFriendedAsColumn() const { return showFriendedAsColumn_; }
    void setShowFriendedAsColumn(bool value) { showFriendedAsColumn_ = value; markDirty("showFriendedAsColumn"); }
    bool isShowFriendedAsColumnDirty() const { return dirtyFlags_.showFriendedAsColumn; }

    bool getShowJobColumn() const { return showJobColumn_; }
    void setShowJobColumn(bool value) { showJobColumn_ = value; markDirty("showJobColumn"); }
    bool isShowJobColumnDirty() const { return dirtyFlags_.showJobColumn; }
    
    bool getShowRankColumn() const { return showRankColumn_; }
    void setShowRankColumn(bool value) { showRankColumn_ = value; markDirty("showRankColumn"); }
    bool isShowRankColumnDirty() const { return dirtyFlags_.showRankColumn; }
    
    bool getShowNationColumn() const { return showNationColumn_; }
    void setShowNationColumn(bool value) { showNationColumn_ = value; markDirty("showNationColumn"); }
    bool isShowNationColumnDirty() const { return dirtyFlags_.showNationColumn; }
    
    bool getShowZoneColumn() const { return showZoneColumn_; }
    void setShowZoneColumn(bool value) { showZoneColumn_ = value; markDirty("showZoneColumn"); }
    bool isShowZoneColumnDirty() const { return dirtyFlags_.showZoneColumn; }
    
    bool getShowLastSeenColumn() const { return showLastSeenColumn_; }
    void setShowLastSeenColumn(bool value) { showLastSeenColumn_ = value; markDirty("showLastSeenColumn"); }
    bool isShowLastSeenColumnDirty() const { return dirtyFlags_.showLastSeenColumn; }
    
    // Friend View Settings
    const Core::FriendViewSettings& getMainFriendView() const { return mainFriendView_; }
    void setMainFriendView(const Core::FriendViewSettings& settings) { mainFriendView_ = settings; markDirty("mainFriendView"); }
    bool isMainFriendViewDirty() const { return dirtyFlags_.mainFriendView; }
    
    const Core::FriendViewSettings& getQuickOnlineFriendView() const { return quickOnlineFriendView_; }
    void setQuickOnlineFriendView(const Core::FriendViewSettings& settings) { quickOnlineFriendView_ = settings; markDirty("quickOnlineFriendView"); }
    bool isQuickOnlineFriendViewDirty() const { return dirtyFlags_.quickOnlineFriendView; }
    
    bool getDebugMode() const { return debugMode_; }
    void setDebugMode(bool value) { debugMode_ = value; markDirty("debugMode"); }
    bool isDebugModeDirty() const { return dirtyFlags_.debugMode; }
    
    // Notification Settings
    float getNotificationDuration() const { return notificationDuration_; }
    void setNotificationDuration(float value) { notificationDuration_ = value; markDirty("notificationDuration"); }
    bool isNotificationDurationDirty() const { return dirtyFlags_.notificationDuration; }
    
    float getNotificationPositionX() const { return notificationPositionX_; }
    void setNotificationPositionX(float value) { notificationPositionX_ = value; markDirty("notificationPositionX"); }
    bool isNotificationPositionXDirty() const { return dirtyFlags_.notificationPositionX; }
    
    float getNotificationPositionY() const { return notificationPositionY_; }
    void setNotificationPositionY(float value) { notificationPositionY_ = value; markDirty("notificationPositionY"); }
    bool isNotificationPositionYDirty() const { return dirtyFlags_.notificationPositionY; }
    
    bool getNotificationSoundsEnabled() const { return notificationSoundsEnabled_; }
    void setNotificationSoundsEnabled(bool value) { notificationSoundsEnabled_ = value; markDirty("notificationSoundsEnabled"); }
    bool isNotificationSoundsEnabledDirty() const { return dirtyFlags_.notificationSoundsEnabled; }
    
    bool getSoundOnFriendOnline() const { return soundOnFriendOnline_; }
    void setSoundOnFriendOnline(bool value) { soundOnFriendOnline_ = value; markDirty("soundOnFriendOnline"); }
    bool isSoundOnFriendOnlineDirty() const { return dirtyFlags_.soundOnFriendOnline; }
    
    bool getSoundOnFriendRequest() const { return soundOnFriendRequest_; }
    void setSoundOnFriendRequest(bool value) { soundOnFriendRequest_ = value; markDirty("soundOnFriendRequest"); }
    bool isSoundOnFriendRequestDirty() const { return dirtyFlags_.soundOnFriendRequest; }
    
    float getNotificationSoundVolume() const { return notificationSoundVolume_; }
    void setNotificationSoundVolume(float value) { notificationSoundVolume_ = value; markDirty("notificationSoundVolume"); }
    bool isNotificationSoundVolumeDirty() const { return dirtyFlags_.notificationSoundVolume; }
    
    // Customizable Close Key
    int getCustomCloseKeyCode() const { return customCloseKeyCode_; }
    void setCustomCloseKeyCode(int value) { customCloseKeyCode_ = value; markDirty("customCloseKeyCode"); }
    bool isCustomCloseKeyCodeDirty() const { return dirtyFlags_.customCloseKeyCode; }
    
    // Helper to get display name for custom key
    std::string getCustomCloseKeyName() const;
    
    // Controller Close Button
    int getControllerCloseButton() const { return controllerCloseButton_; }
    void setControllerCloseButton(int value) { controllerCloseButton_ = value; markDirty("controllerCloseButton"); }
    bool isControllerCloseButtonDirty() const { return dirtyFlags_.controllerCloseButton; }
    
    // Helper to get display name for controller button
    std::string getControllerCloseButtonName() const;
    
    // Window Lock
    bool getWindowsLocked() const { return windowsLocked_; }
    void setWindowsLocked(bool value) { windowsLocked_ = value; markDirty("windowsLocked"); }
    bool isWindowsLockedDirty() const { return dirtyFlags_.windowsLocked; }
    
    // Dirty tracking
    bool hasDirtyFields() const;
    void clearDirtyFlags();
    
    // Error state
    const std::string& getError() const { return error_; }
    void setError(const std::string& error) { error_ = error; }
    void clearError() { error_ = ""; }
    bool hasError() const { return !error_.empty(); }
    
    void updateFromPreferences(const Core::Preferences& prefs);
    
    Core::Preferences toPreferences() const;
    
    bool isLoaded() const { return loaded_; }
    void setLoaded(bool loaded) { loaded_ = loaded; }
    
    Core::MemoryStats getMemoryStats() const;

private:
    bool useServerNotes_;
    bool shareFriendsAcrossAlts_;
    bool overwriteNotesOnUpload_;
    bool overwriteNotesOnDownload_;
    bool shareJobWhenAnonymous_;
    bool showOnlineStatus_;
    bool shareLocation_;
    bool showFriendedAsColumn_;
    bool showJobColumn_;
    bool showRankColumn_;
    bool showNationColumn_;
    bool showZoneColumn_;
    bool showLastSeenColumn_;

    XIFriendList::Core::FriendViewSettings mainFriendView_;
    XIFriendList::Core::FriendViewSettings quickOnlineFriendView_;
    bool debugMode_;
    float notificationDuration_;
    float notificationPositionX_;  // -1 means "use default", otherwise absolute X position
    float notificationPositionY_;  // -1 means "use default", otherwise absolute Y position
    int customCloseKeyCode_;
    int controllerCloseButton_;
    bool windowsLocked_;
    
    bool notificationSoundsEnabled_;
    bool soundOnFriendOnline_;
    bool soundOnFriendRequest_;
    float notificationSoundVolume_;
    
    // Dirty flags (track which fields have changed)
    struct DirtyFlags {
        bool useServerNotes;
        bool shareFriendsAcrossAlts;
        bool overwriteNotesOnUpload;
        bool overwriteNotesOnDownload;
        bool shareJobWhenAnonymous;
        bool showOnlineStatus;
        bool shareLocation;
        bool showFriendedAsColumn;
        bool showJobColumn;
        bool showRankColumn;
        bool showNationColumn;
        bool showZoneColumn;
        bool showLastSeenColumn;
        bool mainFriendView;
        bool quickOnlineFriendView;
        bool debugMode;
        bool notificationDuration;
        bool notificationPositionX;
        bool notificationPositionY;
        bool customCloseKeyCode;
        bool controllerCloseButton;
        bool windowsLocked;
        bool notificationSoundsEnabled;
        bool soundOnFriendOnline;
        bool soundOnFriendRequest;
        bool notificationSoundVolume;
        
        DirtyFlags() : useServerNotes(false), shareFriendsAcrossAlts(false), overwriteNotesOnUpload(false), overwriteNotesOnDownload(false),
                      shareJobWhenAnonymous(false), showOnlineStatus(false), shareLocation(false),
                      showFriendedAsColumn(false),
                      showJobColumn(false), showRankColumn(false), showNationColumn(false), showZoneColumn(false),
                      showLastSeenColumn(false), mainFriendView(false), quickOnlineFriendView(false), debugMode(false), notificationDuration(false),
                      notificationPositionX(false), notificationPositionY(false), customCloseKeyCode(false), controllerCloseButton(false), windowsLocked(false),
                      notificationSoundsEnabled(false), soundOnFriendOnline(false), soundOnFriendRequest(false),
                      notificationSoundVolume(false) {}
    } dirtyFlags_;
    
    std::string error_;
    bool loaded_;
    
    // Helper to mark a field as dirty
    void markDirty(const std::string& field);
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_OPTIONS_VIEW_MODEL_H

