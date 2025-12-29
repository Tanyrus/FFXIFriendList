
#include "ResponseDecoder.h"
#include "JsonUtils.h"
#include "ProtocolVersion.h"
#include <algorithm>

namespace XIFriendList {
namespace Protocol {

DecodeResult ResponseDecoder::decode(const std::string& json, ResponseMessage& out) {
    out = ResponseMessage();
    
    if (!JsonUtils::isValidJson(json)) {
        return DecodeResult::InvalidJson;
    }
    
    std::string versionField;
    if (!JsonUtils::extractField(json, "protocolVersion", versionField)) {
        return DecodeResult::MissingField;
    }
    
    if (!JsonUtils::decodeString(versionField, out.protocolVersion)) {
        return DecodeResult::MissingField;
    }
    
    if (!validateVersion(out.protocolVersion)) {
        return DecodeResult::InvalidVersion;
    }
    std::string typeField;
    if (!JsonUtils::extractField(json, "type", typeField)) {
        return DecodeResult::MissingField;
    }
    
    std::string typeStr;
    if (!JsonUtils::decodeString(typeField, typeStr)) {
        return DecodeResult::InvalidType;
    }
    
    if (!stringToResponseType(typeStr, out.type)) {
        return DecodeResult::InvalidType;
    }
    if (!JsonUtils::extractBooleanField(json, "success", out.success)) {
        return DecodeResult::MissingField;
    }
    JsonUtils::extractField(json, "payload", out.payload);
    
    // This handles server responses that put data directly in the response instead of in a payload field
    if (out.payload.empty()) {
        std::string friendsArray;
        bool hasFriends = JsonUtils::extractField(json, "friends", friendsArray);
        
        std::string eventsArray;
        bool hasEvents = JsonUtils::extractField(json, "events", eventsArray);
        
        if (hasFriends && hasEvents) {
            // HeartbeatResponse: synthesize statuses from friends for heartbeat decoder
            // The server uses "friends" but plugin expects "statuses"
            out.payload = "{\"statuses\":" + friendsArray + ",\"events\":" + eventsArray + "}";
        } else if (hasFriends) {
            // The server's friend view response includes full status data
            out.payload = "{\"statuses\":" + friendsArray + "}";
        }
        
        std::string statusesArray;
        if (out.payload.empty() && JsonUtils::extractField(json, "statuses", statusesArray)) {
            out.payload = "{\"statuses\":" + statusesArray + "}";
        }
        
        std::string messagesArray;
        if (out.payload.empty() && JsonUtils::extractField(json, "messages", messagesArray)) {
            out.payload = "{\"messages\":" + messagesArray + "}";
        }
        
        std::string incomingArray, outgoingArray;
        if (out.payload.empty()) {
            bool hasIncoming = JsonUtils::extractField(json, "incoming", incomingArray);
            bool hasOutgoing = JsonUtils::extractField(json, "outgoing", outgoingArray);
            if (hasIncoming || hasOutgoing) {
                out.payload = "{\"incoming\":" + (hasIncoming ? incomingArray : "[]") + 
                              ",\"outgoing\":" + (hasOutgoing ? outgoingArray : "[]") + "}";
            }
        }
        
        int64_t unreadCount = 0;
        if (out.payload.empty() && JsonUtils::extractNumberField(json, "unreadCount", unreadCount)) {
            out.payload = "{\"unreadCount\":" + std::to_string(unreadCount) + "}";
        }
        
        std::string prefsObj;
        if (out.payload.empty() && JsonUtils::extractField(json, "preferences", prefsObj)) {
            out.payload = prefsObj;  // Pass the preferences object directly
        }
        
        std::string notesArray;
        if (out.payload.empty() && JsonUtils::extractField(json, "notes", notesArray)) {
            out.payload = "{\"notes\":" + notesArray + "}";
        }
        
        std::string noteObj;
        if (out.payload.empty() && JsonUtils::extractField(json, "note", noteObj)) {
            out.payload = "{\"note\":" + noteObj + "}";
        }
        
        std::string scenariosArray;
        if (out.payload.empty() && JsonUtils::extractField(json, "scenarios", scenariosArray)) {
            out.payload = "{\"scenarios\":" + scenariosArray + "}";
        }
        
        std::string linkedCharsArray;
        std::string charName;
        if (out.payload.empty() && JsonUtils::extractField(json, "linkedCharacters", linkedCharsArray)) {
            JsonUtils::extractStringField(json, "characterName", charName);
            out.payload = "{\"characterName\":\"" + charName + "\",\"linkedCharacters\":" + linkedCharsArray + "}";
        }
        
        std::string msgId;
        int64_t sentAt = 0;
        if (out.payload.empty() && JsonUtils::extractStringField(json, "messageId", msgId)) {
            JsonUtils::extractNumberField(json, "sentAt", sentAt);
            out.payload = "{\"messageId\":\"" + msgId + "\",\"createdAt\":" + std::to_string(sentAt) + "}";
        }
        
        // These fields are often sent together for friend request/add friend responses
        std::string requestId;
        std::string action;
        std::string message;
        bool hasRequestId = JsonUtils::extractStringField(json, "requestId", requestId);
        bool hasAction = JsonUtils::extractStringField(json, "action", action);
        bool hasMessage = JsonUtils::extractStringField(json, "message", message);
        
        if (out.payload.empty() && (hasRequestId || hasAction || hasMessage)) {
            std::string payloadFields = "{";
            bool first = true;
            if (hasRequestId) {
                if (!first) payloadFields += ",";
                payloadFields += "\"requestId\":\"" + requestId + "\"";
                first = false;
            }
            if (hasAction) {
                if (!first) payloadFields += ",";
                payloadFields += "\"action\":\"" + action + "\"";
                first = false;
            }
            if (hasMessage) {
                if (!first) payloadFields += ",";
                payloadFields += "\"message\":\"" + message + "\"";
                first = false;
            }
            payloadFields += "}";
            out.payload = payloadFields;
        } else if (out.payload.empty() && hasRequestId) {
            out.payload = "{\"requestId\":\"" + requestId + "\"}";
        } else if (out.payload.empty() && hasMessage) {
            out.payload = "{\"message\":\"" + message + "\"}";
        }
    }
    
    JsonUtils::extractStringField(json, "error", out.error);
    
    JsonUtils::extractStringField(json, "errorCode", out.errorCode);
    
    JsonUtils::extractField(json, "details", out.details);
    
    JsonUtils::extractStringField(json, "requestId", out.requestId);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeFriendListPayload(const std::string& payloadJson, 
                                                      FriendListResponsePayload& out) {
    out = FriendListResponsePayload();
    
    if (payloadJson.empty()) {
        return DecodeResult::MissingField;  // Empty payload
    }
    
    // So payloadJson might be a JSON-encoded string like "{\"friendsData\":[...]}"
    // Try to decode it first
    std::string decodedPayload = payloadJson;
    if (payloadJson[0] == '"' && payloadJson.length() > 1) {
        // Might be a JSON-encoded string, try to decode it
        std::string temp;
        if (JsonUtils::decodeString(payloadJson, temp)) {
            decodedPayload = temp;
        }
    }
    
    // Canonical format: statuses array (array of status objects)
    std::string statusesArray;
    if (!JsonUtils::extractField(decodedPayload, "statuses", statusesArray)) {
        return DecodeResult::MissingField;  // No statuses field
    }
    
    if (statusesArray.empty() || statusesArray[0] != '[') {
        return DecodeResult::InvalidPayload;
    }
    
    std::string& friendsDataArray = statusesArray;  // Alias for existing code below
    
    size_t pos = 1;
    while (pos < friendsDataArray.length()) {
        while (pos < friendsDataArray.length() && 
               std::isspace(static_cast<unsigned char>(friendsDataArray[pos]))) {
            ++pos;
        }
        
        if (pos >= friendsDataArray.length()) break;
        
        if (friendsDataArray[pos] == ']') break;
        
        if (friendsDataArray[pos] != '{') {
            return DecodeResult::InvalidPayload;
        }
        
        // Find matching closing brace
        int depth = 1;
        size_t objStart = pos;
        size_t objEnd = pos + 1;
        while (objEnd < friendsDataArray.length() && depth > 0) {
            if (friendsDataArray[objEnd] == '{') ++depth;
            else if (friendsDataArray[objEnd] == '}') --depth;
            else if (friendsDataArray[objEnd] == '"') {
                ++objEnd;
                while (objEnd < friendsDataArray.length() && friendsDataArray[objEnd] != '"') {
                    if (friendsDataArray[objEnd] == '\\' && objEnd + 1 < friendsDataArray.length()) {
                        objEnd += 2;
                    } else {
                        ++objEnd;
                    }
                }
            }
            ++objEnd;
        }
        
        if (depth != 0) {
            return DecodeResult::InvalidPayload;
        }
        
        std::string objJson = friendsDataArray.substr(objStart, objEnd - objStart);
        FriendData friendData;
        DecodeResult result = decodeFriendData(objJson, friendData);
        if (result != DecodeResult::Success) {
            return result;
        }
        out.friendsData.push_back(friendData);
        
        pos = objEnd;
        while (pos < friendsDataArray.length() && 
               std::isspace(static_cast<unsigned char>(friendsDataArray[pos]))) {
            ++pos;
        }
        
        if (pos < friendsDataArray.length() && friendsDataArray[pos] == ',') {
            ++pos;
        }
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeStatusPayload(const std::string& payloadJson,
                                                  StatusResponsePayload& out) {
    out = StatusResponsePayload();
    
    std::string statusesArray;
    if (!JsonUtils::extractField(payloadJson, "statuses", statusesArray)) {
        return DecodeResult::MissingField;
    }
    
    if (statusesArray.empty() || statusesArray[0] != '[') {
        return DecodeResult::InvalidPayload;
    }
    
    size_t pos = 1;
    while (pos < statusesArray.length()) {
        while (pos < statusesArray.length() && 
               std::isspace(static_cast<unsigned char>(statusesArray[pos]))) {
            ++pos;
        }
        
        if (pos >= statusesArray.length()) break;
        if (statusesArray[pos] == ']') break;
        
        if (statusesArray[pos] != '{') {
            return DecodeResult::InvalidPayload;
        }
        
        int depth = 1;
        size_t objStart = pos;
        size_t objEnd = pos + 1;
        while (objEnd < statusesArray.length() && depth > 0) {
            if (statusesArray[objEnd] == '{') ++depth;
            else if (statusesArray[objEnd] == '}') --depth;
            else if (statusesArray[objEnd] == '"') {
                ++objEnd;
                while (objEnd < statusesArray.length() && statusesArray[objEnd] != '"') {
                    if (statusesArray[objEnd] == '\\' && objEnd + 1 < statusesArray.length()) {
                        objEnd += 2;
                    } else {
                        ++objEnd;
                    }
                }
            }
            ++objEnd;
        }
        
        if (depth != 0) {
            return DecodeResult::InvalidPayload;
        }
        
        std::string objJson = statusesArray.substr(objStart, objEnd - objStart);
        FriendStatusData statusData;
        DecodeResult result = decodeFriendStatusData(objJson, statusData);
        if (result != DecodeResult::Success) {
            return result;
        }
        out.statuses.push_back(statusData);
        
        pos = objEnd;
        while (pos < statusesArray.length() && 
               std::isspace(static_cast<unsigned char>(statusesArray[pos]))) {
            ++pos;
        }
        
        if (pos < statusesArray.length() && statusesArray[pos] == ',') {
            ++pos;
        }
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeFriendRequestPayload(const std::string& payloadJson,
                                                         FriendRequestPayload& out) {
    return decodeFriendRequestData(payloadJson, out);
}

DecodeResult ResponseDecoder::decodeFriendRequestsPayload(const std::string& payloadJson,
                                                          FriendRequestsResponsePayload& out) {
    out = FriendRequestsResponsePayload();
    
    std::string incomingArray;
    if (JsonUtils::extractField(payloadJson, "incoming", incomingArray)) {
        if (incomingArray[0] == '[') {
            size_t pos = 1;
            while (pos < incomingArray.length()) {
                while (pos < incomingArray.length() && 
                       std::isspace(static_cast<unsigned char>(incomingArray[pos]))) {
                    ++pos;
                }
                
                if (pos >= incomingArray.length() || incomingArray[pos] == ']') break;
                
                if (incomingArray[pos] != '{') {
                    return DecodeResult::InvalidPayload;
                }
                
                int depth = 1;
                size_t objStart = pos;
                size_t objEnd = pos + 1;
                while (objEnd < incomingArray.length() && depth > 0) {
                    if (incomingArray[objEnd] == '{') ++depth;
                    else if (incomingArray[objEnd] == '}') --depth;
                    else if (incomingArray[objEnd] == '"') {
                        ++objEnd;
                        while (objEnd < incomingArray.length() && incomingArray[objEnd] != '"') {
                            if (incomingArray[objEnd] == '\\' && objEnd + 1 < incomingArray.length()) {
                                objEnd += 2;
                            } else {
                                ++objEnd;
                            }
                        }
                    }
                    ++objEnd;
                }
                
                std::string objJson = incomingArray.substr(objStart, objEnd - objStart);
                FriendRequestPayload request;
                DecodeResult result = decodeFriendRequestData(objJson, request);
                if (result != DecodeResult::Success) {
                    return result;
                }
                out.incoming.push_back(request);
                
                pos = objEnd;
                while (pos < incomingArray.length() && 
                       std::isspace(static_cast<unsigned char>(incomingArray[pos]))) {
                    ++pos;
                }
                
                if (pos < incomingArray.length() && incomingArray[pos] == ',') {
                    ++pos;
                }
            }
        }
    }
    
    std::string outgoingArray;
    if (JsonUtils::extractField(payloadJson, "outgoing", outgoingArray)) {
        if (outgoingArray[0] == '[') {
            size_t pos = 1;
            while (pos < outgoingArray.length()) {
                while (pos < outgoingArray.length() && 
                       std::isspace(static_cast<unsigned char>(outgoingArray[pos]))) {
                    ++pos;
                }
                
                if (pos >= outgoingArray.length() || outgoingArray[pos] == ']') break;
                
                if (outgoingArray[pos] != '{') {
                    return DecodeResult::InvalidPayload;
                }
                
                int depth = 1;
                size_t objStart = pos;
                size_t objEnd = pos + 1;
                while (objEnd < outgoingArray.length() && depth > 0) {
                    if (outgoingArray[objEnd] == '{') ++depth;
                    else if (outgoingArray[objEnd] == '}') --depth;
                    else if (outgoingArray[objEnd] == '"') {
                        ++objEnd;
                        while (objEnd < outgoingArray.length() && outgoingArray[objEnd] != '"') {
                            if (outgoingArray[objEnd] == '\\' && objEnd + 1 < outgoingArray.length()) {
                                objEnd += 2;
                            } else {
                                ++objEnd;
                            }
                        }
                    }
                    ++objEnd;
                }
                
                std::string objJson = outgoingArray.substr(objStart, objEnd - objStart);
                FriendRequestPayload request;
                DecodeResult result = decodeFriendRequestData(objJson, request);
                if (result != DecodeResult::Success) {
                    return result;
                }
                out.outgoing.push_back(request);
                
                pos = objEnd;
                while (pos < outgoingArray.length() && 
                       std::isspace(static_cast<unsigned char>(outgoingArray[pos]))) {
                    ++pos;
                }
                
                if (pos < outgoingArray.length() && outgoingArray[pos] == ',') {
                    ++pos;
                }
            }
        }
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeHeartbeatPayload(const std::string& payloadJson,
                                                     HeartbeatResponsePayload& out) {
    out = HeartbeatResponsePayload();
    
    std::string statusesJson;
    if (JsonUtils::extractField(payloadJson, "statuses", statusesJson)) {
        StatusResponsePayload statusPayload;
        DecodeResult result = decodeStatusPayload("{\"statuses\":" + statusesJson + "}", statusPayload);
        if (result == DecodeResult::Success) {
            out.statuses = statusPayload.statuses;
        }
    }
    
    // NOTE: Server currently returns a mixed "events" array (online events, request events, etc.).
    // For backward compatibility, we decode only well-formed FriendRequestPayload items and
    // silently skip unknown event shapes instead of failing the entire heartbeat decode.
    std::string eventsJson;
    if (JsonUtils::extractField(payloadJson, "events", eventsJson)) {
        if (eventsJson[0] == '[') {
            size_t pos = 1;
            while (pos < eventsJson.length()) {
                while (pos < eventsJson.length() && 
                       std::isspace(static_cast<unsigned char>(eventsJson[pos]))) {
                    ++pos;
                }
                
                if (pos >= eventsJson.length() || eventsJson[pos] == ']') break;
                
                if (eventsJson[pos] != '{') {
                    break;
                }
                
                int depth = 1;
                size_t objStart = pos;
                size_t objEnd = pos + 1;
                while (objEnd < eventsJson.length() && depth > 0) {
                    if (eventsJson[objEnd] == '{') ++depth;
                    else if (eventsJson[objEnd] == '}') --depth;
                    else if (eventsJson[objEnd] == '"') {
                        ++objEnd;
                        while (objEnd < eventsJson.length() && eventsJson[objEnd] != '"') {
                            if (eventsJson[objEnd] == '\\' && objEnd + 1 < eventsJson.length()) {
                                objEnd += 2;
                            } else {
                                ++objEnd;
                            }
                        }
                    }
                    ++objEnd;
                }
                
                std::string objJson = eventsJson.substr(objStart, objEnd - objStart);
                FriendRequestPayload request;
                DecodeResult result = decodeFriendRequestData(objJson, request);
                if (result == DecodeResult::Success) {
                    out.events.push_back(request);
                }
                
                pos = objEnd;
                while (pos < eventsJson.length() && 
                       std::isspace(static_cast<unsigned char>(eventsJson[pos]))) {
                    ++pos;
                }
                
                if (pos < eventsJson.length() && eventsJson[pos] == ',') {
                    ++pos;
                }
            }
        }
    }
    
    JsonUtils::extractNumberField(payloadJson, "lastEventTimestamp", out.lastEventTimestamp);
    JsonUtils::extractNumberField(payloadJson, "lastRequestEventTimestamp", out.lastRequestEventTimestamp);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeFriendStatusData(const std::string& json, FriendStatusData& out) {
    out = FriendStatusData();
    
    // Server uses "name" for the active character name (canonical format)
    if (!JsonUtils::extractStringField(json, "name", out.characterName)) {
        return DecodeResult::MissingField;
    }
    
    // displayName is the same as name in canonical format
    out.displayName = out.characterName;
    
    JsonUtils::extractBooleanField(json, "isOnline", out.isOnline);
    JsonUtils::extractStringField(json, "job", out.job);
    JsonUtils::extractStringField(json, "rank", out.rank);
    JsonUtils::extractStringField(json, "zone", out.zone);
    
    std::string lastSeenAtValue;
    if (JsonUtils::extractField(json, "lastSeenAt", lastSeenAtValue)) {
        if (lastSeenAtValue == "null" || lastSeenAtValue.empty()) {
            out.lastSeenAt = 0;  // "Never"
        } else {
            uint64_t lastSeenAtNum = 0;
            if (JsonUtils::decodeNumber(lastSeenAtValue, lastSeenAtNum)) {
                out.lastSeenAt = lastSeenAtNum;
            } else {
                out.lastSeenAt = 0;  // Invalid value, treat as "Never"
            }
        }
    }
    
    // Server sends nation as number or null - handle both
    // Use -1 as sentinel value for "not set" or "hidden"
    out.nation = -1;  // Default to hidden/not set
    int nationValue;
    if (JsonUtils::extractNumberField(json, "nation", nationValue)) {
        // Server sent a valid nation value (0-3)
        out.nation = nationValue;
    }
    
    // Server uses "friendedAsName" (canonical format)
    JsonUtils::extractStringField(json, "friendedAsName", out.friendedAs);
    
    // Server provides linkedCharacters array
    JsonUtils::extractStringArrayField(json, "linkedCharacters", out.linkedCharacters);
    
    // Server computes visibility; sharesOnlineStatus indicates whether online status is shared
    JsonUtils::extractBooleanField(json, "sharesOnlineStatus", out.showOnlineStatus);
    if (!out.showOnlineStatus) {
        out.showOnlineStatus = true;  // Default to true if not specified
    }
    
    // These are plugin-computed fields based on linkedCharacters
    out.isLinkedCharacter = out.linkedCharacters.size() > 1;
    out.isOnAltCharacter = false;  // Would need to compare with expected name
    out.altCharacterName = "";
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeFriendData(const std::string& json, FriendData& out) {
    out = FriendData();
    
    // Accept both "name" (legacy) and "characterName" (canonical) for active character name
    if (!JsonUtils::extractStringField(json, "name", out.name)) {
        if (!JsonUtils::extractStringField(json, "characterName", out.name)) {
            return DecodeResult::MissingField;
        }
    }
    
    // Accept both "friendedAsName" (legacy) and "friendedAs" (canonical)
    if (!JsonUtils::extractStringField(json, "friendedAsName", out.friendedAs)) {
        JsonUtils::extractStringField(json, "friendedAs", out.friendedAs);
    }
    
    // Server provides linkedCharacters array with all character names for this friend's account
    JsonUtils::extractStringArrayField(json, "linkedCharacters", out.linkedCharacters);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeFriendRequestData(const std::string& json, FriendRequestPayload& out) {
    out = FriendRequestPayload();
    
    if (!JsonUtils::extractStringField(json, "requestId", out.requestId)) {
        return DecodeResult::MissingField;
    }
    
    JsonUtils::extractStringField(json, "fromCharacterName", out.fromCharacterName);
    JsonUtils::extractStringField(json, "toCharacterName", out.toCharacterName);
    JsonUtils::extractNumberField(json, "fromAccountId", out.fromAccountId);
    JsonUtils::extractNumberField(json, "toAccountId", out.toAccountId);
    JsonUtils::extractStringField(json, "status", out.status);
    JsonUtils::extractNumberField(json, "createdAt", out.createdAt);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodePreferencesPayload(const std::string& payloadJson,
                                                      PreferencesResponsePayload& out) {
    out = PreferencesResponsePayload();
    
    // All fields are optional with defaults, so we just extract what's available
    JsonUtils::extractBooleanField(payloadJson, "useServerNotes", out.useServerNotes);
    JsonUtils::extractBooleanField(payloadJson, "shareFriendsAcrossAlts", out.shareFriendsAcrossAlts);
    JsonUtils::extractBooleanField(payloadJson, "showFriendedAsColumn", out.showFriendedAsColumn);
    JsonUtils::extractBooleanField(payloadJson, "showJobColumn", out.showJobColumn);
    JsonUtils::extractBooleanField(payloadJson, "showRankColumn", out.showRankColumn);
    JsonUtils::extractBooleanField(payloadJson, "showNationColumn", out.showNationColumn);
    JsonUtils::extractBooleanField(payloadJson, "showZoneColumn", out.showZoneColumn);
    JsonUtils::extractBooleanField(payloadJson, "showLastSeenColumn", out.showLastSeenColumn);

    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowFriendedAsColumn", out.quickOnlineShowFriendedAsColumn);
    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowJobColumn", out.quickOnlineShowJobColumn);
    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowRankColumn", out.quickOnlineShowRankColumn);
    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowNationColumn", out.quickOnlineShowNationColumn);
    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowZoneColumn", out.quickOnlineShowZoneColumn);
    JsonUtils::extractBooleanField(payloadJson, "quickOnlineShowLastSeenColumn", out.quickOnlineShowLastSeenColumn);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeMailMessageData(const std::string& json, MailMessageData& out) {
    out = MailMessageData();
    
    if (!JsonUtils::extractStringField(json, "messageId", out.messageId)) {
        return DecodeResult::MissingField;
    }
    
    // Server uses fromName/toName (new canonical format)
    JsonUtils::extractStringField(json, "fromName", out.fromUserId);
    JsonUtils::extractStringField(json, "toName", out.toUserId);
    
    JsonUtils::extractStringField(json, "subject", out.subject);
    JsonUtils::extractStringField(json, "body", out.body);
    
    // Server uses sentAt (new canonical format)
    JsonUtils::extractNumberField(json, "sentAt", out.createdAt);
    JsonUtils::extractNumberField(json, "readAt", out.readAt);
    JsonUtils::extractBooleanField(json, "isRead", out.isRead);
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeMailPayload(const std::string& payloadJson, MailMessageData& out) {
    return decodeMailMessageData(payloadJson, out);
}

DecodeResult ResponseDecoder::decodeMailListPayload(const std::string& payloadJson, MailListResponsePayload& out) {
    out = MailListResponsePayload();
    
    std::string messagesArray;
    if (!JsonUtils::extractField(payloadJson, "messages", messagesArray)) {
        // Try direct array (if payload is just the array)
        if (payloadJson[0] == '[') {
            messagesArray = payloadJson;
        } else {
            return DecodeResult::MissingField;
        }
    }
    
    if (messagesArray[0] == '[') {
        size_t pos = 1;
        while (pos < messagesArray.length()) {
            while (pos < messagesArray.length() && 
                   std::isspace(static_cast<unsigned char>(messagesArray[pos]))) {
                ++pos;
            }
            
            if (pos >= messagesArray.length() || messagesArray[pos] == ']') break;
            
            if (messagesArray[pos] != '{') {
                return DecodeResult::InvalidPayload;
            }
            
            int depth = 1;
            size_t objStart = pos;
            size_t objEnd = pos + 1;
            while (objEnd < messagesArray.length() && depth > 0) {
                if (messagesArray[objEnd] == '{') ++depth;
                else if (messagesArray[objEnd] == '}') --depth;
                else if (messagesArray[objEnd] == '"') {
                    ++objEnd;
                    while (objEnd < messagesArray.length()) {
                        if (messagesArray[objEnd] == '\\') {
                            objEnd += 2;  // Skip escaped character
                        } else if (messagesArray[objEnd] == '"') {
                            ++objEnd;
                            break;
                        } else {
                            ++objEnd;
                        }
                    }
                    continue;
                }
                ++objEnd;
            }
            
            if (depth != 0) {
                return DecodeResult::InvalidPayload;
            }
            
            std::string objJson = messagesArray.substr(objStart, objEnd - objStart);
            MailMessageData message;
            DecodeResult result = decodeMailMessageData(objJson, message);
            if (result != DecodeResult::Success) {
                return result;
            }
            out.messages.push_back(message);
            
            pos = objEnd;
            while (pos < messagesArray.length() && 
                   std::isspace(static_cast<unsigned char>(messagesArray[pos]))) {
                ++pos;
            }
            
            if (pos < messagesArray.length() && messagesArray[pos] == ',') {
                ++pos;
            }
        }
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeMailUnreadCountPayload(const std::string& payloadJson, MailUnreadCountResponsePayload& out) {
    out = MailUnreadCountResponsePayload();
    
    // Server uses "unreadCount" (new canonical format)
    JsonUtils::extractNumberField(payloadJson, "unreadCount", out.count);
    
    return DecodeResult::Success;
}

bool ResponseDecoder::validateVersion(const std::string& version) {
    Version v;
    if (!Version::parse(version, v)) {
        return false;
    }
    
    Version current = getCurrentVersion();
    return v.isCompatibleWith(current);
}

DecodeResult ResponseDecoder::decodeFeedbackResponsePayload(const std::string& payloadJson, FeedbackResponsePayload& out) {
    out = FeedbackResponsePayload();
    
    // Server returns feedbackId directly in response body (not in payload field)
    if (!JsonUtils::extractNumberField(payloadJson, "feedbackId", out.feedbackId)) {
        return DecodeResult::MissingField;
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeIssueResponsePayload(const std::string& payloadJson, IssueResponsePayload& out) {
    out = IssueResponsePayload();
    
    // Server returns issueId directly in response body (not in payload field)
    if (!JsonUtils::extractNumberField(payloadJson, "issueId", out.issueId)) {
        return DecodeResult::MissingField;
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeNoteData(const std::string& json, NoteData& out) {
    out = NoteData();
    
    if (!JsonUtils::extractStringField(json, "friendName", out.friendName)) {
        return DecodeResult::MissingField;
    }
    if (!JsonUtils::extractStringField(json, "note", out.note)) {
        return DecodeResult::MissingField;
    }
    if (!JsonUtils::extractNumberField(json, "updatedAt", out.updatedAt)) {
        return DecodeResult::MissingField;
    }
    
    return DecodeResult::Success;
}

DecodeResult ResponseDecoder::decodeNotePayload(const std::string& payloadJson, NoteResponsePayload& out) {
    out = NoteResponsePayload();
    
    std::string noteJson;
    if (!JsonUtils::extractField(payloadJson, "note", noteJson)) {
        return DecodeResult::MissingField;
    }
    
    return decodeNoteData(noteJson, out.note);
}

DecodeResult ResponseDecoder::decodeNotesListPayload(const std::string& payloadJson, NotesListResponsePayload& out) {
    out = NotesListResponsePayload();
    
    std::string notesArray;
    if (!JsonUtils::extractField(payloadJson, "notes", notesArray)) {
        // Try direct array (if payload is just the array)
        if (payloadJson[0] == '[') {
            notesArray = payloadJson;
        } else {
            return DecodeResult::MissingField;
        }
    }
    
    if (notesArray[0] == '[') {
        size_t pos = 1;
        while (pos < notesArray.length()) {
            while (pos < notesArray.length() && 
                   std::isspace(static_cast<unsigned char>(notesArray[pos]))) {
                ++pos;
            }
            
            if (pos >= notesArray.length() || notesArray[pos] == ']') break;
            
            if (notesArray[pos] != '{') {
                return DecodeResult::InvalidPayload;
            }
            
            int depth = 1;
            size_t objStart = pos;
            size_t objEnd = pos + 1;
            while (objEnd < notesArray.length() && depth > 0) {
                if (notesArray[objEnd] == '{') ++depth;
                else if (notesArray[objEnd] == '}') --depth;
                else if (notesArray[objEnd] == '"') {
                    ++objEnd;
                    while (objEnd < notesArray.length()) {
                        if (notesArray[objEnd] == '\\') {
                            objEnd += 2;  // Skip escaped character
                        } else if (notesArray[objEnd] == '"') {
                            ++objEnd;
                            break;
                        } else {
                            ++objEnd;
                        }
                    }
                    continue;
                }
                ++objEnd;
            }
            
            if (depth != 0) {
                return DecodeResult::InvalidPayload;
            }
            
            std::string objJson = notesArray.substr(objStart, objEnd - objStart);
            NoteData note;
            DecodeResult result = decodeNoteData(objJson, note);
            if (result != DecodeResult::Success) {
                return result;
            }
            out.notes.push_back(note);
            
            pos = objEnd;
            while (pos < notesArray.length() && 
                   std::isspace(static_cast<unsigned char>(notesArray[pos]))) {
                ++pos;
            }
            
            if (pos < notesArray.length() && notesArray[pos] == ',') {
                ++pos;
            }
        }
    } else {
        return DecodeResult::InvalidPayload;
    }
    
    return DecodeResult::Success;
}

} // namespace Protocol
} // namespace XIFriendList

