
#ifndef PROTOCOL_MESSAGE_TYPES_H
#define PROTOCOL_MESSAGE_TYPES_H

#include <string>
#include <vector>
#include <cstdint>

namespace XIFriendList {
namespace Core {
    struct Friend;
    struct FriendStatus;
    struct Presence;
    struct Preferences;
    struct MailMessage;
}
}

namespace XIFriendList {
namespace Protocol {

enum class RequestType {
    GetFriendList,
    SetFriendList,
    GetStatus,
    UpdatePresence,
    UpdateMyStatus,
    SendFriendRequest,
    AcceptFriendRequest,
    RejectFriendRequest,
    CancelFriendRequest,
    GetFriendRequests,
    GetHeartbeat,
    GetPreferences,
    SetPreferences,
    SendMail,
    GetMailInbox,
    GetMailInboxMeta,
    GetMailAll,
    GetMailAllMeta,
    GetMailBatch,
    GetMailUnreadCount,
    MarkMailRead,
    DeleteMail,
    // Notes operations
    GetNotes,
    GetNote,
    PutNote,
    DeleteNote,
    SetActiveCharacter,
    SubmitFeedback,
    SubmitIssue
};

enum class ResponseType {
    FriendList,
    Status,
    Presence,
    AuthEnsureResponse,
    FriendRequest,
    FriendRequests,
    Heartbeat,
    Preferences,
    Mail,
    MailList,
    MailUnreadCount,
    NotesList,
    Note,
    StateUpdate,
    FeedbackResponse,
    IssueResponse,
    AltVisibility,
    Success,
    Error
};

struct RequestMessage {
    std::string protocolVersion;
    RequestType type;
    std::string payload;
    
    RequestMessage() : type(RequestType::GetFriendList) {}
};

struct ResponseMessage {
    std::string protocolVersion;
    ResponseType type;
    bool success;
    std::string payload;
    std::string error;
    std::string errorCode;
    std::string details;
    std::string requestId;
    
    ResponseMessage() : type(ResponseType::Error), success(false) {}
};

struct FriendData {
    std::string name;
    std::string friendedAs;
    std::vector<std::string> linkedCharacters;
    
    FriendData() = default;
};

struct FriendStatusData {
    std::string characterName;
    std::string displayName;
    bool isOnline;
    std::string job;
    std::string rank;
    int nation;
    std::string zone;
    uint64_t lastSeenAt;
    bool showOnlineStatus;
    bool isLinkedCharacter;
    bool isOnAltCharacter;
    std::string altCharacterName;
    std::string friendedAs;
    std::vector<std::string> linkedCharacters;
    
    FriendStatusData()
        : isOnline(false)
        , nation(0)
        , lastSeenAt(0)
        , showOnlineStatus(true)
        , isLinkedCharacter(false)
        , isOnAltCharacter(false)
    {}
};

struct FriendListRequestPayload {
    std::vector<std::string> friends;
    
    FriendListRequestPayload() = default;
};

struct FriendListResponsePayload {
    std::vector<FriendData> friendsData;
    
    FriendListResponsePayload() = default;
};

struct StatusRequestPayload {
    std::string characterName;
    
    StatusRequestPayload() = default;
};

struct StatusResponsePayload {
    std::vector<FriendStatusData> statuses;
    
    StatusResponsePayload() = default;
};

struct PresenceRequestPayload {
    std::string characterName;
    std::string job;
    std::string rank;
    int nation;
    std::string zone;
    bool isAnonymous;
    uint64_t timestamp;
    
    PresenceRequestPayload()
        : nation(0)
        , isAnonymous(false)
        , timestamp(0)
    {}
};

struct FriendRequestPayload {
    std::string requestId;
    std::string fromCharacterName;  // Sender's character name (for incoming requests)
    std::string toCharacterName;    // Recipient's character name (for outgoing requests)
    int fromAccountId;              // Sender's account ID
    int toAccountId;                // Recipient's account ID
    std::string status;
    uint64_t createdAt;
    
