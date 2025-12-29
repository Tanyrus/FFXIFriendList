// NotesViewModelTest.cpp
// Unit tests for NotesViewModel (state management)

#include <catch2/catch_test_macros.hpp>
#include "UI/ViewModels/NotesViewModel.h"
#include "Core/NotesCore.h"
#include <cstdint>

using namespace XIFriendList::UI;
using namespace XIFriendList::Core;

TEST_CASE("NotesViewModel - Initial state", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE(viewModel.getCurrentFriendName().empty());
    REQUIRE(viewModel.getCurrentNoteText().empty());
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE_FALSE(viewModel.isServerMode());
    REQUIRE_FALSE(viewModel.isLoading());
    REQUIRE_FALSE(viewModel.hasError());
    REQUIRE_FALSE(viewModel.hasStatus());
    REQUIRE_FALSE(viewModel.isEditorOpen());
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    REQUIRE_FALSE(viewModel.getActionStatus().visible);
}

TEST_CASE("NotesViewModel - Open editor", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    viewModel.openEditor("friend1");
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.getCurrentNoteText().empty());
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE(viewModel.isEditorOpen());
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Load note", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    Note note;
    note.friendName = "friend1";
    note.note = "Test note";
    note.updatedAt = 1000;
    
    viewModel.loadNote(note);
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.getCurrentNoteText() == "Test note");
    REQUIRE(viewModel.getLastSavedAt() == 1000);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Set note text", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    viewModel.openEditor("friend1");
    viewModel.setCurrentNoteText("New note text");
    
    REQUIRE(viewModel.getCurrentNoteText() == "New note text");
    REQUIRE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Mark saved", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    viewModel.openEditor("friend1");
    viewModel.setCurrentNoteText("Test note");
    REQUIRE(viewModel.hasUnsavedChanges());
    
    viewModel.markSaved(2000);
    
    REQUIRE(viewModel.getLastSavedAt() == 2000);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Mark deleted", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    Note note;
    note.friendName = "friend1";
    note.note = "Test note";
    note.updatedAt = 1000;
    
    viewModel.loadNote(note);
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.getCurrentNoteText() == "Test note");
    REQUIRE(viewModel.isEditorOpen());
    
    viewModel.markDeleted();
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");  // Editor stays open
    REQUIRE(viewModel.getCurrentNoteText().empty());  // Text is cleared
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    REQUIRE(viewModel.isEditorOpen());  // Editor remains open after delete
}

TEST_CASE("NotesViewModel - Close editor", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    Note note;
    note.friendName = "friend1";
    note.note = "Test note";
    note.updatedAt = 1000;
    
    viewModel.loadNote(note);
    viewModel.setError("Test error");
    viewModel.setStatus("Test status");
    viewModel.setActionStatusSuccess("Saved", 2000);
    
    viewModel.closeEditor();
    
    REQUIRE(viewModel.getCurrentFriendName().empty());
    REQUIRE(viewModel.getCurrentNoteText().empty());
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE_FALSE(viewModel.hasError());
    REQUIRE_FALSE(viewModel.hasStatus());
    REQUIRE_FALSE(viewModel.getActionStatus().visible);
    REQUIRE_FALSE(viewModel.isEditorOpen());
}

TEST_CASE("NotesViewModel - Unsaved changes detection", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    // Load note
    Note note;
    note.friendName = "friend1";
    note.note = "Original note";
    note.updatedAt = 1000;
    viewModel.loadNote(note);
    
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    
    // Modify text
    viewModel.setCurrentNoteText("Modified note");
    REQUIRE(viewModel.hasUnsavedChanges());
    
    // Mark saved
    viewModel.markSaved(2000);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    
    // Modify again
    viewModel.setCurrentNoteText("Modified again");
    REQUIRE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Server mode", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE_FALSE(viewModel.isServerMode());
    REQUIRE(viewModel.getStorageModeText() == "Local");
    
    viewModel.setServerMode(true);
    REQUIRE(viewModel.isServerMode());
    REQUIRE(viewModel.getStorageModeText() == "Server");
    
    viewModel.setServerMode(false);
    REQUIRE_FALSE(viewModel.isServerMode());
    REQUIRE(viewModel.getStorageModeText() == "Local");
}

TEST_CASE("NotesViewModel - Error handling", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE_FALSE(viewModel.hasError());
    REQUIRE(viewModel.getError().empty());
    
    viewModel.setError("Test error");
    REQUIRE(viewModel.hasError());
    REQUIRE(viewModel.getError() == "Test error");
    
    viewModel.clearError();
    REQUIRE_FALSE(viewModel.hasError());
    REQUIRE(viewModel.getError().empty());
}

TEST_CASE("NotesViewModel - Status handling", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE_FALSE(viewModel.hasStatus());
    REQUIRE(viewModel.getStatus().empty());
    
    viewModel.setStatus("Test status");
    REQUIRE(viewModel.hasStatus());
    REQUIRE(viewModel.getStatus() == "Test status");
    
    viewModel.clearStatus();
    REQUIRE_FALSE(viewModel.hasStatus());
    REQUIRE(viewModel.getStatus().empty());
}

