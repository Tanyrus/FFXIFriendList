
#ifndef PROTOCOL_REQUEST_ENCODER_H
#define PROTOCOL_REQUEST_ENCODER_H

#include "MessageTypes.h"
#include "ProtocolVersion.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace Core {
    struct Friend;
    struct Presence;
    struct Preferences;
    struct MailMessage;
}
}

namespace XIFriendList {
namespace Protocol {

class RequestEncoder {
public:
    static std::string encode(const RequestMessage& request);
    static std::string encodeGetFriendList();
    static std::string encodeSetFriendList(const std::vector<XIFriendList::Core::Friend>& friends);
    static std::string encodeGetStatus(const std::string& characterName);
    static std::string encodeUpdatePresence(const XIFriendList::Core::Presence& presence);
    static std::string encodeUpdateMyStatus(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous);
    static std::string encodeSendFriendRequest(const std::string& toUserId);
    static std::string encodeAcceptFriendRequest(const std::string& requestId);
    static std::string encodeRejectFriendRequest(const std::string& requestId);
    
    static std::string encodeCancelFriendRequest(const std::string& requestId);
    
    static std::string encodeGetFriendRequests(const std::string& characterName);
    
    static std::string encodeGetHeartbeat(const std::string& characterName, 
                                         uint64_t lastEventTimestamp,
                                         uint64_t lastRequestEventTimestamp,
                                         const std::string& pluginVersion = "");
    
    static std::string encodeGetPreferences();
    
    static std::string encodeSetPreferences(const XIFriendList::Core::Preferences& prefs);
    
    static std::string encodeSendMail(const std::string& toUserId, const std::string& subject, const std::string& body);
    
    static std::string encodeGetMailInbox(int limit = 50, int offset = 0);
    
    static std::string encodeGetMailInboxMeta(int limit = 50, int offset = 0);
    
    static std::string encodeGetMailAll(const std::string& folder, int limit = 100, int offset = 0, uint64_t since = 0);
    
    static std::string encodeGetMailAllMeta(const std::string& folder, int limit = 100, int offset = 0, uint64_t since = 0);
    
    static std::string encodeGetMailBatch(const std::string& mailbox, const std::vector<std::string>& messageIds);
    
    static std::string encodeGetMailUnreadCount();
    
    static std::string encodeMarkMailRead(const std::string& messageId);
    
    static std::string encodeDeleteMail(const std::string& messageId);
    
    static std::string encodeGetNotes();
    
    static std::string encodeGetNote(const std::string& friendName);
    
    static std::string encodePutNote(const std::string& friendName, const std::string& noteText);
    
    static std::string encodeDeleteNote(const std::string& friendName);
    
    static std::string encodeSubmitFeedback(const std::string& subject, const std::string& message);
    
    static std::string encodeSubmitIssue(const std::string& subject, const std::string& message);
};

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_REQUEST_ENCODER_H

