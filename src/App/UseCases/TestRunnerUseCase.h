
#ifndef APP_TEST_RUNNER_USE_CASE_H
#define APP_TEST_RUNNER_USE_CASE_H

#include "App/Interfaces/INetClient.h"
#include "App/Interfaces/IClock.h"
#include "App/Interfaces/ILogger.h"
#include "App/State/ApiKeyState.h"
#include "App/UseCases/FriendsUseCases.h"
#include "App/UseCases/ConnectionUseCases.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct TestScenario {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> expectedAssertions;
};

struct TestResult {
    bool passed;
    std::string scenarioId;
    std::string scenarioName;
    std::string details;
    std::string error;
    uint64_t durationMs;
};

struct TestRunSummary {
    int total;
    int passed;
    int failed;
    uint64_t durationMs;
    std::vector<TestResult> results;
};

class TestRunnerUseCase {
public:
    TestRunnerUseCase(INetClient& netClient,
                     IClock& clock,
                     ILogger& logger,
                     XIFriendList::App::State::ApiKeyState& apiKeyState)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , apiKeyState_(apiKeyState)
    {}
    
    std::vector<TestScenario> getScenarios();
    TestResult runScenario(const TestScenario& scenario, const std::string& characterName);
    TestRunSummary runAllTests(const std::string& characterName);
    TestResult runSafe(const TestScenario& scenario, std::function<TestResult()> testFn);
    bool resetTestDatabase(const std::string& characterName);

private:
    std::string getApiKey(const std::string& characterName);
    
    XIFriendList::App::HttpResponse makeTestApiCall(const std::string& method,
                                const std::string& endpoint,
                                const std::string& characterName,
                                const std::string& body = "");
    
    struct PrivacySnapshot {
        bool shareOnlineStatus;
        bool shareCharacterData;
        bool shareLocation;
        bool isValid;
        
        PrivacySnapshot() : shareOnlineStatus(true), shareCharacterData(true), shareLocation(true), isValid(false) {}
    };
    
    PrivacySnapshot getPrivacySnapshot(const std::string& apiKey, const std::string& characterName);
    bool restorePrivacySnapshot(const std::string& apiKey, const std::string& characterName, const PrivacySnapshot& snapshot);
    bool restoreShareFriendsAcrossAlts(const std::string& apiKey, const std::string& characterName, bool value);
    
    struct Expect {
        static bool that(bool condition, const std::string& message, TestResult& result);
        
        static bool eq(const std::string& a, const std::string& b, const std::string& message, TestResult& result);
        static bool eq(int a, int b, const std::string& message, TestResult& result);
        static bool eq(uint64_t a, uint64_t b, const std::string& message, TestResult& result);
        static bool eq(bool a, bool b, const std::string& message, TestResult& result);
        
        static bool ne(const std::string& a, const std::string& b, const std::string& message, TestResult& result);
        static bool ne(int a, int b, const std::string& message, TestResult& result);
        
        static bool contains(const std::string& haystack, const std::string& needle, const std::string& message, TestResult& result);
        static bool notContains(const std::string& haystack, const std::string& needle, const std::string& message, TestResult& result);
        
        static bool httpStatus(const XIFriendList::App::HttpResponse& response, int expectedStatus, const std::string& message, TestResult& result);
        static bool httpStatusIn(const XIFriendList::App::HttpResponse& response, const std::vector<int>& expectedStatuses, const std::string& message, TestResult& result);
        static bool httpSuccess(const XIFriendList::App::HttpResponse& response, const std::string& message, TestResult& result);
        
        static bool jsonHas(const std::string& json, const std::string& path, const std::string& message, TestResult& result);
        static bool jsonEq(const std::string& json, const std::string& path, const std::string& expectedValue, const std::string& message, TestResult& result);
        static bool jsonEq(const std::string& json, const std::string& path, bool expectedValue, const std::string& message, TestResult& result);
        static bool jsonEq(const std::string& json, const std::string& path, int expectedValue, const std::string& message, TestResult& result);
    };
    
    struct TestHttp {
        static XIFriendList::App::HttpResponse getJson(INetClient& netClient,
                                   ILogger& logger,
                                   const std::string& path,
                                   const std::string& apiKey,
                                   const std::string& characterName,
                                   int timeoutMs = 1500,
                                   size_t maxBytes = 256 * 1024);
        
        static XIFriendList::App::HttpResponse postJson(INetClient& netClient,
                                    ILogger& logger,
                                    const std::string& path,
                                    const std::string& apiKey,
                                    const std::string& characterName,
                                    const std::string& body,
                                    int timeoutMs = 1500,
                                    size_t maxBytes = 256 * 1024);
        
        static XIFriendList::App::HttpResponse deleteJson(INetClient& netClient,
                                      ILogger& logger,
                                      const std::string& path,
                                      const std::string& apiKey,
                                      const std::string& characterName,
                                      int timeoutMs = 1500,
                                      size_t maxBytes = 256 * 1024);
        
        static bool validateJson(const std::string& json, ILogger& logger);
    };
    
