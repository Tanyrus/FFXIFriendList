// FriendListViewModelTest.cpp
// Unit tests for FriendListViewModel

#include <catch2/catch_test_macros.hpp>
#include "UI/ViewModels/FriendListViewModel.h"
#include "Core/FriendsCore.h"
#include "App/StateMachines/ConnectionState.h"
#include "Protocol/MessageTypes.h"

using namespace XIFriendList::UI;
using namespace XIFriendList::Core;
using namespace XIFriendList::App;
using namespace XIFriendList::Protocol;

TEST_CASE("FriendListViewModel - Initial state", "[UI][ViewModel]") {
    FriendListViewModel vm;
    
    REQUIRE(vm.getFriendRows().empty());
    REQUIRE(vm.getConnectionState() == ConnectionState::Disconnected);
    REQUIRE_FALSE(vm.isConnected());
    REQUIRE(vm.getErrorMessage().empty());
}

TEST_CASE("FriendListViewModel - Connection state text", "[UI][ViewModel]") {
    FriendListViewModel vm;
    
    vm.setConnectionState(ConnectionState::Disconnected);
    REQUIRE(vm.getConnectionStatusText() == "Disconnected");
    
    vm.setConnectionState(ConnectionState::Connecting);
    REQUIRE(vm.getConnectionStatusText() == "Connecting...");
    
    vm.setConnectionState(ConnectionState::Connected);
    REQUIRE(vm.getConnectionStatusText() == "Connected");
    REQUIRE(vm.isConnected());
    
    vm.setConnectionState(ConnectionState::Reconnecting);
    REQUIRE(vm.getConnectionStatusText() == "Reconnecting...");
    
    vm.setConnectionState(ConnectionState::Failed);
    REQUIRE(vm.getConnectionStatusText() == "Connection Failed");
}

TEST_CASE("FriendListViewModel - Error message", "[UI][ViewModel]") {
    FriendListViewModel vm;
    
    REQUIRE(vm.getErrorMessage().empty());
    
    vm.setErrorMessage("Test error");
    REQUIRE(vm.getErrorMessage() == "Test error");
    
    vm.clearError();
    REQUIRE(vm.getErrorMessage().empty());
}

TEST_CASE("FriendListViewModel - Update with empty friend list", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    std::vector<FriendStatus> statuses;
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses, currentTime);
    
    REQUIRE(vm.getFriendRows().empty());
}

TEST_CASE("FriendListViewModel - Update with friends", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    friendList.addFriend("friend1", "FriendOne");
    friendList.addFriend("friend2", "FriendTwo");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friendone";  // Originally friended name (normalized)
    status1.isOnline = true;
    status1.job = "WHM";
    status1.rank = "75";
    status1.zone = "Windurst";
    status1.lastSeenAt = 1000;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";  // Active online character
    status2.friendedAs = "friendtwo";  // Originally friended name (normalized)
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses.push_back(status2);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 2);
    
    // Online friend should be first
    // Name column shows displayName (active online character), FriendedAs column shows friendedAs
    REQUIRE(rows[0].name == "friend1");  // displayName (active online character)
    REQUIRE(rows[0].friendedAs == "friendone");  // friendedAs (originally friended name, normalized)
    REQUIRE(rows[0].isOnline);
    REQUIRE(rows[0].sortKey == 0);
    
    // Offline friend should be second
    REQUIRE(rows[1].name == "friend2");  // displayName (active online character)
    REQUIRE(rows[1].friendedAs == "friendtwo");  // friendedAs (originally friended name, normalized)
    REQUIRE_FALSE(rows[1].isOnline);
    REQUIRE(rows[1].sortKey == 1);
}

