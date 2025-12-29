#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("RejectFriendRequestUseCase - Success", "[RejectFriendRequestUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"RejectFriendRequestResponse","success":true})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/reject") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
    
    REQUIRE(result.success == true);
    REQUIRE(result.errorCode.empty());
    REQUIRE(result.userMessage == "Request rejected.");
}

TEST_CASE("RejectFriendRequestUseCase - Request ID validation", "[RejectFriendRequestUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
    
    SECTION("Empty request ID") {
        auto result = useCase.rejectRequest("test-api-key", "testchar", "");
        REQUIRE(result.success == false);
        REQUIRE(result.userMessage.find("required") != std::string::npos);
    }
    
    SECTION("Empty API key") {
        auto result = useCase.rejectRequest("", "testchar", "request123");
        REQUIRE(result.success == false);
        REQUIRE(result.userMessage.find("required") != std::string::npos);
    }
    
    SECTION("Empty character name") {
        auto result = useCase.rejectRequest("test-api-key", "", "request123");
        REQUIRE(result.success == false);
        REQUIRE(result.userMessage.find("required") != std::string::npos);
    }
}

TEST_CASE("RejectFriendRequestUseCase - HTTP error", "[RejectFriendRequestUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
        
        REQUIRE(result.success == false);
        bool hasError = (result.userMessage.find("Network") != std::string::npos || result.userMessage.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/requests/reject") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
        
        REQUIRE(result.success == false);
        REQUIRE(result.userMessage.empty() == false);
    }
    
    SECTION("Client error with error code") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/requests/reject") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 400;
                resp.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Request not found","errorCode":"REQUEST_NOT_FOUND"})";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
        
        REQUIRE(result.success == false);
        REQUIRE(result.errorCode == "REQUEST_NOT_FOUND");
        REQUIRE(result.userMessage.find("not found") != std::string::npos);
    }
}

TEST_CASE("RejectFriendRequestUseCase - Invalid response format", "[RejectFriendRequestUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/requests/reject") != std::string::npos) {
            HttpResponse resp;
            resp.statusCode = 200;
            resp.body = "invalid json";
            return resp;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
    auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
    
    REQUIRE(result.success == false);
    REQUIRE(result.userMessage == "Invalid response format");
}

TEST_CASE("RejectFriendRequestUseCase - Retry logic", "[RejectFriendRequestUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Retry on network errors") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            if (url.find("/api/friends/requests/reject") != std::string::npos) {
                if (callCount < 2) {
                    return HttpResponse{0, "", "Network error"};
                }
                HttpResponse response;
                response.statusCode = 200;
                response.body = R"({"protocolVersion":"2.0.0","type":"RejectFriendRequestResponse","success":true})";
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
        
        REQUIRE(result.success == true);
        REQUIRE(callCount >= 2);
    }
    
    SECTION("No retry on client errors (4xx)") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            if (url.find("/api/friends/requests/reject") != std::string::npos) {
                return HttpResponse{400, "", "Bad Request"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        RejectFriendRequestUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.rejectRequest("test-api-key", "testchar", "request123");
        
        REQUIRE(result.success == false);
        REQUIRE(callCount == 4);
    }
}

} // namespace App
} // namespace XIFriendList

