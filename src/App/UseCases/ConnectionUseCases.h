
#ifndef APP_CONNECTION_USE_CASES_H
#define APP_CONNECTION_USE_CASES_H

#include "App/Interfaces/INetClient.h"
#include "App/Interfaces/IClock.h"
#include "App/Interfaces/ILogger.h"
#include "App/State/ApiKeyState.h"
#include "App/StateMachines/ConnectionState.h"
#include "App/Events/AppEvents.h"
#include <string>
#include <memory>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct ConnectResult {
    bool success;
    std::string error;
    std::string apiKey;
    std::string username;
};

class ConnectUseCase {
public:
    ConnectUseCase(INetClient& netClient,
                   IClock& clock,
                   ILogger& logger,
                   XIFriendList::App::State::ApiKeyState* apiKeyState = nullptr)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , apiKeyState_(apiKeyState)
        , stateMachine_()
    {}
    
    ConnectResult connect(const std::string& username, 
                          const std::string& apiKey = "");
    ConnectResult autoConnect(const std::string& username);
    void disconnect();
    XIFriendList::App::ConnectionState getState() const {
        return stateMachine_.getState();
    }
    bool isConnected() const {
        return stateMachine_.isConnected();
    }
    void setStoredApiKey(const std::string& apiKey) {
        storedApiKey_ = apiKey;
    }
    std::string getStoredApiKey() const {
        return storedApiKey_;
    }

private:
    ConnectResult attemptRegister(const std::string& username);
    ConnectResult attemptLogin(const std::string& username);
    ConnectResult parseAuthResponse(const XIFriendList::App::HttpResponse& response, 
                                     const std::string& username);

    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    XIFriendList::App::State::ApiKeyState* apiKeyState_;
    XIFriendList::App::ConnectionStateMachine stateMachine_;
    std::string storedApiKey_;
    std::string currentUsername_;
};

struct CharacterChangeResult {
    bool success;
    std::string error;
    std::string errorCode;
    
    int accountId;
    int characterId;
    std::string characterName;
    std::string realmId;
    std::string apiKey;
    bool wasCreated;
    bool wasMerged;
    bool wasBanned;
    bool wasDeferred;
    
    CharacterChangeResult()
        : success(false)
        , accountId(0)
        , characterId(0)
        , wasCreated(false)
        , wasMerged(false)
        , wasBanned(false)
        , wasDeferred(false)
    {}
};

class HandleCharacterChangedUseCase {
public:
    HandleCharacterChangedUseCase(INetClient& netClient,
                                  IClock& clock,
                                  ILogger& logger,
                                  XIFriendList::App::State::ApiKeyState* apiKeyState)
        : netClient_(netClient)
        , clock_(clock)
        , logger_(logger)
        , apiKeyState_(apiKeyState)
    {}
    
    CharacterChangeResult handleCharacterChanged(const XIFriendList::App::Events::CharacterChanged& event, 
                                                 const std::string& currentApiKey = "");
    
private:
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    XIFriendList::App::State::ApiKeyState* apiKeyState_;
};

struct ZoneChangeResult {
    bool success;
    std::string error;
    bool refreshTriggered;
    
    ZoneChangeResult()
        : success(false)
        , refreshTriggered(false)
    {}
};

class HandleZoneChangedUseCase {
public:
    HandleZoneChangedUseCase(IClock& clock, ILogger& logger)
        : clock_(clock)
        , logger_(logger)
        , lastRefreshTime_(0)
        , debounceDelayMs_(2000)
    {}
    
    ZoneChangeResult handleZoneChanged(const XIFriendList::App::Events::ZoneChanged& event);
    uint16_t getCurrentZoneId() const { return currentZoneId_; }
    const std::string& getCurrentZoneName() const { return currentZoneName_; }
    void setDebounceDelay(uint64_t delayMs) { debounceDelayMs_ = delayMs; }

private:
    IClock& clock_;
    ILogger& logger_;
    uint16_t currentZoneId_ = 0;
    std::string currentZoneName_;
    uint64_t lastRefreshTime_;
    uint64_t debounceDelayMs_;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_CONNECTION_USE_CASES_H