TEST_CASE("FriendListViewModel - Presence fields never render blank (Unknown/Hidden)", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");

    std::vector<FriendStatus> statuses;
    FriendStatus status;
    status.characterName = "friend1";
    status.displayName = "friend1";
    status.friendedAs = "friendone";
    status.isOnline = true;
    status.showOnlineStatus = true;
    status.job = "";          // missing
    status.rank = "";         // missing
    status.zone = "";         // missing
    status.nation = -1;       // hidden/not set (server sent null)
    statuses.push_back(status);

    vm.setShowJobColumn(true);
    vm.setShowZoneColumn(true);
    vm.setShowNationRankColumn(true);
    vm.update(friendList, statuses, 1000000);
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].jobText == "Hidden");
    REQUIRE(rows[0].zoneText == "Hidden");
    REQUIRE(rows[0].nationRankText == "Hidden");  // Combined column shows "Hidden" when nation=-1 or rank missing
}

TEST_CASE("FriendListViewModel - Status change detection", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses1;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friendone";  // Originally friended name (normalized)
    status1.isOnline = false;
    status1.showOnlineStatus = true;
    statuses1.push_back(status1);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses1, currentTime);
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 1);
    REQUIRE(rows1[0].hasStatusChanged);  // First update, should be marked as changed
    
    // Update with online status
    std::vector<FriendStatus> statuses2;
    FriendStatus status2;
    status2.characterName = "friend1";
    status2.displayName = "friend1";  // Active online character
    status2.friendedAs = "friendone";  // Originally friended name (normalized)
    status2.isOnline = true;
    status2.showOnlineStatus = true;
    statuses2.push_back(status2);
    
    vm.update(friendList, statuses2, currentTime);
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 1);
    REQUIRE(rows2[0].hasOnlineStatusChanged);  // Online status changed
}

TEST_CASE("FriendListViewModel - Sent friend requests appear in friend list", "[UI][ViewModel][FriendRequests]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add one regular friend
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friendone";  // Originally friended name (normalized)
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    // Add sent friend requests
    std::vector<FriendRequestPayload> outgoingRequests;
    FriendRequestPayload request1;
    request1.requestId = "req1";
    request1.toCharacterName = "pendingfriend1";
    request1.toAccountId = 10;
    outgoingRequests.push_back(request1);
    
    FriendRequestPayload request2;
    request2.requestId = "req2";
    request2.toCharacterName = "pendingfriend2";
    request2.toAccountId = 11;
    outgoingRequests.push_back(request2);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.updateWithRequests(friendList, statuses, outgoingRequests, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 3);  // 1 friend + 2 pending requests
    
    // Regular friend should be first (online, sortKey = 0)
    REQUIRE(rows[0].name == "friend1");  // displayName (active online character)
    REQUIRE(rows[0].friendedAs == "friendone");  // friendedAs (originally friended name, normalized)
    REQUIRE(rows[0].isOnline);
    REQUIRE_FALSE(rows[0].isPending);
    REQUIRE(rows[0].sortKey == 0);
    
    // Pending requests should be at the bottom (sortKey = 2)
    REQUIRE(rows[1].name == "pendingfriend1");
    REQUIRE(rows[1].isPending);
    REQUIRE_FALSE(rows[1].isOnline);
    REQUIRE(rows[1].statusText == "[Pending]");
    REQUIRE(rows[1].sortKey == 2);
    REQUIRE(rows[1].jobText.empty());
    REQUIRE(rows[1].zoneText.empty());
    
    REQUIRE(rows[2].name == "pendingfriend2");
    REQUIRE(rows[2].isPending);
    REQUIRE(rows[2].sortKey == 2);
}

TEST_CASE("FriendListViewModel - Sent requests sorted alphabetically at bottom", "[UI][ViewModel][FriendRequests]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add offline friend
    friendList.addFriend("offlinefriend", "OfflineFriend");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "offlinefriend";
    status1.displayName = "offlinefriend";  // Active online character
    status1.friendedAs = "offlinefriend";  // Originally friended name (normalized)
    status1.isOnline = false;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    // Add sent requests in non-alphabetical order
    std::vector<FriendRequestPayload> outgoingRequests;
    FriendRequestPayload request1;
    request1.requestId = "req1";
    request1.toCharacterName = "ZebraFriend";
    request1.toAccountId = 10;
    outgoingRequests.push_back(request1);
    
    FriendRequestPayload request2;
    request2.requestId = "req2";
    request2.toCharacterName = "AlphaFriend";
    request2.toAccountId = 11;
    outgoingRequests.push_back(request2);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.updateWithRequests(friendList, statuses, outgoingRequests, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 3);
    
    // Offline friend first (sortKey = 1)
    REQUIRE(rows[0].name == "offlinefriend");
    REQUIRE_FALSE(rows[0].isPending);
    
    // Pending requests at bottom, sorted alphabetically
    REQUIRE(rows[1].name == "alphafriend");  // AlphaFriend should come before ZebraFriend
    REQUIRE(rows[1].isPending);
    REQUIRE(rows[2].name == "zebrafriend");
    REQUIRE(rows[2].isPending);
}

