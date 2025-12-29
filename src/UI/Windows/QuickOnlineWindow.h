// Quick Online friend list window (online-only, full table system with minimal default columns).

#ifndef UI_QUICK_ONLINE_WINDOW_H
#define UI_QUICK_ONLINE_WINDOW_H

#include "UI/ViewModels/FriendListViewModel.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/Widgets/Tables.h"
#include "Core/MemoryStats.h"

#include <string>
#include <vector>

namespace XIFriendList {
namespace UI {

class QuickOnlineWindow {
public:
    QuickOnlineWindow();
    ~QuickOnlineWindow() = default;

    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
        friendTable_.setCommandHandler(handler);
    }

    void setViewModel(FriendListViewModel* viewModel) {
        viewModel_ = viewModel;
        friendTable_.setViewModel(viewModel);
    }

    void setIconManager(void* iconManager) {
        iconManager_ = iconManager;
        friendTable_.setIconManager(iconManager);
    }

    void render();

    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    const std::string& getTitle() const { return title_; }
    
    void setShareFriendsAcrossAlts(bool enabled) {
        friendTable_.setShareFriendsAcrossAlts(enabled);
    }
    
    void setFriendViewSettings(const XIFriendList::Core::FriendViewSettings& settings) {
        friendTable_.setViewSettings(settings);
    }
    
    void setSelectedFriendForDetails(const std::string& friendName) {
        selectedFriendForDetails_ = friendName;
    }
    
    const std::string& getSelectedFriendForDetails() const {
        return selectedFriendForDetails_;
    }
    
    void clearSelectedFriendForDetails() {
        selectedFriendForDetails_.clear();
    }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    void renderTopBar();
    void emitCommand(WindowCommandType type, const std::string& data = "");

    FriendListViewModel* viewModel_{ nullptr };
    IWindowCommandHandler* commandHandler_{ nullptr };
    void* iconManager_{ nullptr };

    bool visible_{ false };
    std::string title_{ "Quick Online" };
    std::string windowId_{ "QuickOnline" };
    bool locked_{ false };
    bool pendingClose_{ false };  // Close requested while a popup/menu is open; defer until clean.

    FriendTableWidget friendTable_;
    std::string selectedFriendForDetails_;  // Friend name for details popup
    
    void renderFriendDetailsPopup();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_QUICK_ONLINE_WINDOW_H


