
#ifndef UI_DEBUG_LOG_WINDOW_H
#define UI_DEBUG_LOG_WINDOW_H

#include <string>
#include <vector>
#include "UI/Commands/WindowCommands.h"

namespace XIFriendList {
namespace UI {
    // Forward declarations
    class IWindowCommandHandler;

class DebugLogWindow {
public:
    DebugLogWindow();
    ~DebugLogWindow() = default;
    
    void render();
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void toggle() { visible_ = !visible_; }
    
    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
    }

private:
    void emitCommand(WindowCommandType type, const std::string& data = "");
    
    IWindowCommandHandler* commandHandler_;
    bool visible_;
    std::string title_;
    std::string windowId_;  // Unique window identifier for lock state
    bool locked_;  // Per-window lock state
    bool pendingClose_{ false };  // Close requested while a popup/menu is open; defer until clean.
    std::string filterText_;
    bool autoScroll_;
    std::vector<std::string> cachedLogLines_;
    size_t lastLogSize_;
    
    void renderToolbar();
    void renderLogContent();
    void copyAllToClipboard();
    void clearLog();
    std::vector<std::string> getFilteredLogLines() const;
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_DEBUG_LOG_WINDOW_H

