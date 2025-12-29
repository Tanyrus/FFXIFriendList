// Manages UI windows and coordinates with App layer

#ifndef UI_WINDOW_MANAGER_H
#define UI_WINDOW_MANAGER_H

#include "UI/Windows/MainWindow.h"
#include "UI/Windows/QuickOnlineWindow.h"
#include "UI/Windows/ThemesWindow.h"
#include "UI/Windows/NoteEditorWindow.h"
#include "UI/Windows/DebugLogWindow.h"
#include "UI/Windows/ServerSelectionWindow.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/ViewModels/FriendListViewModel.h"
#include "UI/ViewModels/ThemesViewModel.h"
#include "UI/ViewModels/NotesViewModel.h"
#include "UI/ViewModels/AltVisibilityViewModel.h"
#include "Core/FriendsCore.h"
#include <memory>
#include <vector>

namespace XIFriendList {
namespace UI {

// Window manager
// Coordinates windows, ViewModels, and App layer commands
class WindowManager {
public:
    WindowManager();
    ~WindowManager() = default;
    
    void setCommandHandler(IWindowCommandHandler* handler);
    
    void setViewModel(FriendListViewModel* viewModel);

    void setQuickOnlineViewModel(FriendListViewModel* viewModel);
    
    void setThemesViewModel(ThemesViewModel* viewModel);
    
    void setNotesViewModel(NotesViewModel* viewModel);
    
    void setOptionsViewModel(OptionsViewModel* viewModel);
    
    void setAltVisibilityViewModel(AltVisibilityViewModel* viewModel);
    void setThemesViewModelForOptions(ThemesViewModel* viewModel);
    
    void setIconManager(void* iconManager);
    
    // Render all windows
    void render();
    
    bool hasAnyVisibleWindow() const;
    
    MainWindow& getMainWindow() {
        return mainWindow_;
    }

    QuickOnlineWindow& getQuickOnlineWindow() {
        return quickOnlineWindow_;
    }
    
    ThemesWindow& getThemesWindow() {
        return themesWindow_;
    }
    
    NoteEditorWindow& getNoteEditorWindow() {
        return noteEditorWindow_;
    }
    
    DebugLogWindow& getDebugLogWindow() {
        return debugLogWindow_;
    }
    
    ServerSelectionWindow& getServerSelectionWindow() {
        return serverSelectionWindow_;
    }
    
    // currentTime: Current timestamp in milliseconds (for last seen calculations)
    void updateViewModel(const XIFriendList::Core::FriendList& friendList,
                        const std::vector<XIFriendList::Core::FriendStatus>& statuses,
                        uint64_t currentTime);

private:
    MainWindow mainWindow_;
    QuickOnlineWindow quickOnlineWindow_;
    ThemesWindow themesWindow_;
    NoteEditorWindow noteEditorWindow_;
    DebugLogWindow debugLogWindow_;
    ServerSelectionWindow serverSelectionWindow_;
    
    FriendListViewModel* viewModel_;  // Pointer to main FriendListViewModel owned by AshitaAdapter
    FriendListViewModel* quickOnlineViewModel_{ nullptr };  // Pointer owned by AshitaAdapter
    UI::IWindowCommandHandler* commandHandler_;
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_WINDOW_MANAGER_H