    TestResult testEnsureAuthRecovery(const std::string& characterName);
    TestResult testFriendsListContainsExpected(const std::string& characterName);
    TestResult testOnlineOfflineComputation(const std::string& characterName);
    TestResult testOfflineTTL(const std::string& characterName);
    TestResult testShareOnlineStatusFalse(const std::string& characterName);
    TestResult testShareCharacterDataFalse(const std::string& characterName);
    TestResult testShareLocationFalse(const std::string& characterName);
    TestResult testVisibilityOnlyMode(const std::string& characterName);
    TestResult testVisibilityOnlyModeInverse(const std::string& characterName);
    TestResult testAddFriendFromAlt(const std::string& characterName);
    TestResult testFriendRequestVisibilityLabeling(const std::string& characterName);
    
    TestResult testEnsureAuthWithKey(const std::string& characterName);
    TestResult testEnsureAuthInvalidKey(const std::string& characterName);
    TestResult testBannedAccountBehavior(const std::string& characterName);
    TestResult testAddCharacterToAccount(const std::string& characterName);
    
    TestResult testSendAcceptFriendRequest(const std::string& characterName);
    TestResult testCancelOutgoingRequest(const std::string& characterName);
    TestResult testRejectIncomingRequest(const std::string& characterName);
    TestResult testRemoveFriend(const std::string& characterName);
    TestResult testRemoveFriendVisibility(const std::string& characterName);
    TestResult testAddFriendFromAltVisibility(const std::string& characterName);
    TestResult testVisibilityRequestAcceptance(const std::string& characterName);
    TestResult testFriendSync(const std::string& characterName);
    
    TestResult testToggleShareOnlineStatus(const std::string& characterName);
    TestResult testToggleShareCharacterData(const std::string& characterName);
    TestResult testToggleShareLocation(const std::string& characterName);
    TestResult testAnonymousMode(const std::string& characterName);
    TestResult testServerAuthoritativeFiltering(const std::string& characterName);
    TestResult testToggleShareFriendsAcrossAlts(const std::string& characterName);
    
    TestResult testFriendComesOnlineNotification(const std::string& characterName);
    TestResult testFriendGoesOfflineNotification(const std::string& characterName);
    TestResult testFriendRequestArrivesNotification(const std::string& characterName);
    
    TestResult testEndpointCoverageSanity(const std::string& characterName);
    
    TestResult testLinkedCharactersVerification(const std::string& characterName);
    TestResult testHeartbeatEndpoint(const std::string& characterName);
    TestResult testCharacterStateUpdate(const std::string& characterName);
    TestResult testGetAllCharacters(const std::string& characterName);
    TestResult testGetAccountInfo(const std::string& characterName);
    TestResult testGetPreferences(const std::string& characterName);
    TestResult testAddFriendByName(const std::string& characterName);
    TestResult testSyncFriendList(const std::string& characterName);
    TestResult testMultipleFriendsDifferentStates(const std::string& characterName);
    TestResult testErrorHandling404(const std::string& characterName);
    TestResult testAltNotVisibleOffline(const std::string& characterName); // T51
    
    TestResult testAltVisibilityWindowData(const std::string& characterName); // T52 - Tests Alt Visibility in Options window
    TestResult testToggleVisibilityCheckboxOn(const std::string& characterName); // T53
    TestResult testToggleVisibilityCheckboxOff(const std::string& characterName); // T54
    TestResult testAcceptFriendRequestUpdatesAltVisibility(const std::string& characterName); // T55
    TestResult testAcceptVisibilityRequestGrantsVisibility(const std::string& characterName); // T56
    TestResult testAltVisibilityShowsAllFriends(const std::string& characterName); // T57
    
    TestResult testGuardSanity(const std::string& characterName);
    
    TestResult testE2EFriendListSyncDisplaysFriends(const std::string& characterName);
    TestResult testE2EFriendRequestSendAcceptFlow(const std::string& characterName);
    TestResult testE2ENotesCreateEditSaveDelete(const std::string& characterName);
    TestResult testE2EThemeApplyPersistsAfterRestart(const std::string& characterName);
    TestResult testE2EWindowLockCannotMove(const std::string& characterName);
    TestResult testE2ENotificationPositioning(const std::string& characterName);
    TestResult testE2EAltVisibilityToggle(const std::string& characterName);
    TestResult testE2EFullConnectionFlow(const std::string& characterName);
    TestResult testE2EUpdatePresenceFlow(const std::string& characterName);
    TestResult testE2EUpdateMyStatus(const std::string& characterName);
    TestResult testE2EPreferencesSync(const std::string& characterName);
    TestResult testE2EFriendRequestRejectFlow(const std::string& characterName);
    TestResult testE2EFriendRequestCancelFlow(const std::string& characterName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    XIFriendList::App::State::ApiKeyState& apiKeyState_;
    
    mutable std::unique_ptr<SyncFriendListUseCase> syncUseCase_;
    mutable std::unique_ptr<ConnectUseCase> connectUseCase_;
    mutable std::unique_ptr<UpdatePresenceUseCase> presenceUseCase_;
    
    SyncFriendListUseCase& getSyncUseCase() const;
    ConnectUseCase& getConnectUseCase() const;
    UpdatePresenceUseCase& getPresenceUseCase() const;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_TEST_RUNNER_USE_CASE_H

