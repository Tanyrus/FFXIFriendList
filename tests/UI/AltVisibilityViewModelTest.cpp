#include <catch2/catch_test_macros.hpp>
#include "UI/ViewModels/AltVisibilityViewModel.h"
#include "Protocol/MessageTypes.h"

namespace XIFriendList {
namespace UI {

TEST_CASE("AltVisibilityViewModel - Initial state", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    REQUIRE(viewModel.getRows().empty());
    REQUIRE(viewModel.getCharacters().empty());
    REQUIRE(viewModel.isLoading() == false);
    REQUIRE(viewModel.hasError() == false);
    REQUIRE(viewModel.getError().empty());
    REQUIRE(viewModel.getLastUpdateTime() == 0);
    REQUIRE(viewModel.needsRefresh() == false);
}

TEST_CASE("AltVisibilityViewModel - UpdateFromResult", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    std::vector<Protocol::AltVisibilityFriendEntry> friends;
    Protocol::AltVisibilityFriendEntry friend1;
    friend1.friendAccountId = 1;
    friend1.friendedAsName = "FriendOne";
    friend1.displayName = "Friend One";
    friend1.visibilityMode = "ALL";
    
    Protocol::CharacterVisibilityState charVis1;
    charVis1.characterId = 1;
    charVis1.characterName = "Char1";
    charVis1.hasVisibility = true;
    charVis1.hasPendingVisibilityRequest = false;
    friend1.characterVisibility.push_back(charVis1);
    friends.push_back(friend1);
    
    std::vector<Protocol::AccountCharacterInfo> characters;
    Protocol::AccountCharacterInfo char1;
    char1.characterId = 1;
    char1.characterName = "Char1";
    char1.isActive = true;
    characters.push_back(char1);
    
    viewModel.updateFromResult(friends, characters);
    
    REQUIRE(viewModel.getRows().size() == 1);
    REQUIRE(viewModel.getRows()[0].friendAccountId == 1);
    REQUIRE(viewModel.getRows()[0].friendedAsName == "FriendOne");
    REQUIRE(viewModel.getRows()[0].displayName == "Friend One");
    REQUIRE(viewModel.getRows()[0].characterVisibility.size() == 1);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].characterId == 1);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::Visible);
    REQUIRE(viewModel.getCharacters().size() == 1);
    REQUIRE(viewModel.getCharacters()[0].characterId == 1);
}

TEST_CASE("AltVisibilityViewModel - Filtering", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    std::vector<Protocol::AltVisibilityFriendEntry> friends;
    
    Protocol::AltVisibilityFriendEntry friend1;
    friend1.friendAccountId = 1;
    friend1.friendedAsName = "Alice";
    friend1.displayName = "Alice";
    friends.push_back(friend1);
    
    Protocol::AltVisibilityFriendEntry friend2;
    friend2.friendAccountId = 2;
    friend2.friendedAsName = "Bob";
    friend2.displayName = "Bob";
    friends.push_back(friend2);
    
    std::vector<Protocol::AccountCharacterInfo> characters;
    
    viewModel.updateFromResult(friends, characters);
    
    auto filtered = viewModel.getFilteredRows("alice");
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].friendedAsName == "Alice");
    
    filtered = viewModel.getFilteredRows("bob");
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].friendedAsName == "Bob");
    
    filtered = viewModel.getFilteredRows("nonexistent");
    REQUIRE(filtered.empty());
    
    filtered = viewModel.getFilteredRows("");
    REQUIRE(filtered.size() == 2);
}

TEST_CASE("AltVisibilityViewModel - Visibility state computation", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    std::vector<Protocol::AltVisibilityFriendEntry> friends;
    Protocol::AltVisibilityFriendEntry friend1;
    friend1.friendAccountId = 1;
    friend1.friendedAsName = "Friend";
    
    SECTION("Visible state") {
        Protocol::CharacterVisibilityState charVis;
        charVis.characterId = 1;
        charVis.characterName = "Char1";
        charVis.hasVisibility = true;
        charVis.hasPendingVisibilityRequest = false;
        friend1.characterVisibility.push_back(charVis);
        friends.push_back(friend1);
        
        std::vector<Protocol::AccountCharacterInfo> characters;
        Protocol::AccountCharacterInfo char1;
        char1.characterId = 1;
        char1.characterName = "Char1";
        characters.push_back(char1);
        
        viewModel.updateFromResult(friends, characters);
        
        REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::Visible);
    }
    
    SECTION("NotVisible state") {
        Protocol::CharacterVisibilityState charVis;
        charVis.characterId = 1;
        charVis.characterName = "Char1";
        charVis.hasVisibility = false;
        charVis.hasPendingVisibilityRequest = false;
        friend1.characterVisibility.push_back(charVis);
        friends.push_back(friend1);
        
        std::vector<Protocol::AccountCharacterInfo> characters;
        Protocol::AccountCharacterInfo char1;
        char1.characterId = 1;
        char1.characterName = "Char1";
        characters.push_back(char1);
        
        viewModel.updateFromResult(friends, characters);
        
        REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::NotVisible);
    }
    
    SECTION("PendingRequest state") {
        Protocol::CharacterVisibilityState charVis;
        charVis.characterId = 1;
        charVis.characterName = "Char1";
        charVis.hasVisibility = false;
        charVis.hasPendingVisibilityRequest = true;
        friend1.characterVisibility.push_back(charVis);
        friends.push_back(friend1);
        
        std::vector<Protocol::AccountCharacterInfo> characters;
        Protocol::AccountCharacterInfo char1;
        char1.characterId = 1;
        char1.characterName = "Char1";
        characters.push_back(char1);
        
        viewModel.updateFromResult(friends, characters);
        
        REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::PendingRequest);
    }
}

