#include "App/UseCases/FriendsUseCases.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageTypes.h"
#include "Protocol/JsonUtils.h"
#include "PluginVersion.h"
#include <functional>
#include <sstream>
#include <algorithm>
#include <thread>
#include <cctype>
#include <iterator>
#include <chrono>


namespace XIFriendList {
namespace App {
namespace UseCases {

SyncResult SyncFriendListUseCase::getFriendList(const std::string& apiKey,
                                                 const std::string& characterName) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", XIFriendList::Core::FriendList() };
    }
    
    logger_.debug("[friend] Getting friend list for " + characterName);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends";
    
    auto requestFunc = [this, url, apiKey, characterName]() {
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetFriendList");
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to get friend list: " + error);
        return { false, error, XIFriendList::Core::FriendList() };
    }
    
    return parseFriendListResponse(response);
}

FriendListWithStatusesResult SyncFriendListUseCase::getFriendListWithStatuses(const std::string& apiKey,
                                                                               const std::string& characterName,
                                                                               UpdatePresenceUseCase& presenceUseCase) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", XIFriendList::Core::FriendList(), {} };
    }
    
    logger_.debug("[friend] Getting friend list with statuses for " + characterName);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends";
    
    auto requestFunc = [this, url, apiKey, characterName]() {
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetFriendListWithStatuses");
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to get friend list with statuses: " + error);
        return { false, error, XIFriendList::Core::FriendList(), {} };
    }
    
    SyncResult friendListResult = parseFriendListResponse(response);
    if (!friendListResult.success) {
        return { false, friendListResult.error, XIFriendList::Core::FriendList(), {} };
    }
    
    PresenceUpdateResult statusResult = presenceUseCase.parseStatusResponse(response);
    std::vector<XIFriendList::Core::FriendStatus> statuses = statusResult.success ? statusResult.friendStatuses : std::vector<XIFriendList::Core::FriendStatus>();
    
    logger_.info("[friend] Successfully retrieved friend list (" + 
                std::to_string(friendListResult.friendList.size()) + " friends) and " +
                std::to_string(statuses.size()) + " statuses");
    
    return { true, "", friendListResult.friendList, statuses };
}

SyncResult SyncFriendListUseCase::setFriendList(const std::string& apiKey,
                                                  const std::string& characterName,
                                                  const XIFriendList::Core::FriendList& friendList) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", XIFriendList::Core::FriendList() };
    }
    
    logger_.debug("[friend] Syncing friend list for " + characterName);
    
    std::vector<std::string> friendNames = friendList.getFriendNames();
    
    std::vector<XIFriendList::Core::Friend> friends;
    for (const auto& name : friendNames) {
        const XIFriendList::Core::Friend* f = friendList.findFriend(name);
        if (f) {
            friends.push_back(*f);
        }
    }
    
    std::string requestJson = XIFriendList::Protocol::RequestEncoder::encodeSetFriendList(friends);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/sync";
    
    auto requestFunc = [this, url, apiKey, characterName, requestJson]() {
        return netClient_.post(url, apiKey, characterName, requestJson);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "SyncFriendList");
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to sync friend list: " + error);
        return { false, error, friendList };
    }
    
    return parseFriendListResponse(response);
}

XIFriendList::App::HttpResponse SyncFriendListUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                      const std::string& operationName) {
    XIFriendList::App::HttpResponse response;
    
    for (int attempt = 0; attempt < maxRetries_; ++attempt) {
        if (attempt > 0) {
            logger_.info("[friend] Retrying " + operationName + 
                        " (attempt " + std::to_string(attempt + 1) + "/" + 
                        std::to_string(maxRetries_) + ")");
            clock_.sleepMs(retryDelayMs_ * attempt);
        }
        
        response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        if (response.statusCode >= 400 && response.statusCode < 500) {
            logger_.warning("[friend] Client error, not retrying");
            break;
        }
    }
    
    return response;
}

SyncResult SyncFriendListUseCase::parseFriendListResponse(const XIFriendList::App::HttpResponse& response) {
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode response";
        std::string details = "DecodeResult: ";
        switch (decodeResult) {
            case XIFriendList::Protocol::DecodeResult::InvalidJson:
                details += "InvalidJson";
                break;
            case XIFriendList::Protocol::DecodeResult::MissingField:
                details += "MissingField";
                break;
            case XIFriendList::Protocol::DecodeResult::InvalidVersion:
                details += "InvalidVersion";
                break;
            case XIFriendList::Protocol::DecodeResult::InvalidType:
                details += "InvalidType";
                break;
            default:
                details += "Unknown";
                break;
        }
        size_t previewLen = std::min(static_cast<size_t>(200), response.body.length());
        details += ", Response preview: " + response.body.substr(0, previewLen);
        logger_.error("[friend] " + error + " (" + details + ")");
        return { false, error, XIFriendList::Core::FriendList() };
    }
    
    if (!msg.success || msg.type != XIFriendList::Protocol::ResponseType::FriendList) {
        std::string error = msg.error.empty() ? "Invalid response type" : msg.error;
        logger_.error("[friend] " + error + " (type=" + 
                     (msg.type == XIFriendList::Protocol::ResponseType::Error ? "Error" : "Other") + 
                     ", success=" + (msg.success ? "true" : "false") + ")");
        return { false, error, XIFriendList::Core::FriendList() };
    }
    
    std::string decodedPayload;
    if (!msg.payload.empty()) {
        logger_.debug("[friend] Raw payload (first 200 chars): " + 
                     msg.payload.substr(0, std::min(static_cast<size_t>(200), msg.payload.length())));
        
        if (msg.payload[0] == '"' && msg.payload.length() > 1) {
            if (XIFriendList::Protocol::JsonUtils::decodeString(msg.payload, decodedPayload)) {
                logger_.debug("[friend] Decoded payload string, length: " + 
                             std::to_string(decodedPayload.length()));
            } else {
                logger_.warning("[friend] Failed to decode payload string, using as-is");
                decodedPayload = msg.payload;
            }
        } else {
            decodedPayload = msg.payload;
            logger_.debug("[friend] Payload not a JSON string, using directly, length: " + 
                         std::to_string(decodedPayload.length()));
        }
    } else {
        logger_.error("[friend] Empty payload");
    }
    
    XIFriendList::Protocol::FriendListResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeFriendListPayload(
        decodedPayload, payload);
    
    logger_.debug("[friend] decodeFriendListPayload result: " + 
                 std::to_string(static_cast<int>(payloadResult)) + 
                 ", friendsData count: " + std::to_string(payload.friendsData.size()));
    
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Friend list response was invalid.";
        std::string debugMessage = "DecodeResult: ";
        std::string errorCode;
        
        switch (payloadResult) {
            case XIFriendList::Protocol::DecodeResult::InvalidPayload:
                errorCode = "DECODE_INVALID_TYPE";
                debugMessage += "InvalidPayload (malformed JSON or wrong format)";
                if (!decodedPayload.empty() && decodedPayload.find("\"friendsData\"") != std::string::npos) {
                    debugMessage += " - friendsData field exists but is not an array";
                }
                break;
            case XIFriendList::Protocol::DecodeResult::MissingField:
                errorCode = "DECODE_MISSING_FIELD";
                debugMessage += "MissingField (friendsData field not found)";
                break;
            default:
                errorCode = "DECODE_ERROR";
                debugMessage += "Unknown decode error";
                break;
        }
        
        size_t previewLen = std::min(static_cast<size_t>(200), decodedPayload.length());
        std::string preview = decodedPayload.substr(0, previewLen);
        for (size_t i = 0; i < preview.length(); ++i) {
            if (preview[i] == '\n' || preview[i] == '\r' || preview[i] == '\t') {
                preview[i] = ' ';
            } else if (static_cast<unsigned char>(preview[i]) < 0x20) {
                preview[i] = '?';
            }
        }
        debugMessage += ", Payload preview: " + preview;
        
        logger_.error("[friend] " + debugMessage + " (errorCode=" + errorCode + ")");
        return { false, error, XIFriendList::Core::FriendList() };
    }
    
    XIFriendList::Core::FriendList friendList;
    for (const auto& friendData : payload.friendsData) {
        XIFriendList::Core::Friend f(friendData.name, friendData.friendedAs);
        f.linkedCharacters = friendData.linkedCharacters;
        friendList.addFriend(f);
    }
    
    logger_.info("[friend] Successfully synced friend list (" + 
                std::to_string(friendList.size()) + " friends)");
    
    return { true, "", friendList };
}


