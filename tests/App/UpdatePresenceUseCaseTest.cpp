#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "Core/ModelsCore.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <PluginVersion.h>
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("UpdatePresenceUseCase::updatePresence - Success", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    Core::Presence presence;
    presence.characterName = "testchar";
    presence.job = "WAR";
    presence.rank = "Captain";
    presence.nation = 1;
    presence.zone = "Bastok Markets";
    presence.isAnonymous = false;
    presence.timestamp = 1234567890;
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"StateUpdateResponse","success":true})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/characters/state") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.updatePresence("test-api-key", "testchar", presence);
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendStatuses.empty());
}

TEST_CASE("UpdatePresenceUseCase::updatePresence - Error handling", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    Core::Presence presence;
    presence.characterName = "testchar";
    presence.job = "WAR";
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updatePresence("test-api-key", "testchar", presence);
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/characters/state") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updatePresence("test-api-key", "testchar", presence);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Empty API key") {
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updatePresence("", "testchar", presence);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("Empty character name") {
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.updatePresence("test-api-key", "", presence);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("UpdatePresenceUseCase::getStatus - Success", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"statuses":[{"name":"friend1","friendedAsName":"friend1","isOnline":true,"zone":"Windurst","job":"WHM 75","rank":"10","linkedCharacters":[]}]})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        bool isNotRequestsEndpoint = (url.find("/requests") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint && isNotRequestsEndpoint) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.getStatus("test-api-key", "testchar");
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendStatuses.size() == 1);
    REQUIRE(result.friendStatuses[0].characterName == "friend1");
    REQUIRE(result.friendStatuses[0].isOnline == true);
}

TEST_CASE("UpdatePresenceUseCase::getStatus - Error handling", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getStatus("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
            bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
            bool isNotRequestsEndpoint = (url.find("/requests") == std::string::npos);
            if (isFriendsEndpoint && isNotSyncEndpoint && isNotRequestsEndpoint) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getStatus("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Empty API key") {
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getStatus("", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("UpdatePresenceUseCase::getHeartbeat - Success", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"HeartbeatResponse","success":true,"friends":[{"name":"friend1","friendedAsName":"friend1","isOnline":true,"zone":"Windurst","linkedCharacters":[]}],"events":[{"requestId":"req1","fromCharacterName":"friend1","toCharacterName":"testchar","status":"pending"}]})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/heartbeat") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendStatuses.size() == 1);
    REQUIRE(result.events.size() == 1);
    REQUIRE(result.events[0].requestId == "req1");
}

TEST_CASE("UpdatePresenceUseCase::getHeartbeat - Error handling", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/heartbeat") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Empty API key") {
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("", "testchar", 0);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("UpdatePresenceUseCase::parseStatusResponse", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    SECTION("StateUpdateResponse") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"protocolVersion":"2.0.0","type":"StateUpdateResponse","success":true})";
        
        auto result = useCase.parseStatusResponse(response);
        
        REQUIRE(result.success == true);
        REQUIRE(result.error.empty());
        REQUIRE(result.friendStatuses.empty());
    }
    
    SECTION("FriendsListResponse with statuses") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"statuses":[{"name":"friend1","friendedAsName":"friend1","isOnline":true,"zone":"Windurst","job":"WHM","rank":"10","linkedCharacters":[]}]})";
        
        auto result = useCase.parseStatusResponse(response);
        
        REQUIRE(result.success == true);
        REQUIRE(result.friendStatuses.size() == 1);
        REQUIRE(result.friendStatuses[0].characterName == "friend1");
        REQUIRE(result.friendStatuses[0].isOnline == true);
    }
    
    SECTION("Invalid JSON") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = "invalid json";
        
        auto result = useCase.parseStatusResponse(response);
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("decode") != std::string::npos);
        bool hasError2 = (result.error.find("Failed") != std::string::npos);
        REQUIRE((hasError1 || hasError2) == true);
    }
    
    SECTION("Server error response") {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Server error"})";
        
        auto result = useCase.parseStatusResponse(response);
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("error") != std::string::npos);
        bool hasError2 = (result.error.find("failure") != std::string::npos);
        REQUIRE((hasError1 || hasError2) == true);
    }
}

