// ViewModel for Notes editor window (UI layer)

#ifndef UI_NOTES_VIEW_MODEL_H
#define UI_NOTES_VIEW_MODEL_H

#include "Core/NotesCore.h"
#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <map>
#include <cstdint>

namespace XIFriendList {
namespace UI {

// ViewModel for Notes editor window
// Holds UI state and provides formatted strings/flags for rendering
class NotesViewModel {
public:
    NotesViewModel();
    ~NotesViewModel() = default;
    
    // Current editor state
    const std::string& getCurrentFriendName() const { return currentFriendName_; }
    void setCurrentFriendName(const std::string& name) { currentFriendName_ = name; }
    
    const std::string& getCurrentNoteText() const { return currentNoteText_; }
    void setCurrentNoteText(const std::string& text) { currentNoteText_ = text; }
    
    uint64_t getLastSavedAt() const { return lastSavedAt_; }
    void setLastSavedAt(uint64_t timestamp) { lastSavedAt_ = timestamp; }
    
    // Storage mode (Server vs Local)
    bool isServerMode() const { return useServerNotes_; }
    void setServerMode(bool useServer) { useServerNotes_ = useServer; }
    
    bool isLoading() const { return isLoading_; }
    void setLoading(bool loading) { isLoading_ = loading; }
    
    bool hasError() const { return !errorMessage_.empty(); }
    const std::string& getError() const { return errorMessage_; }
    void setError(const std::string& message) { errorMessage_ = message; }
    void clearError() { errorMessage_.clear(); }
    
    bool hasStatus() const { return !statusMessage_.empty(); }
    const std::string& getStatus() const { return statusMessage_; }
    void setStatus(const std::string& message) { statusMessage_ = message; }
    void clearStatus() { statusMessage_.clear(); }
    
    // Action status for UI feedback (similar to FriendListViewModel)
    struct ActionStatus {
        bool visible;
        bool success;
        std::string message;
        uint64_t timestampMs;
        std::string errorCode;
        
        ActionStatus()
            : visible(false)
            , success(false)
            , timestampMs(0)
        {}
    };
    
    void setActionStatusSuccess(const std::string& message, uint64_t timestampMs);
    void setActionStatusError(const std::string& message, const std::string& errorCode, uint64_t timestampMs);
    void clearActionStatus();
    const ActionStatus& getActionStatus() const { return actionStatus_; }
    
    bool hasUnsavedChanges() const;
    
    bool isEditorOpen() const { return !currentFriendName_.empty(); }
    
    // Close editor (clear current state)
    void closeEditor();
    
    // Open editor for a friend (sets current friend, clears note text)
    void openEditor(const std::string& friendName);
    
    void loadNote(const XIFriendList::Core::Note& note);
    
    // Mark current note as saved (called after use case saves note)
    void markSaved(uint64_t timestamp);
    
    // Mark current note as deleted (called after use case deletes note)
    void markDeleted();
    
    // Format timestamp for display
    static std::string formatTimestamp(uint64_t timestamp);
    
    std::string getStorageModeText() const {
        return useServerNotes_ ? "Server" : "Local";
    }
    
    void updateNote(const XIFriendList::Core::Note& note);
    
    // Clear current note (but keep friend name)
    void clearCurrentNote();
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    std::string currentFriendName_;  // Currently editing friend
    std::string currentNoteText_;     // Current note text (editable buffer)
    uint64_t lastSavedAt_;           // Timestamp of last save
    bool useServerNotes_;            // Storage mode: true = server, false = local
    bool isLoading_;                 // Loading state
    std::string errorMessage_;      // Error message
    std::string statusMessage_;      // Status message (info)
    ActionStatus actionStatus_;      // Action status for UI feedback
    
    // Original note text (for detecting unsaved changes)
    std::string originalNoteText_;
    uint64_t originalUpdatedAt_;
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_NOTES_VIEW_MODEL_H

