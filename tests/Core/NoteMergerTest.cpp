// NoteMergerTest.cpp
// Unit tests for NoteMerger

#include <catch2/catch_test_macros.hpp>
#include "Core/NotesCore.h"
#include <string>

using namespace XIFriendList::Core;

TEST_CASE("NoteMerger - Both notes empty returns empty", "[Core][NoteMerger]") {
    std::string result = NoteMerger::merge("", 1000, "", 2000);
    REQUIRE(result.empty());
}

TEST_CASE("NoteMerger - Only local note present", "[Core][NoteMerger]") {
    std::string result = NoteMerger::merge("Local note content", 1000, "", 0);
    REQUIRE(result == "Local note content");
}

TEST_CASE("NoteMerger - Only server note present", "[Core][NoteMerger]") {
    std::string result = NoteMerger::merge("", 0, "Server note content", 2000);
    REQUIRE(result == "Server note content");
}

TEST_CASE("NoteMerger - Identical notes returns one copy", "[Core][NoteMerger]") {
    std::string note = "Same note content";
    std::string result = NoteMerger::merge(note, 1000, note, 2000);
    REQUIRE(result == note);
}

TEST_CASE("NoteMerger - Identical notes with whitespace differences", "[Core][NoteMerger]") {
    std::string localNote = "  Same note content  \n";
    std::string serverNote = "Same note content";
    std::string result = NoteMerger::merge(localNote, 1000, serverNote, 2000);
    // Should recognize as equal after trimming
    REQUIRE(result == "Same note content");
}

TEST_CASE("NoteMerger - Different notes get merged with divider", "[Core][NoteMerger]") {
    std::string localNote = "Local version";
    std::string serverNote = "Server version";
    
    std::string result = NoteMerger::merge(localNote, 2000, serverNote, 1000);
    
    // Newer note (local) should come first
    REQUIRE(result.find("=== Local Note") != std::string::npos);
    REQUIRE(result.find("=== Server Note") != std::string::npos);
    REQUIRE(result.find("--- Merged Notes ---") != std::string::npos);
    REQUIRE(result.find("Local version") != std::string::npos);
    REQUIRE(result.find("Server version") != std::string::npos);
    
    // Local should come before server since it's newer
    size_t localPos = result.find("=== Local Note");
    size_t serverPos = result.find("=== Server Note");
    REQUIRE(localPos < serverPos);
}

TEST_CASE("NoteMerger - Newer server note comes first", "[Core][NoteMerger]") {
    std::string localNote = "Old local version";
    std::string serverNote = "New server version";
    
    std::string result = NoteMerger::merge(localNote, 1000, serverNote, 2000);
    
    // Server note should come first since it's newer
    size_t localPos = result.find("=== Local Note");
    size_t serverPos = result.find("=== Server Note");
    REQUIRE(serverPos < localPos);
}

TEST_CASE("NoteMerger - containsMergeMarker detects divider", "[Core][NoteMerger]") {
    std::string withDivider = "Some content\n--- Merged Notes ---\nMore content";
    std::string withHeader = "=== Local Note (2024-01-01 12:00) ===\nContent";
    std::string withServerHeader = "=== Server Note (2024-01-01 12:00) ===\nContent";
    std::string withoutMarker = "Regular note without any markers";
    
    REQUIRE(NoteMerger::containsMergeMarker(withDivider));
    REQUIRE(NoteMerger::containsMergeMarker(withHeader));
    REQUIRE(NoteMerger::containsMergeMarker(withServerHeader));
    REQUIRE_FALSE(NoteMerger::containsMergeMarker(withoutMarker));
}

TEST_CASE("NoteMerger - Avoids infinite nesting when both have markers", "[Core][NoteMerger]") {
    // Simulate already-merged notes
    std::string localMerged = "=== Local Note (2024-01-01 12:00) ===\nOld content\n--- Merged Notes ---\n=== Server Note ===\nOther content";
    std::string serverMerged = "=== Server Note (2024-01-02 12:00) ===\nNewer content\n--- Merged Notes ---\n=== Local Note ===\nStale content";
    
    // When both have markers, should use the newer one to avoid exponential growth
    std::string result = NoteMerger::merge(localMerged, 1000, serverMerged, 2000);
    
    // Should return the newer one (server, timestamp 2000)
    REQUIRE(result == serverMerged);
    
    // Test the opposite case
    std::string result2 = NoteMerger::merge(localMerged, 3000, serverMerged, 2000);
    REQUIRE(result2 == localMerged);
}

TEST_CASE("NoteMerger - areNotesEqual ignores whitespace", "[Core][NoteMerger]") {
    REQUIRE(NoteMerger::areNotesEqual("  test  ", "test"));
    REQUIRE(NoteMerger::areNotesEqual("test\n", "test"));
    REQUIRE(NoteMerger::areNotesEqual("\t\ntest\r\n", "test"));
    REQUIRE_FALSE(NoteMerger::areNotesEqual("test1", "test2"));
}

TEST_CASE("NoteMerger - formatTimestamp handles milliseconds", "[Core][NoteMerger]") {
    // Timestamp 0 returns "unknown"
    std::string unknown = NoteMerger::formatTimestamp(0);
    REQUIRE(unknown == "unknown");
    
    // Large timestamp (milliseconds) is handled
    // 1704067200000 = 2024-01-01 00:00:00 UTC in ms
    std::string msTimestamp = NoteMerger::formatTimestamp(1704067200000ULL);
    REQUIRE_FALSE(msTimestamp.empty());
    REQUIRE(msTimestamp != "unknown");
    
    // Small timestamp (seconds) is handled
    std::string secTimestamp = NoteMerger::formatTimestamp(1704067200);
    REQUIRE_FALSE(secTimestamp.empty());
    REQUIRE(secTimestamp != "unknown");
}

TEST_CASE("NoteMerger - Deterministic output", "[Core][NoteMerger]") {
    std::string local = "Local content";
    std::string server = "Server content";
    uint64_t localTs = 1000;
    uint64_t serverTs = 2000;
    
    // Same inputs should produce same output
    std::string result1 = NoteMerger::merge(local, localTs, server, serverTs);
    std::string result2 = NoteMerger::merge(local, localTs, server, serverTs);
    
    REQUIRE(result1 == result2);
}

// Note: trim() is private, but we test its behavior through areNotesEqual
TEST_CASE("NoteMerger - Whitespace handling via areNotesEqual", "[Core][NoteMerger]") {
    // Empty strings
    REQUIRE(NoteMerger::areNotesEqual("", ""));
    REQUIRE(NoteMerger::areNotesEqual("  ", ""));
    REQUIRE(NoteMerger::areNotesEqual("", "  "));
    
    // Normal content with various whitespace
    REQUIRE(NoteMerger::areNotesEqual("test", "test"));
    REQUIRE(NoteMerger::areNotesEqual("  test  ", "test"));
    REQUIRE(NoteMerger::areNotesEqual("test", "  test  "));
    REQUIRE(NoteMerger::areNotesEqual("\t\ntest\r\n", "test"));
    
    // Multiline - internal whitespace preserved
    REQUIRE(NoteMerger::areNotesEqual("multi\nline", "multi\nline"));
}


