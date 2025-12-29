// Window manager implementation

#include "UI/Windows/WindowManager.h"


namespace XIFriendList {
namespace UI {

WindowManager::WindowManager()
    : viewModel_(nullptr)
    , commandHandler_(nullptr)
{
    // ViewModel will be set externally by AshitaAdapter
}

void WindowManager::setCommandHandler(IWindowCommandHandler* handler) {
    commandHandler_ = handler;
    if (handler != nullptr) {
        mainWindow_.setCommandHandler(handler);
        quickOnlineWindow_.setCommandHandler(handler);
        themesWindow_.setCommandHandler(handler);
        noteEditorWindow_.setCommandHandler(handler);
        serverSelectionWindow_.setCommandHandler(handler);
    }
}

void WindowManager::setViewModel(FriendListViewModel* viewModel) {
    viewModel_ = viewModel;
    if (viewModel_ != nullptr) {
        mainWindow_.setFriendListViewModel(viewModel_);
    }
}

void WindowManager::setQuickOnlineViewModel(FriendListViewModel* viewModel) {
    quickOnlineViewModel_ = viewModel;
    if (quickOnlineViewModel_ != nullptr) {
        quickOnlineWindow_.setViewModel(quickOnlineViewModel_);
    }
}

void WindowManager::setThemesViewModel(ThemesViewModel* viewModel) {
    if (viewModel != nullptr) {
        themesWindow_.setViewModel(viewModel);
    }
}

void WindowManager::setNotesViewModel(NotesViewModel* viewModel) {
    if (viewModel != nullptr) {
        noteEditorWindow_.setViewModel(viewModel);
    }
}

void WindowManager::setAltVisibilityViewModel(AltVisibilityViewModel* viewModel) {
    if (viewModel != nullptr) {
        mainWindow_.setAltVisibilityViewModel(viewModel);
    }
}

void WindowManager::setThemesViewModelForOptions(ThemesViewModel* viewModel) {
    if (viewModel != nullptr) {
        mainWindow_.setThemesViewModel(viewModel);
    }
}

void WindowManager::setOptionsViewModel(OptionsViewModel* viewModel) {
    if (viewModel != nullptr) {
        mainWindow_.setOptionsViewModel(viewModel);
    }
}

void WindowManager::setIconManager(void* iconManager) {
    if (iconManager != nullptr) {
        mainWindow_.setIconManager(iconManager);
        quickOnlineWindow_.setIconManager(iconManager);
        noteEditorWindow_.setIconManager(iconManager);
    }
}

void WindowManager::render() {
    if (quickOnlineWindow_.isVisible()) {
        quickOnlineWindow_.render();
    }

    // Main window (combines Friends List and Options)
    if (mainWindow_.isVisible()) {
        mainWindow_.render();
    }
    
    if (noteEditorWindow_.isVisible()) {
        noteEditorWindow_.render();
    }
    
    if (debugLogWindow_.isVisible()) {
        debugLogWindow_.render();
    }
    
    if (serverSelectionWindow_.isVisible()) {
        serverSelectionWindow_.render();
    }
}

bool WindowManager::hasAnyVisibleWindow() const {
    return quickOnlineWindow_.isVisible() ||
           mainWindow_.isVisible() ||
           noteEditorWindow_.isVisible() ||
           debugLogWindow_.isVisible() ||
           serverSelectionWindow_.isVisible();
}

void WindowManager::updateViewModel(const XIFriendList::Core::FriendList& friendList,
                                    const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                                    uint64_t currentTime) {
    if (viewModel_ != nullptr) {
        viewModel_->update(friendList, statuses, currentTime);
        // Note: Pending requests are updated separately via viewModel_->updatePendingRequests()
    }
}

} // namespace UI
} // namespace XIFriendList

