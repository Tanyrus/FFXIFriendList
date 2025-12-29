// RemoveFriendVisibilityUseCaseTest.cpp
// Unit tests for RemoveFriendVisibilityUseCase
// Tests canonical DELETE /api/friends/:friendName/visibility endpoint

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

TEST_CASE("RemoveFriendVisibilityUseCase - Success", "[App][RemoveFriendVisibility]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    // Set up successful response for DELETE /api/friends/:friendName/visibility
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        // Canonical endpoint: DELETE /api/friends/:friendName/visibility
        // URL will be full: http://localhost:3000/api/friends/testfriend/visibility
        if (url.find("/api/friends/") != std::string::npos && url.find("/visibility") != std::string::npos) {
            HttpResponse successResponse;
            successResponse.statusCode = 200;
            // Server canonical response type
            successResponse.body = R"({"protocolVersion":"2.0.0","type":"RemoveFriendVisibilityResponse","success":true,"friendshipDeleted":false})";
            return successResponse;
        }
        return { 404, "", "URL not found: " + url };
    });
    
    bool callbackCalled = false;
    RemoveFriendVisibilityResult result;
    
    useCase.removeFriendVisibility("test-api-key", "TestChar", "testfriend", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    // Wait for async callback (max 2 seconds)
    for (int i = 0; i < 200 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    
    // Verify DELETE /api/friends/:friendName/visibility was called
    auto lastDelRequest = netClient->getLastDelRequest();
    REQUIRE(lastDelRequest.url == "http://localhost:3000/api/friends/testfriend/visibility");
    REQUIRE(lastDelRequest.apiKey == "test-api-key");
    REQUIRE(lastDelRequest.characterName == "TestChar");
}

TEST_CASE("RemoveFriendVisibilityUseCase - Friend not found (idempotent)", "[App][RemoveFriendVisibility]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    // 404 response should be treated as success (idempotent delete)
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        if (url.find("/api/friends/") != std::string::npos && url.find("/visibility") != std::string::npos) {
            HttpResponse notFoundResponse;
            notFoundResponse.statusCode = 404;
            notFoundResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Friend not found"})";
            return notFoundResponse;
        }
        return { 500, "", "Unexpected URL" };
    });
    
    bool callbackCalled = false;
    RemoveFriendVisibilityResult result;
    
    useCase.removeFriendVisibility("test-api-key", "TestChar", "nonexistent", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    for (int i = 0; i < 100 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    // 404 on delete is treated as success (visibility already removed)
    REQUIRE(result.success);
}

TEST_CASE("RemoveFriendVisibilityUseCase - Missing parameters", "[App][RemoveFriendVisibility]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    bool callbackCalled = false;
    RemoveFriendVisibilityResult result;
    
    // Empty API key
    useCase.removeFriendVisibility("", "TestChar", "TestFriend", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    REQUIRE(result.error.find("required") != std::string::npos);
    
    callbackCalled = false;
    // Empty character name
    useCase.removeFriendVisibility("api-key", "", "TestFriend", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    
    callbackCalled = false;
    // Empty friend name
    useCase.removeFriendVisibility("api-key", "TestChar", "", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
}

TEST_CASE("RemoveFriendVisibilityUseCase - HTTP error", "[App][RemoveFriendVisibility]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RemoveFriendVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    // Set up server error response
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        if (url.find("/api/friends/") != std::string::npos && url.find("/visibility") != std::string::npos) {
            HttpResponse errorResponse;
            errorResponse.statusCode = 500;
            errorResponse.error = "Internal server error";
            return errorResponse;
        }
        return { 404, "", "URL not found" };
    });
    
    bool callbackCalled = false;
    RemoveFriendVisibilityResult result;
    
    useCase.removeFriendVisibility("test-api-key", "TestChar", "testfriend", 
        [&callbackCalled, &result](RemoveFriendVisibilityResult r) {
            callbackCalled = true;
            result = r;
        });
    
    // Wait for async callback with retries (max 5 seconds to account for retry delays)
    for (int i = 0; i < 500 && !callbackCalled; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    REQUIRE(callbackCalled);
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("RemoveFriendVisibilityUseCase - Use case construction", "[App][RemoveFriendVisibility]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    // Verify use case can be constructed
    RemoveFriendVisibilityUseCase useCase(*netClient, *clock, *logger);
    REQUIRE(true);
}