GetFriendRequestsResult GetFriendRequestsUseCase::getRequests(const std::string& apiKey,
                                                              const std::string& characterName) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", {}, {} };
    }
    
    logger_.debug("[friend] Getting friend requests for " + characterName);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/requests";
    
    auto requestFunc = [this, url, apiKey, characterName]() {
        std::string payload = XIFriendList::Protocol::RequestEncoder::encodeGetFriendRequests(characterName);
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetFriendRequests");
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to get friend requests: " + error);
        return { false, error, {}, {} };
    }
    
    return parseFriendRequestsResponse(response);
}

GetFriendRequestsResult GetFriendRequestsUseCase::parseFriendRequestsResponse(const XIFriendList::App::HttpResponse& response) {
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to decode response");
        return { false, "Invalid response format", {}, {} };
    }
    
    if (!responseMsg.success) {
        logger_.error("[friend] Server returned error: " + responseMsg.error);
        return { false, responseMsg.error, {}, {} };
    }
    
    if (responseMsg.type != XIFriendList::Protocol::ResponseType::FriendRequests) {
        logger_.error("[friend] Unexpected response type");
        return { false, "Unexpected response type", {}, {} };
    }
    
    std::string decodedPayload;
    if (!responseMsg.payload.empty()) {
        if (!XIFriendList::Protocol::JsonUtils::decodeString(responseMsg.payload, decodedPayload)) {
            decodedPayload = responseMsg.payload;
        }
    }
    
    XIFriendList::Protocol::FriendRequestsResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeFriendRequestsPayload(decodedPayload, payload);
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to parse friend requests payload");
        return { false, "Failed to parse payload", {}, {} };
    }
    
    logger_.info("[friend] Retrieved " + 
                std::to_string(payload.incoming.size()) + " incoming, " +
                std::to_string(payload.outgoing.size()) + " outgoing requests");
    
    return { true, "", payload.incoming, payload.outgoing };
}

XIFriendList::App::HttpResponse GetFriendRequestsUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                         const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}


AcceptFriendRequestResult AcceptFriendRequestUseCase::acceptRequest(const std::string& apiKey,
                                                                    const std::string& characterName,
                                                                    const std::string& requestId) {
    if (apiKey.empty() || characterName.empty() || requestId.empty()) {
        return { false, "", "API key, character name, and request ID required", "", "" };
    }
    
    logger_.info("[friend] Accepting friend request " + requestId);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/requests/accept";
    
    auto requestFunc = [this, url, apiKey, characterName, requestId]() {
        std::string payload = "{\"requestId\":" + XIFriendList::Protocol::JsonUtils::encodeString(requestId) + "}";
        return netClient_.post(url, apiKey, characterName, payload);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "AcceptFriendRequest");
    
    std::ostringstream debugMsg;
    debugMsg << "AcceptFriendRequest " << url << " HTTP " << response.statusCode;
    std::string debugMessage = debugMsg.str();
    
    if (!response.isSuccess()) {
        if (response.statusCode == 0) {
            std::string error = response.error.empty() ? "Network error: failed to send request" : response.error;
            logger_.error("[friend] Network error: " + error);
            return { false, "", error, debugMessage, "" };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                std::string errorCode = responseMsg.errorCode;
                std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
                if (!responseMsg.requestId.empty()) {
                    debugMessage += " serverRequestId=" + responseMsg.requestId;
                }
                logger_.error("[friend] " + debugMessage + " errorCode=" + errorCode + " error=" + userMessage);
                return { false, errorCode, userMessage, debugMessage, "" };
            } else {
                std::string error = "AcceptFriendRequest failed: HTTP " + std::to_string(response.statusCode);
                logger_.error("[friend] " + error);
                return { false, "", error, debugMessage, "" };
            }
        }
        
        std::string error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[friend] Failed: " + error);
        return { false, "", error, debugMessage, "" };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to decode response");
        return { false, "", "Invalid response format", debugMessage, "" };
    }
    
    if (!responseMsg.success) {
        std::string errorCode = responseMsg.errorCode;
        std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
        if (!responseMsg.requestId.empty()) {
            debugMessage += " serverRequestId=" + responseMsg.requestId;
        }
        logger_.error("[friend] Server returned error: " + userMessage + " errorCode=" + errorCode);
        return { false, errorCode, userMessage, debugMessage, "" };
    }
    
    std::string friendName;
    if (!responseMsg.payload.empty()) {
        XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "friendName", friendName);
        if (friendName.empty()) {
            XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "friend", friendName);
            if (friendName.empty()) {
                XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "fromCharacterName", friendName);
            }
        }
    }
    
    logger_.info("[friend] Friend request accepted successfully" + 
                 (friendName.empty() ? "" : " (friend: " + friendName + ")"));
    return { true, "", "Request accepted.", debugMessage, friendName };
}

XIFriendList::App::HttpResponse AcceptFriendRequestUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                          const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}


