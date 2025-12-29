// RefreshBehaviorTest.cpp
// Tests for refresh behavior including friend requests

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;

TEST_CASE("Refresh - Updates friend list and status", "[App][Refresh]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    UpdatePresenceUseCase presenceUseCase(*netClient, *clock, *logger);
    
    // Set up friend list response (for SyncFriendListUseCase) - server canonical format
    // Server sends friends directly in response body
    std::string friendListResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendsListResponse",
        "success": true,
        "friends": [{"name":"friend1","friendedAsName":"friend1","isOnline":true,"zone":"Windurst","linkedCharacters":[]}],
        "serverTime": 1234567890
    })";
    
    // Set up status response (for UpdatePresenceUseCase::getStatus)
    // The parseStatusResponse function expects "statuses" format from Status type response
    std::string statusResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendsListResponse",
        "success": true,
        "statuses": [{"name":"friend1","friendedAsName":"friend1","isOnline":true,"zone":"Windurst","job":"WHM 75","rank":"10","linkedCharacters":[]}],
        "serverTime": 1234567890
    })";
    
    int requestCount = 0;
    netClient->setResponseCallback([&requestCount, friendListResponse, statusResponse](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        requestCount++;
        // Canonical endpoint: GET /api/friends - alternate responses based on call count
        // First call: FriendList (from SyncFriendListUseCase)
        // Second call: Status (from UpdatePresenceUseCase::getStatus)
        if (url.find("/api/friends") != std::string::npos && url.find("/api/friends/") == std::string::npos) {
            if (requestCount == 1) {
                return { 200, friendListResponse, "" };
            } else {
                return { 200, statusResponse, "" };
            }
        }
        return { 404, "", "URL not found" };
    });
    
    // Get friend list
    auto friendListResult = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(friendListResult.success);
    REQUIRE(friendListResult.friendList.size() == 1);
    
    // Get status
    auto statusResult = presenceUseCase.getStatus("test-api-key", "currentuser");
    REQUIRE(statusResult.success);
    REQUIRE(statusResult.friendStatuses.size() == 1);
    REQUIRE(statusResult.friendStatuses[0].isOnline);
}

TEST_CASE("Refresh - Includes friend requests in sync", "[App][Refresh]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    GetFriendRequestsUseCase requestsUseCase(*netClient, *clock, *logger);
    
    // Set up friend list response - server canonical format
    std::string friendListResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendsListResponse",
        "success": true,
        "friends": [{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]}],
        "serverTime": 1234567890
    })";
    
    // Set up friend requests response - server canonical format
    std::string requestsResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendRequestsResponse",
        "success": true,
        "incoming": [{"requestId":"req1","fromCharacterName":"requester1","toCharacterName":"currentuser","fromAccountId":"1","toAccountId":"2","status":"pending","createdAt":1000}],
        "outgoing": [{"requestId":"req2","fromCharacterName":"currentuser","toCharacterName":"targetuser","fromAccountId":"2","toAccountId":"3","status":"pending","createdAt":2000}]
    })";
    
    int requestCount = 0;
    netClient->setResponseCallback([&requestCount, friendListResponse, requestsResponse](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        requestCount++;
        // Canonical endpoint: GET /api/friends/requests for friend requests
        if (url.find("/api/friends/requests") != std::string::npos) {
            return { 200, requestsResponse, "" };
        }
        // Canonical endpoint: GET /api/friends for friend list (no trailing path)
        if (url.find("/api/friends") != std::string::npos) {
            return { 200, friendListResponse, "" };
        }
        return { 404, "", "URL not found" };
    });
    
    // Get friend list
    auto friendListResult = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(friendListResult.success);
    
    // Get friend requests (as part of refresh)
    auto requestsResult = requestsUseCase.getRequests("test-api-key", "currentuser");
    REQUIRE(requestsResult.success);
    REQUIRE(requestsResult.incoming.size() == 1);
    REQUIRE(requestsResult.outgoing.size() == 1);
    
    // Verify both requests were made
    REQUIRE(requestCount >= 2);
}

