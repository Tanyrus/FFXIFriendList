// WindowClosePolicyTest.cpp
// Unit tests for WindowClosePolicy

#include <catch2/catch_test_macros.hpp>
#include "UI/Windows/WindowClosePolicy.h"
#include "UI/Windows/WindowManager.h"
#include <memory>

namespace XIFriendList {
namespace UI {

// Mock WindowManager for testing
class MockWindowManager : public WindowManager {
public:
    MockWindowManager() {
        // Set all windows to invisible by default
        getMainWindow().setVisible(false);
        getQuickOnlineWindow().setVisible(false);
        getNoteEditorWindow().setVisible(false);
    }
    
    // Helper methods to set window visibility for testing
    void setMainVisible(bool visible) { getMainWindow().setVisible(visible); }
    void setQuickOnlineVisible(bool visible) { getQuickOnlineWindow().setVisible(visible); }
    void setOptionsVisible(bool visible) { getMainWindow().setVisible(visible); }  // MainWindow now includes Options
    void setNoteEditorVisible(bool visible) { getNoteEditorWindow().setVisible(visible); }
};

TEST_CASE("WindowClosePolicy - anyWindowOpen returns false when no windows open", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    WindowClosePolicy policy(mockManager.get());
    
    REQUIRE_FALSE(policy.anyWindowOpen());
}

TEST_CASE("WindowClosePolicy - anyWindowOpen returns true when main window open", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    REQUIRE(policy.anyWindowOpen());
}

TEST_CASE("WindowClosePolicy - anyWindowOpen returns true when QuickOnline window open", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setQuickOnlineVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    REQUIRE(policy.anyWindowOpen());
}

TEST_CASE("WindowClosePolicy - closeTopMostWindow closes highest priority window first", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    mockManager->setNoteEditorVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    // NoteEditor should close first (highest priority)
    std::string closed = policy.closeTopMostWindow();
    REQUIRE(closed == "NoteEditor");
    REQUIRE_FALSE(mockManager->getNoteEditorWindow().isVisible());
    REQUIRE(mockManager->getMainWindow().isVisible());
}

TEST_CASE("WindowClosePolicy - closeTopMostWindow closes QuickOnline before FriendList", "[WindowClosePolicy]") {
    // Priority order: NoteEditor > QuickOnline > Main
    // So when both MainWindow and QuickOnline are visible, QuickOnline closes first
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    mockManager->setQuickOnlineVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    // QuickOnline closes first (higher priority than Main)
    REQUIRE(policy.closeTopMostWindow() == "QuickOnline");
    REQUIRE_FALSE(mockManager->getQuickOnlineWindow().isVisible());
    REQUIRE(mockManager->getMainWindow().isVisible());
    
    // Second ESC should close MainWindow (returns "FriendList")
    REQUIRE(policy.closeTopMostWindow() == "FriendList");
    REQUIRE_FALSE(mockManager->getMainWindow().isVisible());
}

TEST_CASE("WindowClosePolicy - closeTopMostWindow closes Main window", "[WindowClosePolicy]") {
    // MainWindow returns "FriendList" when closed
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    // MainWindow should close and return "FriendList"
    std::string closed = policy.closeTopMostWindow();
    REQUIRE(closed == "FriendList");
    REQUIRE_FALSE(mockManager->getMainWindow().isVisible());
}

TEST_CASE("WindowClosePolicy - closeTopMostWindow returns empty string when no windows open", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    WindowClosePolicy policy(mockManager.get());
    
    std::string closed = policy.closeTopMostWindow();
    REQUIRE(closed.empty());
}

TEST_CASE("WindowClosePolicy - closeAllWindows closes all windows", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    mockManager->setQuickOnlineVisible(true);
    mockManager->setOptionsVisible(true);
    mockManager->setNoteEditorVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    policy.closeAllWindows();
    
    REQUIRE_FALSE(mockManager->getMainWindow().isVisible());
    REQUIRE_FALSE(mockManager->getQuickOnlineWindow().isVisible());
    REQUIRE_FALSE(mockManager->getNoteEditorWindow().isVisible());
}

TEST_CASE("WindowClosePolicy - getTopMostWindowName returns correct window name", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setQuickOnlineVisible(true);
    mockManager->setNoteEditorVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    REQUIRE(policy.getTopMostWindowName() == "NoteEditor");
    
    // Close NoteEditor
    policy.closeTopMostWindow();
    REQUIRE(policy.getTopMostWindowName() == "QuickOnline");
}

TEST_CASE("WindowClosePolicy - priority order: NoteEditor > QuickOnline > Main", "[WindowClosePolicy]") {
    auto mockManager = std::make_unique<MockWindowManager>();
    mockManager->setMainVisible(true);
    mockManager->setQuickOnlineVisible(true);
    mockManager->setNoteEditorVisible(true);
    WindowClosePolicy policy(mockManager.get());
    
    // Close in priority order: NoteEditor > QuickOnline > Main
    REQUIRE(policy.closeTopMostWindow() == "NoteEditor");
    REQUIRE(policy.closeTopMostWindow() == "QuickOnline");
    REQUIRE(policy.closeTopMostWindow() == "FriendList");
    // All windows closed, should return empty
    REQUIRE(policy.closeTopMostWindow().empty());
}

} // namespace UI
} // namespace XIFriendList