TEST_CASE("UpdatePresenceUseCase::getHeartbeat - Response parsing", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Invalid JSON in heartbeat response") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/heartbeat") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = "invalid json";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
        
        REQUIRE(result.success == false);
        bool hasError1 = (result.error.find("decode") != std::string::npos);
        bool hasError2 = (result.error.find("Failed") != std::string::npos);
        REQUIRE((hasError1 || hasError2) == true);
    }
    
    SECTION("Wrong response type in heartbeat") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/heartbeat") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true})";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Server error in heartbeat response") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/heartbeat") != std::string::npos) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = R"({"protocolVersion":"2.0.0","type":"HeartbeatResponse","success":false,"error":"Server error"})";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getHeartbeat("test-api-key", "testchar", 0);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - warns once per latest_version", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    clock->setTime(1000);
    
    HeartbeatResult result1;
    result1.isOutdated = true;
    result1.latestVersion = "1.2.0";
    result1.releaseUrl = "https://example.com/releases/latest";
    
    std::string warningMessage1;
    bool shouldWarn1 = useCase.shouldShowOutdatedWarning(result1, warningMessage1);
    
    REQUIRE(shouldWarn1 == true);
    REQUIRE(warningMessage1.find("1.2.0") != std::string::npos);
    
    HeartbeatResult result2;
    result2.isOutdated = true;
    result2.latestVersion = "1.2.0";
    
    std::string warningMessage2;
    bool shouldWarn2 = useCase.shouldShowOutdatedWarning(result2, warningMessage2);
    
    REQUIRE(shouldWarn2 == false);
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - warns again after throttle window", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    clock->setTime(1000);
    
    HeartbeatResult result;
    result.isOutdated = true;
    result.latestVersion = "1.2.0";
    
    std::string warningMessage1;
    bool shouldWarn1 = useCase.shouldShowOutdatedWarning(result, warningMessage1);
    REQUIRE(shouldWarn1 == true);
    
    clock->advance(5 * 60 * 60 * 1000);
    
    std::string warningMessage2;
    bool shouldWarn2 = useCase.shouldShowOutdatedWarning(result, warningMessage2);
    REQUIRE(shouldWarn2 == false);
    
    clock->advance(2 * 60 * 60 * 1000);
    
    std::string warningMessage3;
    bool shouldWarn3 = useCase.shouldShowOutdatedWarning(result, warningMessage3);
    REQUIRE(shouldWarn3 == true);
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - warns again if latest_version changes", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    clock->setTime(1000);
    
    HeartbeatResult result1;
    result1.isOutdated = true;
    result1.latestVersion = "1.2.0";
    
    std::string warningMessage1;
    bool shouldWarn1 = useCase.shouldShowOutdatedWarning(result1, warningMessage1);
    REQUIRE(shouldWarn1 == true);
    
    HeartbeatResult result2;
    result2.isOutdated = true;
    result2.latestVersion = "1.3.0";
    
    std::string warningMessage2;
    bool shouldWarn2 = useCase.shouldShowOutdatedWarning(result2, warningMessage2);
    REQUIRE(shouldWarn2 == true);
    REQUIRE(warningMessage2.find("1.3.0") != std::string::npos);
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - does not warn if not outdated", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    HeartbeatResult result;
    result.isOutdated = false;
    result.latestVersion = "1.2.0";
    
    std::string warningMessage;
    bool shouldWarn = useCase.shouldShowOutdatedWarning(result, warningMessage);
    
    REQUIRE(shouldWarn == false);
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - does not warn if latest_version empty", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    HeartbeatResult result;
    result.isOutdated = true;
    result.latestVersion = "";
    
    std::string warningMessage;
    bool shouldWarn = useCase.shouldShowOutdatedWarning(result, warningMessage);
    
    REQUIRE(shouldWarn == false);
}

TEST_CASE("UpdatePresenceUseCase::shouldShowOutdatedWarning - includes release URL when provided", "[UpdatePresenceUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    UpdatePresenceUseCase useCase(*netClient, *clock, *logger);
    
    clock->setTime(1000);
    
    HeartbeatResult result;
    result.isOutdated = true;
    result.latestVersion = "1.2.0";
    result.releaseUrl = "https://github.com/owner/repo/releases/latest";
    
    std::string warningMessage;
    bool shouldWarn = useCase.shouldShowOutdatedWarning(result, warningMessage);
    
    REQUIRE(shouldWarn == true);
    REQUIRE(warningMessage.find("1.2.0") != std::string::npos);
    REQUIRE(warningMessage.find("https://github.com/owner/repo/releases/latest") != std::string::npos);
}

} // namespace App
} // namespace XIFriendList

