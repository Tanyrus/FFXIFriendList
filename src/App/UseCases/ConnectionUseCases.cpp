
#include "App/UseCases/ConnectionUseCases.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include "Protocol/JsonUtils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <mutex>

namespace XIFriendList {
namespace App {
namespace UseCases {

namespace {
    std::string toLower(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        std::transform(s.begin(), s.end(), std::back_inserter(result),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }
} // namespace

ConnectResult ConnectUseCase::connect(const std::string& username, 
                                      const std::string& apiKey) {
    if (username.empty()) {
        return { false, "Username cannot be empty", "", "" };
    }
    
    std::string normalized = username;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    currentUsername_ = normalized;
    
    logger_.info("[connect] Attempting to connect as " + normalized);
    
    stateMachine_.startConnecting();
    
    if (!apiKey.empty()) {
        storedApiKey_ = apiKey;
        ConnectResult result = attemptLogin(normalized);
        if (result.success) {
            stateMachine_.setConnected();
            if (apiKeyState_ && !result.apiKey.empty()) {
                (*apiKeyState_).apiKeys[normalized] = result.apiKey;
            }
            return result;
        }
        logger_.warning("[connect] Login failed, attempting registration");
    }
    
    ConnectResult result = attemptRegister(normalized);
    if (result.success) {
        stateMachine_.setConnected();
        storedApiKey_ = result.apiKey;
        if (apiKeyState_ && !result.apiKey.empty()) {
            (*apiKeyState_).apiKeys[normalized] = result.apiKey;
        }
        return result;
    }
    
    if (!storedApiKey_.empty()) {
        result = attemptLogin(normalized);
        if (result.success) {
            stateMachine_.setConnected();
            if (apiKeyState_ && !result.apiKey.empty()) {
                (*apiKeyState_).apiKeys[normalized] = result.apiKey;
            }
            return result;
        }
    }
    
    stateMachine_.setFailed();
    return { false, result.error.empty() ? "Connection failed" : result.error, "", normalized };
}

ConnectResult ConnectUseCase::autoConnect(const std::string& username) {
    if (username.empty()) {
        return { false, "Username cannot be empty", "", "" };
    }
    
    std::string normalized = toLower(username);
    currentUsername_ = normalized;
    
    logger_.info("[connect] Auto-connecting as " + normalized + " (server: " + netClient_.getBaseUrl() + ")");
    
    stateMachine_.startConnecting();
    
    std::string loadedApiKey;
    if (apiKeyState_) {
        auto it = apiKeyState_->apiKeys.find(normalized);
        if (it != apiKeyState_->apiKeys.end() && !it->second.empty()) {
            loadedApiKey = it->second;
            logger_.debug("[connect] Loaded API key from state for " + normalized);
            storedApiKey_ = loadedApiKey;
        } else {
            logger_.debug("[connect] No stored API key found for " + normalized);
        }
    }
    
    if (!loadedApiKey.empty()) {
        logger_.info("[connect] Attempting login with stored API key");
        ConnectResult result = attemptLogin(normalized);
        if (result.success) {
            stateMachine_.setConnected();
            if (apiKeyState_ && !result.apiKey.empty() && result.apiKey != loadedApiKey) {
                std::string normalizedChar = normalized;
                std::transform(normalizedChar.begin(), normalizedChar.end(), normalizedChar.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                (*apiKeyState_).apiKeys[normalizedChar] = result.apiKey;
            }
            storedApiKey_ = result.apiKey;
            logger_.info("[connect] Login successful");
            return result;
        }
        logger_.warning("[connect] Auto-login failed: " + result.error + ", attempting registration");
    } else {
        logger_.info("[connect] No stored API key, attempting registration");
    }
    
    ConnectResult result = attemptRegister(normalized);
    if (result.success) {
        stateMachine_.setConnected();
        storedApiKey_ = result.apiKey;
        if (apiKeyState_ && !result.apiKey.empty()) {
            (*apiKeyState_).apiKeys[normalized] = result.apiKey;
        }
        return result;
    }
    
    if (!loadedApiKey.empty() && loadedApiKey != storedApiKey_) {
        storedApiKey_ = loadedApiKey;
        result = attemptLogin(normalized);
        if (result.success) {
            stateMachine_.setConnected();
            storedApiKey_ = result.apiKey;
            if (apiKeyState_ && !result.apiKey.empty()) {
                (*apiKeyState_).apiKeys[normalized] = result.apiKey;
            }
            return result;
        }
    }
    
    stateMachine_.setFailed();
    return { false, result.error.empty() ? "Auto-connection failed" : result.error, "", normalized };
}

ConnectResult ConnectUseCase::attemptRegister(const std::string& username) {
    std::string url = netClient_.getBaseUrl() + "/api/auth/ensure";
    
    std::ostringstream body;
    body << "{\"characterName\":\"" << username << "\",\"realmId\":\"" << netClient_.getRealmId() << "\"}";
    
    logger_.debug("[connect] Ensuring character " + username + " (realm: " + netClient_.getRealmId() + ")");
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, "", username, body.str());
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[connect] Ensure failed: " + error);
        return { false, error, "", username };
    }
    