TEST_CASE("FriendListViewModel - No sent requests when list is empty", "[UI][ViewModel][FriendRequests]") {
    FriendListViewModel vm;
    FriendList friendList;
    std::vector<FriendStatus> statuses;
    std::vector<FriendRequestPayload> outgoingRequests;  // Empty
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.updateWithRequests(friendList, statuses, outgoingRequests, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.empty());
}

TEST_CASE("FriendListViewModel - Sent request not shown if already a friend", "[UI][ViewModel][FriendRequests]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add friend
    friendList.addFriend("existingfriend", "ExistingFriend");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status1;
    status1.characterName = "existingfriend";
    status1.displayName = "existingfriend";  // Active online character
    status1.friendedAs = "existingfriend";  // Originally friended name (normalized)
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses.push_back(status1);
    
    // Try to add sent request for same friend (shouldn't appear twice)
    std::vector<FriendRequestPayload> outgoingRequests;
    FriendRequestPayload request1;
    request1.requestId = "req1";
    request1.toCharacterName = "existingfriend";  // Same as friend name
    request1.toAccountId = 10;
    outgoingRequests.push_back(request1);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.updateWithRequests(friendList, statuses, outgoingRequests, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);  // Only the friend, not the pending request
    REQUIRE(rows[0].name == "existingfriend");  // displayName (active online character)
    REQUIRE_FALSE(rows[0].isPending);
}

TEST_CASE("FriendListViewModel - Pending requests section data", "[UI][ViewModel][FriendRequests]") {
    FriendListViewModel vm;
    
    // Initially empty
    REQUIRE(vm.getIncomingRequests().empty());
    REQUIRE(vm.getOutgoingRequests().empty());
    
    // Update with pending requests
    std::vector<FriendRequestPayload> incoming;
    FriendRequestPayload incomingReq;
    incomingReq.requestId = "incoming1";
    incomingReq.fromCharacterName = "requester1";
    incomingReq.toCharacterName = "currentuser";
    incomingReq.fromAccountId = 1;
    incomingReq.toAccountId = 2;
    incoming.push_back(incomingReq);
    
    std::vector<FriendRequestPayload> outgoing;
    FriendRequestPayload outgoingReq;
    outgoingReq.requestId = "outgoing1";
    outgoingReq.fromCharacterName = "currentuser";
    outgoingReq.toCharacterName = "targetuser";
    outgoingReq.fromAccountId = 2;
    outgoingReq.toAccountId = 3;
    outgoing.push_back(outgoingReq);
    
    vm.updatePendingRequests(incoming, outgoing);
    
    const auto& incomingReqs = vm.getIncomingRequests();
    REQUIRE(incomingReqs.size() == 1);
    REQUIRE(incomingReqs[0].requestId == "incoming1");
    REQUIRE(incomingReqs[0].fromCharacterName == "requester1");
    
    const auto& outgoingReqs = vm.getOutgoingRequests();
    REQUIRE(outgoingReqs.size() == 1);
    REQUIRE(outgoingReqs[0].requestId == "outgoing1");
    REQUIRE(outgoingReqs[0].toCharacterName == "targetuser");
}

