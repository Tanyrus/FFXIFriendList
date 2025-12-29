
#include "RequestEncoder.h"
#include "Core/FriendsCore.h"
#include "Core/ModelsCore.h"
#include "JsonUtils.h"
#include <sstream>

namespace XIFriendList {
namespace Protocol {

std::string RequestEncoder::encode(const RequestMessage& request) {
    std::vector<std::pair<std::string, std::string>> fields;
    fields.push_back({"protocolVersion", JsonUtils::encodeString(request.protocolVersion)});
    fields.push_back({"type", JsonUtils::encodeString(requestTypeToString(request.type))});
    if (!request.payload.empty()) {
        fields.push_back({"payload", request.payload});
    }
    return JsonUtils::encodeObject(fields);
}

std::string RequestEncoder::encodeGetFriendList() {
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetFriendList;
    msg.payload = "{}";
    return encode(msg);
}

std::string RequestEncoder::encodeSetFriendList(const std::vector<Core::Friend>& friends) {
    // Canonical format: statuses array of objects
    std::ostringstream statusesArray;
    statusesArray << "[";
    for (size_t i = 0; i < friends.size(); ++i) {
        if (i > 0) statusesArray << ",";
        const auto& f = friends[i];
        statusesArray << "{";
        statusesArray << JsonUtils::encodeString("name") << ":" << JsonUtils::encodeString(f.name);
        if (!f.friendedAs.empty() && f.friendedAs != f.name) {
            statusesArray << "," << JsonUtils::encodeString("friendedAs") << ":" << JsonUtils::encodeString(f.friendedAs);
        }
        if (!f.linkedCharacters.empty()) {
            statusesArray << "," << JsonUtils::encodeString("linkedCharacters") << ":" << JsonUtils::encodeStringArray(f.linkedCharacters);
        }
        statusesArray << "}";
    }
    statusesArray << "]";
    
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"statuses", statusesArray.str()});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SetFriendList;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetStatus(const std::string& characterName) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"characterName", JsonUtils::encodeString(characterName)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetStatus;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeUpdatePresence(const Core::Presence& presence) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"characterName", JsonUtils::encodeString(presence.characterName)});
    payloadFields.push_back({"job", JsonUtils::encodeString(presence.job)});
    payloadFields.push_back({"rank", JsonUtils::encodeString(presence.rank)});
    payloadFields.push_back({"nation", JsonUtils::encodeNumber(presence.nation)});
    payloadFields.push_back({"zone", JsonUtils::encodeString(presence.zone)});
    payloadFields.push_back({"isAnonymous", JsonUtils::encodeBoolean(presence.isAnonymous)});
    payloadFields.push_back({"timestamp", JsonUtils::encodeNumber(presence.timestamp)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::UpdatePresence;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeUpdateMyStatus(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"showOnlineStatus", JsonUtils::encodeBoolean(showOnlineStatus)});
    payloadFields.push_back({"shareLocation", JsonUtils::encodeBoolean(shareLocation)});
    payloadFields.push_back({"isAnonymous", JsonUtils::encodeBoolean(isAnonymous)});
    payloadFields.push_back({"shareJobWhenAnonymous", JsonUtils::encodeBoolean(shareJobWhenAnonymous)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::UpdateMyStatus;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeSendFriendRequest(const std::string& toUserId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"toUserId", JsonUtils::encodeString(toUserId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SendFriendRequest;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeAcceptFriendRequest(const std::string& requestId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"requestId", JsonUtils::encodeString(requestId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::AcceptFriendRequest;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeRejectFriendRequest(const std::string& requestId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"requestId", JsonUtils::encodeString(requestId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::RejectFriendRequest;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeCancelFriendRequest(const std::string& requestId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"requestId", JsonUtils::encodeString(requestId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::CancelFriendRequest;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetFriendRequests(const std::string& characterName) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"characterName", JsonUtils::encodeString(characterName)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetFriendRequests;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetHeartbeat(const std::string& characterName,
                                              uint64_t lastEventTimestamp,
                                              uint64_t lastRequestEventTimestamp,
                                              const std::string& pluginVersion) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"characterName", JsonUtils::encodeString(characterName)});
    payloadFields.push_back({"lastEventTimestamp", JsonUtils::encodeNumber(lastEventTimestamp)});
    payloadFields.push_back({"lastRequestEventTimestamp", JsonUtils::encodeNumber(lastRequestEventTimestamp)});
    if (!pluginVersion.empty()) {
        payloadFields.push_back({"clientVersion", JsonUtils::encodeString(pluginVersion)});
    }
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetHeartbeat;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetPreferences() {
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetPreferences;
    msg.payload = "{}";
    return encode(msg);
}

std::string RequestEncoder::encodeSetPreferences(const Core::Preferences& prefs) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"useServerNotes", JsonUtils::encodeBoolean(prefs.useServerNotes)});
    payloadFields.push_back({"shareFriendsAcrossAlts", JsonUtils::encodeBoolean(prefs.shareFriendsAcrossAlts)});
    
    // Map new FriendViewSettings to old server protocol format for backward compatibility
    payloadFields.push_back({"showJobColumn", JsonUtils::encodeBoolean(prefs.mainFriendView.showJob)});
    payloadFields.push_back({"showZoneColumn", JsonUtils::encodeBoolean(prefs.mainFriendView.showZone)});
    payloadFields.push_back({"showNationColumn", JsonUtils::encodeBoolean(prefs.mainFriendView.showNationRank)});
    payloadFields.push_back({"showRankColumn", JsonUtils::encodeBoolean(prefs.mainFriendView.showNationRank)});
    payloadFields.push_back({"showLastSeenColumn", JsonUtils::encodeBoolean(prefs.mainFriendView.showLastSeen)});
    
    payloadFields.push_back({"quickOnlineShowJobColumn", JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showJob)});
    payloadFields.push_back({"quickOnlineShowZoneColumn", JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showZone)});
    payloadFields.push_back({"quickOnlineShowNationColumn", JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showNationRank)});
    payloadFields.push_back({"quickOnlineShowRankColumn", JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showNationRank)});
    payloadFields.push_back({"quickOnlineShowLastSeenColumn", JsonUtils::encodeBoolean(prefs.quickOnlineFriendView.showLastSeen)});
    
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SetPreferences;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeSendMail(const std::string& toUserId, const std::string& subject, const std::string& body) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"toUserId", JsonUtils::encodeString(toUserId)});
    payloadFields.push_back({"subject", JsonUtils::encodeString(subject)});
    payloadFields.push_back({"body", JsonUtils::encodeString(body)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SendMail;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetMailInbox(int limit, int offset) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"limit", JsonUtils::encodeNumber(limit)});
    payloadFields.push_back({"offset", JsonUtils::encodeNumber(offset)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetMailInbox;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetMailInboxMeta(int limit, int offset) {
    // Meta mode uses query parameter, not protocol envelope
    // This is just a helper that returns empty (we'll append ?mode=meta to URL)
    (void)limit;
    (void)offset;
    return "";
}

std::string RequestEncoder::encodeGetMailAll(const std::string& folder, int limit, int offset, uint64_t since) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"folder", JsonUtils::encodeString(folder)});
    payloadFields.push_back({"limit", JsonUtils::encodeNumber(limit)});
    payloadFields.push_back({"offset", JsonUtils::encodeNumber(offset)});
    if (since > 0) {
        payloadFields.push_back({"since", JsonUtils::encodeNumber(since)});
    }
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetMailAll;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetMailAllMeta(const std::string& folder, int limit, int offset, uint64_t since) {
    // Meta mode uses query parameter, not protocol envelope
    // This is just a helper that returns empty (we'll append ?mode=meta to URL)
    (void)folder;
    (void)limit;
    (void)offset;
    (void)since;
    return "";
}

std::string RequestEncoder::encodeGetMailBatch(const std::string& mailbox, const std::vector<std::string>& messageIds) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"mailbox", JsonUtils::encodeString(mailbox)});
    payloadFields.push_back({"ids", JsonUtils::encodeStringArray(messageIds)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetMailBatch;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetMailUnreadCount() {
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetMailUnreadCount;
    msg.payload = "{}";
    return encode(msg);
}

std::string RequestEncoder::encodeMarkMailRead(const std::string& messageId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"messageId", JsonUtils::encodeString(messageId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::MarkMailRead;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeDeleteMail(const std::string& messageId) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"messageId", JsonUtils::encodeString(messageId)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::DeleteMail;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeGetNotes() {
    // Empty payload for GET all notes
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetNotes;
    msg.payload = "{}";
    return encode(msg);
}

std::string RequestEncoder::encodeGetNote(const std::string& friendName) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"friendName", JsonUtils::encodeString(friendName)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::GetNote;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodePutNote(const std::string& friendName, const std::string& noteText) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"friendName", JsonUtils::encodeString(friendName)});
    payloadFields.push_back({"note", JsonUtils::encodeString(noteText)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::PutNote;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeDeleteNote(const std::string& friendName) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"friendName", JsonUtils::encodeString(friendName)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::DeleteNote;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeSubmitFeedback(const std::string& subject, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"subject", JsonUtils::encodeString(subject)});
    payloadFields.push_back({"message", JsonUtils::encodeString(message)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SubmitFeedback;
    msg.payload = payloadJson;
    return encode(msg);
}

std::string RequestEncoder::encodeSubmitIssue(const std::string& subject, const std::string& message) {
    std::vector<std::pair<std::string, std::string>> payloadFields;
    payloadFields.push_back({"subject", JsonUtils::encodeString(subject)});
    payloadFields.push_back({"message", JsonUtils::encodeString(message)});
    std::string payloadJson = JsonUtils::encodeObject(payloadFields);
    
    RequestMessage msg;
    msg.protocolVersion = PROTOCOL_VERSION;
    msg.type = RequestType::SubmitIssue;
    msg.payload = payloadJson;
    return encode(msg);
}

} // namespace Protocol
} // namespace XIFriendList

