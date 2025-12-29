#include <catch2/catch_test_macros.hpp>
#include "App/UseCases/PreferencesUseCases.h"
#include "FakeNetClient.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include "FakePreferencesStore.h"
#include <memory>

namespace XIFriendList {
namespace App {
using namespace UseCases;

TEST_CASE("PreferencesUseCase - Initial state", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    
    REQUIRE(useCase.isLoaded() == false);
    
    Core::Preferences prefs = useCase.getPreferences();
    REQUIRE(prefs.useServerNotes == false);
    REQUIRE(prefs.shareFriendsAcrossAlts == true);
    REQUIRE(prefs.debugMode == false);
    REQUIRE(prefs.showOnlineStatus == true);
}

TEST_CASE("PreferencesUseCase - Load preferences", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    Core::Preferences storedServerPrefs;
    storedServerPrefs.useServerNotes = true;
    storedServerPrefs.shareFriendsAcrossAlts = false;
    preferencesStore->serverPreferences_ = storedServerPrefs;
    
    Core::Preferences storedLocalPrefs;
    storedLocalPrefs.debugMode = true;
    storedLocalPrefs.showOnlineStatus = false;
    preferencesStore->localPreferences_ = storedLocalPrefs;
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    
    useCase.loadPreferences();
    
    REQUIRE(useCase.isLoaded() == true);
    REQUIRE(useCase.getServerPreferences().useServerNotes == true);
    REQUIRE(useCase.getServerPreferences().shareFriendsAcrossAlts == false);
    REQUIRE(useCase.getLocalPreferences().debugMode == true);
    REQUIRE(useCase.getLocalPreferences().showOnlineStatus == false);
}

TEST_CASE("PreferencesUseCase - Save preferences", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    
    useCase.loadPreferences();
    
    Core::Preferences prefs = useCase.getPreferences();
    prefs.useServerNotes = true;
    prefs.debugMode = true;
    
    useCase.updateServerPreferences(prefs, "", "");
    useCase.updateLocalPreferences(prefs);
    useCase.savePreferences();
    
    REQUIRE(preferencesStore->serverPreferences_.useServerNotes == true);
    REQUIRE(preferencesStore->localPreferences_.debugMode == true);
}

TEST_CASE("PreferencesUseCase - Update server preference (bool)", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    auto result = useCase.updateServerPreference("useServerNotes", true);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getServerPreferences().useServerNotes == true);
    
    result = useCase.updateServerPreference("shareFriendsAcrossAlts", false);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getServerPreferences().shareFriendsAcrossAlts == false);
    
    result = useCase.updateServerPreference("unknownField", true);
    REQUIRE(result.success == false);
    REQUIRE(result.error.find("Unknown") != std::string::npos);
}

TEST_CASE("PreferencesUseCase - Update server preference (string)", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    auto result = useCase.updateServerPreference("unknownField", "value");
    REQUIRE(result.success == false);
    REQUIRE(result.error.find("Unknown") != std::string::npos);
}

TEST_CASE("PreferencesUseCase - Update local preference (bool)", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    auto result = useCase.updateLocalPreference("debugMode", true);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getLocalPreferences().debugMode == true);
    
    result = useCase.updateLocalPreference("showOnlineStatus", false);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getLocalPreferences().showOnlineStatus == false);
    
    result = useCase.updateLocalPreference("unknownField", true);
    REQUIRE(result.success == false);
    REQUIRE(result.error.find("Unknown") != std::string::npos);
}

TEST_CASE("PreferencesUseCase - Update local preference (float)", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    auto result = useCase.updateLocalPreference("notificationDuration", 10.5f);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getLocalPreferences().notificationDuration == 10.5f);
    
    result = useCase.updateLocalPreference("notificationSoundVolume", 0.8f);
    REQUIRE(result.success == true);
    REQUIRE(useCase.getLocalPreferences().notificationSoundVolume == 0.8f);
    
    result = useCase.updateLocalPreference("unknownField", 1.0f);
    REQUIRE(result.success == false);
    REQUIRE(result.error.find("Unknown") != std::string::npos);
}

TEST_CASE("PreferencesUseCase - Sync from server", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    HttpResponse response;
    response.statusCode = 200;
    response.body = R"({"protocolVersion":"2.0.0","type":"PreferencesResponse","success":true,"payload":"{\"useServerNotes\":true,\"shareFriendsAcrossAlts\":false}"})";
    
    netClient->setResponseCallback([response](const std::string& url, const std::string&, const std::string&) {
        if (url.find("/api/preferences") != std::string::npos) {
            return response;
        }
        return HttpResponse{404, "", "URL not found"};
    });
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    
    auto result = useCase.syncFromServer("test-api-key", "testchar");
    REQUIRE(result.success == true);
    REQUIRE(useCase.getServerPreferences().useServerNotes == true);
    REQUIRE(useCase.getServerPreferences().shareFriendsAcrossAlts == false);
}

TEST_CASE("PreferencesUseCase - Merge logic", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    Core::Preferences serverPrefs;
    serverPrefs.useServerNotes = true;
    serverPrefs.shareFriendsAcrossAlts = false;
    serverPrefs.quickOnlineFriendView.showJob = true;
    useCase.updateServerPreferences(serverPrefs, "", "");
    
    Core::Preferences localPrefs;
    localPrefs.debugMode = true;
    localPrefs.showOnlineStatus = false;
    localPrefs.notificationDuration = 10.0f;
    useCase.updateLocalPreferences(localPrefs);
    
    Core::Preferences merged = useCase.getPreferences();
    REQUIRE(merged.useServerNotes == true);
    REQUIRE(merged.shareFriendsAcrossAlts == false);
    REQUIRE(merged.quickOnlineFriendView.showJob == true);
    REQUIRE(merged.debugMode == true);
    REQUIRE(merged.showOnlineStatus == false);
    REQUIRE(merged.notificationDuration == 10.0f);
}

TEST_CASE("PreferencesUseCase - Reset preferences", "[PreferencesUseCase]") {
    auto netClient = std::make_unique<FakeNetClient>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    auto preferencesStore = std::make_unique<FakePreferencesStore>();
    
    PreferencesUseCase useCase(*netClient, *clock, *logger, preferencesStore.get());
    useCase.loadPreferences();
    
    useCase.updateServerPreference("useServerNotes", true);
    useCase.updateLocalPreference("debugMode", true);
    
    auto result = useCase.resetPreferences();
    REQUIRE(result.success == true);
    
    Core::Preferences prefs = useCase.getPreferences();
    REQUIRE(prefs.useServerNotes == false);
    REQUIRE(prefs.debugMode == false);
}

} // namespace App
} // namespace XIFriendList