RejectFriendRequestResult RejectFriendRequestUseCase::rejectRequest(const std::string& apiKey,
                                                                    const std::string& characterName,
                                                                    const std::string& requestId) {
    if (apiKey.empty() || characterName.empty() || requestId.empty()) {
        return { false, "", "API key, character name, and request ID required", "" };
    }
    
    logger_.info("[friend] Rejecting friend request " + requestId);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/requests/reject";
    
    auto requestFunc = [this, url, apiKey, characterName, requestId]() {
        std::string payload = "{\"requestId\":" + XIFriendList::Protocol::JsonUtils::encodeString(requestId) + "}";
        return netClient_.post(url, apiKey, characterName, payload);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "RejectFriendRequest");
    
    std::ostringstream debugMsg;
    debugMsg << "RejectFriendRequest " << url << " HTTP " << response.statusCode;
    std::string debugMessage = debugMsg.str();
    
    if (!response.isSuccess()) {
        if (response.statusCode == 0) {
            std::string error = response.error.empty() ? "Network error: failed to send request" : response.error;
            logger_.error("[friend] Network error: " + error);
            return { false, "", error, debugMessage };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                std::string errorCode = responseMsg.errorCode;
                std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
                if (!responseMsg.requestId.empty()) {
                    debugMessage += " serverRequestId=" + responseMsg.requestId;
                }
                logger_.error("[friend] " + debugMessage + " errorCode=" + errorCode + " error=" + userMessage);
                return { false, errorCode, userMessage, debugMessage };
            } else {
                std::string error = "RejectFriendRequest failed: HTTP " + std::to_string(response.statusCode);
                logger_.error("[friend] " + error);
                return { false, "", error, debugMessage };
            }
        }
        
        std::string error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[friend] Failed: " + error);
        return { false, "", error, debugMessage };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to decode response");
        return { false, "", "Invalid response format", debugMessage };
    }
    
    if (!responseMsg.success) {
        std::string errorCode = responseMsg.errorCode;
        std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
        if (!responseMsg.requestId.empty()) {
            debugMessage += " serverRequestId=" + responseMsg.requestId;
        }
        logger_.error("[friend] Server returned error: " + userMessage + " errorCode=" + errorCode);
        return { false, errorCode, userMessage, debugMessage };
    }
    
    logger_.info("[friend] Friend request rejected successfully");
    return { true, "", "Request rejected.", debugMessage };
}

XIFriendList::App::HttpResponse RejectFriendRequestUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                          const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}


CancelFriendRequestResult CancelFriendRequestUseCase::cancelRequest(const std::string& apiKey,
                                                                    const std::string& characterName,
                                                                    const std::string& requestId) {
    if (apiKey.empty() || characterName.empty() || requestId.empty()) {
        return { false, "", "API key, character name, and request ID required", "" };
    }
    
    logger_.info("[friend] Canceling friend request " + requestId);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/requests/cancel";
    
    auto requestFunc = [this, url, apiKey, characterName, requestId]() {
        std::string payload = "{\"requestId\":" + XIFriendList::Protocol::JsonUtils::encodeString(requestId) + "}";
        return netClient_.post(url, apiKey, characterName, payload);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "CancelFriendRequest");
    
    std::ostringstream debugMsg;
    debugMsg << "CancelFriendRequest " << url << " HTTP " << response.statusCode;
    std::string debugMessage = debugMsg.str();
    
    if (!response.isSuccess()) {
        if (response.statusCode == 0) {
            std::string error = response.error.empty() ? "Network error: failed to send request" : response.error;
            logger_.error("[friend] Network error: " + error);
            return { false, "", error, debugMessage };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                std::string errorCode = responseMsg.errorCode;
                std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
                if (!responseMsg.requestId.empty()) {
                    debugMessage += " serverRequestId=" + responseMsg.requestId;
                }
                logger_.error("[friend] " + debugMessage + " errorCode=" + errorCode + " error=" + userMessage);
                return { false, errorCode, userMessage, debugMessage };
            } else {
                std::string error = "CancelFriendRequest failed: HTTP " + std::to_string(response.statusCode);
                logger_.error("[friend] " + error);
                return { false, "", error, debugMessage };
            }
        }
        
        std::string error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[friend] Failed: " + error);
        return { false, "", error, debugMessage };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to decode response");
        return { false, "", "Invalid response format", debugMessage };
    }
    
    if (!responseMsg.success) {
        std::string errorCode = responseMsg.errorCode;
        std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
        if (!responseMsg.requestId.empty()) {
            debugMessage += " serverRequestId=" + responseMsg.requestId;
        }
        logger_.error("[friend] Server returned error: " + userMessage + " errorCode=" + errorCode);
        return { false, errorCode, userMessage, debugMessage };
    }
    
    logger_.info("[friend] Friend request canceled successfully");
    return { true, "", "Request canceled.", debugMessage };
}

XIFriendList::App::HttpResponse CancelFriendRequestUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                          const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}