TEST_CASE("FriendListViewModel - Stable ordering preserves position when sort keys equal", "[UI][ViewModel][StableOrdering]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add multiple offline friends (all have sortKey = 1)
    friendList.addFriend("friendA", "FriendA");
    friendList.addFriend("friendB", "FriendB");
    friendList.addFriend("friendC", "FriendC");
    
    std::vector<FriendStatus> statuses;
    FriendStatus statusA;
    statusA.characterName = "friendA";
    statusA.displayName = "friendA";  // Active online character
    statusA.friendedAs = "frienda";  // Originally friended name (normalized)
    statusA.isOnline = false;
    statusA.showOnlineStatus = true;
    statuses.push_back(statusA);
    
    FriendStatus statusB;
    statusB.characterName = "friendB";
    statusB.displayName = "friendB";  // Active online character
    statusB.friendedAs = "friendb";  // Originally friended name (normalized)
    statusB.isOnline = false;
    statusB.showOnlineStatus = true;
    statuses.push_back(statusB);
    
    FriendStatus statusC;
    statusC.characterName = "friendC";
    statusC.displayName = "friendC";  // Active online character
    statusC.friendedAs = "friendc";  // Originally friended name (normalized)
    statusC.isOnline = false;
    statusC.showOnlineStatus = true;
    statuses.push_back(statusC);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses, currentTime);
    
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 3);
    
    // Record initial order
    std::vector<std::string> initialOrder;
    for (const auto& row : rows1) {
        initialOrder.push_back(row.name);
    }
    
    // Update with same data (presence update, no sort key changes)
    vm.update(friendList, statuses, currentTime);
    
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 3);
    
    // Order should be preserved (stable sort)
    for (size_t i = 0; i < rows2.size(); ++i) {
        REQUIRE(rows2[i].name == initialOrder[i]);
    }
}

TEST_CASE("FriendListViewModel - Reconciliation preserves row position when updating existing friends", "[UI][ViewModel][Reconciliation]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add friends in specific order
    friendList.addFriend("friend1", "Friend1");
    friendList.addFriend("friend2", "Friend2");
    friendList.addFriend("friend3", "Friend3");
    
    std::vector<FriendStatus> statuses1;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friend1";  // Originally friended name (normalized)
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    statuses1.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";  // Active online character
    status2.friendedAs = "friend2";  // Originally friended name (normalized)
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses1.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "friend3";
    status3.displayName = "friend3";  // Active online character
    status3.friendedAs = "friend3";  // Originally friended name (normalized)
    status3.isOnline = false;
    status3.showOnlineStatus = true;
    statuses1.push_back(status3);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses1, currentTime);
    
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 3);
    REQUIRE(rows1[0].name == "friend1");  // Online first
    REQUIRE(rows1[1].name == "friend2");  // Offline second
    REQUIRE(rows1[2].name == "friend3");  // Offline third
    
    // Update presence for friend2 (goes online) - should preserve relative order of friend2 and friend3
    std::vector<FriendStatus> statuses2;
    statuses2.push_back(status1);  // friend1 still online
    
    FriendStatus status2Updated;
    status2Updated.characterName = "friend2";
    status2Updated.displayName = "friend2";  // Active online character
    status2Updated.friendedAs = "friend2";  // Originally friended name (normalized)
    status2Updated.isOnline = true;  // Now online
    status2Updated.showOnlineStatus = true;
    statuses2.push_back(status2Updated);
    
    statuses2.push_back(status3);  // friend3 still offline
    
    vm.update(friendList, statuses2, currentTime);
    
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 3);
    
    // Both friend1 and friend2 are now online (sortKey = 0)
    // They should be sorted alphabetically, but friend1 should still be first
    // (stable sort preserves original order when keys are equal)
    REQUIRE(rows2[0].name == "friend1");
    REQUIRE(rows2[0].isOnline);
    REQUIRE(rows2[1].name == "friend2");
    REQUIRE(rows2[1].isOnline);
    REQUIRE(rows2[2].name == "friend3");
    REQUIRE_FALSE(rows2[2].isOnline);
}

