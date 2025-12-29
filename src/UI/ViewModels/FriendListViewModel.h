// ViewModel for friend list UI state

#ifndef UI_FRIEND_LIST_VIEW_MODEL_H
#define UI_FRIEND_LIST_VIEW_MODEL_H

#include "Core/FriendsCore.h"
#include "Core/MemoryStats.h"
#include "App/StateMachines/ConnectionState.h"
#include "Protocol/MessageTypes.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <optional>

namespace XIFriendList {
namespace UI {

struct FriendRowData {
    std::string name;           // Display name (friendedAs or name)
    std::string originalName;    // Original friend name (for reconciliation/matching)
    std::string friendedAs;      // Original name that was friended (for "Friended As" column)
    std::string statusText;     // Preformatted status string (e.g., "Online - Windurst")
    std::string jobText;        // Preformatted job string (e.g., "WHM75")
    std::string zoneText;       // Zone name
    std::string nationText;     // Nation name (e.g., "Bastok")
    std::string rankText;       // Rank (e.g., "10")
    std::string nationRankText; // Combined nation icon + rank (e.g., "S 10")
    std::string lastSeenText;   // Preformatted last seen (e.g., "2 minutes ago")
    int nation;                 // Nation value (0-3, -1 = hidden) for icon lookup
    bool isOnline;              // Online status flag
    bool hasStatusChanged;       // Flag indicating status changed since last update
    bool hasOnlineStatusChanged; // Flag indicating online status changed
    bool isPending;              // True if this is a pending friend request (sent, not yet accepted)
    int sortKey;                // Sort key for ordering (0 = online, 1 = offline, 2 = pending)
    
    FriendRowData()
        : isOnline(false)
        , hasStatusChanged(false)
        , hasOnlineStatusChanged(false)
        , isPending(false)
        , sortKey(1)
        , nation(-1)
    {}
};

// Action status for displaying success/error messages to user
struct ActionStatus {
    bool visible;
    bool success;
    std::string message;
    uint64_t timestampMs;
    std::string errorCode;
    
    ActionStatus()
        : visible(false)
        , success(false)
        , timestampMs(0)
    {}
};

// ViewModel for friend list window
// Transforms Core domain models into UI-ready data
class FriendListViewModel {
public:
    FriendListViewModel();
    ~FriendListViewModel() = default;

    void setDebugEnabled(bool enabled) {
        debugEnabled_ = enabled;
    }

    bool isDebugEnabled() const {
        return debugEnabled_;
    }
    
    // currentTime: Current timestamp in milliseconds (for last seen calculations)
    void update(const XIFriendList::Core::FriendList& friendList,
                const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                uint64_t currentTime);
    
    // Sent and incoming requests will appear in friend rows with isPending=true
    // currentTime: Current timestamp in milliseconds (for last seen calculations)
    void updateWithRequests(const XIFriendList::Core::FriendList& friendList,
                            const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                            const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoingRequests,
                            uint64_t currentTime);
    
    // Both types of requests will appear in friend rows with isPending=true
    // currentTime: Current timestamp in milliseconds (for last seen calculations)
    void updateWithRequests(const XIFriendList::Core::FriendList& friendList,
                            const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                            const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoingRequests,
                            const std::vector<XIFriendList::Protocol::FriendRequestPayload>& incomingRequests,
                            uint64_t currentTime);
    
    // Add optimistic pending request (A1: immediate UI update)
    // Adds a pending request to the friend rows immediately, before server confirmation
    void addOptimisticPendingRequest(const std::string& toUserId);
    
    void removeOptimisticPendingRequest(const std::string& toUserId);
    
    const std::vector<FriendRowData>& getFriendRows() const {
        return friendRows_;
    }
    
    const std::string& getConnectionStatusText() const {
        return connectionStatusText_;
    }
    
    const std::string& getCurrentCharacterName() const {
        return currentCharacterName_;
    }
    
    void setCurrentCharacterName(const std::string& name) {
        currentCharacterName_ = name;
    }
    
    void setConnectionState(XIFriendList::App::ConnectionState state) {
        if (connectionState_ != state) {
            connectionState_ = state;
            updateCachedStrings();  // Update cached text when state changes
        }
    }
    
    XIFriendList::App::ConnectionState getConnectionState() const {
        return connectionState_;
    }
    
    bool isConnected() const {
        return connectionState_ == XIFriendList::App::ConnectionState::Connected;
    }
    
    const std::string& getErrorMessage() const {
        return errorMessage_;
    }
    
    void setErrorMessage(const std::string& message) {
        errorMessage_ = message;
    }
    
    // Clear error message
    void clearError() {
        errorMessage_.clear();
    }
    
    uint64_t getLastUpdateTime() const {
        return lastUpdateTime_;
    }
    
    // Pending friend requests
    void updatePendingRequests(const std::vector<XIFriendList::Protocol::FriendRequestPayload>& incoming,
                              const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoing);
    
