// SendFriendRequestUseCaseTest.cpp
// Unit tests for SendFriendRequestUseCase

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include <memory>

using namespace XIFriendList::App;
using namespace XIFriendList::App::UseCases;

TEST_CASE("SendFriendRequestUseCase - Success", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up successful response (server canonical format)
    HttpResponse successResponse;
    successResponse.statusCode = 200;
    // Server sends requestId directly in response
    successResponse.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"requestId":"req123","message":"Friend request sent successfully"})";
    
    netClient->setResponseCallback([successResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return successResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.errorCode.empty());
    REQUIRE(result.userMessage.empty());
    REQUIRE(result.requestId == "req123");
    
    auto lastRequest = netClient->getLastPostRequest();
    REQUIRE(lastRequest.url == "http://localhost:3000/api/friends/requests/request");
    REQUIRE(lastRequest.apiKey == "test-api-key");
    REQUIRE(lastRequest.characterName == "TestChar");
    REQUIRE(lastRequest.body.find("TargetUser") != std::string::npos);
}

TEST_CASE("SendFriendRequestUseCase - Friend not found (404)", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    // Disable retries for faster test
    useCase.setRetryConfig(0, 0);
    
    // Set up 400 error response with FRIEND_TARGET_NOT_FOUND error code
    HttpResponse errorResponse;
    errorResponse.statusCode = 400;
    errorResponse.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"errorCode":"FRIEND_TARGET_NOT_FOUND","error":"User not found. They may not have the addon installed.","requestId":null})";
    
    netClient->setResponseCallback([errorResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return errorResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "NonExistentUser");
    
    // Must complete without blocking (test should finish quickly)
    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorCode == "FRIEND_TARGET_NOT_FOUND");
    REQUIRE(result.userMessage == "User not found. They may not have the addon installed.");
    REQUIRE(result.requestId.empty());
    REQUIRE_FALSE(result.debugMessage.empty());
    REQUIRE(result.debugMessage.find("HTTP 400") != std::string::npos);
}

TEST_CASE("SendFriendRequestUseCase - Network error", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    // Disable retries for faster test
    useCase.setRetryConfig(0, 0);
    
    // Set up network error (statusCode == 0)
    HttpResponse networkError;
    networkError.statusCode = 0;
    networkError.error = "Connection timeout";
    
    netClient->setResponseCallback([networkError](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return networkError;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorCode.empty());
    REQUIRE(result.userMessage == "Connection timeout");
    REQUIRE(result.requestId.empty());
}

TEST_CASE("SendFriendRequestUseCase - Missing parameters", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Empty API key
    SendFriendRequestResult result = useCase.sendRequest("", "TestChar", "TargetUser");
    REQUIRE_FALSE(result.success);
    REQUIRE(result.userMessage.find("required") != std::string::npos);
    
    // Empty character name
    result = useCase.sendRequest("api-key", "", "TargetUser");
    REQUIRE_FALSE(result.success);
    
    // Empty target user ID
    result = useCase.sendRequest("api-key", "TestChar", "");
    REQUIRE_FALSE(result.success);
}

TEST_CASE("SendFriendRequestUseCase - Invalid response format", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    // Disable retries for faster test
    useCase.setRetryConfig(0, 0);
    
    // Set up invalid JSON response
    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "not valid json";
    
    netClient->setResponseCallback([invalidResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return invalidResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE_FALSE(result.success);
    REQUIRE(result.userMessage == "Invalid response format");
    REQUIRE(result.requestId.empty());
}

TEST_CASE("SendFriendRequestUseCase - Action: PENDING_ACCEPT", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up response with PENDING_ACCEPT action
    // ResponseDecoder will synthesize payload from top-level fields
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"action":"PENDING_ACCEPT","requestId":"req456","message":"Friend request sent to TargetUser."})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.action == "PENDING_ACCEPT");
    REQUIRE(result.message == "Friend request sent to TargetUser.");
    REQUIRE(result.requestId == "req456");
}

TEST_CASE("SendFriendRequestUseCase - Action: ALREADY_VISIBLE", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up response with ALREADY_VISIBLE action
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"action":"ALREADY_VISIBLE","message":"Already friends with TargetUser."})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.action == "ALREADY_VISIBLE");
    REQUIRE(result.message == "Already friends with TargetUser.");
    REQUIRE(result.requestId.empty()); // No requestId for ALREADY_VISIBLE
}

TEST_CASE("SendFriendRequestUseCase - Action: VISIBILITY_GRANTED", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up response with VISIBILITY_GRANTED action
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"action":"VISIBILITY_GRANTED","message":"Visibility granted for TargetUser."})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.action == "VISIBILITY_GRANTED");
    REQUIRE(result.message == "Visibility granted for TargetUser.");
    REQUIRE(result.requestId.empty()); // No requestId for VISIBILITY_GRANTED
}

TEST_CASE("SendFriendRequestUseCase - Action: VISIBILITY_REQUEST_SENT", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up response with VISIBILITY_REQUEST_SENT action
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"action":"VISIBILITY_REQUEST_SENT","requestId":"vis-req789","message":"Visibility request sent to TargetUser."})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.action == "VISIBILITY_REQUEST_SENT");
    REQUIRE(result.message == "Visibility request sent to TargetUser.");
    REQUIRE(result.requestId == "vis-req789");
}

TEST_CASE("SendFriendRequestUseCase - Action field in payload (JSON-encoded)", "[App][SendFriendRequest]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SendFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    // Set up response with action in payload (double-encoded JSON string)
    HttpResponse response;
    response.statusCode = 200;
    // Payload is a JSON-encoded string containing the actual JSON object
    response.body = R"({"protocolVersion":"2.0.0","type":"SendFriendRequestResponse","success":true,"payload":"{\"action\":\"PENDING_ACCEPT\",\"requestId\":\"req999\",\"message\":\"Friend request sent.\"}"})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/request") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SendFriendRequestResult result = useCase.sendRequest("test-api-key", "TestChar", "TargetUser");
    
    REQUIRE(result.success);
    REQUIRE(result.action == "PENDING_ACCEPT");
    REQUIRE(result.message == "Friend request sent.");
    REQUIRE(result.requestId == "req999");
}

