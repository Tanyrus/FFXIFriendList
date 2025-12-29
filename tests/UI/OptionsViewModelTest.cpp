// OptionsViewModelTest.cpp
// Unit tests for OptionsViewModel

#include <catch2/catch_test_macros.hpp>
#include "UI/ViewModels/OptionsViewModel.h"
#include "Core/ModelsCore.h"

using namespace XIFriendList::UI;
using namespace XIFriendList::Core;

TEST_CASE("OptionsViewModel - Initial state", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    REQUIRE_FALSE(vm.getUseServerNotes());
    REQUIRE_FALSE(vm.getOverwriteNotesOnUpload());
    REQUIRE_FALSE(vm.getOverwriteNotesOnDownload());
    REQUIRE_FALSE(vm.getShareJobWhenAnonymous());
    REQUIRE(vm.getShowOnlineStatus());
    REQUIRE(vm.getShareLocation());
    REQUIRE(vm.getMainFriendView().showJob);
    REQUIRE_FALSE(vm.getMainFriendView().showZone);
    REQUIRE_FALSE(vm.getMainFriendView().showNationRank);
    REQUIRE_FALSE(vm.getMainFriendView().showLastSeen);
    REQUIRE_FALSE(vm.getDebugMode());
    REQUIRE_FALSE(vm.hasDirtyFields());
}

TEST_CASE("OptionsViewModel - Setter marks dirty", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    vm.setUseServerNotes(true);
    REQUIRE(vm.getUseServerNotes());
    REQUIRE(vm.isUseServerNotesDirty());
    REQUIRE(vm.hasDirtyFields());
    
    vm.clearDirtyFlags();
    REQUIRE_FALSE(vm.hasDirtyFields());
    REQUIRE_FALSE(vm.isUseServerNotesDirty());
}

TEST_CASE("OptionsViewModel - All preference setters", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    vm.setUseServerNotes(true);
    vm.setOverwriteNotesOnUpload(true);
    vm.setOverwriteNotesOnDownload(true);
    vm.setShareJobWhenAnonymous(true);
    vm.setShowOnlineStatus(false);
    vm.setShareLocation(false);
    FriendViewSettings mainView = vm.getMainFriendView();
    mainView.showJob = false;
    mainView.showZone = false;
    mainView.showNationRank = false;
    mainView.showLastSeen = false;
    vm.setMainFriendView(mainView);
    vm.setDebugMode(true);
    
    REQUIRE(vm.getUseServerNotes());
    REQUIRE(vm.getOverwriteNotesOnUpload());
    REQUIRE(vm.getOverwriteNotesOnDownload());
    REQUIRE(vm.getShareJobWhenAnonymous());
    REQUIRE_FALSE(vm.getShowOnlineStatus());
    REQUIRE_FALSE(vm.getShareLocation());
    REQUIRE_FALSE(vm.getMainFriendView().showJob);
    REQUIRE_FALSE(vm.getMainFriendView().showZone);
    REQUIRE_FALSE(vm.getMainFriendView().showNationRank);
    REQUIRE_FALSE(vm.getMainFriendView().showLastSeen);
    REQUIRE(vm.getDebugMode());
    REQUIRE(vm.hasDirtyFields());
}

TEST_CASE("OptionsViewModel - Update from preferences", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    Preferences prefs;
    prefs.useServerNotes = true;
        prefs.mainFriendView.showJob = false;
    prefs.debugMode = true;
    
    vm.updateFromPreferences(prefs);
    
    REQUIRE(vm.getUseServerNotes());
    REQUIRE_FALSE(vm.getMainFriendView().showJob);
    REQUIRE(vm.getDebugMode());
    REQUIRE_FALSE(vm.hasDirtyFields());  // Should clear dirty flags
}

TEST_CASE("OptionsViewModel - To preferences", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    vm.setUseServerNotes(true);
    FriendViewSettings mainView = vm.getMainFriendView();
    mainView.showJob = false;
    vm.setMainFriendView(mainView);
    vm.setDebugMode(true);
    
    Preferences prefs = vm.toPreferences();
    
    REQUIRE(prefs.useServerNotes);
    REQUIRE_FALSE(prefs.mainFriendView.showJob);
    REQUIRE(prefs.debugMode);
}

TEST_CASE("OptionsViewModel - Error handling", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    REQUIRE_FALSE(vm.hasError());
    REQUIRE(vm.getError().empty());
    
    vm.setError("Test error");
    REQUIRE(vm.hasError());
    REQUIRE(vm.getError() == "Test error");
    
    vm.clearError();
    REQUIRE_FALSE(vm.hasError());
    REQUIRE(vm.getError().empty());
}

TEST_CASE("OptionsViewModel - Main and Quick Online have separate settings", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    FriendViewSettings mainView = vm.getMainFriendView();
    mainView.showJob = false;
    mainView.showZone = true;
    vm.setMainFriendView(mainView);
    
    FriendViewSettings quickView = vm.getQuickOnlineFriendView();
    quickView.showJob = true;
    quickView.showZone = false;
    vm.setQuickOnlineFriendView(quickView);
    
    REQUIRE_FALSE(vm.getMainFriendView().showJob);
    REQUIRE(vm.getMainFriendView().showZone);
    REQUIRE(vm.getQuickOnlineFriendView().showJob);
    REQUIRE_FALSE(vm.getQuickOnlineFriendView().showZone);
}

TEST_CASE("OptionsViewModel - Column visibility settings affect both views independently", "[UI][ViewModel]") {
    OptionsViewModel vm;
    
    FriendViewSettings mainView = vm.getMainFriendView();
    mainView.showJob = true;
    mainView.showZone = true;
    mainView.showNationRank = false;
    mainView.showLastSeen = false;
    vm.setMainFriendView(mainView);
    
    FriendViewSettings quickView = vm.getQuickOnlineFriendView();
    quickView.showJob = false;
    quickView.showZone = false;
    quickView.showNationRank = true;
    quickView.showLastSeen = true;
    vm.setQuickOnlineFriendView(quickView);
    
    const auto& main = vm.getMainFriendView();
    const auto& quick = vm.getQuickOnlineFriendView();
    
    REQUIRE(main.showJob);
    REQUIRE(main.showZone);
    REQUIRE_FALSE(main.showNationRank);
    REQUIRE_FALSE(main.showLastSeen);
    
    REQUIRE_FALSE(quick.showJob);
    REQUIRE_FALSE(quick.showZone);
    REQUIRE(quick.showNationRank);
    REQUIRE(quick.showLastSeen);
}