TEST_CASE("FriendListViewModel - Case-insensitive sorting for deterministic tie-breaking", "[UI][ViewModel][StableOrdering]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    // Add friends with mixed case (will be normalized by FriendList)
    friendList.addFriend("FriendA", "FriendA");
    friendList.addFriend("friendB", "friendB");
    friendList.addFriend("FRIENDC", "FRIENDC");
    
    std::vector<FriendStatus> statuses;
    FriendStatus statusA;
    statusA.characterName = "FriendA";
    statusA.displayName = "frienda";  // Active online character (normalized)
    statusA.friendedAs = "frienda";  // Originally friended name (normalized)
    statusA.isOnline = false;
    statusA.showOnlineStatus = true;
    statuses.push_back(statusA);
    
    FriendStatus statusB;
    statusB.characterName = "friendB";
    statusB.displayName = "friendb";  // Active online character (normalized)
    statusB.friendedAs = "friendb";  // Originally friended name (normalized)
    statusB.isOnline = false;
    statusB.showOnlineStatus = true;
    statuses.push_back(statusB);
    
    FriendStatus statusC;
    statusC.characterName = "FRIENDC";
    statusC.displayName = "friendc";  // Active online character (normalized)
    statusC.friendedAs = "friendc";  // Originally friended name (normalized)
    statusC.isOnline = false;
    statusC.showOnlineStatus = true;
    statuses.push_back(statusC);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses, currentTime);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 3);
    
    // All offline (sortKey = 1), should be sorted case-insensitively by name
    // Note: FriendList normalizes names to lowercase, so display names will be lowercase
    // But the sort should be deterministic based on case-insensitive comparison
    REQUIRE(rows[0].name == "frienda");
    REQUIRE(rows[1].name == "friendb");
    REQUIRE(rows[2].name == "friendc");
}

TEST_CASE("FriendListViewModel - Presence updates don't reorder when sort keys unchanged", "[UI][ViewModel][Reconciliation]") {
    FriendListViewModel vm;
    FriendList friendList;
    
    friendList.addFriend("friend1", "Friend1");
    friendList.addFriend("friend2", "Friend2");
    
    std::vector<FriendStatus> statuses1;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friendone";  // Originally friended name (normalized)
    status1.isOnline = true;
    status1.job = "WHM";
    status1.zone = "Windurst";
    status1.showOnlineStatus = true;
    statuses1.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";  // Active online character
    status2.friendedAs = "friendtwo";  // Originally friended name (normalized)
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses1.push_back(status2);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList, statuses1, currentTime);
    
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 2);
    std::string firstFriendName = rows1[0].name;
    std::string secondFriendName = rows1[1].name;
    
    // Update presence (zone change, but still online - sortKey unchanged)
    std::vector<FriendStatus> statuses2;
    FriendStatus status1Updated;
    status1Updated.characterName = "friend1";
    status1Updated.displayName = "friend1";  // Active online character
    status1Updated.friendedAs = "friendone";  // Originally friended name (normalized)
    status1Updated.isOnline = true;  // Still online
    status1Updated.job = "WHM";
    status1Updated.zone = "Bastok";  // Zone changed
    status1Updated.showOnlineStatus = true;
    statuses2.push_back(status1Updated);
    statuses2.push_back(status2);  // friend2 unchanged
    
    vm.update(friendList, statuses2, currentTime);
    
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 2);
    
    // Order should be preserved (sort keys didn't change)
    REQUIRE(rows2[0].name == firstFriendName);
    REQUIRE(rows2[1].name == secondFriendName);
    REQUIRE(rows2[0].zoneText == "Bastok");  // Zone updated
}