    return parseAuthResponse(response, username);
}

ConnectResult ConnectUseCase::attemptLogin(const std::string& username) {
    std::string url = netClient_.getBaseUrl() + "/api/auth/ensure";
    
    std::ostringstream body;
    body << "{\"characterName\":\"" << username << "\",\"realmId\":\"" << netClient_.getRealmId() << "\"}";
    
    logger_.debug("[connect] Logging in user " + username + " (realm: " + netClient_.getRealmId() + ")");
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, storedApiKey_, username, body.str());
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[connect] Login failed: " + error);
        return { false, error, "", username };
    }
    
    return parseAuthResponse(response, username);
}

ConnectResult ConnectUseCase::parseAuthResponse(const XIFriendList::App::HttpResponse& response, 
                                                 const std::string& username) {
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode response";
        logger_.error("[connect] " + error + " (body: " + response.body.substr(0, 200) + ")");
        return { false, error, "", username };
    }
    
    if (!msg.success || msg.type != XIFriendList::Protocol::ResponseType::AuthEnsureResponse) {
        std::string error = msg.error.empty() ? "Authentication failed" : msg.error;
        logger_.error("[connect] " + error);
        return { false, error, "", username };
    }
    
    std::string apiKey;
    XIFriendList::Protocol::JsonUtils::extractStringField(response.body, "apiKey", apiKey);
    
    if (apiKey.empty()) {
        logger_.warning("[connect] No API key in response, using stored key");
        apiKey = storedApiKey_;
    }
    
    logger_.info("[connect] Successfully authenticated " + username);
    return { true, "", apiKey, username };
}

void ConnectUseCase::disconnect() {
    logger_.info("[connect] Disconnecting");
    stateMachine_.setDisconnected();
    storedApiKey_.clear();
    currentUsername_.clear();
}