    const std::vector<XIFriendList::Protocol::FriendRequestPayload>& getIncomingRequests() const {
        return incomingRequests_;
    }
    
    const std::vector<XIFriendList::Protocol::FriendRequestPayload>& getOutgoingRequests() const {
        return outgoingRequests_;
    }
    
    void setShowFriendedAsColumn(bool show) { showFriendedAsColumn_ = show; }
    void setShowJobColumn(bool show) { showJobColumn_ = show; }
    void setShowRankColumn(bool show) { showRankColumn_ = show; }
    void setShowNationColumn(bool show) { showNationColumn_ = show; }
    void setShowNationRankColumn(bool show) { showNationRankColumn_ = show; }
    void setShowZoneColumn(bool show) { showZoneColumn_ = show; }
    void setShowLastSeenColumn(bool show) { showLastSeenColumn_ = show; }
    
    bool& getShowFriendedAsColumn() { return showFriendedAsColumn_; }
    bool& getShowJobColumn() { return showJobColumn_; }
    bool& getShowRankColumn() { return showRankColumn_; }
    bool& getShowNationColumn() { return showNationColumn_; }
    bool& getShowNationRankColumn() { return showNationRankColumn_; }
    bool& getShowZoneColumn() { return showZoneColumn_; }
    bool& getShowLastSeenColumn() { return showLastSeenColumn_; }
    
    bool getShowFriendedAsColumn() const { return showFriendedAsColumn_; }
    bool getShowJobColumn() const { return showJobColumn_; }
    bool getShowRankColumn() const { return showRankColumn_; }
    bool getShowNationColumn() const { return showNationColumn_; }
    bool getShowNationRankColumn() const { return showNationRankColumn_; }
    bool getShowZoneColumn() const { return showZoneColumn_; }
    bool getShowLastSeenColumn() const { return showLastSeenColumn_; }
    
    // Helper methods for context menu
    bool friendHasLinkedCharacters(const std::string& friendName) const;
    std::string getRequestIdForFriend(const std::string& friendName) const;
    
    // Get friend details for popup (returns row data and linked characters)
    struct FriendDetails {
        FriendRowData rowData;
        std::vector<std::string> linkedCharacters;
    };
    std::optional<FriendDetails> getFriendDetails(const std::string& friendName) const;
    
    // Action status methods
    void setActionStatusSuccess(const std::string& message, uint64_t timestampMs);
    void setActionStatusError(const std::string& message, const std::string& errorCode, uint64_t timestampMs);
    void clearActionStatus();
    const ActionStatus& getActionStatus() const {
        return actionStatus_;
    }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    // Format status text from FriendStatus
    std::string formatStatusText(const XIFriendList::Core::FriendStatus& status) const;
    
    // Format job text
    std::string formatJobText(const XIFriendList::Core::FriendStatus& status) const;
    
    // Format nation text
    std::string formatNationText(const XIFriendList::Core::FriendStatus& status) const;
    
    // Format combined nation + rank text (icon + rank number)
    std::string formatNationRankText(const XIFriendList::Core::FriendStatus& status) const;
    
    // Format last seen text
    std::string formatLastSeenText(uint64_t lastSeenAt, uint64_t currentTime, bool isOnline) const;
    
    // Calculate sort key (0 = online, 1 = offline, then alphabetical)
    int calculateSortKey(const XIFriendList::Core::FriendStatus& status) const;

    std::vector<FriendRowData> friendRows_;
    // Stable display order map: username -> position index (for reconciliation)
    // This preserves order when updating existing friends
    std::map<std::string, size_t> friendDisplayOrder_;
    XIFriendList::App::ConnectionState connectionState_ = XIFriendList::App::ConnectionState::Disconnected;
    std::string errorMessage_;
    uint64_t lastUpdateTime_ = 0;
    
    // Cached formatted strings (updated on data change, not per-frame)
    std::string connectionStatusText_;
    std::string currentCharacterName_;  // Current character name (for display in header)
    
    // Previous statuses for change detection
    std::vector<XIFriendList::Core::FriendStatus> previousStatuses_;
    
    // Pending friend requests
    std::vector<XIFriendList::Protocol::FriendRequestPayload> incomingRequests_;
    std::vector<XIFriendList::Protocol::FriendRequestPayload> outgoingRequests_;
    
    std::map<std::string, XIFriendList::Core::Friend> friendDataMap_;
    
    bool showFriendedAsColumn_ = false;
    bool showJobColumn_ = true;
    bool showRankColumn_ = false;
    bool showNationColumn_ = false;
    bool showNationRankColumn_ = true;
    bool showZoneColumn_ = true;
    bool showLastSeenColumn_ = true;
    
    // Action status for UI feedback
    ActionStatus actionStatus_;

    bool debugEnabled_ = false;
    
    void updateCachedStrings();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_FRIEND_LIST_VIEW_MODEL_H

