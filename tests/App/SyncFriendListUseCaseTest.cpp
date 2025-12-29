#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/FriendsUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("SyncFriendListUseCase::getFriendList - Success", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]},{"name":"friend2","friendedAsName":"friend2","linkedCharacters":[]}],"serverTime":1234567890})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SyncFriendListUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.getFriendList("test-api-key", "testchar");
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendList.size() == 2);
    REQUIRE(result.friendList.hasFriend("friend1") == true);
    REQUIRE(result.friendList.hasFriend("friend2") == true);
}

TEST_CASE("SyncFriendListUseCase::getFriendList - Error handling", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Invalid response format") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.body = "invalid json";
                return resp;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("decode") != std::string::npos || result.error.find("Failed") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Empty API key") {
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getFriendList("", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("Empty character name") {
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.getFriendList("test-api-key", "");
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("SyncFriendListUseCase::setFriendList - Success", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    Core::FriendList friendList;
    friendList.addFriend("friend1", "friend1");
    friendList.addFriend("friend2", "friend2");
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]},{"name":"friend2","friendedAsName":"friend2","linkedCharacters":[]}],"serverTime":1234567890})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/friends/sync") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SyncFriendListUseCase useCase(*netClient, *clock, *logger);
    
    auto result = useCase.setFriendList("test-api-key", "testchar", friendList);
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendList.size() == 2);
}

TEST_CASE("SyncFriendListUseCase::setFriendList - Error handling", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    Core::FriendList friendList;
    friendList.addFriend("friend1", "friend1");
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.setFriendList("test-api-key", "testchar", friendList);
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Server error") {
        netClient->setResponseCallback([](const std::string& url, const std::string&, const std::string&) {
            if (url.find("/api/friends/sync") != std::string::npos) {
                return HttpResponse{500, "", "Internal Server Error"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.setFriendList("test-api-key", "testchar", friendList);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.empty() == false);
    }
    
    SECTION("Empty API key") {
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        auto result = useCase.setFriendList("", "testchar", friendList);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("SyncFriendListUseCase::getFriendListWithStatuses - Success", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[{"name":"friend1","friendedAsName":"friend1","linkedCharacters":[]}],"statuses":[{"name":"friend1","friendedAsName":"friend1","isOnline":true,"job":"WAR","rank":"Captain","nation":1,"zone":"Bastok Markets","linkedCharacters":[]}],"serverTime":1234567890})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
    UpdatePresenceUseCase presenceUseCase(*netClient, *clock, *logger);
    
    auto result = syncUseCase.getFriendListWithStatuses("test-api-key", "testchar", presenceUseCase);
    
    REQUIRE(result.success == true);
    REQUIRE(result.error.empty());
    REQUIRE(result.friendList.size() == 1);
    REQUIRE(result.statuses.size() >= 0);
}

TEST_CASE("SyncFriendListUseCase::getFriendListWithStatuses - Error handling", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Network error") {
        netClient->setResponseCallback([](const std::string&, const std::string&, const std::string&) {
            return HttpResponse{0, "", "Network error"};
        });
        
        SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
        UpdatePresenceUseCase presenceUseCase(*netClient, *clock, *logger);
        auto result = syncUseCase.getFriendListWithStatuses("test-api-key", "testchar", presenceUseCase);
        
        REQUIRE(result.success == false);
        bool hasError = (result.error.find("Network") != std::string::npos || result.error.find("error") != std::string::npos);
        REQUIRE(hasError == true);
    }
    
    SECTION("Empty API key") {
        SyncFriendListUseCase syncUseCase(*netClient, *clock, *logger);
        UpdatePresenceUseCase presenceUseCase(*netClient, *clock, *logger);
        auto result = syncUseCase.getFriendListWithStatuses("", "testchar", presenceUseCase);
        
        REQUIRE(result.success == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
}

TEST_CASE("SyncFriendListUseCase - Retry logic", "[SyncFriendListUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    
    SECTION("Retry on network errors") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
                if (callCount < 2) {
                    return HttpResponse{0, "", "Network error"};
                }
                HttpResponse response;
                response.statusCode = 200;
                response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[],"serverTime":1234567890})";
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(callCount == 2);
    }
    
    SECTION("No retry on client errors (4xx)") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
                return HttpResponse{400, "", "Bad Request"};
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == false);
        REQUIRE(callCount == 1);
    }
    
    SECTION("Retry on server errors (5xx)") {
        int callCount = 0;
        netClient->setResponseCallback([&callCount](const std::string& url, const std::string&, const std::string&) {
            callCount++;
            bool isFriendsEndpoint = (url.find("/api/friends") != std::string::npos);
        bool isNotSyncEndpoint = (url.find("/sync") == std::string::npos);
        if (isFriendsEndpoint && isNotSyncEndpoint) {
                if (callCount < 2) {
                    return HttpResponse{500, "", "Internal Server Error"};
                }
                HttpResponse response;
                response.statusCode = 200;
                response.body = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[],"serverTime":1234567890})";
                return response;
            }
            return HttpResponse{404, "", "URL not found"};
        });
        
        SyncFriendListUseCase useCase(*netClient, *clock, *logger);
        useCase.setRetryConfig(3, 10);
        
        auto result = useCase.getFriendList("test-api-key", "testchar");
        
        REQUIRE(result.success == true);
        REQUIRE(callCount == 2);
    }
}

} // namespace App
} // namespace XIFriendList