SendFriendRequestResult SendFriendRequestUseCase::sendRequest(const std::string& apiKey,
                                                              const std::string& characterName,
                                                              const std::string& toUserId) {
    logger_.info("[DEBUG] [friend]:sendRequest: ENTRY");
    
    if (apiKey.empty() || characterName.empty() || toUserId.empty()) {
        logger_.info("[DEBUG] [friend]:sendRequest: Validation failed, returning");
        return { false, "", "API key, character name, and target user ID required", "", "", "", "" };
    }
    
    logger_.info("[friend] Sending friend request to " + toUserId);
    logger_.info("[DEBUG] [friend]:sendRequest: Building URL");
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/requests/request";
    logger_.info("[DEBUG] [friend]:sendRequest: URL=" + url);
    
    auto requestFunc = [this, url, apiKey, characterName, toUserId]() {
        logger_.info("[DEBUG] [friend]:requestFunc: ENTRY (inside lambda)");
        std::string payload = "{\"toUserId\":" + XIFriendList::Protocol::JsonUtils::encodeString(toUserId) + "}";
        logger_.info("[DEBUG] [friend]:requestFunc: Encoded, payload length=" + std::to_string(payload.length()));
        logger_.info("[DEBUG] [friend]:requestFunc: About to call netClient_.post()");
        auto response = netClient_.post(url, apiKey, characterName, payload);
        logger_.info("[DEBUG] [friend]:requestFunc: netClient_.post() returned, statusCode=" + std::to_string(response.statusCode));
        return response;
    };
    
    logger_.info("[DEBUG] [friend]:sendRequest: About to call executeWithRetry()");
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "SendFriendRequest");
    logger_.info("[DEBUG] [friend]:sendRequest: executeWithRetry() returned, statusCode=" + std::to_string(response.statusCode));
    
    std::ostringstream debugMsg;
    debugMsg << "SendFriendRequest " << url << " HTTP " << response.statusCode;
    std::string debugMessage = debugMsg.str();
    
    if (!response.isSuccess()) {
        if (response.statusCode == 0) {
            std::string error = response.error.empty() ? "Network error: failed to send request" : response.error;
            logger_.error("[friend] Network error: " + error);
            return { false, "", error, debugMessage, "", "", "" };
        }
        
        if (response.statusCode >= 400) {
            XIFriendList::Protocol::ResponseMessage responseMsg;
            XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
            if (decodeResult == XIFriendList::Protocol::DecodeResult::Success) {
                std::string errorCode = responseMsg.errorCode;
                std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
                if (!responseMsg.requestId.empty()) {
                    debugMessage += " serverRequestId=" + responseMsg.requestId;
                }
                logger_.error("[friend] " + debugMessage + " errorCode=" + errorCode + " error=" + userMessage);
                return { false, errorCode, userMessage, debugMessage, "", "", "" };
            } else {
                std::string error = "SendFriendRequest failed: HTTP " + std::to_string(response.statusCode);
                logger_.error("[friend] " + error);
                return { false, "", error, debugMessage, "", "", "" };
            }
        }
        
        std::string error = "HTTP " + std::to_string(response.statusCode);
        logger_.error("[friend] Failed: " + error);
        return { false, "", error, debugMessage, "", "", "" };
    }
    
    XIFriendList::Protocol::ResponseMessage responseMsg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        logger_.error("[friend] Failed to decode response");
        return { false, "", "Invalid response format", debugMessage, "", "", "" };
    }
    
    if (!responseMsg.success) {
        std::string errorCode = responseMsg.errorCode;
        std::string userMessage = responseMsg.error.empty() ? "Request failed" : responseMsg.error;
        if (!responseMsg.requestId.empty()) {
            debugMessage += " serverRequestId=" + responseMsg.requestId;
        }
        logger_.error("[friend] Server returned error: " + userMessage + " errorCode=" + errorCode);
        return { false, errorCode, userMessage, debugMessage, "", "", "" };
    }
    
    std::string requestId;
    std::string action;
    std::string message;
    if (!responseMsg.payload.empty()) {
        std::string decodedPayload;
        if (XIFriendList::Protocol::JsonUtils::decodeString(responseMsg.payload, decodedPayload)) {
            XIFriendList::Protocol::JsonUtils::extractStringField(decodedPayload, "action", action);
            XIFriendList::Protocol::JsonUtils::extractStringField(decodedPayload, "message", message);
            XIFriendList::Protocol::JsonUtils::extractStringField(decodedPayload, "requestId", requestId);
            if (requestId.empty()) {
                XIFriendList::Protocol::JsonUtils::extractStringField(decodedPayload, "id", requestId);
            }
        } else {
            XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "action", action);
            XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "message", message);
            XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "requestId", requestId);
            if (requestId.empty()) {
                XIFriendList::Protocol::JsonUtils::extractStringField(responseMsg.payload, "id", requestId);
            }
        }
    }
    
    logger_.info("[friend] Friend request sent successfully" + 
                 (requestId.empty() ? "" : " (requestId: " + requestId + ")") +
                 (action.empty() ? "" : " (action: " + action + ")"));
    return { true, "", "", debugMessage, requestId, action, message };
}

XIFriendList::App::HttpResponse SendFriendRequestUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                        const std::string& operationName) {
    logger_.info("[DEBUG] executeWithRetry: ENTRY, maxRetries_=" + std::to_string(maxRetries_) + ", retryDelayMs_=" + std::to_string(retryDelayMs_));
    
    logger_.info("[DEBUG] executeWithRetry: About to make first request");
    XIFriendList::App::HttpResponse response = requestFunc();
    logger_.info("[DEBUG] executeWithRetry: First request completed, statusCode=" + std::to_string(response.statusCode) + ", isSuccess=" + std::string(response.isSuccess() ? "true" : "false"));
    
    if (response.isSuccess()) {
        logger_.info("[DEBUG] executeWithRetry: First request succeeded, returning");
        return response;
    }
    
    if (response.statusCode >= 400 && response.statusCode < 500) {
        logger_.warning(operationName + ": Client error " + std::to_string(response.statusCode) + 
                       ", not retrying");
        logger_.info("[DEBUG] executeWithRetry: Client error detected, returning immediately");
        return response;
    }
    
    logger_.info("[DEBUG] executeWithRetry: Not a client error, checking if retries needed");
    
    int attempts = 1;
    while (attempts <= maxRetries_ && maxRetries_ > 0) {
        logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                       " failed (statusCode=" + std::to_string(response.statusCode) + 
                       "), retrying in " + std::to_string(retryDelayMs_) + "ms");
        logger_.info("[DEBUG] executeWithRetry: About to sleep for " + std::to_string(retryDelayMs_) + "ms");
        
        clock_.sleepMs(retryDelayMs_);
        logger_.info("[DEBUG] executeWithRetry: Sleep completed, about to retry");
        
        logger_.info("[DEBUG] executeWithRetry: About to make retry request #" + std::to_string(attempts));
        response = requestFunc();
        logger_.info("[DEBUG] executeWithRetry: Retry request #" + std::to_string(attempts) + " completed, statusCode=" + std::to_string(response.statusCode));
        
        if (response.isSuccess()) {
            logger_.info("[DEBUG] executeWithRetry: Retry succeeded, returning");
            return response;
        }
        
        if (response.statusCode >= 400 && response.statusCode < 500) {
            logger_.warning(operationName + ": Client error " + std::to_string(response.statusCode) + 
                           " on retry, not retrying further");
            logger_.info("[DEBUG] executeWithRetry: Client error on retry, returning");
            return response;
        }
        
        attempts++;
        logger_.info("[DEBUG] executeWithRetry: Incremented attempts to " + std::to_string(attempts));
    }
    
    logger_.info("[DEBUG] executeWithRetry: All retries exhausted, returning final response");
    return response;
}


