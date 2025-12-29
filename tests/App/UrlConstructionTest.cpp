// UrlConstructionTest.cpp
// Tests for URL construction and base URL handling

#include <catch2/catch_test_macros.hpp>
#include "FakeNetClient.h"
#include "App/UseCases/ConnectionUseCases.h"
#include "App/UseCases/FriendsUseCases.h"
#include "FakeClock.h"
#include "FakeLogger.h"

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;

TEST_CASE("Base URL defaults and can be changed", "[url]") {
    FakeNetClient netClient;
    
    // FakeNetClient defaults to localhost for testing (AshitaNetClient defaults to production)
    REQUIRE(netClient.getBaseUrl() == "http://localhost:3000");
    
    // Can be changed to production server URL
    netClient.setBaseUrl("https://api.horizonfriendlist.com");
    REQUIRE(netClient.getBaseUrl() == "https://api.horizonfriendlist.com");
}

TEST_CASE("URLs are constructed correctly with base URL", "[url]") {
    FakeNetClient netClient;
    FakeClock clock;
    FakeLogger logger;
    
    // Set production base URL
    netClient.setBaseUrl("https://api.horizonfriendlist.com");
    
    SECTION("ConnectUseCase constructs correct auth URLs") {
        ConnectUseCase useCase(netClient, clock, logger, nullptr);
        
        // Connect with empty API key uses the idempotent /api/auth/ensure endpoint
        netClient.setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            // Canonical endpoint: POST /api/auth/ensure for both register and login
            if (url.find("/api/auth/ensure") != std::string::npos) {
                return HttpResponse{200, R"({"protocolVersion":"2.0.0","type":"Presence","success":true,"payload":"{\"apiKey\":\"test-key\",\"accountId\":1,\"characterId\":1}"})", ""};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        auto result = useCase.connect("TestUser", "");
        // Verify canonical /api/auth/ensure endpoint was called
        auto lastRequest = netClient.getLastPostRequest();
        REQUIRE(lastRequest.url.find("/api/auth/ensure") != std::string::npos);
    }
    
    SECTION("SyncFriendListUseCase constructs correct friend list URL") {
        SyncFriendListUseCase useCase(netClient, clock, logger);
        
        // Canonical endpoint: GET /api/friends
        netClient.setResponse("https://api.horizonfriendlist.com/api/friends", { 200, R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[]})", "" });
        useCase.getFriendList("apiKey123", "TestUser");
        
        auto lastRequest = netClient.getLastGetRequest();
        REQUIRE(lastRequest.url == "https://api.horizonfriendlist.com/api/friends");
    }
    
}

TEST_CASE("URL construction handles paths correctly", "[url]") {
    FakeNetClient netClient;
    
    SECTION("Base URL with trailing slash") {
        netClient.setBaseUrl("https://api.horizonfriendlist.com/");
        
        // URLs should still be constructed correctly
        REQUIRE(netClient.getBaseUrl() == "https://api.horizonfriendlist.com/");
    }
    
    SECTION("Base URL without trailing slash") {
        netClient.setBaseUrl("https://api.horizonfriendlist.com");
        
        REQUIRE(netClient.getBaseUrl() == "https://api.horizonfriendlist.com");
    }
}

TEST_CASE("URL construction includes required headers", "[url]") {
    FakeNetClient netClient;
    FakeClock clock;
    FakeLogger logger;
    
    netClient.setBaseUrl("https://api.horizonfriendlist.com");
    
    SECTION("API key is included in requests") {
        SyncFriendListUseCase useCase(netClient, clock, logger);
        
        // Canonical endpoint: GET /api/friends
        netClient.setResponse("https://api.horizonfriendlist.com/api/friends", { 200, R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[]})", "" });
        useCase.getFriendList("test-api-key-123", "TestUser");
        
        auto lastRequest = netClient.getLastGetRequest();
        REQUIRE(lastRequest.apiKey == "test-api-key-123");
        REQUIRE(lastRequest.characterName == "TestUser");
    }
    
}