TEST_CASE("AltVisibilityViewModel - SetVisibility", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    std::vector<Protocol::AltVisibilityFriendEntry> friends;
    Protocol::AltVisibilityFriendEntry friend1;
    friend1.friendAccountId = 1;
    friend1.friendedAsName = "Friend";
    
    Protocol::CharacterVisibilityState charVis;
    charVis.characterId = 1;
    charVis.characterName = "Char1";
    charVis.hasVisibility = false;
    charVis.hasPendingVisibilityRequest = false;
    friend1.characterVisibility.push_back(charVis);
    friends.push_back(friend1);
    
    std::vector<Protocol::AccountCharacterInfo> characters;
    Protocol::AccountCharacterInfo char1;
    char1.characterId = 1;
    char1.characterName = "Char1";
    characters.push_back(char1);
    
    viewModel.updateFromResult(friends, characters);
    
    bool result = viewModel.setVisibility(1, 1, true);
    REQUIRE(result == true);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].isBusy == true);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::PendingRequest);
    
    result = viewModel.setVisibility(1, 1, false);
    REQUIRE(result == true);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].visibilityState == AltVisibilityState::NotVisible);
    
    result = viewModel.setVisibility(999, 1, true);
    REQUIRE(result == false);
    
    result = viewModel.setVisibility(1, 999, true);
    REQUIRE(result == false);
}

TEST_CASE("AltVisibilityViewModel - SetBusy", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    std::vector<Protocol::AltVisibilityFriendEntry> friends;
    Protocol::AltVisibilityFriendEntry friend1;
    friend1.friendAccountId = 1;
    friend1.friendedAsName = "Friend";
    
    Protocol::CharacterVisibilityState charVis;
    charVis.characterId = 1;
    charVis.characterName = "Char1";
    charVis.hasVisibility = true;
    charVis.hasPendingVisibilityRequest = false;
    friend1.characterVisibility.push_back(charVis);
    friends.push_back(friend1);
    
    std::vector<Protocol::AccountCharacterInfo> characters;
    Protocol::AccountCharacterInfo char1;
    char1.characterId = 1;
    char1.characterName = "Char1";
    characters.push_back(char1);
    
    viewModel.updateFromResult(friends, characters);
    
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].isBusy == false);
    
    viewModel.setBusy(1, 1, true);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].isBusy == true);
    
    viewModel.setBusy(1, 1, false);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].isBusy == false);
    
    viewModel.setBusy(999, 1, true);
    REQUIRE(viewModel.getRows()[0].characterVisibility[0].isBusy == false);
}

TEST_CASE("AltVisibilityViewModel - Error handling", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    REQUIRE(viewModel.hasError() == false);
    REQUIRE(viewModel.getError().empty());
    
    viewModel.setError("Test error");
    REQUIRE(viewModel.hasError() == true);
    REQUIRE(viewModel.getError() == "Test error");
    
    viewModel.clearError();
    REQUIRE(viewModel.hasError() == false);
    REQUIRE(viewModel.getError().empty());
}

TEST_CASE("AltVisibilityViewModel - Loading state", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    REQUIRE(viewModel.isLoading() == false);
    
    viewModel.setLoading(true);
    REQUIRE(viewModel.isLoading() == true);
    
    viewModel.setLoading(false);
    REQUIRE(viewModel.isLoading() == false);
}

TEST_CASE("AltVisibilityViewModel - Refresh flag", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    REQUIRE(viewModel.needsRefresh() == false);
    
    viewModel.markNeedsRefresh();
    REQUIRE(viewModel.needsRefresh() == true);
    
    viewModel.clearNeedsRefresh();
    REQUIRE(viewModel.needsRefresh() == false);
}

TEST_CASE("AltVisibilityViewModel - Last update time", "[AltVisibilityViewModel]") {
    AltVisibilityViewModel viewModel;
    
    REQUIRE(viewModel.getLastUpdateTime() == 0);
    
    viewModel.setLastUpdateTime(1234567890);
    REQUIRE(viewModel.getLastUpdateTime() == 1234567890);
}

} // namespace UI
} // namespace XIFriendList