void RemoveFriendUseCase::removeFriend(const std::string& apiKey, const std::string& characterName, const std::string& friendName,
                                       std::function<void(RemoveFriendResult)> callback) {
    if (apiKey.empty() || characterName.empty() || friendName.empty()) {
        callback({false, "API key, character name, and friend name required"});
        return;
    }

    logger_.info("[friend] Removing friend " + friendName + " via DELETE /api/friends/" + friendName);

    std::thread([this, apiKey, characterName, friendName, callback]() {
        RemoveFriendResult result;
        
        std::string url = netClient_.getBaseUrl() + "/api/friends/" + friendName;
        
        auto requestFunc = [this, url, apiKey, characterName]() {
            return netClient_.del(url, apiKey, characterName, "");
        };
        
        XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "RemoveFriend");
        
        if (!response.isSuccess()) {
            std::string error;
            if (response.statusCode == 0) {
                error = response.error.empty() ? "Network error: failed to remove friend" : response.error;
            } else if (response.statusCode == 404) {
                logger_.info("[friend] Friend not found (already removed)");
                result.success = true;
                callback(result);
                return;
            } else if (response.statusCode >= 400) {
                XIFriendList::Protocol::ResponseMessage responseMsg;
                XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
                if (decodeResult == XIFriendList::Protocol::DecodeResult::Success && !responseMsg.error.empty()) {
                    error = "RemoveFriend failed: " + responseMsg.error;
                } else {
                    error = "RemoveFriend failed: HTTP " + std::to_string(response.statusCode);
                }
            } else {
                error = "RemoveFriend failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[friend] " + error);
            result.success = false;
            result.error = error;
            callback(result);
            return;
        }
        
        XIFriendList::Protocol::ResponseMessage responseMsg;
        XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
        if (decodeResult != XIFriendList::Protocol::DecodeResult::Success || !responseMsg.success) {
            result.success = false;
            result.error = responseMsg.error.empty() ? "Failed to parse response" : responseMsg.error;
            logger_.error("[friend] " + result.error);
            callback(result);
            return;
        }
        
        result.success = true;
        logger_.info("[friend] Friend " + friendName + " removed successfully");
        callback(result);
    }).detach();
}

XIFriendList::App::HttpResponse RemoveFriendUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc, const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        if (response.isSuccess() && response.statusCode >= 200 && response.statusCode < 300) {
            return response;
        }
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    return requestFunc();
}



void RemoveFriendVisibilityUseCase::removeFriendVisibility(const std::string& apiKey, 
                                                           const std::string& characterName, 
                                                           const std::string& friendName,
                                                           std::function<void(RemoveFriendVisibilityResult)> callback) {
    if (apiKey.empty() || characterName.empty() || friendName.empty()) {
        RemoveFriendVisibilityResult result;
        result.error = "API key, character name, and friend name required";
        result.userMessage = result.error;
        callback(result);
        return;
    }

    logger_.info("[friend] Removing friend visibility " + friendName + " via DELETE /api/friends/" + friendName + "/visibility");

    std::thread([this, apiKey, characterName, friendName, callback]() {
        RemoveFriendVisibilityResult result;
        
        std::string url = netClient_.getBaseUrl() + "/api/friends/" + friendName + "/visibility";
        
        auto requestFunc = [this, url, apiKey, characterName]() {
            return netClient_.del(url, apiKey, characterName, "");
        };
        
        XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "RemoveFriendVisibility");
        
        if (!response.isSuccess()) {
            std::string error;
            if (response.statusCode == 0) {
                error = response.error.empty() ? "Network error: failed to remove friend visibility" : response.error;
            } else if (response.statusCode == 404) {
                logger_.info("[friend] Friend not found (already removed)");
                result.success = true;
                result.userMessage = "Friend visibility removed";
                callback(result);
                return;
            } else if (response.statusCode >= 400) {
                XIFriendList::Protocol::ResponseMessage responseMsg;
                XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
                if (decodeResult == XIFriendList::Protocol::DecodeResult::Success && !responseMsg.error.empty()) {
                    error = "RemoveFriendVisibility failed: " + responseMsg.error;
                } else {
                    error = "RemoveFriendVisibility failed: HTTP " + std::to_string(response.statusCode);
                }
            } else {
                error = "RemoveFriendVisibility failed: HTTP " + std::to_string(response.statusCode);
            }
            logger_.error("[friend] " + error);
            result.success = false;
            result.error = error;
            result.userMessage = error;
            callback(result);
            return;
        }
        
        XIFriendList::Protocol::ResponseMessage responseMsg;
        XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, responseMsg);
        
        if (decodeResult != XIFriendList::Protocol::DecodeResult::Success || !responseMsg.success) {
            std::string error = responseMsg.error.empty() ? "Failed to remove friend visibility" : responseMsg.error;
            logger_.error("[friend] " + error);
            result.success = false;
            result.error = error;
            result.userMessage = error;
            callback(result);
            return;
        }
        
        if (!response.body.empty()) {
            bool friendshipDeleted = false;
            XIFriendList::Protocol::JsonUtils::extractBooleanField(response.body, "friendshipDeleted", friendshipDeleted);
            result.friendshipDeleted = friendshipDeleted;
        }
        
        logger_.info("[friend] Friend visibility " + friendName + " removed successfully");
        result.success = true;
        result.userMessage = "Friend removed from this character's view";
        callback(result);
    }).detach();
}

XIFriendList::App::HttpResponse RemoveFriendVisibilityUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc, const std::string& operationName) {
    const int maxRetries = 2;
    const uint64_t retryDelayMs = 1000;
    
    for (int attempt = 0; attempt <= maxRetries; ++attempt) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess() || response.statusCode == 404) {
            return response;
        }
        
        if (attempt < maxRetries && response.statusCode >= 500) {
            logger_.warning("[friend] " + operationName + " failed (attempt " + 
                          std::to_string(attempt + 1) + "/" + std::to_string(maxRetries + 1) + 
                          "), retrying in " + std::to_string(retryDelayMs) + "ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        }
        
        return response;
    }
    
    return requestFunc();
}


GetAltVisibilityResult GetAltVisibilityUseCase::getVisibility(const std::string& apiKey,
                                                              const std::string& characterName) {
    GetAltVisibilityResult result;
    if (apiKey.empty() || characterName.empty()) {
        result.success = false;
        result.error = "API key and character name required";
        return result;
    }
    
    logger_.debug("[friend] Getting alt visibility for " + characterName);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends/visibility";
    
    auto requestFunc = [this, url, apiKey, characterName]() {
        return netClient_.get(url, apiKey, characterName);
    };
    
    XIFriendList::App::HttpResponse response = executeWithRetry(requestFunc, "GetAltVisibility");
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to get alt visibility: " + error);
        result.success = false;
        result.error = error;
        return result;
    }
    
    return parseAltVisibilityResponse(response, characterName);
}

