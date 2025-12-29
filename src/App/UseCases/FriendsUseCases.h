
#ifndef APP_FRIENDS_USE_CASES_H
#define APP_FRIENDS_USE_CASES_H

#include "App/Interfaces/INetClient.h"
#include "App/Interfaces/IClock.h"
#include "App/Interfaces/ILogger.h"
#include "Core/FriendsCore.h"
#include "Core/ModelsCore.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace XIFriendList {
namespace App {
namespace UseCases {

class UpdatePresenceUseCase;

struct SyncResult {
    bool success;
    std::string error;
    XIFriendList::Core::FriendList friendList;
};

struct FriendListWithStatusesResult {
    bool success;
    std::string error;
    XIFriendList::Core::FriendList friendList;
    std::vector<XIFriendList::Core::FriendStatus> statuses;
};

class SyncFriendListUseCase {
public:
    SyncFriendListUseCase(INetClient& netClient,
                          IClock& clock,
                          ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    SyncResult getFriendList(const std::string& apiKey, 
                             const std::string& characterName);
    FriendListWithStatusesResult getFriendListWithStatuses(const std::string& apiKey,
                                                            const std::string& characterName,
                                                            UpdatePresenceUseCase& presenceUseCase);
    SyncResult setFriendList(const std::string& apiKey,
                             const std::string& characterName,
                             const XIFriendList::Core::FriendList& friendList);
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    SyncResult parseFriendListResponse(const XIFriendList::App::HttpResponse& response);

    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct GetFriendRequestsResult {
    bool success;
    std::string error;
    std::vector<XIFriendList::Protocol::FriendRequestPayload> incoming;
    std::vector<XIFriendList::Protocol::FriendRequestPayload> outgoing;
};

class GetFriendRequestsUseCase {
public:
    GetFriendRequestsUseCase(INetClient& netClient,
                             IClock& clock,
                             ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    GetFriendRequestsResult getRequests(const std::string& apiKey,
                                       const std::string& characterName);

    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    GetFriendRequestsResult parseFriendRequestsResponse(const XIFriendList::App::HttpResponse& response);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct AcceptFriendRequestResult {
    bool success;
    std::string errorCode;
    std::string userMessage;
    std::string debugMessage;
    std::string friendName;
};

class AcceptFriendRequestUseCase {
public:
    AcceptFriendRequestUseCase(INetClient& netClient,
                              IClock& clock,
                              ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    AcceptFriendRequestResult acceptRequest(const std::string& apiKey,
                                           const std::string& characterName,
                                           const std::string& requestId);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct RejectFriendRequestResult {
    bool success;
    std::string errorCode;
    std::string userMessage;
    std::string debugMessage;
};

class RejectFriendRequestUseCase {
public:
    RejectFriendRequestUseCase(INetClient& netClient,
                              IClock& clock,
                              ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    RejectFriendRequestResult rejectRequest(const std::string& apiKey,
                                           const std::string& characterName,
                                           const std::string& requestId);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct CancelFriendRequestResult {
    bool success;
    std::string errorCode;
    std::string userMessage;
    std::string debugMessage;
};

class CancelFriendRequestUseCase {
public:
    CancelFriendRequestUseCase(INetClient& netClient,
                               IClock& clock,
                               ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    CancelFriendRequestResult cancelRequest(const std::string& apiKey,
                                           const std::string& characterName,
                                           const std::string& requestId);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct SendFriendRequestResult {
    bool success;
    std::string errorCode;
    std::string userMessage;
    std::string debugMessage;
    std::string requestId;
    std::string action;
    std::string message;
};

class SendFriendRequestUseCase {
public:
    SendFriendRequestUseCase(INetClient& netClient,
                            IClock& clock,
                            ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    SendFriendRequestResult sendRequest(const std::string& apiKey,
                                       const std::string& characterName,
                                       const std::string& toUserId);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct RemoveFriendResult {
    bool success;
    std::string error;
};

class RemoveFriendUseCase {
public:
    RemoveFriendUseCase(INetClient& netClient, IClock& clock, ILogger& logger)
        : netClient_(netClient), clock_(clock), logger_(logger), maxRetries_(3), retryDelayMs_(1000) {}

    void removeFriend(const std::string& apiKey, const std::string& characterName, const std::string& friendName,
                      std::function<void(RemoveFriendResult)> callback);

    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc, const std::string& operationName);

    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct RemoveFriendVisibilityResult {
    bool success;
    std::string error;
    std::string userMessage;
    std::string debugMessage;
    bool friendshipDeleted;
    
    RemoveFriendVisibilityResult()
        : success(false)
        , friendshipDeleted(false)
    {}
};

class RemoveFriendVisibilityUseCase {
public:
    RemoveFriendVisibilityUseCase(INetClient& netClient, IClock& clock, ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
    {}
    
    void removeFriendVisibility(const std::string& apiKey, 
                               const std::string& characterName, 
                               const std::string& friendName,
                               std::function<void(RemoveFriendVisibilityResult)> callback);
    
private:
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc, const std::string& operationName);
};

struct GetAltVisibilityResult {
    bool success;
    std::string error;
    std::vector<XIFriendList::Protocol::AltVisibilityFriendEntry> friends;
    std::vector<XIFriendList::Protocol::AccountCharacterInfo> characters;
    uint64_t serverTime;
    
    GetAltVisibilityResult()
        : success(false)
        , serverTime(0)
    {}
};

class GetAltVisibilityUseCase {
public:
    GetAltVisibilityUseCase(INetClient& netClient,
                            IClock& clock,
                            ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , maxRetries_(3)
        , retryDelayMs_(1000)
    {}
    
    GetAltVisibilityResult getVisibility(const std::string& apiKey,
                                        const std::string& characterName);
    
    void setRetryConfig(int maxRetries, uint64_t retryDelayMs) {
        maxRetries_ = maxRetries;
        retryDelayMs_ = retryDelayMs;
    }

private:
    XIFriendList::App::HttpResponse executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                   const std::string& operationName);
    
    GetAltVisibilityResult parseAltVisibilityResponse(const XIFriendList::App::HttpResponse& response, const std::string& characterName);
    
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    int maxRetries_;
    uint64_t retryDelayMs_;
};

struct PresenceUpdateResult {
    bool success;
    std::string error;
    std::vector<XIFriendList::Core::FriendStatus> friendStatuses;
};

struct HeartbeatResult {
    bool success;
    std::string error;
    std::vector<XIFriendList::Core::FriendStatus> friendStatuses;
    std::vector<XIFriendList::Protocol::FriendRequestPayload> events;
    bool isOutdated;
    std::string latestVersion;
    std::string releaseUrl;
    
    HeartbeatResult()
        : success(false)
        , isOutdated(false)
    {}
};

class UpdatePresenceUseCase {
public:
    UpdatePresenceUseCase(INetClient& netClient,
                          IClock& clock,
                          ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , lastWarnAtMs_(0)
    {}
    
    PresenceUpdateResult updatePresence(const std::string& apiKey,
                                        const std::string& characterName,
                                        const XIFriendList::Core::Presence& presence);
    
    PresenceUpdateResult getStatus(const std::string& apiKey,
                                   const std::string& characterName);
    
    HeartbeatResult getHeartbeat(const std::string& apiKey,
                                 const std::string& characterName,
                                 uint64_t lastEventTimestamp = 0,
                                 const std::string& pluginVersion = "");
    
    PresenceUpdateResult parseStatusResponse(const XIFriendList::App::HttpResponse& response);
    
    bool shouldShowOutdatedWarning(const HeartbeatResult& result, std::string& warningMessage);

private:
    
    HeartbeatResult parseHeartbeatResponse(const XIFriendList::App::HttpResponse& response);

    INetClient& netClient_;
    std::string lastWarnedLatestVersion_;
    uint64_t lastWarnAtMs_;
    IClock& clock_;
    ILogger& logger_;
};

struct UpdateMyStatusResult {
    bool success;
    std::string error;
    
    UpdateMyStatusResult() : success(false) {}
    UpdateMyStatusResult(bool success, const std::string& error = "")
        : success(success), error(error) {}
};

class UpdateMyStatusUseCase {
public:
    UpdateMyStatusUseCase(INetClient& netClient,
                          IClock& clock,
                          ILogger& logger)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
    {}
    
    UpdateMyStatusResult updateStatus(const std::string& apiKey,
                                     const std::string& characterName,
                                     bool showOnlineStatus,
                                     bool shareLocation,
                                     bool isAnonymous,
                                     bool shareJobWhenAnonymous);

private:
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_FRIENDS_USE_CASES_H

