// ViewModel implementation for friend list UI

#include "UI/ViewModels/FriendListViewModel.h"
#include "Core/FriendsCore.h"
#include "Core/MemoryStats.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <cctype>

namespace XIFriendList {
namespace UI {

FriendListViewModel::FriendListViewModel()
    : connectionState_(XIFriendList::App::ConnectionState::Disconnected)
    , lastUpdateTime_(0)
    , currentCharacterName_("")
{
    // Initialize cached strings to ensure ViewModel is always in valid state
    updateCachedStrings();
}

void FriendListViewModel::update(const XIFriendList::Core::FriendList& friendList,
                                 const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                                 uint64_t currentTime) {
    std::map<std::string, XIFriendList::Core::FriendStatus> statusMap;
    for (const auto& status : statuses) {
        statusMap[status.characterName] = status;
    }
    
    // Store friend data map for quick lookup
    friendDataMap_.clear();
    for (const auto& friendItem : friendList.getFriends()) {
        friendDataMap_[friendItem.name] = friendItem;
    }
    
    // RECONCILIATION: Update existing rows in-place to preserve order
    std::map<std::string, size_t> existingRowMap;  // normalized original name -> row index
    for (size_t i = 0; i < friendRows_.size(); ++i) {
        if (!friendRows_[i].originalName.empty()) {
            std::string nameLower = friendRows_[i].originalName;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            existingRowMap[nameLower] = i;
        }
    }
    
    // Track which friends we've updated
    std::set<std::string> updatedFriends;
    
    for (const auto& friendItem : friendList.getFriends()) {
        std::string nameLower = friendItem.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        auto rowIt = existingRowMap.find(nameLower);
        if (rowIt != existingRowMap.end()) {
            size_t rowIndex = rowIt->second;
            FriendRowData& row = friendRows_[rowIndex];
            
            // A4: Clear pending flag when friend is accepted (now a real friend)
            row.isPending = false;
            
            row.originalName = friendItem.name;  // Store original name for future matching (stable ID)
            
            auto statusIt = statusMap.find(friendItem.name);
            if (statusIt != statusMap.end()) {
                const auto& status = statusIt->second;
                // Use displayName (active online character) for Name column
                row.name = status.displayName.empty() ? status.characterName : status.displayName;
                // Use friendedAs from status (original name that was friended) for FriendedAs column
                row.friendedAs = status.friendedAs.empty() ? status.characterName : status.friendedAs;
                row.isOnline = status.isOnline;

                // Performance: Only compute/format values for visible columns.
                // This is especially important for Quick Online where most columns are hidden by default.
                row.statusText = "";  // Currently not displayed as a column; keep empty.
                row.jobText = showJobColumn_ ? formatJobText(status) : "";
                row.zoneText = showZoneColumn_ ? (status.zone.empty() ? "Hidden" : status.zone) : "";
                row.nationText = showNationColumn_ ? formatNationText(status) : "";
                row.rankText = showRankColumn_ ? (status.rank.empty() ? "Hidden" : status.rank) : "";
                row.nationRankText = showNationRankColumn_ ? formatNationRankText(status) : "";
                row.nation = status.nation;
                row.lastSeenText = showLastSeenColumn_ ? formatLastSeenText(status.lastSeenAt, currentTime, status.isOnline) : "";
                row.sortKey = calculateSortKey(status);
                
                auto prevIt = std::find_if(previousStatuses_.begin(), previousStatuses_.end(),
                    [&status](const XIFriendList::Core::FriendStatus& prev) {
                        return prev.characterName == status.characterName;
                    });
                
                if (prevIt != previousStatuses_.end()) {
                    row.hasStatusChanged = status.hasStatusChanged(*prevIt);
                    row.hasOnlineStatusChanged = status.hasOnlineStatusChanged(*prevIt);
                } else {
                    row.hasStatusChanged = true;
                    row.hasOnlineStatusChanged = true;
                }
            } else {
                // No status available - use friendedAs or name
                // No status available - use friendItem data as fallback
                row.name = friendItem.friendedAs.empty() ? friendItem.name : friendItem.friendedAs;
                row.friendedAs = friendItem.friendedAs.empty() ? friendItem.name : friendItem.friendedAs;
                row.isOnline = false;
                row.statusText = "Unknown";
                row.jobText = "";
                row.zoneText = "";
                row.nationText = "";
                row.rankText = "";
                row.nationRankText = "";
                row.nation = -1;
                row.lastSeenText = showLastSeenColumn_ ? "Never" : "";
                row.sortKey = 1;  // Offline
            }
            
            updatedFriends.insert(nameLower);
        }
    }
    
    // Remove rows for friends that no longer exist, or mark them offline if they're still showing
    friendRows_.erase(
        std::remove_if(friendRows_.begin(), friendRows_.end(),
            [&friendList](const FriendRowData& row) {
                if (row.originalName.empty()) {
                    return true;  // Remove rows without original name (shouldn't happen)
                }
                for (const auto& friendItem : friendList.getFriends()) {
                    if (friendItem.name == row.originalName) {
                        return false;  // Keep this row
                    }
                }
                return true;  // Remove this row
            }),
        friendRows_.end()
    );
    
    // Also mark any remaining rows as offline if their friend is not in the friend list
    // (This handles the case where a friend was deleted but the row still exists)
    for (auto& row : friendRows_) {
        if (row.originalName.empty()) {
            continue;
        }
        bool found = false;
        for (const auto& friendItem : friendList.getFriends()) {
            if (friendItem.name == row.originalName) {
                found = true;
                break;
            }
        }
        if (!found) {
            row.isOnline = false;
            row.statusText = "Offline";
            row.sortKey = 1;  // Offline
            row.hasStatusChanged = true;
            row.hasOnlineStatusChanged = true;
        }
    }
    
    // Add new friends (not in existing rows) - add to end to preserve order
    for (const auto& friendItem : friendList.getFriends()) {
        std::string nameLower = friendItem.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (updatedFriends.find(nameLower) == updatedFriends.end()) {
            // New friend - add to end
            FriendRowData row;
            row.originalName = friendItem.name;  // Store original name for future matching
            row.friendedAs = friendItem.friendedAs;  // Store friendedAs for column display
            row.isPending = false;  // A4: New friends are not pending
            
            auto statusIt = statusMap.find(friendItem.name);
            if (statusIt != statusMap.end()) {
                const auto& status = statusIt->second;
                // Use displayName (active online character) for Name column
                row.name = status.displayName.empty() ? status.characterName : status.displayName;
                // Use friendedAs from status (original name that was friended) for FriendedAs column
                row.friendedAs = status.friendedAs.empty() ? status.characterName : status.friendedAs;
                row.isOnline = status.isOnline;
                row.statusText = "";
                row.jobText = showJobColumn_ ? formatJobText(status) : "";
                row.zoneText = showZoneColumn_ ? (status.zone.empty() ? "Hidden" : status.zone) : "";
                row.nationText = showNationColumn_ ? formatNationText(status) : "";
                std::string rankValue = status.rank.empty() ? "Hidden" : status.rank;
                if (rankValue != "Hidden" && rankValue.find("Rank") != std::string::npos) {
                    size_t rankPos = rankValue.find("Rank");
                    if (rankPos != std::string::npos) {
                        rankValue = rankValue.substr(rankPos + 4);
                        while (!rankValue.empty() && std::isspace(static_cast<unsigned char>(rankValue[0]))) {
                            rankValue = rankValue.substr(1);
                        }
                    }
                    if (rankValue.empty()) {
                        rankValue = "Hidden";
                    }
                }
                row.rankText = showRankColumn_ ? rankValue : "";
                row.nationRankText = showNationRankColumn_ ? formatNationRankText(status) : "";
                row.nation = status.nation;
                row.lastSeenText = showLastSeenColumn_ ? formatLastSeenText(status.lastSeenAt, currentTime, status.isOnline) : "";
                row.sortKey = calculateSortKey(status);
                row.hasStatusChanged = true;
                row.hasOnlineStatusChanged = true;
            } else {
                // No status available - use friendedAs or name
                // No status available - use friendItem data as fallback
                row.name = friendItem.friendedAs.empty() ? friendItem.name : friendItem.friendedAs;
                row.friendedAs = friendItem.friendedAs.empty() ? friendItem.name : friendItem.friendedAs;
                row.isOnline = false;
                row.statusText = "Unknown";
                row.jobText = "";
                row.zoneText = "";
                row.nationText = "";
                row.rankText = "";
                row.nationRankText = "";
                row.nation = -1;
                row.lastSeenText = showLastSeenColumn_ ? "Never" : "";
                row.sortKey = 1;
                row.hasStatusChanged = true;
                row.hasOnlineStatusChanged = true;
            }
            
            friendRows_.push_back(row);
        }
    }
    
    // STABLE SORT: Use stable_sort to preserve relative order of equal elements
    // Case-insensitive comparison for deterministic tie-breaker
    std::stable_sort(friendRows_.begin(), friendRows_.end(),
        [](const FriendRowData& a, const FriendRowData& b) {
            // Primary sort: by sortKey (online=0, offline=1, pending=2)
            if (a.sortKey != b.sortKey) {
                return a.sortKey < b.sortKey;
            }
            // Secondary sort: case-insensitive alphabetical by name (deterministic tie-breaker)
            std::string aLower = a.name;
            std::string bLower = b.name;
            std::transform(aLower.begin(), aLower.end(), aLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(bLower.begin(), bLower.end(), bLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return aLower < bLower;
        });
    
    // Store current statuses for next update
    previousStatuses_ = statuses;
    lastUpdateTime_ = currentTime;
    
    updateCachedStrings();
}

void FriendListViewModel::updateWithRequests(const XIFriendList::Core::FriendList& friendList,
                                             const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                                             const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoingRequests,
                                             uint64_t currentTime) {
    // Call the overload with empty incoming requests
    updateWithRequests(friendList, statuses, outgoingRequests, std::vector<XIFriendList::Protocol::FriendRequestPayload>(), currentTime);
}

void FriendListViewModel::updateWithRequests(const XIFriendList::Core::FriendList& friendList,
                                             const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                                             const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoingRequests,
                                             const std::vector<XIFriendList::Protocol::FriendRequestPayload>& incomingRequests,
                                             uint64_t currentTime) {
    update(friendList, statuses, currentTime);
    
    // Helper function to add a pending request row
    auto addPendingRequest = [&](const std::string& characterName, const std::string& statusLabel) {
        bool alreadyFriend = false;
        for (const auto& friendItem : friendList.getFriends()) {
            std::string friendNameLower = friendItem.name;
            std::transform(friendNameLower.begin(), friendNameLower.end(), friendNameLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (friendNameLower == characterName) {
                alreadyFriend = true;
                break;
            }
        }
        
        for (const auto& row : friendRows_) {
            if (row.name == characterName) {
                alreadyFriend = true;
                break;
            }
        }
        
        if (!alreadyFriend) {
            FriendRowData row;
            row.originalName = characterName;  // Store original name for matching
            row.name = characterName;
            row.isPending = true;
            row.isOnline = false;  // No status for pending requests
            row.statusText = statusLabel;
            row.jobText = "";
            row.zoneText = "";
            row.nationText = "";
            row.rankText = "";
            row.nationRankText = "";
            row.nation = -1;
            row.lastSeenText = "";
            row.sortKey = 2;  // Pending requests at bottom (2 = after offline)
            row.hasStatusChanged = false;
            row.hasOnlineStatusChanged = false;
            
            friendRows_.push_back(row);
        }
    };
    
    // Add outgoing (sent) requests
    for (const auto& request : outgoingRequests) {
        std::string normalizedName = request.toCharacterName;
        std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        addPendingRequest(normalizedName, "[Pending]");
    }
    
    // Add incoming (received) requests
    for (const auto& request : incomingRequests) {
        std::string normalizedName = request.fromCharacterName;
        std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        addPendingRequest(normalizedName, "[Incoming Request]");
    }
    
    // STABLE SORT: Re-sort to ensure pending requests are at the bottom
    // Use stable_sort with case-insensitive comparison for deterministic ordering
    std::stable_sort(friendRows_.begin(), friendRows_.end(),
        [](const FriendRowData& a, const FriendRowData& b) {
            // Primary sort: by sortKey (online=0, offline=1, pending=2)
            if (a.sortKey != b.sortKey) {
                return a.sortKey < b.sortKey;
            }
            // Secondary sort: case-insensitive alphabetical by name (deterministic tie-breaker)
            std::string aLower = a.name;
            std::string bLower = b.name;
            std::transform(aLower.begin(), aLower.end(), aLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(bLower.begin(), bLower.end(), bLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return aLower < bLower;
        });
}

void FriendListViewModel::updatePendingRequests(const std::vector<XIFriendList::Protocol::FriendRequestPayload>& incoming,
                                                const std::vector<XIFriendList::Protocol::FriendRequestPayload>& outgoing) {
    incomingRequests_ = incoming;
    outgoingRequests_ = outgoing;
    // No need to update cached strings for requests
}

void FriendListViewModel::addOptimisticPendingRequest(const std::string& toUserId) {
    std::string normalizedName = toUserId;
    std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    for (const auto& row : friendRows_) {
        if (row.name == normalizedName && row.isPending) {
            return;  // Already exists
        }
    }
    
    // Add optimistic pending request row
    FriendRowData row;
    row.originalName = normalizedName;
    row.name = normalizedName;
    row.isPending = true;
    row.isOnline = false;
    row.statusText = "[Pending]";
    row.jobText = "";
    row.zoneText = "";
    row.nationText = "";
    row.rankText = "";
    row.nationRankText = "";
    row.nation = -1;
    row.lastSeenText = "";
    row.sortKey = 2;  // Pending requests at bottom
    row.hasStatusChanged = false;
    row.hasOnlineStatusChanged = false;
    
    friendRows_.push_back(row);
    
    // Re-sort to maintain order
    std::stable_sort(friendRows_.begin(), friendRows_.end(),
        [](const FriendRowData& a, const FriendRowData& b) {
            if (a.sortKey != b.sortKey) {
                return a.sortKey < b.sortKey;
            }
            std::string aLower = a.name;
            std::string bLower = b.name;
            std::transform(aLower.begin(), aLower.end(), aLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(bLower.begin(), bLower.end(), bLower.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return aLower < bLower;
        });
}

void FriendListViewModel::removeOptimisticPendingRequest(const std::string& toUserId) {
    std::string normalizedName = toUserId;
    std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Remove matching pending request
    friendRows_.erase(
        std::remove_if(friendRows_.begin(), friendRows_.end(),
            [&normalizedName](const FriendRowData& row) {
                return row.name == normalizedName && row.isPending;
            }),
        friendRows_.end()
    );
}

void FriendListViewModel::updateCachedStrings() {
    switch (connectionState_) {
        case XIFriendList::App::ConnectionState::Disconnected:
            connectionStatusText_ = "Disconnected";
            break;
        case XIFriendList::App::ConnectionState::Connecting:
            connectionStatusText_ = "Connecting...";
            break;
        case XIFriendList::App::ConnectionState::Connected:
            connectionStatusText_ = "Connected";
            break;
        case XIFriendList::App::ConnectionState::Reconnecting:
            connectionStatusText_ = "Reconnecting...";
            break;
        case XIFriendList::App::ConnectionState::Failed:
            connectionStatusText_ = "Connection Failed";
            break;
        default:
            connectionStatusText_ = "Unknown";
            break;
    }
}

std::string FriendListViewModel::formatStatusText(const XIFriendList::Core::FriendStatus& status) const {
    // But we optimize to avoid ostringstream when possible
    if (!status.showOnlineStatus) {
        // Requirement: invisible friends should be treated as OFFLINE (not Unknown).
        return "Offline";
    }
    
    if (status.isOnline) {
        if (status.zone.empty() || status.zone == "Hidden") {
            return "Online";
        }
        // Pre-allocate to avoid reallocation
        std::string result;
        result.reserve(status.zone.length() + 10);  // "Online - " + zone
        result = "Online - ";
        result += status.zone;
        return result;
    } else {
        return "Offline";
    }
}

std::string FriendListViewModel::formatJobText(const XIFriendList::Core::FriendStatus& status) const {
    // Server returns null/empty for missing data - show "Hidden"
    return status.job.empty() ? "Hidden" : status.job;
}

std::string FriendListViewModel::formatNationText(const XIFriendList::Core::FriendStatus& status) const {
    // Nation: 0 = San d'Oria, 1 = Bastok, 2 = Windurst, 3 = Jeuno
    // -1 = Hidden/not set (server sent null or field missing due to privacy)
    // Trust the server: if nation is -1, it's hidden. If job/rank is empty, nation should also be hidden.
    if (status.nation == -1 || status.job.empty() || status.rank.empty()) {
        return "Hidden";
    }

    switch (status.nation) {
        case 0: return "San d'Oria";
        case 1: return "Bastok";
        case 2: return "Windurst";
        case 3: return "Jeuno";
        default: return "Hidden";  // Invalid values (shouldn't happen, but handle gracefully)
    }
}

std::string FriendListViewModel::formatNationRankText(const XIFriendList::Core::FriendStatus& status) const {
    if (status.nation == -1 || status.job.empty() || status.rank.empty()) {
        return "Hidden";
    }
    
    std::string nationIcon;
    switch (status.nation) {
        case 0: nationIcon = "S"; break;  // San d'Oria
        case 1: nationIcon = "B"; break;  // Bastok
        case 2: nationIcon = "W"; break;  // Windurst
        case 3: nationIcon = "J"; break;  // Jeuno
        default: return "Hidden";
    }
    
    return nationIcon + " " + status.rank;
}

std::string FriendListViewModel::formatLastSeenText(uint64_t lastSeenAt, uint64_t currentTime, bool isOnline) const {
    if (isOnline) {
        return "Now";
    }
    
    if (lastSeenAt == 0 || currentTime == 0) {
        return "Never";
    }
    
    if (currentTime < lastSeenAt) {
        return "Unknown";
    }
    
    uint64_t diffMs = currentTime - lastSeenAt;
    uint64_t diffSeconds = diffMs / 1000;
    uint64_t diffMinutes = diffSeconds / 60;
    uint64_t diffHours = diffMinutes / 60;
    uint64_t diffDays = diffHours / 24;
    
    // Use string concatenation instead of ostringstream for better performance
    std::string result;
    result.reserve(32);  // Pre-allocate
    
    if (diffDays > 0) {
        result = std::to_string(diffDays);
        result += " day";
        if (diffDays > 1) {
            result += "s";
        }
        result += " ago";
    } else if (diffHours > 0) {
        result = std::to_string(diffHours);
        result += " hour";
        if (diffHours > 1) {
            result += "s";
        }
        result += " ago";
    } else if (diffMinutes > 0) {
        result = std::to_string(diffMinutes);
        result += " minute";
        if (diffMinutes > 1) {
            result += "s";
        }
        result += " ago";
    } else {
        result = "Just now";
    }
    
    return result;
}

int FriendListViewModel::calculateSortKey(const XIFriendList::Core::FriendStatus& status) const {
    // 0 = online, 1 = offline
    return status.isOnline ? 0 : 1;
}

bool FriendListViewModel::friendHasLinkedCharacters(const std::string& friendName) const {
    std::string nameLower = friendName;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = friendDataMap_.find(nameLower);
    if (it != friendDataMap_.end()) {
        return it->second.hasLinkedCharacters();
    }
    return false;
}

std::string FriendListViewModel::getRequestIdForFriend(const std::string& friendName) const {
    std::string nameLower = friendName;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& req : outgoingRequests_) {
        std::string reqToLower = req.toCharacterName;
        std::transform(reqToLower.begin(), reqToLower.end(), reqToLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (reqToLower == nameLower) {
            return req.requestId;
        }
    }
    return "";
}

std::optional<FriendListViewModel::FriendDetails> FriendListViewModel::getFriendDetails(const std::string& friendName) const {
    FriendDetails details;
    
    // Find the friend row
    for (const auto& row : friendRows_) {
        if (row.name == friendName || row.originalName == friendName || row.friendedAs == friendName) {
            details.rowData = row;
            
            // Get linked characters from friendDataMap_
            std::string nameLower = row.originalName.empty() ? row.name : row.originalName;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            auto it = friendDataMap_.find(nameLower);
            if (it != friendDataMap_.end()) {
                details.linkedCharacters = it->second.linkedCharacters;
            }
            
            return details;
        }
    }
    
    return std::nullopt;
}

void FriendListViewModel::setActionStatusSuccess(const std::string& message, uint64_t timestampMs) {
    actionStatus_.visible = true;
    actionStatus_.success = true;
    actionStatus_.message = message;
    actionStatus_.timestampMs = timestampMs;
    actionStatus_.errorCode.clear();
}

void FriendListViewModel::setActionStatusError(const std::string& message, const std::string& errorCode, uint64_t timestampMs) {
    actionStatus_.visible = true;
    actionStatus_.success = false;
    actionStatus_.message = message;
    actionStatus_.errorCode = errorCode;
    actionStatus_.timestampMs = timestampMs;
}

void FriendListViewModel::clearActionStatus() {
    actionStatus_.visible = false;
    actionStatus_.success = false;
    actionStatus_.message.clear();
    actionStatus_.errorCode.clear();
    actionStatus_.timestampMs = 0;
}

XIFriendList::Core::MemoryStats FriendListViewModel::getMemoryStats() const {
    size_t bytes = sizeof(FriendListViewModel);
    
    for (const auto& row : friendRows_) {
        bytes += sizeof(FriendRowData);
        bytes += row.name.capacity();
        bytes += row.originalName.capacity();
        bytes += row.friendedAs.capacity();
        bytes += row.statusText.capacity();
        bytes += row.jobText.capacity();
        bytes += row.zoneText.capacity();
        bytes += row.nationText.capacity();
        bytes += row.rankText.capacity();
        bytes += row.nationRankText.capacity();
        bytes += row.lastSeenText.capacity();
    }
    bytes += friendRows_.capacity() * sizeof(FriendRowData);
    
    bytes += friendDisplayOrder_.size() * (sizeof(std::string) + sizeof(size_t));
    
    bytes += connectionStatusText_.capacity();
    bytes += currentCharacterName_.capacity();
    bytes += errorMessage_.capacity();
    
    for (const auto& status : previousStatuses_) {
        bytes += sizeof(XIFriendList::Core::FriendStatus);
        bytes += status.characterName.capacity();
        bytes += status.displayName.capacity();
        bytes += status.job.capacity();
        bytes += status.rank.capacity();
        bytes += status.zone.capacity();
        bytes += status.altCharacterName.capacity();
        bytes += status.friendedAs.capacity();
        for (const auto& linked : status.linkedCharacters) {
            bytes += linked.capacity();
        }
        bytes += status.linkedCharacters.capacity() * sizeof(std::string);
    }
    bytes += previousStatuses_.capacity() * sizeof(XIFriendList::Core::FriendStatus);
    
    for (const auto& req : incomingRequests_) {
        bytes += sizeof(XIFriendList::Protocol::FriendRequestPayload);
        bytes += req.requestId.capacity();
        bytes += req.fromCharacterName.capacity();
        bytes += req.toCharacterName.capacity();
    }
    bytes += incomingRequests_.capacity() * sizeof(XIFriendList::Protocol::FriendRequestPayload);
    
    for (const auto& req : outgoingRequests_) {
        bytes += sizeof(XIFriendList::Protocol::FriendRequestPayload);
        bytes += req.requestId.capacity();
        bytes += req.fromCharacterName.capacity();
        bytes += req.toCharacterName.capacity();
    }
    bytes += outgoingRequests_.capacity() * sizeof(XIFriendList::Protocol::FriendRequestPayload);
    
    for (const auto& pair : friendDataMap_) {
        bytes += pair.first.capacity();
        bytes += sizeof(XIFriendList::Core::Friend);
        bytes += pair.second.name.capacity();
        bytes += pair.second.friendedAs.capacity();
        for (const auto& linked : pair.second.linkedCharacters) {
            bytes += linked.capacity();
        }
        bytes += pair.second.linkedCharacters.capacity() * sizeof(std::string);
    }
    bytes += friendDataMap_.size() * sizeof(std::string);
    
    bytes += actionStatus_.message.capacity();
    bytes += actionStatus_.errorCode.capacity();
    
    size_t count = friendRows_.size() + previousStatuses_.size() + 
                   incomingRequests_.size() + outgoingRequests_.size() + 
                   friendDataMap_.size();
    
    return XIFriendList::Core::MemoryStats(count, bytes, "FriendList ViewModel");
}

} // namespace UI
} // namespace XIFriendList