GetAltVisibilityResult GetAltVisibilityUseCase::parseAltVisibilityResponse(const XIFriendList::App::HttpResponse& response, const std::string& characterName) {
    bool success = false;
    if (!XIFriendList::Protocol::JsonUtils::extractBooleanField(response.body, "success", success) || !success) {
        std::string error;
        XIFriendList::Protocol::JsonUtils::extractStringField(response.body, "error", error);
        logger_.error("[friend] Server returned error: " + error);
        GetAltVisibilityResult result;
        result.success = false;
        result.error = error.empty() ? "Server returned success=false" : error;
        return result;
    }
    
    std::string friendsArrayJson;
    if (!XIFriendList::Protocol::JsonUtils::extractField(response.body, "friends", friendsArrayJson)) {
        logger_.error("[friend] Failed to extract friends array");
        GetAltVisibilityResult result;
        result.success = false;
        result.error = "Invalid response format: missing friends array";
        return result;
    }
    
    uint64_t serverTime = 0;
    XIFriendList::Protocol::JsonUtils::extractNumberField(response.body, "serverTime", serverTime);
    
    std::vector<XIFriendList::Protocol::AltVisibilityFriendEntry> friends;
    
    size_t arrayStart = friendsArrayJson.find('[');
    size_t arrayEnd = friendsArrayJson.rfind(']');
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        logger_.error("[friend] Invalid friends array format");
        GetAltVisibilityResult result;
        result.success = false;
        result.error = "Invalid friends array format";
        return result;
    }
    
    std::string arrayContent = friendsArrayJson.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    size_t pos = 0;
    while (pos < arrayContent.length()) {
        size_t objStart = arrayContent.find('{', pos);
        if (objStart == std::string::npos) {
            break;
        }
        
        int braceCount = 0;
        size_t objEnd = objStart;
        for (size_t i = objStart; i < arrayContent.length(); i++) {
            if (arrayContent[i] == '{') {
                braceCount++;
            } else if (arrayContent[i] == '}') {
                braceCount--;
                if (braceCount == 0) {
                    objEnd = i;
                    break;
                }
            }
        }
        
        if (braceCount != 0) {
            logger_.warning("[friend] Unmatched braces in friend entry");
            pos = objEnd + 1;
            continue;
        }
        
        std::string friendJson = arrayContent.substr(objStart, objEnd - objStart + 1);
        
        XIFriendList::Protocol::AltVisibilityFriendEntry entry;
        
        XIFriendList::Protocol::JsonUtils::extractNumberField(friendJson, "friendAccountId", entry.friendAccountId);
        XIFriendList::Protocol::JsonUtils::extractStringField(friendJson, "friendedAsName", entry.friendedAsName);
        XIFriendList::Protocol::JsonUtils::extractStringField(friendJson, "displayName", entry.displayName);
        XIFriendList::Protocol::JsonUtils::extractStringField(friendJson, "visibilityMode", entry.visibilityMode);
        XIFriendList::Protocol::JsonUtils::extractNumberField(friendJson, "createdAt", entry.createdAt);
        XIFriendList::Protocol::JsonUtils::extractNumberField(friendJson, "updatedAt", entry.updatedAt);
        
        std::string characterVisibilityJson;
        bool hasNewFormat = XIFriendList::Protocol::JsonUtils::extractField(friendJson, "characterVisibility", characterVisibilityJson);
        
        if (hasNewFormat && !characterVisibilityJson.empty()) {
            size_t charVisPos = 0;
            while ((charVisPos = characterVisibilityJson.find('"', charVisPos)) != std::string::npos) {
                size_t keyStart = charVisPos + 1;
                size_t keyEnd = characterVisibilityJson.find('"', keyStart);
                if (keyEnd == std::string::npos) break;
                
                std::string charIdStr = characterVisibilityJson.substr(keyStart, keyEnd - keyStart);
                int characterId = 0;
                try {
                    characterId = std::stoi(charIdStr);
                } catch (...) {
                    charVisPos = keyEnd + 1;
                    continue;
                }
                
                size_t valueStart = characterVisibilityJson.find('{', keyEnd);
                if (valueStart == std::string::npos) {
                    charVisPos = keyEnd + 1;
                    continue;
                }
                
                int charVisBraceCount = 0;
                size_t valueEnd = valueStart;
                for (size_t i = valueStart; i < characterVisibilityJson.length(); i++) {
                    if (characterVisibilityJson[i] == '{') {
                        charVisBraceCount++;
                    } else if (characterVisibilityJson[i] == '}') {
                        charVisBraceCount--;
                        if (charVisBraceCount == 0) {
                            valueEnd = i;
                            break;
                        }
                    }
                }
                
                if (charVisBraceCount != 0) {
                    charVisPos = keyEnd + 1;
                    continue;
                }
                
                std::string charVisJson = characterVisibilityJson.substr(valueStart, valueEnd - valueStart + 1);
                
                XIFriendList::Protocol::CharacterVisibilityState charVis;
                charVis.characterId = characterId;
                XIFriendList::Protocol::JsonUtils::extractStringField(charVisJson, "characterName", charVis.characterName);
                XIFriendList::Protocol::JsonUtils::extractBooleanField(charVisJson, "hasVisibility", charVis.hasVisibility);
                XIFriendList::Protocol::JsonUtils::extractBooleanField(charVisJson, "hasPendingVisibilityRequest", charVis.hasPendingVisibilityRequest);
                
                entry.characterVisibility.push_back(charVis);
                
                charVisPos = valueEnd + 1;
            }
        }
        
        if (entry.characterVisibility.empty()) {
            bool hasVisibility = false;
            bool hasPendingVisibilityRequest = false;
            XIFriendList::Protocol::JsonUtils::extractBooleanField(friendJson, "hasVisibility", hasVisibility);
            XIFriendList::Protocol::JsonUtils::extractBooleanField(friendJson, "hasPendingVisibilityRequest", hasPendingVisibilityRequest);
            
            XIFriendList::Protocol::CharacterVisibilityState charVis;
            charVis.characterId = 0;
            charVis.characterName = characterName;
            charVis.hasVisibility = hasVisibility;
            charVis.hasPendingVisibilityRequest = hasPendingVisibilityRequest;
            
            entry.characterVisibility.push_back(charVis);
        }
        
        friends.push_back(entry);
        
        pos = objEnd + 1;
    }
    
    std::string charactersArrayJson;
    std::vector<XIFriendList::Protocol::AccountCharacterInfo> characters;
    bool hasCharactersArray = XIFriendList::Protocol::JsonUtils::extractField(response.body, "characters", charactersArrayJson);
    
    if (hasCharactersArray && !charactersArrayJson.empty()) {
        size_t charsArrayStart = charactersArrayJson.find('[');
        size_t charsArrayEnd = charactersArrayJson.rfind(']');
        if (charsArrayStart != std::string::npos && charsArrayEnd != std::string::npos && charsArrayEnd > charsArrayStart) {
            std::string charsArrayContent = charactersArrayJson.substr(charsArrayStart + 1, charsArrayEnd - charsArrayStart - 1);
            size_t charsPos = 0;
            while (charsPos < charsArrayContent.length()) {
                size_t objStart = charsArrayContent.find('{', charsPos);
                if (objStart == std::string::npos) {
                    break;
                }
                
                int charsBraceCount = 0;
                size_t objEnd = objStart;
                for (size_t i = objStart; i < charsArrayContent.length(); i++) {
                    if (charsArrayContent[i] == '{') {
                        charsBraceCount++;
                    } else if (charsArrayContent[i] == '}') {
                        charsBraceCount--;
                        if (charsBraceCount == 0) {
                            objEnd = i;
                            break;
                        }
                    }
                }
                
                if (charsBraceCount != 0) {
                    charsPos = objEnd + 1;
                    continue;
                }
                
                std::string charJson = charsArrayContent.substr(objStart, objEnd - objStart + 1);
                
                XIFriendList::Protocol::AccountCharacterInfo charInfo;
                XIFriendList::Protocol::JsonUtils::extractNumberField(charJson, "characterId", charInfo.characterId);
                XIFriendList::Protocol::JsonUtils::extractStringField(charJson, "characterName", charInfo.characterName);
                XIFriendList::Protocol::JsonUtils::extractBooleanField(charJson, "isActive", charInfo.isActive);
                
                characters.push_back(charInfo);
                
                charsPos = objEnd + 1;
            }
        }
    }
    
    if (characters.empty()) {
        XIFriendList::Protocol::AccountCharacterInfo charInfo;
        charInfo.characterId = 0;
        charInfo.characterName = characterName;
        charInfo.isActive = true;
        characters.push_back(charInfo);
    }
    
    logger_.info("[friend] Retrieved " + std::to_string(friends.size()) + " friends with visibility state for " + std::to_string(characters.size()) + " characters");
    
    GetAltVisibilityResult result;
    result.success = true;
    result.friends = friends;
    result.characters = characters;
    result.serverTime = serverTime;
    return result;
}

