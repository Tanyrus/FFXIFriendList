// GetFriendRequestsUseCaseTest.cpp
// Unit tests for GetFriendRequestsUseCase

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;
using namespace XIFriendList::Protocol;

TEST_CASE("GetFriendRequestsUseCase - Success with incoming and outgoing", "[App][FriendRequests]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetFriendRequestsUseCase useCase(*netClient, *clock, *logger);
    
    // Set up successful response (server canonical format)
    // Server sends incoming/outgoing directly in response body
    std::string responseBody = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendRequestsResponse",
        "success": true,
        "incoming": [{"requestId":"req1","fromCharacterName":"requester1","toCharacterName":"currentuser","fromAccountId":"1","toAccountId":"2","status":"pending","createdAt":1000}],
        "outgoing": [{"requestId":"req2","fromCharacterName":"currentuser","toCharacterName":"targetuser","fromAccountId":"2","toAccountId":"3","status":"pending","createdAt":2000}]
    })";
    
    netClient->setResponse("http://localhost:3000/api/friends/requests", 
                          { 200, responseBody, "" });
    
    auto result = useCase.getRequests("test-api-key", "currentuser");
    
    REQUIRE(result.success);
    REQUIRE(result.error.empty());
    REQUIRE(result.incoming.size() == 1);
    REQUIRE(result.incoming[0].requestId == "req1");
    REQUIRE(result.incoming[0].fromCharacterName == "requester1");
    REQUIRE(result.outgoing.size() == 1);
    REQUIRE(result.outgoing[0].requestId == "req2");
    REQUIRE(result.outgoing[0].toCharacterName == "targetuser");
}

TEST_CASE("GetFriendRequestsUseCase - Empty requests", "[App][FriendRequests]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetFriendRequestsUseCase useCase(*netClient, *clock, *logger);
    
    // Server canonical format: sends incoming/outgoing directly
    std::string responseBody = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendRequestsResponse",
        "success": true,
        "incoming": [],
        "outgoing": []
    })";
    
    netClient->setResponse("http://localhost:3000/api/friends/requests", 
                          { 200, responseBody, "" });
    
    auto result = useCase.getRequests("test-api-key", "currentuser");
    
    REQUIRE(result.success);
    REQUIRE(result.incoming.empty());
    REQUIRE(result.outgoing.empty());
}

TEST_CASE("GetFriendRequestsUseCase - HTTP error", "[App][FriendRequests]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetFriendRequestsUseCase useCase(*netClient, *clock, *logger);
    
    netClient->setResponse("http://localhost:3000/api/friends/requests", 
                          { 500, "", "Internal server error" });
    
    auto result = useCase.getRequests("test-api-key", "currentuser");
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.incoming.empty());
    REQUIRE(result.outgoing.empty());
}

TEST_CASE("GetFriendRequestsUseCase - Invalid response format", "[App][FriendRequests]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetFriendRequestsUseCase useCase(*netClient, *clock, *logger);
    
    netClient->setResponse("http://localhost:3000/api/friends/requests", 
                          { 200, "invalid json", "" });
    
    auto result = useCase.getRequests("test-api-key", "currentuser");
    
    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("GetFriendRequestsUseCase - Missing API key or character name", "[App][FriendRequests]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetFriendRequestsUseCase useCase(*netClient, *clock, *logger);
    
    // Missing API key
    auto result1 = useCase.getRequests("", "currentuser");
    REQUIRE_FALSE(result1.success);
    REQUIRE(result1.error.find("API key") != std::string::npos);
    
    // Missing character name
    auto result2 = useCase.getRequests("test-api-key", "");
    REQUIRE_FALSE(result2.success);
    REQUIRE(result2.error.find("character name") != std::string::npos);
}

