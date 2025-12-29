
#include "MessageTypes.h"
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace Protocol {

std::string requestTypeToString(RequestType type) {
    switch (type) {
        case RequestType::GetFriendList: return "GetFriendList";
        case RequestType::SetFriendList: return "SetFriendList";
        case RequestType::GetStatus: return "GetStatus";
        case RequestType::UpdatePresence: return "UpdatePresence";
        case RequestType::UpdateMyStatus: return "UpdateMyStatus";
        case RequestType::SendFriendRequest: return "SendFriendRequest";
        case RequestType::AcceptFriendRequest: return "AcceptFriendRequest";
        case RequestType::RejectFriendRequest: return "RejectFriendRequest";
        case RequestType::CancelFriendRequest: return "CancelFriendRequest";
        case RequestType::GetFriendRequests: return "GetFriendRequests";
        case RequestType::GetHeartbeat: return "GetHeartbeat";
        case RequestType::GetPreferences: return "GetPreferences";
        case RequestType::SetPreferences: return "SetPreferences";
        case RequestType::SendMail: return "SendMail";
        case RequestType::GetMailInbox: return "GetMailInbox";
        case RequestType::GetMailInboxMeta: return "GetMailInboxMeta";
        case RequestType::GetMailAll: return "GetMailAll";
        case RequestType::GetMailAllMeta: return "GetMailAllMeta";
        case RequestType::GetMailBatch: return "GetMailBatch";
        case RequestType::GetMailUnreadCount: return "GetMailUnreadCount";
        case RequestType::MarkMailRead: return "MarkMailRead";
        case RequestType::DeleteMail: return "DeleteMail";
        case RequestType::GetNotes: return "GetNotes";
        case RequestType::GetNote: return "GetNote";
        case RequestType::PutNote: return "PutNote";
        case RequestType::DeleteNote: return "DeleteNote";
        case RequestType::SetActiveCharacter: return "SetActiveCharacter";
        case RequestType::SubmitFeedback: return "SubmitFeedback";
        case RequestType::SubmitIssue: return "SubmitIssue";
        default: return "Unknown";
    }
}

bool stringToRequestType(const std::string& str, RequestType& out) {
    if (str == "GetFriendList") {
        out = RequestType::GetFriendList;
        return true;
    } else if (str == "SetFriendList") {
        out = RequestType::SetFriendList;
        return true;
    } else if (str == "GetStatus") {
        out = RequestType::GetStatus;
        return true;
    } else if (str == "UpdatePresence") {
        out = RequestType::UpdatePresence;
        return true;
    } else if (str == "UpdateMyStatus") {
        out = RequestType::UpdateMyStatus;
        return true;
    } else if (str == "SendFriendRequest") {
        out = RequestType::SendFriendRequest;
        return true;
    } else if (str == "AcceptFriendRequest") {
        out = RequestType::AcceptFriendRequest;
        return true;
    } else if (str == "RejectFriendRequest") {
        out = RequestType::RejectFriendRequest;
        return true;
    } else if (str == "CancelFriendRequest") {
        out = RequestType::CancelFriendRequest;
        return true;
    } else if (str == "GetFriendRequests") {
        out = RequestType::GetFriendRequests;
        return true;
    } else if (str == "GetHeartbeat") {
        out = RequestType::GetHeartbeat;
        return true;
    } else if (str == "GetPreferences") {
        out = RequestType::GetPreferences;
        return true;
    } else if (str == "SetPreferences") {
        out = RequestType::SetPreferences;
        return true;
    } else if (str == "SendMail") {
        out = RequestType::SendMail;
        return true;
    } else if (str == "GetMailInbox") {
        out = RequestType::GetMailInbox;
        return true;
    } else if (str == "GetMailInboxMeta") {
        out = RequestType::GetMailInboxMeta;
        return true;
    } else if (str == "GetMailAll") {
        out = RequestType::GetMailAll;
        return true;
    } else if (str == "GetMailAllMeta") {
        out = RequestType::GetMailAllMeta;
        return true;
    } else if (str == "GetMailBatch") {
        out = RequestType::GetMailBatch;
        return true;
    } else if (str == "GetMailUnreadCount") {
        out = RequestType::GetMailUnreadCount;
        return true;
    } else if (str == "MarkMailRead") {
        out = RequestType::MarkMailRead;
        return true;
    } else if (str == "DeleteMail") {
        out = RequestType::DeleteMail;
        return true;
    } else if (str == "GetNotes") {
        out = RequestType::GetNotes;
        return true;
    } else if (str == "GetNote") {
        out = RequestType::GetNote;
        return true;
    } else if (str == "PutNote") {
        out = RequestType::PutNote;
        return true;
    } else if (str == "DeleteNote") {
        out = RequestType::DeleteNote;
        return true;
    } else if (str == "SetActiveCharacter") {
        out = RequestType::SetActiveCharacter;
        return true;
    } else if (str == "SubmitFeedback") {
        out = RequestType::SubmitFeedback;
        return true;
    } else if (str == "SubmitIssue") {
        out = RequestType::SubmitIssue;
        return true;
    }
    return false;
}

std::string responseTypeToString(ResponseType type) {
    switch (type) {
        case ResponseType::FriendList: return "FriendList";
        case ResponseType::Status: return "Status";
        case ResponseType::Presence: return "Presence";
        case ResponseType::AuthEnsureResponse: return "AuthEnsureResponse";
        case ResponseType::FriendRequest: return "FriendRequest";
        case ResponseType::FriendRequests: return "FriendRequests";
        case ResponseType::Heartbeat: return "Heartbeat";
        case ResponseType::Preferences: return "Preferences";
        case ResponseType::Mail: return "Mail";
        case ResponseType::MailList: return "MailList";
        case ResponseType::MailUnreadCount: return "MailUnreadCount";
        case ResponseType::NotesList: return "NotesList";
        case ResponseType::Note: return "Note";
        case ResponseType::StateUpdate: return "StateUpdate";
        case ResponseType::FeedbackResponse: return "FeedbackResponse";
        case ResponseType::IssueResponse: return "IssueResponse";
        case ResponseType::Success: return "Success";
        case ResponseType::Error: return "Error";
        default: return "Unknown";
    }
}

bool stringToResponseType(const std::string& str, ResponseType& out) {
    if (str == "FriendsListResponse") {
        out = ResponseType::FriendList;
        return true;
    } else if (str == "AuthEnsureResponse" || str == "MeResponse" || str == "AddCharacterResponse") {
        out = ResponseType::AuthEnsureResponse;
        return true;
    } else if (str == "FriendRequestsResponse") {
        out = ResponseType::FriendRequests;
        return true;
    } else if (str == "SendFriendRequestResponse" || str == "AcceptFriendRequestResponse" || 
               str == "RejectFriendRequestResponse" || str == "CancelFriendRequestResponse") {
        out = ResponseType::FriendRequest;
        return true;
    } else if (str == "HeartbeatResponse") {
        out = ResponseType::Heartbeat;
        return true;
    } else if (str == "PreferencesResponse" || str == "PreferencesUpdateResponse") {
        out = ResponseType::Preferences;
        return true;
    } else if (str == "MailSentResponse" || str == "MailMessageResponse") {
        out = ResponseType::Mail;
        return true;
    } else if (str == "MailListResponse") {
        out = ResponseType::MailList;
        return true;
    } else if (str == "UnreadCountResponse") {
        out = ResponseType::MailUnreadCount;
        return true;
    } else if (str == "StateUpdateResponse") {
        out = ResponseType::StateUpdate;
        return true;
    } else if (str == "NotesListResponse") {
        out = ResponseType::NotesList;
        return true;
    } else if (str == "NoteResponse" || str == "NoteUpdateResponse" || str == "NoteDeleteResponse") {
        out = ResponseType::Note;
        return true;
    } else if (str == "Error") {
        out = ResponseType::Error;
        return true;
    } else if (str == "AddFriendResponse" || str == "RemoveFriendResponse" || str == "RemoveFriendVisibilityResponse" || str == "SyncFriendsResponse") {
        out = ResponseType::Success;
        return true;
    } else if (str == "StateUpdateResponse" || str == "PrivacyUpdateResponse" || 
               str == "SetActiveCharacterResponse" || str == "CharactersListResponse") {
        out = ResponseType::Success;
        return true;
    } else if (str == "BatchMarkReadResponse" || str == "MarkReadResponse" || str == "MailDeleteResponse") {
        out = ResponseType::Success;
        return true;
    } else if (str == "FeedbackResponse") {
        out = ResponseType::FeedbackResponse;
        return true;
    } else if (str == "IssueResponse") {
        out = ResponseType::IssueResponse;
        return true;
    }
    return false;
}

} // namespace Protocol
} // namespace XIFriendList