XIFriendList::App::HttpResponse GetAltVisibilityUseCase::executeWithRetry(std::function<XIFriendList::App::HttpResponse()> requestFunc,
                                                       const std::string& operationName) {
    int attempts = 0;
    while (attempts < maxRetries_) {
        XIFriendList::App::HttpResponse response = requestFunc();
        
        if (response.isSuccess()) {
            return response;
        }
        
        attempts++;
        if (attempts < maxRetries_) {
            logger_.warning(operationName + ": Attempt " + std::to_string(attempts) + 
                           " failed, retrying in " + std::to_string(retryDelayMs_) + "ms");
            clock_.sleepMs(retryDelayMs_);
        }
    }
    
    return requestFunc();
}

PresenceUpdateResult UpdatePresenceUseCase::updatePresence(const std::string& apiKey,
                                                            const std::string& characterName,
                                                            const XIFriendList::Core::Presence& presence) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", {} };
    }
    
    logger_.debug("[friend] Updating character state for " + characterName);
    
    std::string requestJson = XIFriendList::Protocol::RequestEncoder::encodeUpdatePresence(presence);
    
    std::string url = netClient_.getBaseUrl() + "/api/characters/state";
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, apiKey, characterName, requestJson);
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to update character state: " + error);
        return { false, error, {} };
    }
    
    return parseStatusResponse(response);
}

PresenceUpdateResult UpdatePresenceUseCase::getStatus(const std::string& apiKey,
                                                       const std::string& characterName) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required", {} };
    }
    
    logger_.debug("[friend] Getting friend statuses for " + characterName);
    
    std::string url = netClient_.getBaseUrl() + "/api/friends";
    
    XIFriendList::App::HttpResponse response = netClient_.get(url, apiKey, characterName);
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to get friend list: " + error);
        return { false, error, {} };
    }
    
    return parseStatusResponse(response);
}

HeartbeatResult UpdatePresenceUseCase::getHeartbeat(const std::string& apiKey,
                                                     const std::string& characterName,
                                                     uint64_t lastEventTimestamp,
                                                     const std::string& pluginVersion) {
    if (apiKey.empty() || characterName.empty()) {
        HeartbeatResult result;
        result.success = false;
        result.error = "API key and character name required";
        return result;
    }
    
    logger_.debug("[friend] Sending heartbeat for " + characterName);
    
    std::string requestJson = XIFriendList::Protocol::RequestEncoder::encodeGetHeartbeat(
        characterName, lastEventTimestamp, 0, pluginVersion);
    
    std::string url = netClient_.getBaseUrl() + "/api/heartbeat";
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, apiKey, characterName, requestJson);
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("[friend] Failed to send heartbeat: " + error);
        HeartbeatResult result;
        result.success = false;
        result.error = error;
        return result;
    }
    
    return parseHeartbeatResponse(response);
}

PresenceUpdateResult UpdatePresenceUseCase::parseStatusResponse(const XIFriendList::App::HttpResponse& response) {
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode response";
        logger_.error("[friend] " + error);
        return { false, error, {} };
    }
    
    if (!msg.success) {
        std::string error = msg.error.empty() ? "Server returned failure" : msg.error;
        logger_.error("[friend] " + error);
        return { false, error, {} };
    }
    
    if (msg.type == XIFriendList::Protocol::ResponseType::StateUpdate) {
        logger_.debug("[friend] State update confirmed");
        return { true, "", {} };
    }
    
    if (msg.type == XIFriendList::Protocol::ResponseType::FriendList || 
        msg.type == XIFriendList::Protocol::ResponseType::Status) {
        std::string decodedPayload = msg.payload;
        if (!msg.payload.empty() && msg.payload[0] == '"') {
            XIFriendList::Protocol::JsonUtils::decodeString(msg.payload, decodedPayload);
        }
        
        XIFriendList::Protocol::StatusResponsePayload payload;
        XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeStatusPayload(
            decodedPayload, payload);
        
        if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
            logger_.debug("[friend] No status data in response (empty list)");
            return { true, "", {} };
        }
        
        std::vector<XIFriendList::Core::FriendStatus> statuses;
        for (const auto& statusData : payload.statuses) {
            XIFriendList::Core::FriendStatus status;
            status.characterName = statusData.characterName;
            status.displayName = statusData.displayName.empty() ? statusData.characterName : statusData.displayName;
            status.isOnline = statusData.isOnline;
            status.job = statusData.job;
            status.rank = statusData.rank;
            status.nation = statusData.nation;
            status.zone = statusData.zone;
            status.lastSeenAt = statusData.lastSeenAt;
            status.showOnlineStatus = statusData.showOnlineStatus;
            status.isLinkedCharacter = statusData.isLinkedCharacter;
            status.isOnAltCharacter = statusData.isOnAltCharacter;
            status.altCharacterName = statusData.altCharacterName;
            status.friendedAs = statusData.friendedAs;
            status.linkedCharacters = statusData.linkedCharacters;
            statuses.push_back(status);
        }
        
        logger_.info("[friend] Successfully retrieved " + 
                    std::to_string(statuses.size()) + " friend statuses");
        return { true, "", statuses };
    }
    
    logger_.debug("[friend] Unknown response type, returning empty list");
    return { true, "", {} };
}

