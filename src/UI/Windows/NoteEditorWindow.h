
#ifndef UI_NOTE_EDITOR_WINDOW_H
#define UI_NOTE_EDITOR_WINDOW_H

#include "UI/Commands/WindowCommands.h"
#include "UI/ViewModels/NotesViewModel.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Core/MemoryStats.h"
#include <string>

namespace XIFriendList {
namespace UI {

class NoteEditorWindow {
public:
    NoteEditorWindow();
    ~NoteEditorWindow() = default;
    
    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
    }
    
    void setViewModel(NotesViewModel* viewModel) {
        viewModel_ = viewModel;
    }
    
    void setIconManager(void* iconManager) {
        iconManager_ = iconManager;
    }
    
    void render();
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    void setFriendName(const std::string& name);
    const std::string& getFriendName() const;
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    void emitCommand(WindowCommandType type, const std::string& data = "");
    void renderError();
    void renderStatus();
    void renderActionStatus();
    void renderTimestamp();
    
    IWindowCommandHandler* commandHandler_;
    NotesViewModel* viewModel_;
    void* iconManager_{ nullptr };
    bool visible_;
    std::string title_;
    std::string windowId_;  // Unique window identifier for lock state
    bool locked_;  // Per-window lock state
    bool pendingClose_{ false };  // Close requested while a popup/menu is open; defer until clean.
    std::string noteInputBuffer_;  // Buffer for multiline input
    bool wasLoading_{ false };  // Track loading state to sync buffer when loading completes
    bool wasInputActive_{ false };  // Track input active state to detect when it loses focus
    static const size_t MAX_NOTE_SIZE = 8192;
    
    // Track friend name (for potential future enhancements)
    std::string lastFriendName_;  // Track last friend name to detect changes
    
    // Auto-save helper: saves note if there are unsaved changes
    void autoSaveIfNeeded();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_NOTE_EDITOR_WINDOW_H

