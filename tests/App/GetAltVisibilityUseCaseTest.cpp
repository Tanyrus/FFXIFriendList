#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("GetAltVisibilityUseCase - Success", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"success":true,"friends":[{"friendAccountId":1,"friendedAsName":"friend1","displayName":"Friend One","visibilityMode":"mutual","createdAt":1000,"updatedAt":2000,"characterVisibility":{"1":{"characterName":"char1","hasVisibility":true,"hasPendingVisibilityRequest":false}}}],"characters":[{"characterId":1,"characterName":"char1","isActive":true}],"serverTime":1234567890})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/visibility") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.getVisibility("test-api-key", "testchar");
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friends.size() == 1);
    REQUIRE(result.characters.size() == 1);
    REQUIRE(result.serverTime == 1234567890);
    REQUIRE(result.friends[0].friendedAsName == "friend1");
}

TEST_CASE("GetAltVisibilityUseCase - Parameter validation", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
    
    SECTION("Empty API key") {
        auto result = useCase.getVisibility("", "testchar");
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("Empty character name") {
        auto result = useCase.getVisibility("test-api-key", "");
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("GetAltVisibilityUseCase - HTTP error", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("Network") != std::string::npos);
        bool hasError2 = (result.error.find("error") != std::string::npos);
        bool hasError = (hasError1 || hasError2);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
}

TEST_CASE("GetAltVisibilityUseCase - Invalid response format", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Invalid JSON") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = "invalid json";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Missing friends array") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = R"({"success":true})";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("friends") != std::string::npos);
        bool hasError2 = (result.error.find("missing") != std::string::npos);
        bool hasError = (hasError1 || hasError2);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error response") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = R"({"success":false,"error":"Server error"})";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("error") != std::string::npos);
        bool hasError2 = (result.error.find("false") != std::string::npos);
        bool hasError = (hasError1 || hasError2);
        REQUIRE(hasError == true);
    }
}

TEST_CASE("GetAltVisibilityUseCase - Response parsing", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Friend entries parsed correctly") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"friends":[{"friendAccountId":1,"friendedAsName":"friend1","displayName":"Friend One","visibilityMode":"mutual","createdAt":1000,"updatedAt":2000}],"serverTime":1234567890})";
        
        netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(result.friends.size() == 1);
        REQUIRE(result.friends[0].friendAccountId == 1);
        REQUIRE(result.friends[0].friendedAsName == "friend1");
        REQUIRE(result.friends[0].displayName == "Friend One");
        REQUIRE(result.friends[0].visibilityMode == "mutual");
    }
    
    SECTION("Character info parsed correctly") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"friends":[],"characters":[{"characterId":1,"characterName":"char1","isActive":true},{"characterId":2,"characterName":"char2","isActive":false}],"serverTime":1234567890})";
        
        netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(result.characters.size() == 2);
        REQUIRE(result.characters[0].characterId == 1);
        REQUIRE(result.characters[0].characterName == "char1");
        REQUIRE(result.characters[0].isActive == true);
        REQUIRE(result.characters[1].characterId == 2);
        REQUIRE(result.characters[1].isActive == false);
    }
    
    SECTION("Empty characters array creates default character") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"friends":[],"serverTime":1234567890})";
        
        netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/visibility") != std::string::npos) {
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(result.characters.size() == 1);
        REQUIRE(result.characters[0].characterName == "testchar");
        REQUIRE(result.characters[0].isActive == true);
    }
}

TEST_CASE("GetAltVisibilityUseCase - Retry logic", "[GetAltVisibilityUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Retry on network errors") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            if (url.find("/api/friends/visibility") != std::string::npos) {
                if (callCount < 2) {
                    return HttpResponse{0, "", "Network error"};
                }
                HttpResponse response;
                response.statusCode = 200;
                response.body = R"({"success":true,"friends":[],"serverTime":1234567890})";
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(callCount >= 2);
    }
    
    SECTION("No retry on client errors (4xx)") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            if (url.find("/api/friends/visibility") != std::string::npos) {
                return HttpResponse{400, "", "Bad Request"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        GetAltVisibilityUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.getVisibility("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(callCount == 4);
    }
}

} // namespace App
} // namespace XIFriendList