TEST_CASE("NotesViewModel - Loading state", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE_FALSE(viewModel.isLoading());
    
    viewModel.setLoading(true);
    REQUIRE(viewModel.isLoading());
    
    viewModel.setLoading(false);
    REQUIRE_FALSE(viewModel.isLoading());
}

TEST_CASE("NotesViewModel - Action status success", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    REQUIRE_FALSE(viewModel.getActionStatus().visible);
    
    viewModel.setActionStatusSuccess("Note saved", 2000);
    
    const auto& status = viewModel.getActionStatus();
    REQUIRE(status.visible);
    REQUIRE(status.success);
    REQUIRE(status.message == "Note saved");
    REQUIRE(status.timestampMs == 2000);
    REQUIRE(status.errorCode.empty());
}

TEST_CASE("NotesViewModel - Action status error", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    viewModel.setActionStatusError("Failed to save", "NETWORK_ERROR", 2000);
    
    const auto& status = viewModel.getActionStatus();
    REQUIRE(status.visible);
    REQUIRE_FALSE(status.success);
    REQUIRE(status.message == "Failed to save");
    REQUIRE(status.errorCode == "NETWORK_ERROR");
    REQUIRE(status.timestampMs == 2000);
}

TEST_CASE("NotesViewModel - Clear action status", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    viewModel.setActionStatusSuccess("Saved", 2000);
    REQUIRE(viewModel.getActionStatus().visible);
    
    viewModel.clearActionStatus();
    
    const auto& status = viewModel.getActionStatus();
    REQUIRE_FALSE(status.visible);
    REQUIRE_FALSE(status.success);
    REQUIRE(status.message.empty());
    REQUIRE(status.timestampMs == 0);
    REQUIRE(status.errorCode.empty());
}

TEST_CASE("NotesViewModel - Update note", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    Note note1;
    note1.friendName = "friend1";
    note1.note = "Note 1";
    note1.updatedAt = 1000;
    
    viewModel.updateNote(note1);
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.getCurrentNoteText() == "Note 1");
    REQUIRE(viewModel.getLastSavedAt() == 1000);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    
    // Update with new note
    Note note2;
    note2.friendName = "friend2";
    note2.note = "Note 2";
    note2.updatedAt = 2000;
    
    viewModel.updateNote(note2);
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend2");
    REQUIRE(viewModel.getCurrentNoteText() == "Note 2");
    REQUIRE(viewModel.getLastSavedAt() == 2000);
}

TEST_CASE("NotesViewModel - Clear current note", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    Note note;
    note.friendName = "friend1";
    note.note = "Test note";
    note.updatedAt = 1000;
    
    viewModel.loadNote(note);
    viewModel.clearCurrentNote();
    
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");  // Friend name remains
    REQUIRE(viewModel.getCurrentNoteText().empty());
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
}

TEST_CASE("NotesViewModel - Format timestamp", "[UI][NotesViewModel]") {
    // Test zero timestamp
    REQUIRE(NotesViewModel::formatTimestamp(0) == "Never");
    
    // Test valid timestamp (2024-01-01 12:00:00 UTC = 1704110400000 ms)
    // Note: This test may be timezone-dependent, so we just check it's not empty and not "Never"
    std::string formatted = NotesViewModel::formatTimestamp(1704110400000);
    REQUIRE_FALSE(formatted.empty());
    REQUIRE(formatted != "Never");
    // Should contain date and time separators
    REQUIRE(formatted.find("-") != std::string::npos);
    REQUIRE(formatted.find(":") != std::string::npos);
}

TEST_CASE("NotesViewModel - Delete note clears text but keeps editor open", "[UI][NotesViewModel]") {
    NotesViewModel viewModel;
    
    // Load a note
    Note note;
    note.friendName = "friend1";
    note.note = "Some note text";
    note.updatedAt = 1000;
    
    viewModel.loadNote(note);
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.getCurrentNoteText() == "Some note text");
    REQUIRE(viewModel.isEditorOpen());
    
    // Modify the text
    viewModel.setCurrentNoteText("Modified text");
    REQUIRE(viewModel.hasUnsavedChanges());
    
    // Delete the note
    viewModel.markDeleted();
    
    // Editor should remain open
    REQUIRE(viewModel.getCurrentFriendName() == "friend1");
    REQUIRE(viewModel.isEditorOpen());
    
    // But text should be cleared
    REQUIRE(viewModel.getCurrentNoteText().empty());
    REQUIRE(viewModel.getLastSavedAt() == 0);
    REQUIRE_FALSE(viewModel.hasUnsavedChanges());
    
    // Error and status should be cleared
    REQUIRE_FALSE(viewModel.hasError());
    REQUIRE_FALSE(viewModel.hasStatus());
}

