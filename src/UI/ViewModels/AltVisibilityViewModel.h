// ViewModel for Alt Visibility window

#ifndef UI_ALT_VISIBILITY_VIEW_MODEL_H
#define UI_ALT_VISIBILITY_VIEW_MODEL_H

#include "Protocol/MessageTypes.h"
#include "Core/MemoryStats.h"
#include <string>
#include <vector>
#include <cstdint>

namespace XIFriendList {
namespace UI {

enum class AltVisibilityState {
    Visible,           // Friend is visible to current character
    NotVisible,        // Friend is not visible to current character
    PendingRequest,    // Visibility request is pending
    Unknown            // Cannot determine state from server
};

// Character visibility state for a specific character
struct CharacterVisibilityData {
    int characterId;
    std::string characterName;
    AltVisibilityState visibilityState;
    bool hasPendingRequest;
    bool isBusy;  // Whether a visibility request is in-flight for this character
    
    CharacterVisibilityData()
        : characterId(0)
        , visibilityState(AltVisibilityState::NotVisible)
        , hasPendingRequest(false)
        , isBusy(false)
    {}
    
    bool checkboxChecked() const {
        return visibilityState == AltVisibilityState::Visible;
    }
    
    bool checkboxEnabled() const {
        return !isBusy && 
               visibilityState != AltVisibilityState::PendingRequest && 
               visibilityState != AltVisibilityState::Unknown;
    }
};

struct AltVisibilityRowData {
    int friendAccountId;
    std::string friendedAsName;      // Original name when friended
    std::string displayName;         // Current display name (active character or friendedAs)
    std::string visibilityMode;      // "ALL" or "ONLY"
    std::vector<CharacterVisibilityData> characterVisibility;  // Visibility state for each character
    
    AltVisibilityRowData()
        : friendAccountId(0)
        , visibilityMode("ALL")
    {}
};

// ViewModel for Alt Visibility window
class AltVisibilityViewModel {
public:
    AltVisibilityViewModel();
    ~AltVisibilityViewModel() = default;
    
    void updateFromResult(const std::vector<XIFriendList::Protocol::AltVisibilityFriendEntry>& friends,
                         const std::vector<XIFriendList::Protocol::AccountCharacterInfo>& characters);
    
    const std::vector<AltVisibilityRowData>& getRows() const { return rows_; }
    
    const std::vector<XIFriendList::Protocol::AccountCharacterInfo>& getCharacters() const { return characters_; }
    
    std::vector<AltVisibilityRowData> getFilteredRows(const std::string& filterText) const;
    
    // Loading state
    bool isLoading() const { return isLoading_; }
    void setLoading(bool loading) { isLoading_ = loading; }
    
    // Error state
    const std::string& getError() const { return error_; }
    void setError(const std::string& error) { error_ = error; }
    void clearError() { error_ = ""; }
    bool hasError() const { return !error_.empty(); }
    
    // Last update time
    uint64_t getLastUpdateTime() const { return lastUpdateTime_; }
    void setLastUpdateTime(uint64_t time) { lastUpdateTime_ = time; }
    
    // Refresh flag
    void markNeedsRefresh() { needsRefresh_ = true; }
    bool needsRefresh() const { return needsRefresh_; }
    void clearNeedsRefresh() { needsRefresh_ = false; }
    
    // Returns true if friend and character were found and state was updated
    bool setVisibility(int friendAccountId, int characterId, bool desiredVisible);
    
    // Mark a friend/character as busy (while request is in-flight)
    void setBusy(int friendAccountId, int characterId, bool busy);
    
    // Find row by friend account ID (non-const for internal use)
    AltVisibilityRowData* findRow(int friendAccountId);
    
    // Find character visibility data in a row
    CharacterVisibilityData* findCharacterVisibility(AltVisibilityRowData* row, int characterId);
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    std::vector<AltVisibilityRowData> rows_;
    std::vector<XIFriendList::Protocol::AccountCharacterInfo> characters_;  // All characters on the account
    bool isLoading_;
    std::string error_;
    uint64_t lastUpdateTime_;
    bool needsRefresh_;
    
    // Helper to compute visibility state from character visibility entry
    AltVisibilityState computeVisibilityState(const XIFriendList::Protocol::CharacterVisibilityState& charVis);
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_ALT_VISIBILITY_VIEW_MODEL_H