    FriendRequestPayload()
        : fromAccountId(0)
        , toAccountId(0)
        , createdAt(0)
    {}
};

struct FriendRequestsResponsePayload {
    std::vector<FriendRequestPayload> incoming;
    std::vector<FriendRequestPayload> outgoing;
    
    FriendRequestsResponsePayload() = default;
};

// Heartbeat response payload
struct HeartbeatResponsePayload {
    std::vector<FriendStatusData> statuses;
    std::vector<FriendRequestPayload> events;
    uint64_t lastEventTimestamp;
    uint64_t lastRequestEventTimestamp;
    
    HeartbeatResponsePayload()
        : lastEventTimestamp(0)
        , lastRequestEventTimestamp(0)
    {}
};

struct PreferencesRequestPayload {
    bool useServerNotes;
    bool showFriendedAsColumn;
    bool showJobColumn;
    bool showRankColumn;
    bool showNationColumn;
    bool showZoneColumn;
    bool showLastSeenColumn;

    bool quickOnlineShowFriendedAsColumn;
    bool quickOnlineShowJobColumn;
    bool quickOnlineShowRankColumn;
    bool quickOnlineShowNationColumn;
    bool quickOnlineShowZoneColumn;
    bool quickOnlineShowLastSeenColumn;
    
    PreferencesRequestPayload()
        : useServerNotes(false)
        , showFriendedAsColumn(true)
        , showJobColumn(true)
        , showRankColumn(true)
        , showNationColumn(true)
        , showZoneColumn(true)
        , showLastSeenColumn(true)
        , quickOnlineShowFriendedAsColumn(false)
        , quickOnlineShowJobColumn(false)
        , quickOnlineShowRankColumn(false)
        , quickOnlineShowNationColumn(false)
        , quickOnlineShowZoneColumn(false)
        , quickOnlineShowLastSeenColumn(false)
    {}
};

struct PreferencesResponsePayload {
    bool useServerNotes;
    bool shareFriendsAcrossAlts;
    bool showFriendedAsColumn;
    bool showJobColumn;
    bool showRankColumn;
    bool showNationColumn;
    bool showZoneColumn;
    bool showLastSeenColumn;

    bool quickOnlineShowFriendedAsColumn;
    bool quickOnlineShowJobColumn;
    bool quickOnlineShowRankColumn;
    bool quickOnlineShowNationColumn;
    bool quickOnlineShowZoneColumn;
    bool quickOnlineShowLastSeenColumn;
    
    PreferencesResponsePayload()
        : useServerNotes(false)
        , shareFriendsAcrossAlts(true)
        , showFriendedAsColumn(true)
        , showJobColumn(true)
        , showRankColumn(true)
        , showNationColumn(true)
        , showZoneColumn(true)
        , showLastSeenColumn(true)
        , quickOnlineShowFriendedAsColumn(false)
        , quickOnlineShowJobColumn(false)
        , quickOnlineShowRankColumn(false)
        , quickOnlineShowNationColumn(false)
        , quickOnlineShowZoneColumn(false)
        , quickOnlineShowLastSeenColumn(false)
    {}
};

struct MailMessageData {
    std::string messageId;
    std::string fromUserId;
    std::string toUserId;
    std::string subject;
    std::string body;
    uint64_t createdAt;
    uint64_t readAt;
    bool isRead;
    
    MailMessageData()
        : createdAt(0)
        , readAt(0)
        , isRead(false)
    {}
};

// Send mail request payload
struct SendMailRequestPayload {
    std::string toUserId;
    std::string subject;
    std::string body;
    
    SendMailRequestPayload() = default;
};

// Send mail response payload
struct SendMailResponsePayload {
    std::string messageId;
    uint64_t createdAt;
    
    SendMailResponsePayload()
        : createdAt(0)
    {}
};

struct GetMailListRequestPayload {
    std::string folder;      // "inbox" or "sent"
    int limit;               // 1-200, default 100
    int offset;              // default 0
    uint64_t since;          // optional timestamp filter
    
    GetMailListRequestPayload()
        : folder("inbox")
        , limit(100)
        , offset(0)
        , since(0)
    {}
};

struct MailListResponsePayload {
    std::vector<MailMessageData> messages;
    