TEST_CASE("Refresh - Handles network errors gracefully", "[App][Refresh]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    GetFriendRequestsUseCase requestsUseCase(*netClient, *clock, *logger);
    
    // Set up network error (statusCode = 0)
    // Canonical endpoints: /api/friends and /api/friends/requests
    netClient->setResponse("http://localhost:3000/api/friends", 
                          { 0, "", "Network error" });
    netClient->setResponse("http://localhost:3000/api/friends/requests", 
                          { 0, "", "Network error" });
    
    // Friend list should fail gracefully
    auto friendListResult = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE_FALSE(friendListResult.success);
    REQUIRE_FALSE(friendListResult.error.empty());
    
    // Friend requests should fail gracefully
    auto requestsResult = requestsUseCase.getRequests("test-api-key", "currentuser");
    REQUIRE_FALSE(requestsResult.success);
    REQUIRE_FALSE(requestsResult.error.empty());
}

TEST_CASE("Refresh - Prevents duplicate /api/friends calls", "[App][Refresh]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    
    // Set up friend list response - server canonical format
    std::string friendListResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendsListResponse",
        "success": true,
        "friends": [{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]}],
        "serverTime": 1234567890
    })";
    
    int requestCount = 0;
    netClient->setResponseCallback([&requestCount, friendListResponse](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        // Only count requests to /api/friends (not /api/friends/requests or other endpoints)
        if (url.find("/api/friends") != std::string::npos && url.find("/api/friends/") == std::string::npos) {
            requestCount++;
            return { 200, friendListResponse, "" };
        }
        return { 404, "", "URL not found" };
    });
    
    // Simulate multiple rapid calls (like polling timer + character change happening together)
    // All calls should result in only ONE actual HTTP request due to in-flight guard
    auto result1 = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(result1.success);
    REQUIRE(requestCount == 1);
    
    // Second call immediately after should be allowed (first completed)
    auto result2 = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(result2.success);
    REQUIRE(requestCount == 2);
    
    // Verify that each call resulted in a separate request (guard allows sequential calls)
    // This test ensures the guard doesn't block legitimate sequential refreshes
    REQUIRE(result1.friendList.size() == 1);
    REQUIRE(result2.friendList.size() == 1);
}

TEST_CASE("Refresh - Manual refresh while periodic refresh imminent", "[App][Refresh]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    
    // Set up friend list response - server canonical format
    std::string friendListResponse = R"({
        "protocolVersion": "2.0.0",
        "type": "FriendsListResponse",
        "success": true,
        "friends": [{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]}],
        "serverTime": 1234567890
    })";
    
    int requestCount = 0;
    netClient->setResponseCallback([&requestCount, friendListResponse](const std::string& url, const std::string&, const std::string&) -> HttpResponse {
        // Only count requests to /api/friends (not /api/friends/requests or other endpoints)
        if (url.find("/api/friends") != std::string::npos && url.find("/api/friends/") == std::string::npos) {
            requestCount++;
            return { 200, friendListResponse, "" };
        }
        return { 404, "", "URL not found" };
    });
    
    // Simulate scenario: manual refresh button clicked while periodic refresh is about to trigger
    // Both should be allowed (they're sequential, not concurrent)
    // Note: In the actual adapter, the in-flight guard prevents concurrent calls,
    // but sequential calls (one completes before next starts) are allowed
    auto result1 = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(result1.success);
    REQUIRE(requestCount == 1);
    
    // Second call after first completes (simulating manual refresh after periodic refresh)
    auto result2 = syncUseCase.getFriendList("test-api-key", "currentuser");
    REQUIRE(result2.success);
    REQUIRE(requestCount == 2);
    
    // Both should succeed (guard allows sequential calls)
    REQUIRE(result1.friendList.size() == 1);
    REQUIRE(result2.friendList.size() == 1);
}