TEST_CASE("FriendListViewModel - New friend added maintains existing order", "[UI][ViewModel][Reconciliation]") {
    FriendListViewModel vm;
    FriendList friendList1;
    
    // Initial state: 2 friends
    friendList1.addFriend("friend1", "Friend1");
    friendList1.addFriend("friend2", "Friend2");
    
    std::vector<FriendStatus> statuses1;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friend1";  // Originally friended name (normalized)
    status1.isOnline = false;
    status1.showOnlineStatus = true;
    statuses1.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";  // Active online character
    status2.friendedAs = "friend2";  // Originally friended name (normalized)
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses1.push_back(status2);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList1, statuses1, currentTime);
    
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 2);
    REQUIRE(rows1[0].name == "friend1");  // displayName (active online character)
    REQUIRE(rows1[1].name == "friend2");  // displayName (active online character)
    
    // Add new friend (friend3)
    FriendList friendList2;
    friendList2.addFriend("friend1", "Friend1");
    friendList2.addFriend("friend2", "Friend2");
    friendList2.addFriend("friend3", "Friend3");  // New friend
    
    std::vector<FriendStatus> statuses2;
    statuses2.push_back(status1);
    statuses2.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "friend3";
    status3.displayName = "friend3";  // Active online character
    status3.friendedAs = "friend3";  // Originally friended name (normalized)
    status3.isOnline = false;
    status3.showOnlineStatus = true;
    statuses2.push_back(status3);
    
    vm.update(friendList2, statuses2, currentTime);
    
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 3);
    
    // Existing friends should maintain their relative order
    REQUIRE(rows2[0].name == "friend1");
    REQUIRE(rows2[1].name == "friend2");
    // New friend added at end (before sorting)
    // After sorting (all offline, alphabetical), order should be: friend1, friend2, friend3
    REQUIRE(rows2[2].name == "friend3");
}

TEST_CASE("FriendListViewModel - Friend removed doesn't affect remaining order", "[UI][ViewModel][Reconciliation]") {
    FriendListViewModel vm;
    FriendList friendList1;
    
    // Initial state: 3 friends
    friendList1.addFriend("friend1", "Friend1");
    friendList1.addFriend("friend2", "Friend2");
    friendList1.addFriend("friend3", "Friend3");
    
    std::vector<FriendStatus> statuses1;
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";  // Active online character
    status1.friendedAs = "friend1";  // Originally friended name (normalized)
    status1.isOnline = false;
    status1.showOnlineStatus = true;
    statuses1.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";  // Active online character
    status2.friendedAs = "friend2";  // Originally friended name (normalized)
    status2.isOnline = false;
    status2.showOnlineStatus = true;
    statuses1.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "friend3";
    status3.displayName = "friend3";  // Active online character
    status3.friendedAs = "friend3";  // Originally friended name (normalized)
    status3.isOnline = false;
    status3.showOnlineStatus = true;
    statuses1.push_back(status3);
    
    uint64_t currentTime = 1000000;  // Test timestamp
    vm.update(friendList1, statuses1, currentTime);
    
    const auto& rows1 = vm.getFriendRows();
    REQUIRE(rows1.size() == 3);
    
    // Remove friend2
    FriendList friendList2;
    friendList2.addFriend("friend1", "Friend1");
    friendList2.addFriend("friend3", "Friend3");  // friend2 removed
    
    std::vector<FriendStatus> statuses2;
    statuses2.push_back(status1);
    statuses2.push_back(status3);
    
    vm.update(friendList2, statuses2, currentTime);
    
    const auto& rows2 = vm.getFriendRows();
    REQUIRE(rows2.size() == 2);
    
    // Remaining friends should maintain their relative order
    REQUIRE(rows2[0].name == "friend1");
    REQUIRE(rows2[1].name == "friend3");
}

TEST_CASE("FriendListViewModel - Friended As column default hidden", "[UI][ViewModel]") {
    FriendListViewModel vm;
    
    REQUIRE_FALSE(vm.getShowFriendedAsColumn());  // Should be false by default
}

TEST_CASE("FriendListViewModel - Friended As column visibility toggle", "[UI][ViewModel]") {
    FriendListViewModel vm;
    
    REQUIRE_FALSE(vm.getShowFriendedAsColumn());
    
    vm.setShowFriendedAsColumn(true);
    REQUIRE(vm.getShowFriendedAsColumn());
    
    vm.setShowFriendedAsColumn(false);
    REQUIRE_FALSE(vm.getShowFriendedAsColumn());
}

TEST_CASE("FriendListViewModel - Nation+Rank combined column format", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status;
    status.characterName = "friend1";
    status.displayName = "friend1";
    status.friendedAs = "friendone";
    status.isOnline = true;
    status.showOnlineStatus = true;
    status.job = "WHM";
    status.rank = "10";
    status.nation = 0;  // San d'Oria
    statuses.push_back(status);
    
    vm.setShowNationRankColumn(true);
    vm.update(friendList, statuses, 1000000);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].nationRankText == "S 10");  // San d'Oria icon + rank
}

