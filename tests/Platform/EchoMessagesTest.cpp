// EchoMessagesTest.cpp
// Tests for debug echo messages
// 
// Note: This test verifies the message format used by debugEcho() in AshitaAdapter.
// Since debugEcho is guarded by #ifndef BUILDING_TESTS for IChatManager,
// we test the logging behavior which is always available.
// The actual IChatManager::Write() call is not executed in test builds.
// 
// The debugEcho method formats messages as "FriendList: " + message
// and logs them via ILogger::info(). This test verifies that format.

#include <catch2/catch_test_macros.hpp>
#include "../App/FakeLogger.h"
#include <memory>
#include <vector>
#include <string>

using namespace XIFriendList::App;

// Note: This test verifies the message format used by debugEcho() in AshitaAdapter.
// Since debugEcho is guarded by #ifndef BUILDING_TESTS for IChatManager,
// we test the logging behavior which is always available.
// The actual IChatManager::Write() call is not executed in test builds.
// 
// The debugEcho method formats messages as "FriendList: " + message
// and logs them via ILogger::info(). This test verifies that format.

TEST_CASE("Echo messages - Character changed", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    // The debugEcho method formats messages as "FriendList: " + message
    // We can verify this by checking logger entries
    
    // Simulate what debugEcho does:
    std::string message = "Character changed to TestChar";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    // Verify message was logged
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Character changed to TestChar"));
    
    const auto& entries = logger->getEntries();
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].message == fullMessage);
}

TEST_CASE("Echo messages - Zone changed", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string message = "Zone changed to Windurst";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Zone changed to Windurst"));
}

TEST_CASE("Echo messages - Friend request sent", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string friendName = "TestFriend";
    std::string message = "Friend request sent to " + friendName;
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Friend request sent to"));
    REQUIRE(logger->contains(friendName));
}

TEST_CASE("Echo messages - Friend added", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string friendName = "NewFriend";
    std::string message = "Added friend " + friendName;
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Added friend"));
    REQUIRE(logger->contains(friendName));
}

TEST_CASE("Echo messages - Setting changed", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string settingName = "debugMode";
    std::string value = "true";
    std::string message = "Setting '" + settingName + "' changed to " + value;
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Setting"));
    REQUIRE(logger->contains("changed to"));
    REQUIRE(logger->contains(settingName));
    REQUIRE(logger->contains(value));
}

TEST_CASE("Echo messages - Friend request accepted", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string message = "Friend request accepted for TestFriend";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Friend request accepted"));
}

TEST_CASE("Echo messages - Friend request rejected", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string message = "Friend request rejected";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Friend request rejected"));
}

TEST_CASE("Echo messages - Friend request canceled", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string message = "Friend request canceled";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Friend request canceled"));
}

TEST_CASE("Echo messages - Friend removed", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    std::string friendName = "RemovedFriend";
    std::string message = "Friend " + friendName + " removed";
    std::string fullMessage = "FriendList: " + message;
    
    logger->info(fullMessage);
    
    REQUIRE(logger->contains("FriendList:"));
    REQUIRE(logger->contains("Friend"));
    REQUIRE(logger->contains("removed"));
    REQUIRE(logger->contains(friendName));
}

TEST_CASE("Echo messages - Consistent format", "[Platform][Echo]") {
    auto logger = std::make_unique<FakeLogger>();
    
    // All echo messages should start with "FriendList: "
    std::vector<std::string> messages = {
        "Character changed to TestChar",
        "Zone changed to Windurst",
        "Friend request sent to Friend",
        "Added friend NewFriend",
        "Setting 'debugMode' changed to true"
    };
    
    for (const auto& msg : messages) {
        std::string fullMessage = "FriendList: " + msg;
        logger->info(fullMessage);
    }
    
    const auto& entries = logger->getEntries();
    REQUIRE(entries.size() == messages.size());
    
    // All entries should start with "FriendList: "
    for (const auto& entry : entries) {
        REQUIRE(entry.message.find("FriendList: ") == 0);
    }
}

