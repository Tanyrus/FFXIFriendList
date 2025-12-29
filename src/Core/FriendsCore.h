
#ifndef CORE_FRIENDS_CORE_H
#define CORE_FRIENDS_CORE_H

#include "MemoryStats.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

namespace XIFriendList {
namespace Core {


struct Friend {
    std::string name;              // Character name (normalized to lowercase)
    std::string friendedAs;        // Original character name that was friended
    std::vector<std::string> linkedCharacters;  // Linked alt characters
    
    Friend() = default;
    Friend(const std::string& name, const std::string& friendedAs)
        : name(name), friendedAs(friendedAs) {}
    
    bool operator==(const Friend& other) const;
    bool operator!=(const Friend& other) const;
    bool matchesCharacter(const std::string& characterName) const;
    bool hasLinkedCharacters() const;
};

struct FriendStatus {
    std::string characterName;     // Originally friended character (stable ID for actions)
    std::string displayName;       // Active online character (may be alt from link group) - shown in Name column
    bool isOnline;                  // Online/offline status
    std::string job;                // Current job
    std::string rank;               // Nation rank
    int nation;                     // Nation (0-3)
    std::string zone;               // Current zone
    uint64_t lastSeenAt;            // Timestamp of last seen (0 if not available)
    bool showOnlineStatus;          // Whether friend has show online status enabled
    bool isLinkedCharacter;         // True if this is a linked character (alt)
    bool isOnAltCharacter;          // True if friend is currently on alt character
    std::string altCharacterName;   // Name of alt character if isOnAltCharacter is true
    std::string friendedAs;         // Original character name that was friended - shown in "Friended As" column
    std::vector<std::string> linkedCharacters;  // Linked alt characters
    
    FriendStatus()
        : isOnline(false)
        , nation(-1)  // -1 = hidden/not set (default to most private)
        , lastSeenAt(0)
        , showOnlineStatus(true)
        , isLinkedCharacter(false)
        , isOnAltCharacter(false)
    {}
    
    bool operator==(const FriendStatus& other) const;
    bool operator!=(const FriendStatus& other) const;
    
    bool hasStatusChanged(const FriendStatus& other) const;
    
    bool hasOnlineStatusChanged(const FriendStatus& other) const;
};


class FriendList {
public:
    FriendList() = default;
    ~FriendList() = default;
    
    FriendList(const FriendList& other) = default;
    FriendList& operator=(const FriendList& other) = default;
    FriendList(FriendList&& other) noexcept = default;
    FriendList& operator=(FriendList&& other) noexcept = default;
    
    bool addFriend(const Friend& f);
    bool addFriend(const std::string& name, const std::string& friendedAs = "");
    
    bool removeFriend(const std::string& name);
    
    bool updateFriend(const Friend& f);
    
    const Friend* findFriend(const std::string& name) const;
    Friend* findFriend(const std::string& name);
    
    bool hasFriend(const std::string& name) const;
    
    const std::vector<Friend>& getFriends() const { return friends_; }
    std::vector<Friend>& getFriends() { return friends_; }
    
    size_t size() const { return friends_.size(); }
    bool empty() const { return friends_.empty(); }
    
    void clear();
    
    std::vector<std::string> getFriendNames() const;
    
    void updateFriendStatus(const FriendStatus& status);
    
    const FriendStatus* getFriendStatus(const std::string& name) const;
    FriendStatus* getFriendStatus(const std::string& name);
    
    const std::vector<FriendStatus>& getFriendStatuses() const { return friendStatuses_; }
    
    MemoryStats getMemoryStats() const;
    
private:
    std::vector<Friend> friends_;
    std::vector<FriendStatus> friendStatuses_;
    
    static std::string normalizeName(const std::string& name);
    
    size_t findFriendIndex(const std::string& name) const;
    
    size_t findFriendStatusIndex(const std::string& name) const;
};


class FriendListSorter {
public:
    static void sortFriendsByStatus(
        std::vector<std::string>& friendNames,
        const std::vector<FriendStatus>& friendStatuses
    );
    
    static void sortFriendsAlphabetically(std::vector<std::string>& friendNames);
    
    static void sortFriendsByStatus(
        std::vector<Friend>& friends,
        const std::vector<FriendStatus>& friendStatuses
    );
    
    static void sortFriendsAlphabetically(std::vector<Friend>& friends);
    
private:
    static bool isFriendOnline(
        const std::string& friendName,
        const std::vector<FriendStatus>& friendStatuses
    );
    
    static int compareStringsIgnoreCase(const std::string& a, const std::string& b);
};


class FriendListFilter {
public:
    using FriendPredicate = std::function<bool(const Friend&)>;
    using FriendStatusPredicate = std::function<bool(const FriendStatus&)>;
    
    static std::vector<Friend> filterByName(
        const std::vector<Friend>& friends,
        const std::string& searchTerm
    );
    
    static std::vector<std::string> filterByOnlineStatus(
        const std::vector<std::string>& friendNames,
        const std::vector<FriendStatus>& friendStatuses,
        bool onlineOnly
    );
    
    static std::vector<Friend> filter(
        const std::vector<Friend>& friends,
        const FriendPredicate& predicate
    );
    
    static std::vector<FriendStatus> filterStatuses(
        const std::vector<FriendStatus>& friendStatuses,
        const FriendStatusPredicate& predicate
    );
    
    static std::vector<std::string> filterOnline(
        const std::vector<std::string>& friendNames,
        const std::vector<FriendStatus>& friendStatuses
    );
    
    static std::vector<std::string> filterOffline(
        const std::vector<std::string>& friendNames,
        const std::vector<FriendStatus>& friendStatuses
    );
};


struct FriendListDiff {
    std::vector<std::string> addedFriends;      // Friends added
    std::vector<std::string> removedFriends;    // Friends removed
    std::vector<std::string> statusChangedFriends;  // Friends with status changes
    
    bool hasChanges() const {
        return !addedFriends.empty()
            || !removedFriends.empty()
            || !statusChangedFriends.empty();
    }
};

class FriendListDiffer {
public:
    static FriendListDiff diff(
        const std::vector<std::string>& oldFriends,
        const std::vector<std::string>& newFriends
    );
    
    static std::vector<std::string> diffStatuses(
        const std::vector<FriendStatus>& oldStatuses,
        const std::vector<FriendStatus>& newStatuses
    );
    
    static std::vector<std::string> diffOnlineStatus(
        const std::vector<FriendStatus>& oldStatuses,
        const std::vector<FriendStatus>& newStatuses
    );
    
private:
    static std::string normalizeName(const std::string& name);
    
    static bool containsName(const std::vector<std::string>& names, const std::string& name);
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_FRIENDS_CORE_H

