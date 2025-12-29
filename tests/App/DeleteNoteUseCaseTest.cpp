// DeleteNoteUseCaseTest.cpp
// Unit tests for DeleteNoteUseCase (server and local modes)

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/NotesUseCases.h"
#include "FakeNetClient.h"
#include "App/State/NotesState.h"
#include <algorithm>
#include <cctype>
#include "FakeClock.h"
#include "FakeLogger.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include <memory>

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;
using namespace XIFriendList::App::State;
using namespace XIFriendList::Core;

TEST_CASE("DeleteNoteUseCase - Delete note from server (success)", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server response (server canonical format)
    HttpResponse serverResponse;
    serverResponse.statusCode = 200;
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"NoteDeleteResponse","success":true})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", serverResponse);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "friend1", true);  // useServerNotes = true
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
}

TEST_CASE("DeleteNoteUseCase - Delete note from server (not found - idempotent)", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up 404 response (note not found)
    HttpResponse serverResponse;
    serverResponse.statusCode = 404;
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Note not found"})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", serverResponse);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "friend1", true);
    
    // 404 should return success (idempotent delete)
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
}

TEST_CASE("DeleteNoteUseCase - Delete note from local storage (success)", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    // Add note to local store (normalized to lowercase)
    std::string normalized = "friend1";
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    Note note(normalized, "Test note", 1000);
    notesState.notes[normalized] = note;
    REQUIRE(notesState.notes.find(normalized) != notesState.notes.end());
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "friend1", false);  // useServerNotes = false
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    
    // Verify note was deleted
    REQUIRE(notesState.notes.find(normalized) == notesState.notes.end());
    REQUIRE(notesState.dirty == true);
}

TEST_CASE("DeleteNoteUseCase - Delete note from local storage (not found)", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    // Try to delete non-existent note
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "nonexistent", false);
    
    // Should succeed (idempotent delete)
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
}

TEST_CASE("DeleteNoteUseCase - Network error from server", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up network error
    HttpResponse errorResponse;
    errorResponse.statusCode = 0;
    errorResponse.error = "Network error";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", errorResponse);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "friend1", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("DeleteNoteUseCase - Server error (500)", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server error
    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Internal server error"})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", errorResponse);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "friend1", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("DeleteNoteUseCase - Missing friend name", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    DeleteNoteResult result = useCase.deleteNote("test-api-key", "TestChar", "", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("Friend name required") != std::string::npos);
}

TEST_CASE("DeleteNoteUseCase - Missing API key or character name", "[App][Notes][DeleteNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    DeleteNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    // Missing API key
    DeleteNoteResult result1 = useCase.deleteNote("", "TestChar", "friend1", true);
    REQUIRE_FALSE(result1.success);
    REQUIRE_FALSE(result1.error.empty());
    
    // Missing character name
    DeleteNoteResult result2 = useCase.deleteNote("test-api-key", "", "friend1", true);
    REQUIRE_FALSE(result2.success);
    REQUIRE_FALSE(result2.error.empty());
}

// Note: "Local storage not available" test removed - NotesState is always available (no null check needed)