HeartbeatResult UpdatePresenceUseCase::parseHeartbeatResponse(const XIFriendList::App::HttpResponse& response) {
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    HeartbeatResult result;
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode response";
        logger_.error("[friend] " + error);
        result.success = false;
        result.error = error;
        return result;
    }
    
    if (!msg.success || msg.type != XIFriendList::Protocol::ResponseType::Heartbeat) {
        std::string error = msg.error.empty() ? "Invalid response type" : msg.error;
        logger_.error("[friend] " + error);
        result.success = false;
        result.error = error;
        return result;
    }
    
    std::string isOutdatedStr;
    XIFriendList::Protocol::JsonUtils::extractField(response.body, "is_outdated", isOutdatedStr);
    if (!isOutdatedStr.empty()) {
        result.isOutdated = (isOutdatedStr == "true" || isOutdatedStr == "1");
    }
    XIFriendList::Protocol::JsonUtils::extractField(response.body, "latest_version", result.latestVersion);
    XIFriendList::Protocol::JsonUtils::extractField(response.body, "release_url", result.releaseUrl);
    
    XIFriendList::Protocol::HeartbeatResponsePayload payload;
    XIFriendList::Protocol::DecodeResult payloadResult = XIFriendList::Protocol::ResponseDecoder::decodeHeartbeatPayload(
        msg.payload, payload);
    
    if (payloadResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode heartbeat payload";
        logger_.error("[friend] " + error);
        result.success = false;
        result.error = error;
        return result;
    }
    
    std::vector<XIFriendList::Core::FriendStatus> statuses;
    for (const auto& statusData : payload.statuses) {
        XIFriendList::Core::FriendStatus status;
        status.characterName = statusData.characterName;
        status.displayName = statusData.displayName.empty() ? statusData.characterName : statusData.displayName;
        status.isOnline = statusData.isOnline;
        status.job = statusData.job;
        status.rank = statusData.rank;
        status.nation = statusData.nation;
        status.zone = statusData.zone;
        status.lastSeenAt = statusData.lastSeenAt;
        status.showOnlineStatus = statusData.showOnlineStatus;
        status.isLinkedCharacter = statusData.isLinkedCharacter;
        status.isOnAltCharacter = statusData.isOnAltCharacter;
        status.altCharacterName = statusData.altCharacterName;
        status.friendedAs = statusData.friendedAs;
        status.linkedCharacters = statusData.linkedCharacters;
        statuses.push_back(status);
    }
    
    logger_.info("[friend] Successfully retrieved heartbeat (" + 
                std::to_string(statuses.size()) + " statuses, " + 
                std::to_string(payload.events.size()) + " events)");
    
    result.success = true;
    result.friendStatuses = statuses;
    result.events = payload.events;
    return result;
}

bool UpdatePresenceUseCase::shouldShowOutdatedWarning(const HeartbeatResult& result, std::string& warningMessage) {
    if (!result.isOutdated || result.latestVersion.empty()) {
        return false;
    }
    
    const uint64_t now = clock_.nowMs();
    const uint64_t THROTTLE_WINDOW_MS = 6 * 60 * 60 * 1000; // 6 hours
    
    if (result.latestVersion != lastWarnedLatestVersion_) {
        lastWarnedLatestVersion_ = result.latestVersion;
        lastWarnAtMs_ = now;
        
        std::ostringstream msg;
        msg << "[FriendList] Update available: you're on " << Plugin::PLUGIN_VERSION_STRING 
            << ", latest is " << result.latestVersion;
        if (!result.releaseUrl.empty()) {
            msg << ". " << result.releaseUrl;
        }
        warningMessage = msg.str();
        return true;
    }
    
    if (now - lastWarnAtMs_ >= THROTTLE_WINDOW_MS) {
        lastWarnAtMs_ = now;
        
        std::ostringstream msg;
        msg << "[FriendList] Update available: you're on " << Plugin::PLUGIN_VERSION_STRING 
            << ", latest is " << result.latestVersion;
        if (!result.releaseUrl.empty()) {
            msg << ". " << result.releaseUrl;
        }
        warningMessage = msg.str();
        return true;
    }
    
    return false;
}

UpdateMyStatusResult UpdateMyStatusUseCase::updateStatus(const std::string& apiKey,
                                                         const std::string& characterName,
                                                         bool showOnlineStatus,
                                                         bool shareLocation,
                                                         bool isAnonymous,
                                                         bool shareJobWhenAnonymous) {
    if (apiKey.empty() || characterName.empty()) {
        return { false, "API key and character name required" };
    }
    
    logger_.debug("UpdateMyStatusUseCase: Updating status flags");
    
    std::string url = netClient_.getBaseUrl() + "/api/characters/privacy";
    
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"shareOnlineStatus", XIFriendList::Protocol::JsonUtils::encodeBoolean(showOnlineStatus)});
    payloadFields.push_back({"shareLocation", XIFriendList::Protocol::JsonUtils::encodeBoolean(shareLocation)});
    payloadFields.push_back({"isAnonymous", XIFriendList::Protocol::JsonUtils::encodeBoolean(isAnonymous)});
    payloadFields.push_back({"shareJobWhenAnonymous", XIFriendList::Protocol::JsonUtils::encodeBoolean(shareJobWhenAnonymous)});
    std::string requestJson = XIFriendList::Protocol::JsonUtils::encodeObject(payloadFields);
    
    XIFriendList::App::HttpResponse response = netClient_.post(url, apiKey, characterName, requestJson);
    
    if (!response.isSuccess()) {
        std::string error = response.error.empty() ? 
            "HTTP " + std::to_string(response.statusCode) : response.error;
        logger_.error("UpdateMyStatusUseCase: Failed to update status: " + error);
        return { false, error };
    }
    
    XIFriendList::Protocol::ResponseMessage msg;
    XIFriendList::Protocol::DecodeResult decodeResult = XIFriendList::Protocol::ResponseDecoder::decode(response.body, msg);
    
    if (decodeResult != XIFriendList::Protocol::DecodeResult::Success) {
        std::string error = "Failed to decode response";
        logger_.error("UpdateMyStatusUseCase: " + error);
        return { false, error };
    }
    
    if (!msg.success) {
        std::string error = msg.error.empty() ? "Update failed" : msg.error;
        logger_.error("UpdateMyStatusUseCase: " + error);
        return { false, error };
    }
    
    logger_.debug("UpdateMyStatusUseCase: Status updated successfully");
    return { true };
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList
