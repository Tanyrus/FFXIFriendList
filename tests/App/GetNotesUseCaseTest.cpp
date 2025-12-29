// GetNotesUseCaseTest.cpp
// Unit tests for GetNotesUseCase (server and local modes)
// NOTE: Server mode tests are disabled ([.] tag) - notes server upload feature is disabled

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

TEST_CASE("GetNotesUseCase - Get all notes from server (success)", "[.][App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);  // Reduce retries for tests
    
    // Set up server response with notes list (server canonical format)
    HttpResponse serverResponse;
    serverResponse.statusCode = 200;
    // Server sends notes directly, decoder synthesizes payload
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"NotesListResponse","success":true,"notes":[{"friendName":"friend1","note":"Note 1","updatedAt":1000},{"friendName":"friend2","note":"Note 2","updatedAt":2000}]})";
    netClient->setResponse("http://localhost:3000/api/notes", serverResponse);
    
    GetNotesResult result = useCase.getNotes("test-api-key", "TestChar", true);  // useServerNotes = true
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.notes.size() == 2);
    REQUIRE(result.notes.find("friend1") != result.notes.end());
    REQUIRE(result.notes.find("friend2") != result.notes.end());
    REQUIRE(result.notes.at("friend1").note == "Note 1");
    REQUIRE(result.notes.at("friend1").updatedAt == 1000);
    REQUIRE(result.notes.at("friend2").note == "Note 2");
    REQUIRE(result.notes.at("friend2").updatedAt == 2000);
}

TEST_CASE("GetNotesUseCase - Get all notes from local storage (success)", "[App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    // Add notes to local store (normalized to lowercase)
    std::string normalized1 = "friend1";
    std::string normalized2 = "friend2";
    std::transform(normalized1.begin(), normalized1.end(), normalized1.begin(), ::tolower);
    std::transform(normalized2.begin(), normalized2.end(), normalized2.begin(), ::tolower);
    Note note1(normalized1, "Local note 1", 1000);
    Note note2(normalized2, "Local note 2", 2000);
    notesState.notes[normalized1] = note1;
    notesState.notes[normalized2] = note2;
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    
    GetNotesResult result = useCase.getNotes("test-api-key", "TestChar", false);  // useServerNotes = false
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.notes.size() == 2);
    REQUIRE(result.notes.find(normalized1) != result.notes.end());
    REQUIRE(result.notes.find(normalized2) != result.notes.end());
    REQUIRE(result.notes.at(normalized1).note == "Local note 1");
    REQUIRE(result.notes.at(normalized2).note == "Local note 2");
}

TEST_CASE("GetNotesUseCase - Get single note from server (success)", "[.][App][Notes][GetNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server response with single note (server canonical format)
    HttpResponse serverResponse;
    serverResponse.statusCode = 200;
    // Server sends note directly, decoder synthesizes payload
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"NoteResponse","success":true,"note":{"friendName":"friend1","note":"Note 1","updatedAt":1000}})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", serverResponse);
    
    GetNotesResult result = useCase.getNote("test-api-key", "TestChar", "friend1", true);  // useServerNotes = true
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.notes.size() == 1);
    REQUIRE(result.notes.find("friend1") != result.notes.end());
    REQUIRE(result.notes.at("friend1").note == "Note 1");
    REQUIRE(result.notes.at("friend1").updatedAt == 1000);
}

TEST_CASE("GetNotesUseCase - Get single note from server (not found)", "[.][App][Notes][GetNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up 404 response
    HttpResponse serverResponse;
    serverResponse.statusCode = 404;
    serverResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Note not found"})";
    netClient->setResponse("http://localhost:3000/api/notes/friend1", serverResponse);
    
    GetNotesResult result = useCase.getNote("test-api-key", "TestChar", "friend1", true);
    
    // 404 should return success with empty notes (not an error)
    REQUIRE(result.success);
    REQUIRE(result.notes.empty());
}

TEST_CASE("GetNotesUseCase - Get single note from local storage (success)", "[App][Notes][GetNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    // Add note to local store (normalized to lowercase)
    std::string normalized = "friend1";
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    Note note(normalized, "Local note 1", 1000);
    notesState.notes[normalized] = note;
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    
    GetNotesResult result = useCase.getNote("test-api-key", "TestChar", "friend1", false);  // useServerNotes = false
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.notes.size() == 1);
    REQUIRE(result.notes.find(normalized) != result.notes.end());
    REQUIRE(result.notes.at(normalized).note == "Local note 1");
}

TEST_CASE("GetNotesUseCase - Get single note from local storage (not found)", "[App][Notes][GetNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    
    GetNotesResult result = useCase.getNote("test-api-key", "TestChar", "nonexistent", false);
    
    // Not found should return success with empty notes (not an error)
    REQUIRE(result.success);
    REQUIRE(result.notes.empty());
}

TEST_CASE("GetNotesUseCase - Network error from server", "[.][App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up network error (statusCode == 0)
    HttpResponse errorResponse;
    errorResponse.statusCode = 0;
    errorResponse.error = "Network error";
    netClient->setResponse("http://localhost:3000/api/notes", errorResponse);
    
    GetNotesResult result = useCase.getNotes("test-api-key", "TestChar", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.notes.empty());
}

TEST_CASE("GetNotesUseCase - Server error (500)", "[.][App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up server error
    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Internal server error"})";
    netClient->setResponse("http://localhost:3000/api/notes", errorResponse);
    
    GetNotesResult result = useCase.getNotes("test-api-key", "TestChar", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.notes.empty());
}

TEST_CASE("GetNotesUseCase - Invalid response format", "[App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    useCase.setRetryConfig(1, 100);
    
    // Set up invalid response
    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "Invalid JSON";
    netClient->setResponse("http://localhost:3000/api/notes", invalidResponse);
    
    GetNotesResult result = useCase.getNotes("test-api-key", "TestChar", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.notes.empty());
}

TEST_CASE("GetNotesUseCase - Missing API key or character name", "[App][Notes][GetNotes]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    
    // Missing API key
    GetNotesResult result1 = useCase.getNotes("", "TestChar", true);
    REQUIRE_FALSE(result1.success);
    REQUIRE_FALSE(result1.error.empty());
    
    // Missing character name
    GetNotesResult result2 = useCase.getNotes("test-api-key", "", true);
    REQUIRE_FALSE(result2.success);
    REQUIRE_FALSE(result2.error.empty());
}

TEST_CASE("GetNotesUseCase - Missing friend name for getNote", "[App][Notes][GetNote]") {
    auto netClient = std::make_unique<FakeNetClient>();
    NotesState notesState;
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetNotesUseCase useCase(*netClient, notesState, *clock, *logger);
    
    GetNotesResult result = useCase.getNote("test-api-key", "TestChar", "", true);
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("Friend name required") != std::string::npos);
}

