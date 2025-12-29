// SaveNoteUseCaseTest.cpp
// Unit tests for SaveNoteUseCase (server and local modes)
// NOTE: Server mode tests are disabled ([.] tag) - notes server upload feature is disabled

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/NotesUseCases.h"
#include "FakeNetClient.h"
#include "App/State/NotesState.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include <memory>
#include <algorithm>
#include <cctype>

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;
using namespace XIFriendList::App::State;
using namespace XIFriendList::Core;

TEST_CASE("SaveNoteUseCase - Save note to server (success)", "[.][App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server response (server canonical format)
    HttpResponse serverResponse;
    serverResponse.statusCode = 200;
    // Server sends note directly, decoder synthesizes payload
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"NoteUpdateResponse","success":true,"note":{"friendName":"friend1","note":"Test note","updatedAt":1000}})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", serverResponse);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", "Test note", true);  // useServerNotes = true
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.note.friendName == "friend1");
    REQUIRE(result.note.note == "Test note");
    REQUIRE(result.note.updatedAt == 1000);
}

TEST_CASE("SaveNoteUseCase - Save note to local storage (success)", "[App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    clock->setTime(1000);
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", "Local note", false);  // useServerNotes = false
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.note.friendName == "friend1");
    REQUIRE(result.note.note == "Local note");
    REQUIRE(result.note.updatedAt == 1000);
    
    // Verify note was saved to local store (normalized to lowercase)
    std::string normalized = "friend1";
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    auto it = notesState.notes.find(normalized);
    REQUIRE(it != notesState.notes.end());
    REQUIRE(it->second.note == "Local note");
    REQUIRE(it->second.updatedAt == 1000);
    REQUIRE(notesState.dirty == true);
}

TEST_CASE("SaveNoteUseCase - Update existing note in local storage", "[App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    // Add existing note (normalized to lowercase)
    std::string normalized = "friend1";
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    Note existingNote(normalized, "Original note", 1000);
    notesState.notes[normalized] = existingNote;
    clock->setTime(2000);
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", "Updated note", false);
    
    REQUIRE(result.success);
    REQUIRE(result.note.note == "Updated note");
    REQUIRE(result.note.updatedAt == 2000);
    
    // Verify note was updated
    auto it = notesState.notes.find(normalized);
    REQUIRE(it != notesState.notes.end());
    REQUIRE(it->second.note == "Updated note");
    REQUIRE(it->second.updatedAt == 2000);
    REQUIRE(notesState.dirty == true);
}

TEST_CASE("SaveNoteUseCase - Network error from server", "[.][App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up network error
    HttpResponse errorResponse;
    errorResponse.statusCode = 0;
    errorResponse.error = "Network error";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", errorResponse);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", "Test note", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.note.friendName.empty());
}

TEST_CASE("SaveNoteUseCase - Server error (500)", "[.][App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server error
    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Internal server error"})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", errorResponse);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", "Test note", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("SaveNoteUseCase - Missing friend name", "[App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "", "Test note", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("Friend name required") != std::string::npos);
}

TEST_CASE("SaveNoteUseCase - Note too long", "[App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    // Create note longer than 8192 characters
    std::string longNote(8193, 'a');
    
    SaveNoteResult result = useCase.saveNote("test-api-key", "TestChar", "friend1", longNote, true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("8192 characters") != std::string::npos);
}

TEST_CASE("SaveNoteUseCase - Missing API key or character name", "[.][App][Notes][SaveNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SaveNoteUseCase useCase(*netClient, notesState, *clock, *logger);
    
    // Missing API key
    SaveNoteResult result1 = useCase.saveNote("", "TestChar", "friend1", "Test note", true);
    REQUIRE_FALSE(result1.success);
    REQUIRE_FALSE(result1.error.empty());
    
    // Missing character name
    SaveNoteResult result2 = useCase.saveNote("test-api-key", "", "friend1", "Test note", true);
    REQUIRE_FALSE(result2.success);
    REQUIRE_FALSE(result2.error.empty());
}

// Note: "Local storage not available" test removed - NotesState is always available (no null check needed)

