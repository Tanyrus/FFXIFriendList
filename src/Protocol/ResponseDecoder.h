
#ifndef PROTOCOL_RESPONSE_DECODER_H
#define PROTOCOL_RESPONSE_DECODER_H

#include "MessageTypes.h"
#include "ProtocolVersion.h"
#include <string>

namespace XIFriendList {
namespace Protocol {

enum class DecodeResult {
    Success,
    InvalidJson,
    MissingField,
    InvalidVersion,
    InvalidType,
    InvalidPayload
};

class ResponseDecoder {
public:
    static DecodeResult decode(const std::string& json, ResponseMessage& out);
    static DecodeResult decodeFriendListPayload(const std::string& payloadJson, 
                                                FriendListResponsePayload& out);
    static DecodeResult decodeStatusPayload(const std::string& payloadJson,
                                           StatusResponsePayload& out);
    static DecodeResult decodeFriendRequestPayload(const std::string& payloadJson,
                                                   FriendRequestPayload& out);
    static DecodeResult decodeFriendRequestsPayload(const std::string& payloadJson,
                                                   FriendRequestsResponsePayload& out);
    static DecodeResult decodeHeartbeatPayload(const std::string& payloadJson,
                                              HeartbeatResponsePayload& out);
    static DecodeResult decodePreferencesPayload(const std::string& payloadJson,
                                                PreferencesResponsePayload& out);
    static DecodeResult decodeFriendStatusData(const std::string& json, FriendStatusData& out);
    static DecodeResult decodeFriendData(const std::string& json, FriendData& out);
    static DecodeResult decodeFriendRequestData(const std::string& json, FriendRequestPayload& out);
    static DecodeResult decodeMailPayload(const std::string& payloadJson, MailMessageData& out);
    static DecodeResult decodeMailListPayload(const std::string& payloadJson, MailListResponsePayload& out);
    static DecodeResult decodeMailUnreadCountPayload(const std::string& payloadJson, MailUnreadCountResponsePayload& out);
    static DecodeResult decodeMailMessageData(const std::string& json, MailMessageData& out);
    static DecodeResult decodeNotesListPayload(const std::string& payloadJson,
                                              NotesListResponsePayload& out);
    static DecodeResult decodeNotePayload(const std::string& payloadJson,
                                         NoteResponsePayload& out);
    static DecodeResult decodeNoteData(const std::string& json, NoteData& out);
    static DecodeResult decodeFeedbackResponsePayload(const std::string& payloadJson,
                                                      FeedbackResponsePayload& out);
    static DecodeResult decodeIssueResponsePayload(const std::string& payloadJson,
                                                   IssueResponsePayload& out);

private:
    static bool validateVersion(const std::string& version);
};

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_RESPONSE_DECODER_H

