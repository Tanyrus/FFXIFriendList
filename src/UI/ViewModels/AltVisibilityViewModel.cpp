
#include "UI/ViewModels/AltVisibilityViewModel.h"
#include "Core/MemoryStats.h"
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace UI {

AltVisibilityViewModel::AltVisibilityViewModel()
    : isLoading_(false)
    , lastUpdateTime_(0)
    , needsRefresh_(false)
{}

void AltVisibilityViewModel::updateFromResult(const std::vector<XIFriendList::Protocol::AltVisibilityFriendEntry>& friends,
                                             const std::vector<XIFriendList::Protocol::AccountCharacterInfo>& characters) {
    rows_.clear();
    rows_.reserve(friends.size());
    characters_ = characters;
    
    for (const auto& friendEntry : friends) {
        AltVisibilityRowData row;
        row.friendAccountId = friendEntry.friendAccountId;
        row.friendedAsName = friendEntry.friendedAsName;
        row.displayName = friendEntry.displayName.empty() ? friendEntry.friendedAsName : friendEntry.displayName;
        row.visibilityMode = friendEntry.visibilityMode;
        
        row.characterVisibility.clear();
        row.characterVisibility.reserve(characters.size());
        
        for (const auto& charInfo : characters) {
            CharacterVisibilityData charVis;
            charVis.characterId = charInfo.characterId;
            charVis.characterName = charInfo.characterName;
            
            // Find visibility state for this character from friend entry
            bool found = false;
            for (const auto& charVisState : friendEntry.characterVisibility) {
                if (charVisState.characterId == charInfo.characterId) {
                    charVis.visibilityState = computeVisibilityState(charVisState);
                    charVis.hasPendingRequest = charVisState.hasPendingVisibilityRequest;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                charVis.visibilityState = AltVisibilityState::NotVisible;
                charVis.hasPendingRequest = false;
            }
            
            row.characterVisibility.push_back(charVis);
        }
        
        rows_.push_back(row);
    }
    
    // Sort by friendedAsName for consistent display
    std::sort(rows_.begin(), rows_.end(), [](const AltVisibilityRowData& a, const AltVisibilityRowData& b) {
        return a.friendedAsName < b.friendedAsName;
    });
    
    needsRefresh_ = false;
}

AltVisibilityState AltVisibilityViewModel::computeVisibilityState(const XIFriendList::Protocol::CharacterVisibilityState& charVis) {
    if (charVis.hasPendingVisibilityRequest) {
        return AltVisibilityState::PendingRequest;
    }
    
    if (charVis.hasVisibility) {
        return AltVisibilityState::Visible;
    }
    
    return AltVisibilityState::NotVisible;
}

std::vector<AltVisibilityRowData> AltVisibilityViewModel::getFilteredRows(const std::string& filterText) const {
    if (filterText.empty()) {
        return rows_;
    }
    
    std::string filterLower = filterText;
    std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    std::vector<AltVisibilityRowData> filtered;
    for (const auto& row : rows_) {
        std::string friendedAsLower = row.friendedAsName;
        std::transform(friendedAsLower.begin(), friendedAsLower.end(), friendedAsLower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        std::string displayNameLower = row.displayName;
        std::transform(displayNameLower.begin(), displayNameLower.end(), displayNameLower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (friendedAsLower.find(filterLower) != std::string::npos ||
            displayNameLower.find(filterLower) != std::string::npos) {
            filtered.push_back(row);
        }
    }
    
    return filtered;
}

bool AltVisibilityViewModel::setVisibility(int friendAccountId, int characterId, bool desiredVisible) {
    AltVisibilityRowData* row = findRow(friendAccountId);
    if (!row) {
        return false;
    }
    
    CharacterVisibilityData* charVis = findCharacterVisibility(row, characterId);
    if (!charVis) {
        return false;
    }
    
    // Mark as busy while request is in-flight
    charVis->isBusy = true;
    
    // Optimistically update state (will be corrected by server response)
    if (desiredVisible) {
        // Setting to visible - will become PendingRequest if request-based, or Visible if immediate
        // We'll update this based on server response
        charVis->visibilityState = AltVisibilityState::PendingRequest;  // Assume request-based initially
    } else {
        // Setting to not visible - will become NotVisible after server confirms
        charVis->visibilityState = AltVisibilityState::NotVisible;
    }
    
    return true;
}

void AltVisibilityViewModel::setBusy(int friendAccountId, int characterId, bool busy) {
    AltVisibilityRowData* row = findRow(friendAccountId);
    if (!row) {
        return;
    }
    
    CharacterVisibilityData* charVis = findCharacterVisibility(row, characterId);
    if (charVis) {
        charVis->isBusy = busy;
    }
}

CharacterVisibilityData* AltVisibilityViewModel::findCharacterVisibility(AltVisibilityRowData* row, int characterId) {
    if (!row) {
        return nullptr;
    }
    
    for (auto& charVis : row->characterVisibility) {
        if (charVis.characterId == characterId) {
            return &charVis;
        }
    }
    
    return nullptr;
}

AltVisibilityRowData* AltVisibilityViewModel::findRow(int friendAccountId) {
    for (auto& row : rows_) {
        if (row.friendAccountId == friendAccountId) {
            return &row;
        }
    }
    return nullptr;
}

XIFriendList::Core::MemoryStats AltVisibilityViewModel::getMemoryStats() const {
    size_t bytes = sizeof(AltVisibilityViewModel);
    
    for (const auto& row : rows_) {
        bytes += sizeof(AltVisibilityRowData);
        bytes += row.friendedAsName.capacity();
        bytes += row.displayName.capacity();
        bytes += row.visibilityMode.capacity();
        for (const auto& charVis : row.characterVisibility) {
            bytes += sizeof(CharacterVisibilityData);
            bytes += charVis.characterName.capacity();
        }
        bytes += row.characterVisibility.capacity() * sizeof(CharacterVisibilityData);
    }
    bytes += rows_.capacity() * sizeof(AltVisibilityRowData);
    
    for (const auto& charInfo : characters_) {
        bytes += sizeof(XIFriendList::Protocol::AccountCharacterInfo);
        bytes += charInfo.characterName.capacity();
    }
    bytes += characters_.capacity() * sizeof(XIFriendList::Protocol::AccountCharacterInfo);
    
    bytes += error_.capacity();
    
    size_t count = rows_.size() + characters_.size();
    
    return XIFriendList::Core::MemoryStats(count, bytes, "AltVisibility ViewModel");
}

} // namespace UI
} // namespace XIFriendList

