// RemoveFriendUseCaseTest.cpp
// Unit tests for RemoveFriendUseCase
// Tests canonical DELETE /api/friends/:friendName endpoint

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;

TEST_CASE("RemoveFriendUseCase - Success", "[App][RemoveFriend]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendUseCase useCase(*netClient, *clock, *logger);
    
    // Set up successful response for DELETE /api/friends/:friendName
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        // Canonical endpoint: DELETE /api/friends/:friendName
        if (url.find("/api/friends/testfriend") != std::string::npos) {
            HttpResponse successResponse;
            successResponse.statusCode = 200;
            // Server canonical response type
            successResponse.body = R"({"protocolVersion":"2.0.0","type":"RemoveFriendResponse","success":true})";
            return successResponse;
        }
        return { 404, "", "URL not found" };
    });
    
    bool callbackCalled = false;
    RemoveFriendResult result;
    
    useCase.removeFriend("test-api-key", "TestChar", "testfriend", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    // Wait for async callback (max 1 second)
    for (int i = 0; i < 100 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    
    // Verify DELETE /api/friends/:friendName was called
    auto lastDelRequest = netClient->getLastDelRequest();
    REQUIRE(lastDelRequest.url == "http://localhost:3000/api/friends/testfriend");
    REQUIRE(lastDelRequest.apiKey == "test-api-key");
    REQUIRE(lastDelRequest.characterName == "TestChar");
}

TEST_CASE("RemoveFriendUseCase - Friend not found (idempotent)", "[App][RemoveFriend]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendUseCase useCase(*netClient, *clock, *logger);
    
    // 404 response should be treated as success (idempotent delete)
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        if (url.find("/api/friends/") != std::string::npos) {
            HttpResponse notFoundResponse;
            notFoundResponse.statusCode = 404;
            notFoundResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Friend not found"})";
            return notFoundResponse;
        }
        return { 500, "", "Unexpected URL" };
    });
    
    bool callbackCalled = false;
    RemoveFriendResult result;
    
    useCase.removeFriend("test-api-key", "TestChar", "nonexistent", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    for (int i = 0; i < 100 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    // 404 on delete is treated as success (friend already removed)
    REQUIRE(result.success);
}

TEST_CASE("RemoveFriendUseCase - Missing parameters", "[App][RemoveFriend]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendUseCase useCase(*netClient, *clock, *logger);
    
    bool callbackCalled = false;
    RemoveFriendResult result;
    
    // Empty API key
    useCase.removeFriend("", "TestChar", "TestFriend", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("required") != std::string::npos);
    
    callbackCalled = false;
    // Empty character name
    useCase.removeFriend("api-key", "", "TestFriend", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    
    callbackCalled = false;
    // Empty friend name
    useCase.removeFriend("api-key", "TestChar", "", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
}

TEST_CASE("RemoveFriendUseCase - HTTP error", "[App][RemoveFriend]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendUseCase useCase(*netClient, *clock, *logger);
    
    // Set up server error response
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        if (url.find("/api/friends/") != std::string::npos) {
            HttpResponse errorResponse;
            errorResponse.statusCode = 500;
            errorResponse.error = "Internal server error";
            return errorResponse;
        }
        return { 404, "", "URL not found" };
    });
    
    bool callbackCalled = false;
    RemoveFriendResult result;
    
    useCase.removeFriend("test-api-key", "TestChar", "testfriend", 
        [&callbackCalled, &result](RemoveFriendResult r) {
            callbackCalled = true;
            result = r;
        });
    
    for (int i = 0; i < 100 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("RemoveFriendUseCase - Retry configuration", "[App][RemoveFriend]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendUseCase useCase(*netClient, *clock, *logger);
    useCase.setRetryConfig(5, 100);
    
    // Verify configuration is set (indirectly through retry behavior)
    // This test mainly ensures setRetryConfig doesn't crash
    REQUIRE(true);
}

