#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("UpdateMyStatusUseCase - Success", "[UpdateMyStatusUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"PreferencesUpdateResponse","success":true})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/characters/privacy") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.updateStatus("test-api-key", "testchar", true, true, false, false);
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
}

TEST_CASE("UpdateMyStatusUseCase - Parameter validation", "[UpdateMyStatusUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
    
    SECTION("Empty API key") {
        auto result = useCase.updateStatus("", "testchar", true, true, false, false);
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("Empty character name") {
        auto result = useCase.updateStatus("test-api-key", "", true, true, false, false);
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("UpdateMyStatusUseCase - HTTP error", "[UpdateMyStatusUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updateStatus("test-api-key", "testchar", true, true, false, false);
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("Network") != std::string::npos);
        bool hasError2 = (result.error.find("error") != std::string::npos);
        REQUIRE((hasError1 || hasError2) == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/characters/privacy") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updateStatus("test-api-key", "testchar", true, true, false, false);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
}

TEST_CASE("UpdateMyStatusUseCase - Invalid response format", "[UpdateMyStatusUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/characters/privacy") != std::string::npos) {
            HttpResponse resp;
            resp.statusCode = 200;
            resp.body = "invalid json";
            return resp;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
    auto result = useCase.updateStatus("test-api-key", "testchar", true, true, false, false);
    
    REQUIRE(result.success == false);
    bool hasError1 = (result.error.find("decode") != std::string::npos);
    bool hasError2 = (result.error.find("Failed") != std::string::npos);
    REQUIRE((hasError1 || hasError2) == true);
}

TEST_CASE("UpdateMyStatusUseCase - Flag encoding", "[UpdateMyStatusUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"PreferencesUpdateResponse","success":true})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/characters/privacy") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdateMyStatusUseCase useCase(*netClient, *clock, *logger);
    
    SECTION("All flags true") {
        auto result = useCase.updateStatus("test-api-key", "testchar", true, true, true, true);
        REQUIRE(result.success == true);
        auto lastRequest = netClient->getLastPostRequest();
        REQUIRE(lastRequest.body.find("shareOnlineStatus") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareLocation") != std::string::npos);
        REQUIRE(lastRequest.body.find("isAnonymous") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareJobWhenAnonymous") != std::string::npos);
    }
    
    SECTION("All flags false") {
        auto result = useCase.updateStatus("test-api-key", "testchar", false, false, false, false);
        REQUIRE(result.success == true);
        auto lastRequest = netClient->getLastPostRequest();
        REQUIRE(lastRequest.body.find("shareOnlineStatus") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareLocation") != std::string::npos);
        REQUIRE(lastRequest.body.find("isAnonymous") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareJobWhenAnonymous") != std::string::npos);
    }
    
    SECTION("Mixed flags") {
        auto result = useCase.updateStatus("test-api-key", "testchar", true, false, true, false);
        REQUIRE(result.success == true);
        auto lastRequest = netClient->getLastPostRequest();
        REQUIRE(lastRequest.body.find("shareOnlineStatus") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareLocation") != std::string::npos);
        REQUIRE(lastRequest.body.find("isAnonymous") != std::string::npos);
        REQUIRE(lastRequest.body.find("shareJobWhenAnonymous") != std::string::npos);
    }
}

} // namespace App
} // namespace XIFriendList

