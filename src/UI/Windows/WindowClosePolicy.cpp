// Window close policy implementation

#include "UI/Windows/WindowClosePolicy.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include <windows.h>

namespace XIFriendList {
namespace UI {

WindowClosePolicy::WindowClosePolicy(WindowManager* windowManager)
    : windowManager_(windowManager)
    , windowsLocked_(false)
{
}

bool WindowClosePolicy::anyWindowOpen() const {
    if (!windowManager_) {
        return false;
    }
    
    return isWindowVisible(WindowPriority::NoteEditor) ||
           isWindowVisible(WindowPriority::QuickOnline) ||
           isWindowVisible(WindowPriority::Main);
}

std::string WindowClosePolicy::closeTopMostWindow() {
    if (!windowManager_) {
        return "";
    }
    
    if (windowsLocked_) {
        return "";
    }
    
    // Close windows in priority order (highest first)
    if (isWindowVisible(WindowPriority::NoteEditor)) {
        if (!Platform::Ashita::AshitaPreferencesStore::loadWindowLockState("NoteEditor")) {
            closeWindow(WindowPriority::NoteEditor);
            return getWindowName(WindowPriority::NoteEditor);
        }
    }
    if (isWindowVisible(WindowPriority::QuickOnline)) {
        if (!XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadWindowLockState("QuickOnline")) {
            closeWindow(WindowPriority::QuickOnline);
            return getWindowName(WindowPriority::QuickOnline);
        }
    }
    if (isWindowVisible(WindowPriority::Main)) {
        if (!XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadWindowLockState("MainWindow")) {
            closeWindow(WindowPriority::Main);
            return getWindowName(WindowPriority::Main);
        }
    }
    
    return "";
}

void WindowClosePolicy::closeAllWindows() {
    if (!windowManager_) {
        return;
    }
    
    windowManager_->getNoteEditorWindow().setVisible(false);
    windowManager_->getThemesWindow().setVisible(false);
    windowManager_->getMainWindow().setVisible(false);
    windowManager_->getQuickOnlineWindow().setVisible(false);
}

std::string WindowClosePolicy::getTopMostWindowName() const {
    if (!windowManager_) {
        return "";
    }
    
    if (isWindowVisible(WindowPriority::NoteEditor)) {
        return getWindowName(WindowPriority::NoteEditor);
    }
    if (isWindowVisible(WindowPriority::QuickOnline)) {
        return getWindowName(WindowPriority::QuickOnline);
    }
    if (isWindowVisible(WindowPriority::Main)) {
        return getWindowName(WindowPriority::Main);
    }
    
    return "";
}

bool WindowClosePolicy::isWindowVisible(WindowPriority priority) const {
    if (!windowManager_) {
        return false;
    }
    
    switch (priority) {
        case WindowPriority::NoteEditor:
            return windowManager_->getNoteEditorWindow().isVisible();
        case WindowPriority::QuickOnline:
            return windowManager_->getQuickOnlineWindow().isVisible();
        case WindowPriority::Main:
            return windowManager_->getMainWindow().isVisible();
        default:
            return false;
    }
}

void WindowClosePolicy::closeWindow(WindowPriority priority) {
    if (!windowManager_) {
        return;
    }
    
    switch (priority) {
        case WindowPriority::NoteEditor:
            windowManager_->getNoteEditorWindow().setVisible(false);
            break;
        case WindowPriority::QuickOnline:
            windowManager_->getQuickOnlineWindow().setVisible(false);
            break;
        case WindowPriority::Main:
            windowManager_->getMainWindow().setVisible(false);
            break;
    }
}

std::string WindowClosePolicy::getWindowName(WindowPriority priority) const {
    switch (priority) {
        case WindowPriority::NoteEditor:
            return "NoteEditor";
        case WindowPriority::QuickOnline:
            return "QuickOnline";
        case WindowPriority::Main:
            return "FriendList";
        default:
            return "";
    }
}

} // namespace UI
} // namespace XIFriendList