CharacterChangeResult HandleCharacterChangedUseCase::handleCharacterChanged(const XIFriendList::App::Events::CharacterChanged& event, 
                                                                             const std::string& currentApiKey) {
    CharacterChangeResult result;
    
    if (event.newCharacterName.empty()) {
        result.error = "Character name cannot be empty";
        result.errorCode = "VALIDATION_ERROR";
        return result;
    }
    
    std::string normalized = event.newCharacterName;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    logger_.info("[connect] Switching to character " + normalized);
    
    std::string apiKey;
    if (apiKeyState_) {
        auto it = apiKeyState_->apiKeys.find(normalized);
        if (it != apiKeyState_->apiKeys.end() && !it->second.empty()) {
            apiKey = it->second;
        }
        if (apiKey.empty() && !currentApiKey.empty()) {
            apiKey = currentApiKey;
            logger_.debug("[connect] Using current connected character's API key for auto-linking");
        }
    } else if (!currentApiKey.empty()) {
        apiKey = currentApiKey;
        logger_.debug("[connect] Using provided current API key (no state available)");
    }
    
    if (apiKey.empty()) {
        logger_.debug("[connect] No API key available, deferring to ConnectUseCase");
        result.success = true;
        result.characterName = normalized;
        result.realmId = netClient_.getRealmId();
        result.wasDeferred = true;
        return result;
    }
    
    std::string url = netClient_.getBaseUrl() + "/api/characters/active";
    
    std::ostringstream body;
    body << "{\"characterName\":\"" << normalized << "\",\"realmId\":\"" << netClient_.getRealmId() << "\"}";
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, apiKey, normalized, body.str());
    
    if (response.statusCode == 0) {
        result.error = response.error.empty() ? "Network error" : response.error;
        result.errorCode = "NETWORK_ERROR";
        logger_.error("[connect] Network error: " + result.error);
        return result;
    }
    
    if (response.statusCode == 403) {
        result.wasBanned = true;
        result.error = "API key revoked; contact plugin owner.";
        result.errorCode = "API_KEY_REVOKED";
        logger_.error("[connect] Account banned - " + result.error);
        return result;
    }
    
    if (!response.isSuccess()) {
        Protocol::ResponseMessage msg;
        Protocol::DecodeResult decodeResult = Protocol::ResponseDecoder::decode(response.body, msg);
        
        if (decodeResult == XIFriendList::Protocol::DecodeResult::Success && !msg.error.empty()) {
            result.error = msg.error;
            result.errorCode = msg.errorCode;
        } else {
            result.error = "HTTP " + std::to_string(response.statusCode);
            result.errorCode = "HTTP_ERROR";
        }
        logger_.error("[connect] Server error: " + result.error);
        return result;
    }
    
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success || !msg.success) {
        result.error = msg.error.empty() ? "Invalid response format" : msg.error;
        result.errorCode = msg.errorCode.empty() ? "PROTOCOL_ERROR" : msg.errorCode;
        logger_.error("[connect] " + result.error);
        return result;
    }
    
    int accountId = 0;
    int characterId = 0;
    std::string activeCharacterName;
    std::string realmId;
    std::string newApiKey;
    bool wasCreated = false;
    bool wasMerged = false;
    
    XIFriendList::Protocol::JsonUtils::extractNumberField(response.body, "accountId", accountId);
    XIFriendList::Protocol::JsonUtils::extractNumberField(response.body, "characterId", characterId);
    XIFriendList::Protocol::JsonUtils::extractStringField(response.body, "activeCharacterName", activeCharacterName);
    XIFriendList::Protocol::JsonUtils::extractStringField(response.body, "realmId", realmId);
    XIFriendList::Protocol::JsonUtils::extractStringField(response.body, "apiKey", newApiKey);
    XIFriendList::Protocol::JsonUtils::extractBooleanField(response.body, "wasCreated", wasCreated);
    XIFriendList::Protocol::JsonUtils::extractBooleanField(response.body, "wasMerged", wasMerged);
    
    if (newApiKey.empty()) {
        logger_.warning("[connect] Server response did not contain API key for " + normalized);
        logger_.debug("[connect] Response body: " + response.body.substr(0, 500));
    } else {
        logger_.debug("[connect] Extracted API key for " + normalized);
    }
    
    if (!newApiKey.empty() && apiKeyState_) {
        std::string charToSave = activeCharacterName.empty() ? normalized : activeCharacterName;
        std::transform(charToSave.begin(), charToSave.end(), charToSave.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        (*apiKeyState_).apiKeys[charToSave] = newApiKey;
        logger_.debug("[connect] Updated API key state for " + activeCharacterName);
    }
    
    result.success = true;
    result.accountId = accountId;
    result.characterId = characterId;
    result.characterName = activeCharacterName.empty() ? normalized : activeCharacterName;
    result.realmId = realmId.empty() ? netClient_.getRealmId() : realmId;
    
    if (newApiKey.empty()) {
        logger_.warning("[connect] Using old API key as fallback for " + normalized + " - this may cause authentication issues");
        result.apiKey = apiKey;
    } else {
        result.apiKey = newApiKey;
    }
    result.wasCreated = wasCreated;
    result.wasMerged = wasMerged;
    
    if (wasCreated) {
        logger_.info("[connect] Server created new character " + result.characterName);
    }
    if (wasMerged) {
        logger_.info("[connect] Server merged accounts for " + result.characterName);
    }
    
    logger_.info("[connect] Successfully switched to " + result.characterName);
    return result;
}

ZoneChangeResult HandleZoneChangedUseCase::handleZoneChanged(const XIFriendList::App::Events::ZoneChanged& event) {
    ZoneChangeResult result;
    
    logger_.debug("[connect] Zone changed to " + std::to_string(event.zoneId) + " (" + event.zoneName + ")");
    
    currentZoneId_ = event.zoneId;
    currentZoneName_ = event.zoneName;
    
    result.refreshTriggered = false;
    
    result.success = true;
    return result;
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

