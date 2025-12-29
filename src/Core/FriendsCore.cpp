
#include "FriendsCore.h"
#include "MemoryStats.h"
#include <algorithm>
#include <cctype>
#include <iterator>

namespace XIFriendList {
namespace Core {

namespace {
    std::string toLower(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        std::transform(s.begin(), s.end(), std::back_inserter(result),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }
}


bool Friend::operator==(const Friend& other) const {
    return toLower(name) == toLower(other.name);
}

bool Friend::operator!=(const Friend& other) const {
    return !(*this == other);
}

bool Friend::matchesCharacter(const std::string& characterName) const {
    std::string lowerName = toLower(name);
    std::string lowerChar = toLower(characterName);
    
    if (lowerName == lowerChar) {
        return true;
    }
    
    for (const auto& linked : linkedCharacters) {
        if (toLower(linked) == lowerChar) {
            return true;
        }
    }
    
    return false;
}

bool Friend::hasLinkedCharacters() const {
    return !linkedCharacters.empty();
}


bool FriendStatus::operator==(const FriendStatus& other) const {
    return characterName == other.characterName
        && displayName == other.displayName
        && isOnline == other.isOnline
        && job == other.job
        && rank == other.rank
        && nation == other.nation
        && zone == other.zone
        && showOnlineStatus == other.showOnlineStatus
        && isLinkedCharacter == other.isLinkedCharacter
        && isOnAltCharacter == other.isOnAltCharacter
        && altCharacterName == other.altCharacterName
        && friendedAs == other.friendedAs
        && linkedCharacters == other.linkedCharacters;
}

bool FriendStatus::operator!=(const FriendStatus& other) const {
    return !(*this == other);
}

bool FriendStatus::hasStatusChanged(const FriendStatus& other) const {
    return characterName != other.characterName
        || displayName != other.displayName
        || isOnline != other.isOnline
        || job != other.job
        || rank != other.rank
        || nation != other.nation
        || zone != other.zone
        || showOnlineStatus != other.showOnlineStatus
        || isLinkedCharacter != other.isLinkedCharacter
        || isOnAltCharacter != other.isOnAltCharacter
        || altCharacterName != other.altCharacterName
        || friendedAs != other.friendedAs
        || linkedCharacters != other.linkedCharacters;
}

bool FriendStatus::hasOnlineStatusChanged(const FriendStatus& other) const {
    return isOnline != other.isOnline;
}


bool FriendList::addFriend(const Friend& f) {
    std::string normalizedName = normalizeName(f.name);
    
    if (findFriendIndex(normalizedName) != friends_.size()) {
        return false;
    }
    
    Friend newFriend = f;
    newFriend.name = normalizedName;
    if (newFriend.friendedAs.empty()) {
        newFriend.friendedAs = normalizedName;
    } else {
        newFriend.friendedAs = normalizeName(newFriend.friendedAs);
    }
    
    friends_.push_back(newFriend);
    return true;
}

bool FriendList::addFriend(const std::string& name, const std::string& friendedAs) {
    return addFriend(Friend(name, friendedAs));
}

bool FriendList::removeFriend(const std::string& name) {
    std::string normalizedName = normalizeName(name);
    size_t index = findFriendIndex(normalizedName);
    
    if (index == friends_.size()) {
        return false;
    }
    
    friends_.erase(friends_.begin() + index);
    
    size_t statusIndex = findFriendStatusIndex(normalizedName);
    if (statusIndex != friendStatuses_.size()) {
        friendStatuses_.erase(friendStatuses_.begin() + statusIndex);
    }
    
    return true;
}

bool FriendList::updateFriend(const Friend& f) {
    std::string normalizedName = normalizeName(f.name);
    size_t index = findFriendIndex(normalizedName);
    
    if (index == friends_.size()) {
        return false;
    }
    
    Friend updatedFriend = f;
    updatedFriend.name = normalizedName;
    if (!updatedFriend.friendedAs.empty()) {
        updatedFriend.friendedAs = normalizeName(updatedFriend.friendedAs);
    }
    
    friends_[index] = updatedFriend;
    return true;
}

const Friend* FriendList::findFriend(const std::string& name) const {
    size_t index = findFriendIndex(normalizeName(name));
    if (index == friends_.size()) {
        return nullptr;
    }
    return &friends_[index];
}

Friend* FriendList::findFriend(const std::string& name) {
    size_t index = findFriendIndex(normalizeName(name));
    if (index == friends_.size()) {
        return nullptr;
    }
    return &friends_[index];
}

bool FriendList::hasFriend(const std::string& name) const {
    return findFriendIndex(normalizeName(name)) != friends_.size();
}

void FriendList::clear() {
    friends_.clear();
    friendStatuses_.clear();
}

std::vector<std::string> FriendList::getFriendNames() const {
    std::vector<std::string> names;
    names.reserve(friends_.size());
    for (const auto& f : friends_) {
        names.push_back(f.name);
    }
    return names;
}

void FriendList::updateFriendStatus(const FriendStatus& status) {
    std::string normalizedName = normalizeName(status.characterName);
    size_t index = findFriendStatusIndex(normalizedName);
    
    FriendStatus updatedStatus = status;
    updatedStatus.characterName = normalizedName;
    
    if (index == friendStatuses_.size()) {
        friendStatuses_.push_back(updatedStatus);
    } else {
        friendStatuses_[index] = updatedStatus;
    }
}

const FriendStatus* FriendList::getFriendStatus(const std::string& name) const {
    size_t index = findFriendStatusIndex(normalizeName(name));
    if (index == friendStatuses_.size()) {
        return nullptr;
    }
    return &friendStatuses_[index];
}

FriendStatus* FriendList::getFriendStatus(const std::string& name) {
    size_t index = findFriendStatusIndex(normalizeName(name));
    if (index == friendStatuses_.size()) {
        return nullptr;
    }
    return &friendStatuses_[index];
}

std::string FriendList::normalizeName(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    std::transform(name.begin(), name.end(), std::back_inserter(result),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

size_t FriendList::findFriendIndex(const std::string& name) const {
    std::string normalized = normalizeName(name);
    for (size_t i = 0; i < friends_.size(); ++i) {
        if (normalizeName(friends_[i].name) == normalized) {
            return i;
        }
    }
    return friends_.size();
}

size_t FriendList::findFriendStatusIndex(const std::string& name) const {
    std::string normalized = normalizeName(name);
    for (size_t i = 0; i < friendStatuses_.size(); ++i) {
        if (normalizeName(friendStatuses_[i].characterName) == normalized) {
            return i;
        }
    }
    return friendStatuses_.size();
}


void FriendListSorter::sortFriendsByStatus(
    std::vector<std::string>& friendNames,
    const std::vector<FriendStatus>& friendStatuses
) {
    std::sort(friendNames.begin(), friendNames.end(),
        [&friendStatuses](const std::string& a, const std::string& b) {
            bool aOnline = isFriendOnline(a, friendStatuses);
            bool bOnline = isFriendOnline(b, friendStatuses);
            
            if (aOnline != bOnline) {
                return aOnline;
            }
            
            return compareStringsIgnoreCase(a, b) < 0;
        }
    );
}

void FriendListSorter::sortFriendsAlphabetically(std::vector<std::string>& friendNames) {
    std::sort(friendNames.begin(), friendNames.end(),
        [](const std::string& a, const std::string& b) {
            return compareStringsIgnoreCase(a, b) < 0;
        }
    );
}

void FriendListSorter::sortFriendsByStatus(
    std::vector<Friend>& friends,
    const std::vector<FriendStatus>& friendStatuses
) {
    std::sort(friends.begin(), friends.end(),
        [&friendStatuses](const Friend& a, const Friend& b) {
            bool aOnline = isFriendOnline(a.name, friendStatuses);
            bool bOnline = isFriendOnline(b.name, friendStatuses);
            
            if (aOnline != bOnline) {
                return aOnline;
            }
            
            return compareStringsIgnoreCase(a.name, b.name) < 0;
        }
    );
}

void FriendListSorter::sortFriendsAlphabetically(std::vector<Friend>& friends) {
    std::sort(friends.begin(), friends.end(),
        [](const Friend& a, const Friend& b) {
            return compareStringsIgnoreCase(a.name, b.name) < 0;
        }
    );
}

bool FriendListSorter::isFriendOnline(
    const std::string& friendName,
    const std::vector<FriendStatus>& friendStatuses
) {
    std::string normalizedName = toLower(friendName);
    
    for (const auto& status : friendStatuses) {
        if (toLower(status.characterName) == normalizedName) {
            return status.isOnline && status.showOnlineStatus;
        }
    }
    
    return false;
}

int FriendListSorter::compareStringsIgnoreCase(const std::string& a, const std::string& b) {
    std::string lowerA = toLower(a);
    std::string lowerB = toLower(b);
    
    if (lowerA < lowerB) return -1;
    if (lowerA > lowerB) return 1;
    return 0;
}


std::vector<Friend> FriendListFilter::filterByName(
    const std::vector<Friend>& friends,
    const std::string& searchTerm
) {
    if (searchTerm.empty()) {
        return friends;
    }
    
    std::string lowerSearch = toLower(searchTerm);
    
    std::vector<Friend> result;
    result.reserve(friends.size());
    
    for (const auto& f : friends) {
        std::string lowerName = toLower(f.name);
        if (lowerName.find(lowerSearch) != std::string::npos) {
            result.push_back(f);
        }
    }
    
    return result;
}

std::vector<std::string> FriendListFilter::filterByOnlineStatus(
    const std::vector<std::string>& friendNames,
    const std::vector<FriendStatus>& friendStatuses,
    bool onlineOnly
) {
    std::vector<std::string> result;
    result.reserve(friendNames.size());
    
    for (const auto& name : friendNames) {
        std::string normalizedName = toLower(name);
        
        bool isOnline = false;
        for (const auto& status : friendStatuses) {
            if (toLower(status.characterName) == normalizedName) {
                isOnline = status.isOnline && status.showOnlineStatus;
                break;
            }
        }
        
        if (onlineOnly == isOnline) {
            result.push_back(name);
        }
    }
    
    return result;
}

std::vector<Friend> FriendListFilter::filter(
    const std::vector<Friend>& friends,
    const FriendPredicate& predicate
) {
    std::vector<Friend> result;
    result.reserve(friends.size());
    
    std::copy_if(friends.begin(), friends.end(), std::back_inserter(result), predicate);
    
    return result;
}

std::vector<FriendStatus> FriendListFilter::filterStatuses(
    const std::vector<FriendStatus>& friendStatuses,
    const FriendStatusPredicate& predicate
) {
    std::vector<FriendStatus> result;
    result.reserve(friendStatuses.size());
    
    std::copy_if(friendStatuses.begin(), friendStatuses.end(),
                 std::back_inserter(result), predicate);
    
    return result;
}

std::vector<std::string> FriendListFilter::filterOnline(
    const std::vector<std::string>& friendNames,
    const std::vector<FriendStatus>& friendStatuses
) {
    return filterByOnlineStatus(friendNames, friendStatuses, true);
}

std::vector<std::string> FriendListFilter::filterOffline(
    const std::vector<std::string>& friendNames,
    const std::vector<FriendStatus>& friendStatuses
) {
    return filterByOnlineStatus(friendNames, friendStatuses, false);
}


FriendListDiff FriendListDiffer::diff(
    const std::vector<std::string>& oldFriends,
    const std::vector<std::string>& newFriends
) {
    FriendListDiff diff;
    
    for (const auto& newFriend : newFriends) {
        if (!containsName(oldFriends, newFriend)) {
            diff.addedFriends.push_back(newFriend);
        }
    }
    
    for (const auto& oldFriend : oldFriends) {
        if (!containsName(newFriends, oldFriend)) {
            diff.removedFriends.push_back(oldFriend);
        }
    }
    
    return diff;
}

std::vector<std::string> FriendListDiffer::diffStatuses(
    const std::vector<FriendStatus>& oldStatuses,
    const std::vector<FriendStatus>& newStatuses
) {
    std::vector<std::string> changed;
    
    std::vector<std::pair<std::string, FriendStatus>> oldMap;
    for (const auto& status : oldStatuses) {
        oldMap.push_back({toLower(status.characterName), status});
    }
    
    for (const auto& newStatus : newStatuses) {
        std::string normalizedName = toLower(newStatus.characterName);
        
        auto it = std::find_if(oldMap.begin(), oldMap.end(),
            [&normalizedName](const auto& pair) {
                return pair.first == normalizedName;
            });
        
        if (it != oldMap.end()) {
            if (newStatus.hasStatusChanged(it->second)) {
                changed.push_back(newStatus.characterName);
            }
        } else {
            changed.push_back(newStatus.characterName);
        }
    }
    
    return changed;
}

std::vector<std::string> FriendListDiffer::diffOnlineStatus(
    const std::vector<FriendStatus>& oldStatuses,
    const std::vector<FriendStatus>& newStatuses
) {
    std::vector<std::string> changed;
    
    std::vector<std::pair<std::string, FriendStatus>> oldMap;
    for (const auto& status : oldStatuses) {
        oldMap.push_back({toLower(status.characterName), status});
    }
    
    for (const auto& newStatus : newStatuses) {
        std::string normalizedName = toLower(newStatus.characterName);
        
        auto it = std::find_if(oldMap.begin(), oldMap.end(),
            [&normalizedName](const auto& pair) {
                return pair.first == normalizedName;
            });
        
        if (it != oldMap.end()) {
            if (newStatus.hasOnlineStatusChanged(it->second)) {
                changed.push_back(newStatus.characterName);
            }
        } else if (newStatus.isOnline) {
            changed.push_back(newStatus.characterName);
        }
    }
    
    return changed;
}

std::string FriendListDiffer::normalizeName(const std::string& name) {
    return toLower(name);
}

bool FriendListDiffer::containsName(const std::vector<std::string>& names, const std::string& name) {
    std::string normalized = normalizeName(name);
    for (const auto& n : names) {
        if (normalizeName(n) == normalized) {
            return true;
        }
    }
    return false;
}

MemoryStats FriendList::getMemoryStats() const {
    size_t friendBytes = 0;
    for (const auto& f : friends_) {
        friendBytes += sizeof(Friend);
        friendBytes += f.name.capacity();
        friendBytes += f.friendedAs.capacity();
        for (const auto& linked : f.linkedCharacters) {
            friendBytes += linked.capacity();
        }
        friendBytes += f.linkedCharacters.capacity() * sizeof(std::string);
    }
    friendBytes += friends_.capacity() * sizeof(Friend);
    
    size_t statusBytes = 0;
    for (const auto& s : friendStatuses_) {
        statusBytes += sizeof(FriendStatus);
        statusBytes += s.characterName.capacity();
        statusBytes += s.displayName.capacity();
        statusBytes += s.job.capacity();
        statusBytes += s.rank.capacity();
        statusBytes += s.zone.capacity();
        statusBytes += s.altCharacterName.capacity();
        statusBytes += s.friendedAs.capacity();
        for (const auto& linked : s.linkedCharacters) {
            statusBytes += linked.capacity();
        }
        statusBytes += s.linkedCharacters.capacity() * sizeof(std::string);
    }
    statusBytes += friendStatuses_.capacity() * sizeof(FriendStatus);
    
    size_t totalBytes = friendBytes + statusBytes;
    size_t totalCount = friends_.size() + friendStatuses_.size();
    
    return MemoryStats(totalCount, totalBytes, "Friends");
}

} // namespace Core
} // namespace XIFriendList

