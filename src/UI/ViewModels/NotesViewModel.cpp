
#include "UI/ViewModels/NotesViewModel.h"
#include "Core/MemoryStats.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace XIFriendList {
namespace UI {

NotesViewModel::NotesViewModel()
    : lastSavedAt_(0)
    , useServerNotes_(false)
    , isLoading_(false)
    , originalUpdatedAt_(0)
{
    actionStatus_.visible = false;
    actionStatus_.success = false;
    actionStatus_.timestampMs = 0;
}

bool NotesViewModel::hasUnsavedChanges() const {
    if (currentFriendName_.empty()) {
        return false;
    }
    
    // Compare current text with original
    return currentNoteText_ != originalNoteText_;
}

void NotesViewModel::closeEditor() {
    currentFriendName_.clear();
    currentNoteText_.clear();
    lastSavedAt_ = 0;
    originalNoteText_.clear();
    originalUpdatedAt_ = 0;
    clearError();
    clearStatus();
    clearActionStatus();
}

void NotesViewModel::openEditor(const std::string& friendName) {
    currentFriendName_ = friendName;
    currentNoteText_.clear();
    lastSavedAt_ = 0;
    originalNoteText_.clear();
    originalUpdatedAt_ = 0;
    clearError();
    clearStatus();
    // Don't clear action status - might want to show previous action result
}

void NotesViewModel::loadNote(const XIFriendList::Core::Note& note) {
    updateNote(note);
    clearError();
    clearStatus();
}

void NotesViewModel::markSaved(uint64_t timestamp) {
    lastSavedAt_ = timestamp;
    originalNoteText_ = currentNoteText_;
    originalUpdatedAt_ = timestamp;
    clearError();
}

void NotesViewModel::markDeleted() {
    // Clear note text and all related state, but keep editor open
    currentNoteText_.clear();
    lastSavedAt_ = 0;
    originalNoteText_.clear();
    originalUpdatedAt_ = 0;
    clearError();
    clearStatus();
    // Do NOT clear friend name - keep editor open with empty content
}

void NotesViewModel::updateNote(const XIFriendList::Core::Note& note) {
    currentFriendName_ = note.friendName;
    currentNoteText_ = note.note;
    lastSavedAt_ = note.updatedAt;
    originalNoteText_ = note.note;
    originalUpdatedAt_ = note.updatedAt;
}

void NotesViewModel::clearCurrentNote() {
    currentNoteText_.clear();
    lastSavedAt_ = 0;
    originalNoteText_.clear();
    originalUpdatedAt_ = 0;
}

void NotesViewModel::setActionStatusSuccess(const std::string& message, uint64_t timestampMs) {
    actionStatus_.visible = true;
    actionStatus_.success = true;
    actionStatus_.message = message;
    actionStatus_.timestampMs = timestampMs;
    actionStatus_.errorCode.clear();
}

void NotesViewModel::setActionStatusError(const std::string& message, const std::string& errorCode, uint64_t timestampMs) {
    actionStatus_.visible = true;
    actionStatus_.success = false;
    actionStatus_.message = message;
    actionStatus_.timestampMs = timestampMs;
    actionStatus_.errorCode = errorCode;
}

void NotesViewModel::clearActionStatus() {
    actionStatus_.visible = false;
    actionStatus_.success = false;
    actionStatus_.message.clear();
    actionStatus_.timestampMs = 0;
    actionStatus_.errorCode.clear();
}

std::string NotesViewModel::formatTimestamp(uint64_t timestamp) {
    if (timestamp == 0) {
        return "Never";
    }
    
    // Convert milliseconds to seconds
    time_t timeSeconds = static_cast<time_t>(timestamp / 1000);
    struct tm timeInfo;
    
#ifdef _WIN32
    localtime_s(&timeInfo, &timeSeconds);
#else
    localtime_r(&timeSeconds, &timeInfo);
#endif
    
    std::ostringstream oss;
    oss << (1900 + timeInfo.tm_year) << "-"
        << std::setfill('0') << std::setw(2) << (timeInfo.tm_mon + 1) << "-"
        << std::setfill('0') << std::setw(2) << timeInfo.tm_mday << " "
        << std::setfill('0') << std::setw(2) << timeInfo.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << timeInfo.tm_min << ":"
        << std::setfill('0') << std::setw(2) << timeInfo.tm_sec;
    
    return oss.str();
}

::XIFriendList::Core::MemoryStats NotesViewModel::getMemoryStats() const {
    size_t bytes = sizeof(NotesViewModel);
    
    bytes += currentFriendName_.capacity();
    bytes += currentNoteText_.capacity();
    bytes += originalNoteText_.capacity();
    bytes += errorMessage_.capacity();
    bytes += statusMessage_.capacity();
    bytes += actionStatus_.message.capacity();
    bytes += actionStatus_.errorCode.capacity();
    
    return ::XIFriendList::Core::MemoryStats(1, bytes, "Notes ViewModel");
}

} // namespace UI
} // namespace XIFriendList
