// ConnectUseCaseTest.cpp
// Unit tests for ConnectUseCase

#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/ConnectionUseCases.h"
#include "App/State/ApiKeyState.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <memory>
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace App {
using namespace UseCases;
using namespace State;

TEST_CASE("ConnectUseCase::autoConnect loads API key from store", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    // Set up stored API key
    apiKeyState.apiKeys["testchar"] = "test-api-key-123";
    
    // Server canonical response: type=AuthEnsureResponse, fields directly in body
    HttpResponse ensureResponse;
    ensureResponse.statusCode = 200;
    ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"test-api-key-123","characterName":"testchar","accountId":1,"characterId":1})";
    // Canonical endpoint: POST /api/auth/ensure
    netClient->setResponseCallback([ensureResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/auth/ensure") != std::string::npos) {
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.autoConnect("TestChar");
    
    REQUIRE(result.success == true);
    REQUIRE(result.apiKey == "test-api-key-123");
    REQUIRE(result.username == "testchar");
    REQUIRE(useCase.isConnected() == true);
}

TEST_CASE("ConnectUseCase::autoConnect creates new account if no API key", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    // No stored API key - will use idempotent ensure endpoint
    
    // Server canonical response: type=AuthEnsureResponse, fields directly in body
    // The ensure endpoint is idempotent - creates new or returns existing
    HttpResponse ensureResponse;
    ensureResponse.statusCode = 200;
    ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"new-api-key-456","characterName":"testchar","accountId":1,"characterId":1})";
    // Canonical endpoint: POST /api/auth/ensure
    netClient->setResponseCallback([ensureResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/auth/ensure") != std::string::npos) {
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.autoConnect("TestChar");
    
    REQUIRE(result.success == true);
    REQUIRE(result.apiKey == "new-api-key-456");
    REQUIRE(result.username == "testchar");
    REQUIRE(useCase.isConnected() == true);
    
    // Verify API key was saved
    REQUIRE(apiKeyState.apiKeys.find("testchar") != apiKeyState.apiKeys.end());
    REQUIRE(apiKeyState.apiKeys["testchar"] == "new-api-key-456");
}

TEST_CASE("ConnectUseCase::autoConnect saves API key after successful ensure", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    apiKeyState.apiKeys["testchar"] = "existing-key";
    
    // Server canonical response: type=AuthEnsureResponse, fields directly in body
    HttpResponse ensureResponse;
    ensureResponse.statusCode = 200;
    ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"updated-key-789","characterName":"testchar","accountId":1,"characterId":1})";
    // Canonical endpoint: POST /api/auth/ensure
    netClient->setResponseCallback([ensureResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/auth/ensure") != std::string::npos) {
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.autoConnect("TestChar");
    
    REQUIRE(result.success == true);
    
    // Verify API key was saved if different
    REQUIRE(apiKeyState.apiKeys.find("testchar") != apiKeyState.apiKeys.end());
    REQUIRE(apiKeyState.apiKeys["testchar"] == "updated-key-789");
}

TEST_CASE("ConnectUseCase::connect - Success with API key", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    HttpResponse ensureResponse;
    ensureResponse.statusCode = 200;
    ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"provided-api-key","characterName":"testchar","accountId":1,"characterId":1})";
    
    netClient->setResponseCallback([ensureResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/auth/ensure") != std::string::npos) {
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.connect("TestChar", "provided-api-key");
    
    REQUIRE(result.success == true);
    REQUIRE(result.apiKey == "provided-api-key");
    REQUIRE(result.username == "testchar");
    REQUIRE(useCase.isConnected() == true);
}

TEST_CASE("ConnectUseCase::connect - Registration fallback", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    int callCount = 0;
    netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
        callCount++;
        if (url.find("/api/auth/ensure") != std::string::npos) {
            if (callCount == 1) {
                return HttpResponse{401, "", "Unauthorized"};
            }
            HttpResponse ensureResponse;
            ensureResponse.statusCode = 200;
            ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"new-registered-key","characterName":"testchar","accountId":1,"characterId":1})";
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.connect("TestChar", "invalid-key");
    
    REQUIRE(result.success == true);
    REQUIRE(result.apiKey == "new-registered-key");
    REQUIRE(useCase.isConnected() == true);
}

TEST_CASE("ConnectUseCase::connect - Error handling", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
        auto result = useCase.connect("TestChar", "test-key");
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
        REQUIRE(useCase.isConnected() == false);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/auth/ensure") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
        auto result = useCase.connect("TestChar", "test-key");
        
        REQUIRE(result.success == false);
        REQUIRE(useCase.isConnected() == false);
    }
    
    SECTION("Invalid response format") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/auth/ensure") != std::string::npos) {
                HttpResponse response;
                response.statusCode = 200;
                response.body = "invalid json";
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
        auto result = useCase.connect("TestChar", "test-key");
        
        REQUIRE(result.success == false);
        REQUIRE(useCase.isConnected() == false);
    }
}

TEST_CASE("ConnectUseCase::disconnect", "[ConnectUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    ApiKeyState apiKeyState;
    
    apiKeyState.apiKeys["testchar"] = "test-api-key";
    
    HttpResponse ensureResponse;
    ensureResponse.statusCode = 200;
    ensureResponse.body = R"({"protocolVersion":"2.0.0","type":"AuthEnsureResponse","success":true,"apiKey":"test-api-key","characterName":"testchar","accountId":1,"characterId":1})";
    
    netClient->setResponseCallback([ensureResponse](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/auth/ensure") != std::string::npos) {
            return ensureResponse;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    ConnectUseCase useCase(*netClient, *clock, *logger, &apiKeyState);
    
    auto result = useCase.autoConnect("TestChar");
    REQUIRE(result.success == true);
    REQUIRE(useCase.isConnected() == true);
    REQUIRE(useCase.getState() == ConnectionState::Connected);
    
    useCase.disconnect();
    
    REQUIRE(useCase.isConnected() == false);
    REQUIRE(useCase.getState() == ConnectionState::Disconnected);
}

} // namespace App
} // namespace XIFriendList