TEST_CASE("FriendListViewModel - Nation+Rank combined column with different nations", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    friendList.addFriend("friend2", "FriendTwo");
    friendList.addFriend("friend3", "FriendThree");
    friendList.addFriend("friend4", "FriendFour");
    
    std::vector<FriendStatus> statuses;
    
    FriendStatus status1;
    status1.characterName = "friend1";
    status1.displayName = "friend1";
    status1.friendedAs = "friendone";
    status1.isOnline = true;
    status1.showOnlineStatus = true;
    status1.job = "WHM";
    status1.rank = "10";
    status1.nation = 0;  // San d'Oria
    statuses.push_back(status1);
    
    FriendStatus status2;
    status2.characterName = "friend2";
    status2.displayName = "friend2";
    status2.friendedAs = "friendtwo";
    status2.isOnline = true;
    status2.showOnlineStatus = true;
    status2.job = "BLM";
    status2.rank = "75";
    status2.nation = 1;  // Bastok
    statuses.push_back(status2);
    
    FriendStatus status3;
    status3.characterName = "friend3";
    status3.displayName = "friend3";
    status3.friendedAs = "friendthree";
    status3.isOnline = true;
    status3.showOnlineStatus = true;
    status3.job = "RDM";
    status3.rank = "50";
    status3.nation = 2;  // Windurst
    statuses.push_back(status3);
    
    FriendStatus status4;
    status4.characterName = "friend4";
    status4.displayName = "friend4";
    status4.friendedAs = "friendfour";
    status4.isOnline = true;
    status4.showOnlineStatus = true;
    status4.job = "WAR";
    status4.rank = "1";
    status4.nation = 3;  // Jeuno
    statuses.push_back(status4);
    
    vm.setShowNationRankColumn(true);
    vm.update(friendList, statuses, 1000000);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 4);
    
    REQUIRE(rows[0].nationRankText == "S 10");   // San d'Oria
    REQUIRE(rows[1].nationRankText == "B 75");   // Bastok
    REQUIRE(rows[2].nationRankText == "W 50");   // Windurst
    REQUIRE(rows[3].nationRankText == "J 1");    // Jeuno
}

TEST_CASE("FriendListViewModel - Nation+Rank combined column shows Hidden when data missing", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status;
    status.characterName = "friend1";
    status.displayName = "friend1";
    status.friendedAs = "friendone";
    status.isOnline = true;
    status.showOnlineStatus = true;
    status.job = "";
    status.rank = "";
    status.nation = -1;  // Hidden
    statuses.push_back(status);
    
    vm.setShowNationRankColumn(true);
    vm.update(friendList, statuses, 1000000);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].nationRankText == "Hidden");
}

TEST_CASE("FriendListViewModel - Nation+Rank combined column empty when column hidden", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status;
    status.characterName = "friend1";
    status.displayName = "friend1";
    status.friendedAs = "friendone";
    status.isOnline = true;
    status.showOnlineStatus = true;
    status.job = "WHM";
    status.rank = "10";
    status.nation = 0;  // San d'Oria
    statuses.push_back(status);
    
    vm.setShowNationRankColumn(false);
    vm.update(friendList, statuses, 1000000);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].nationRankText.empty());  // Should be empty when column is hidden
}

TEST_CASE("FriendListViewModel - Friended As value available in row data even when column hidden", "[UI][ViewModel]") {
    FriendListViewModel vm;
    FriendList friendList;
    friendList.addFriend("friend1", "FriendOne");
    
    std::vector<FriendStatus> statuses;
    FriendStatus status;
    status.characterName = "friend1";
    status.displayName = "friend1";
    status.friendedAs = "friendone";
    status.isOnline = true;
    status.showOnlineStatus = true;
    statuses.push_back(status);
    
    vm.setShowFriendedAsColumn(false);  // Column hidden
    vm.update(friendList, statuses, 1000000);
    
    const auto& rows = vm.getFriendRows();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].friendedAs == "friendone");  // Value still available in row data for context menu
}