    MailListResponsePayload() = default;
};

struct MailUnreadCountResponsePayload {
    int count;
    
    MailUnreadCountResponsePayload()
        : count(0)
    {}
};

// Mark mail read request payload
struct MarkMailReadRequestPayload {
    std::string messageId;
    
    MarkMailReadRequestPayload() = default;
};

// Delete mail request payload
struct DeleteMailRequestPayload {
    std::string messageId;
    
    DeleteMailRequestPayload() = default;
};

struct NoteData {
    std::string friendName;
    std::string note;
    uint64_t updatedAt;
    
    NoteData()
        : updatedAt(0)
    {}
};

// Notes list response payload (GET /api/notes)
struct NotesListResponsePayload {
    std::vector<NoteData> notes;
    
    NotesListResponsePayload() = default;
};

struct NoteResponsePayload {
    NoteData note;
    
    NoteResponsePayload() = default;
};

// Put note request payload (PUT /api/notes/:friendName)
struct PutNoteRequestPayload {
    std::string note;
    
    PutNoteRequestPayload() = default;
};

// SetActiveCharacter response payload (SERVER-DECIDES model)
struct SetActiveCharacterResponsePayload {
    int accountId;
    int characterId;
    std::string characterName;
    std::string realmId;
    std::string apiKey;
    bool wasCreated;   // true if character was auto-created
    bool wasMerged;    // true if accounts were auto-merged
    int mergedFromAccountId;  // only valid if wasMerged
    
    SetActiveCharacterResponsePayload()
        : accountId(0)
        , characterId(0)
        , wasCreated(false)
        , wasMerged(false)
        , mergedFromAccountId(0)
    {}
};

// Submit feedback/issue request payload
struct SubmitSupportRequestPayload {
    std::string subject;
    std::string message;
    
    SubmitSupportRequestPayload() = default;
};

// Feedback response payload
struct FeedbackResponsePayload {
    int feedbackId;
    
    FeedbackResponsePayload()
        : feedbackId(0)
    {}
};

// Issue response payload
struct IssueResponsePayload {
    int issueId;
    
    IssueResponsePayload()
        : issueId(0)
    {}
};

// Character visibility state for a specific character
struct CharacterVisibilityState {
    int characterId;
    std::string characterName;
    bool hasVisibility;          // Whether this character can see this friend
    bool hasPendingVisibilityRequest;  // Whether there's a pending visibility request from this character
    
    CharacterVisibilityState()
        : characterId(0)
        , hasVisibility(false)
        , hasPendingVisibilityRequest(false)
    {}
};

struct AltVisibilityFriendEntry {
    int friendAccountId;
    std::string friendedAsName;
    std::string displayName;
    std::string visibilityMode;  // "ALL" or "ONLY"
    std::vector<CharacterVisibilityState> characterVisibility;  // Visibility state for each character on the account
    uint64_t createdAt;
    uint64_t updatedAt;
    
    AltVisibilityFriendEntry()
        : friendAccountId(0)
        , visibilityMode("ALL")
        , createdAt(0)
        , updatedAt(0)
    {}
};

// Account character info
struct AccountCharacterInfo {
    int characterId;
    std::string characterName;
    bool isActive;
    
    AccountCharacterInfo()
        : characterId(0)
        , isActive(false)
    {}
};

struct AltVisibilityResponsePayload {
    std::vector<AltVisibilityFriendEntry> friends;
    std::vector<AccountCharacterInfo> characters;  // All characters on the account
    uint64_t serverTime;
    
    AltVisibilityResponsePayload()
        : serverTime(0)
    {}
};

// Convert RequestType to string
std::string requestTypeToString(RequestType type);

// Convert string to RequestType
bool stringToRequestType(const std::string& str, RequestType& out);

// Convert ResponseType to string
std::string responseTypeToString(ResponseType type);

// Convert string to ResponseType
bool stringToResponseType(const std::string& str, ResponseType& out);

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_MESSAGE_TYPES_H

