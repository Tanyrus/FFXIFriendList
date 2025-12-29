// Command interface for UI -> App layer communication

#ifndef UI_WINDOW_COMMANDS_H
#define UI_WINDOW_COMMANDS_H

#include "App/Theming/ThemeTokens.h"
#include "Core/ModelsCore.h"
#include <string>
#include <functional>
#include <optional>

namespace XIFriendList {
namespace UI {

// Command types that windows can emit to App layer
enum class WindowCommandType {
    // Connection (auto-connect handled by Platform layer, no manual commands)
    SyncFriendList,
    RefreshStatus,
    UpdatePresence,
    
    SendFriendRequest,
    AcceptFriendRequest,
    RejectFriendRequest,
    CancelFriendRequest,
    
    RemoveFriend,
    RemoveFriendVisibility,  // Remove friend from current character's view only (when visibility_mode='ONLY')
    
    // Notes
    OpenNoteEditor,
    SaveNote,
    DeleteNote,
    UploadNotes,
    DownloadNotes,
    
    // Friend Details
    ViewFriendDetails,  // Show friend details popup (data = friendName)
    
    // UI state
    OpenOptions,
    OpenThemes,
    ToggleColumnVisibility,
    
    ApplyTheme,
    SetCustomTheme,  // Set custom theme by name (data = theme name)
    UpdateThemeColors,  // Update theme colors immediately (for per-color changes)
    SetBackgroundAlpha,  // Update in memory (no save)
    SetTextAlpha,  // Update in memory (no save)
    SaveThemeAlpha,  // Save theme alpha to disk (debounced)
    SaveCustomTheme,
    DeleteCustomTheme,
    SetCustomThemeByName,  // Set custom theme by name (data = theme name)
    RefreshThemesList,  // Refresh themes list from disk (ensures all themes are visible)
    SetThemePreset,  // Set theme preset by name (data = preset name: "XIUI Default", "Classic", etc.)
    UpdateQuickOnlineThemeColors,  // Update Quick Online theme colors immediately (for per-color changes)
    UpdateNotificationThemeColors,  // Update Notification theme colors immediately (for per-color changes)
    
    LoadPreferences,  // Load preferences from server/local
    UpdatePreference,  // Update a single preference (data = JSON: {"field": "value"})
    SavePreferences,  // Save all preferences
    ResetPreferences,  // Reset all preferences to defaults
    StartCapturingCustomKey,  // Start capturing a custom key for closing windows
    UpdateWindowLock,  // Update per-window lock state (data = JSON: {"windowId": "windowName", "locked": true/false})
    
    // Debug window
    ToggleDebugWindow,  // Toggle debug log window visibility (data = "true" or "false")
    
    OpenAltVisibility,  // Open Alt Visibility window
    RefreshAltVisibility, // Refresh alt visibility data
    AddFriendVisibility,  // Add/request visibility for a friend (data = friendName)
    ToggleFriendVisibility,  // Toggle visibility (data = "friendAccountId|friendName|true/false")
    
    // Server Selection
    SaveServerSelection,  // Save selected server (data = serverId)
    RefreshServerList  // Refresh server list from server
};

// Command structure
struct WindowCommand {
    WindowCommandType type;
    std::string data;  // JSON or simple string data
    
    WindowCommand(WindowCommandType type, const std::string& data = "")
        : type(type), data(data) {}
};

// Command handler interface
// App layer will provide implementation
class IWindowCommandHandler {
public:
    virtual ~IWindowCommandHandler() = default;
    
    virtual void handleCommand(const WindowCommand& command) = 0;
    
    // Returns empty optional if no theme provider is available
    virtual std::optional<XIFriendList::App::Theming::ThemeTokens> getCurrentThemeTokens() const {
        return std::nullopt;  // Default implementation returns empty
    }
    
    virtual XIFriendList::Core::CustomTheme getQuickOnlineTheme() const {
        return XIFriendList::Core::CustomTheme();  // Default implementation returns empty theme
    }
    
    virtual XIFriendList::Core::CustomTheme getNotificationTheme() const {
        return XIFriendList::Core::CustomTheme();  // Default implementation returns empty theme
    }
    
    virtual void updateQuickOnlineThemeColors(const XIFriendList::Core::CustomTheme& colors) {
    }
    
    virtual void updateNotificationThemeColors(const XIFriendList::Core::CustomTheme& colors) {
    }
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_WINDOW_COMMANDS_H

