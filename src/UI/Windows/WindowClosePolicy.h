// Centralized window close policy for ESC/Backspace key handling

#ifndef UI_WINDOW_CLOSE_POLICY_H
#define UI_WINDOW_CLOSE_POLICY_H

#include "WindowManager.h"
#include <string>

namespace XIFriendList {
namespace UI {

// Window priority order (higher number = higher priority, closes first)
enum class WindowPriority {
    QuickOnline = 1,
    Main = 2,
    NoteEditor = 3  // Highest priority - closes first
};

// Window close policy manager
// Tracks window visibility and provides methods to close windows in priority order
class WindowClosePolicy {
public:
    WindowClosePolicy(WindowManager* windowManager);
    
    void setWindowsLocked(bool locked) { windowsLocked_ = locked; }
    bool areWindowsLocked() const { return windowsLocked_; }
    
    bool anyWindowOpen() const;
    
    // Close the top-most (highest priority) window
    // Returns the name of the window that was closed, or empty string if none
    // Returns empty string if windows are locked
    std::string closeTopMostWindow();
    
    // Close all windows (ignores lock - use with caution)
    void closeAllWindows();
    
    std::string getTopMostWindowName() const;

private:
    WindowManager* windowManager_;
    bool windowsLocked_;
    
    // Helper to check if a window is visible
    bool isWindowVisible(WindowPriority priority) const;
    
    // Helper to close a window by priority
    void closeWindow(WindowPriority priority);
    
    // Helper to get window name by priority
    std::string getWindowName(WindowPriority priority) const;
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_WINDOW_CLOSE_POLICY_H

