// DebugLogTest.cpp
// Catch2 tests for DebugLog ring buffer

#include <catch2/catch_test_macros.hpp>
#include "Debug/DebugLog.h"
#include <vector>
#include <string>

using namespace XIFriendList::Debug;

TEST_CASE("DebugLog - Empty log initially", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    REQUIRE(log.size() == 0);
    REQUIRE(log.maxLines() == 1000);
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.empty());
}

TEST_CASE("DebugLog - Push single message", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    log.push("Test message 1");
    
    REQUIRE(log.size() == 1);
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == 1);
    REQUIRE(snapshot[0].message == "Test message 1");
}

TEST_CASE("DebugLog - Push multiple messages", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    log.push("Message 1");
    log.push("Message 2");
    log.push("Message 3");
    
    REQUIRE(log.size() == 3);
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == 3);
    REQUIRE(snapshot[0].message == "Message 1");
    REQUIRE(snapshot[1].message == "Message 2");
    REQUIRE(snapshot[2].message == "Message 3");
}

TEST_CASE("DebugLog - Ring buffer wraps at maxLines", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    // Push exactly maxLines messages
    for (size_t i = 0; i < log.maxLines(); ++i) {
        log.push("Message " + std::to_string(i));
    }
    
    REQUIRE(log.size() == log.maxLines());
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == log.maxLines());
    REQUIRE(snapshot[0].message == "Message 0");
    REQUIRE(snapshot[log.maxLines() - 1].message == "Message " + std::to_string(log.maxLines() - 1));
    
    // Push one more - should wrap around
    log.push("Message " + std::to_string(log.maxLines()));
    
    REQUIRE(log.size() == log.maxLines());
    
    auto snapshot2 = log.snapshot();
    REQUIRE(snapshot2.size() == log.maxLines());
    // First message should now be "Message 1" (oldest after wrap)
    REQUIRE(snapshot2[0].message == "Message 1");
    // Last message should be the new one
    REQUIRE(snapshot2[log.maxLines() - 1].message == "Message " + std::to_string(log.maxLines()));
}

TEST_CASE("DebugLog - Push more than maxLines", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    // Push more than maxLines
    size_t pushCount = log.maxLines() + 100;
    for (size_t i = 0; i < pushCount; ++i) {
        log.push("Message " + std::to_string(i));
    }
    
    REQUIRE(log.size() == log.maxLines());
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == log.maxLines());
    
    // Should contain only the last maxLines messages
    // First message should be from index 100 (after wrap)
    REQUIRE(snapshot[0].message == "Message 100");
    // Last message should be the most recent
    REQUIRE(snapshot[log.maxLines() - 1].message == "Message " + std::to_string(pushCount - 1));
}

TEST_CASE("DebugLog - Clear empties log", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    // Add some messages
    log.push("Message 1");
    log.push("Message 2");
    log.push("Message 3");
    
    REQUIRE(log.size() == 3);
    
    // Clear
    log.clear();
    
    REQUIRE(log.size() == 0);
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.empty());
}

TEST_CASE("DebugLog - Snapshot order is oldest to newest", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    // Push messages in order
    for (size_t i = 0; i < 10; ++i) {
        log.push("Message " + std::to_string(i));
    }
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == 10);
    
    // Verify order: oldest first, newest last
    for (size_t i = 0; i < 10; ++i) {
        REQUIRE(snapshot[i].message == "Message " + std::to_string(i));
    }
}

TEST_CASE("DebugLog - Snapshot order maintained after wrap", "[Debug][DebugLog]") {
    DebugLog& log = DebugLog::getInstance();
    log.clear();
    
    // Fill buffer and wrap
    for (size_t i = 0; i < log.maxLines() + 5; ++i) {
        log.push("Message " + std::to_string(i));
    }
    
    auto snapshot = log.snapshot();
    REQUIRE(snapshot.size() == log.maxLines());
    
    // Verify order: oldest first (index 5), newest last (index maxLines + 4)
    REQUIRE(snapshot[0].message == "Message 5");
    REQUIRE(snapshot[log.maxLines() - 1].message == "Message " + std::to_string(log.maxLines() + 4));
    
    // Verify sequential order
    for (size_t i = 0; i < log.maxLines() - 1; ++i) {
        size_t expectedIndex = 5 + i;
        REQUIRE(snapshot[i].message == "Message " + std::to_string(expectedIndex));
        REQUIRE(snapshot[i + 1].message == "Message " + std::to_string(expectedIndex + 1));
    }
}

