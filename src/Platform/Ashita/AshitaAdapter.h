// Adapter between Ashita callbacks and App/UI layers

#ifndef PLATFORM_ASHITA_ADAPTER_H
#define PLATFORM_ASHITA_ADAPTER_H

#include "PluginVersion.h"
#include "App/UseCases/ConnectionUseCases.h"
#include "App/UseCases/FriendsUseCases.h"
#include "App/UseCases/ThemingUseCases.h"
#include "App/UseCases/PreferencesUseCases.h"
#include "App/UseCases/NotificationUseCases.h"
#ifndef DISABLE_NOTES
#include "App/UseCases/NotesUseCases.h"
#endif
#include "App/UseCases/TestRunnerUseCase.h"
#include "App/Events/AppEvents.h"
#include "App/Interfaces/IPreferencesStore.h"
#include "Platform/Ashita/AshitaEventQueue.h"
#include "UI/Windows/WindowManager.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/ViewModels/FriendListViewModel.h"
#include "UI/ViewModels/ThemesViewModel.h"
#include "UI/ViewModels/OptionsViewModel.h"
#include "UI/ViewModels/NotesViewModel.h"
#include "UI/ViewModels/AltVisibilityViewModel.h"
#include "Platform/Ashita/AshitaNetClient.h"
#include "Platform/Ashita/AshitaClock.h"
#include "Platform/Ashita/AshitaLogger.h"
#include "Platform/Ashita/ApiKeyPersistence.h"
#include "App/State/ApiKeyState.h"
#include "Platform/Ashita/ThemePersistence.h"
#include "Platform/Ashita/ServerSelectionPersistence.h"
#include "Platform/Ashita/CacheMigration.h"
#include "Platform/Ashita/AshitaRealmDetector.h"
#include "App/State/ServerSelectionState.h"
#include "App/ServerSelectionGate.h"
#include "App/UseCases/ServerListUseCases.h"
#include "Core/ServerListCore.h"
#include "App/State/ThemeState.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/NotesPersistence.h"
#include "App/State/NotesState.h"
#include "Platform/Ashita/AshitaUiRenderer.h"
#include "Platform/Ashita/AshitaSoundPlayer.h"
#include "App/NotificationSoundService.h"
#include "Platform/Ashita/FriendListMenuDetector.h"
#include "Platform/Ashita/KeyEdgeDetector.h"
#include "UI/Windows/WindowClosePolicy.h"
#include "Core/FriendsCore.h"
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <set>
#include <map>
#include <atomic>

// Forward declarations - actual Ashita types included in .cpp only
struct IAshitaCore;
struct ILogManager;
struct IGuiManager;
struct IChatManager;
struct IDirect3DDevice8;  // Forward declaration for D3D8 device


// Forward declaration for IconManager (in XIFriendList::Platform::Ashita namespace)
namespace XIFriendList {
namespace Platform {
namespace Ashita {
    class IconManager;
}
}
}
// Note: ImGuiStyle is NOT forward-declared here - all ImGui types are hidden in .cpp implementation

namespace XIFriendList {
namespace Platform {
namespace Ashita {

// Adapter that wires Ashita callbacks to App/UI layers
// This is the ONLY place that should include Ashita headers
class AshitaAdapter : public ::XIFriendList::UI::IWindowCommandHandler {
public:
    AshitaAdapter();
    ~AshitaAdapter();
    
    // Initialize adapter with Ashita interfaces
    // Called from plugin Initialize() callback
    bool initialize(IAshitaCore* core, ILogManager* logger, uint32_t pluginId);
    
    // Release resources
    // Called from plugin Release() callback
    void release();
    
    // Render callback
    // Called from plugin Direct3D render callback
    void render();
    
    // Called from plugin update/tick callback
    void update();
    
    void toggleWindow();

    // Open Quick Online window (used by in-game menu integration)
    void openQuickOnlineWindow();
    void closeQuickOnlineWindow();
    
    void toggleDebugWindow();
    
    bool isWindowVisible() const;

    bool isQuickOnlineWindowVisible() const;

    
    // Trigger refresh when window opens (called from /fl command)
    void triggerRefreshOnOpen();
    
    void sendFriendRequestFromCommand(const std::string& friendName);
    
    // Trigger a test notification (called from /fl notify command)
    void triggerTestNotification();
    
    // Test runner commands (called from /fl test commands)
    void handleTestList();
    void handleTestRun(const std::string& scenarioId = "");
    void handleTestReset();
    
    // Test mode: pause/resume background work
    // Called by TestRunGuard to prevent background workers from interfering with tests
    void pauseBackgroundForTests();
    void resumeBackgroundAfterTests();
    bool waitForIdleForTests(uint64_t timeoutMs = 2000);
    int getActiveJobsCount() const { return activeJobs_.load(); }
    bool isBackgroundPausedForTests() const { return backgroundPausedForTests_.load(); }
    std::string getServerBaseUrl() const { return netClient_ ? netClient_->getBaseUrl() : ""; }
    
    void handleZoneChangePacket();
    
    // Initialize IconManager with D3D8 device (loads icons from embedded resources)
    // Called from XIFriendList::Direct3DInitialize
    void initializeIconManager(IDirect3DDevice8* device);
    
    // Implements IWindowCommandHandler interface
    void handleCommand(const ::XIFriendList::UI::WindowCommand& command) override;
    std::optional<::XIFriendList::App::Theming::ThemeTokens> getCurrentThemeTokens() const override;
    ::XIFriendList::Core::CustomTheme getQuickOnlineTheme() const override;
    ::XIFriendList::Core::CustomTheme getNotificationTheme() const override;
    void updateQuickOnlineThemeColors(const ::XIFriendList::Core::CustomTheme& colors) override;
    void updateNotificationThemeColors(const ::XIFriendList::Core::CustomTheme& colors) override;
    
    const char* getName() const { return "XI FriendList"; }
    const char* getAuthor() const { return "Tanyrus"; }
    const char* getDescription() const { return "A Friendlist Management Plugin"; }
    
    void printMemoryStats();
    ::XIFriendList::Core::MemoryStats getAdapterCacheStats() const;
    double getVersion() const { return Plugin::PLUGIN_VERSION; }
    std::string getVersionString() const;
    uint32_t getFlags() const;
    

private:
    // Helper: show error notification AND log it to debug window
    void showErrorNotification(const std::string& message, const std::string& context = "");
    
    // Query game state from Ashita
    ::XIFriendList::Core::Presence queryPlayerPresence() const;
    
    std::string getCharacterName() const;
    
    void updatePresenceAsync();
    
    
    void presenceHeartbeatTick();  // Lightweight presence update (~20s)
    void fullRefreshTick();  // Full refresh: friend list, status, requests (~60s)
    void playerDataChangeDetectionTick();  // Local player data change detection (~5s)
    
    // Auto-connect when character name becomes available (async, runs in worker thread)
    void attemptAutoConnectAsync();
    
    // Internal: perform auto-connect in worker thread
    void attemptAutoConnectWorker(const std::string& characterName);
    
    void handleSyncFriendListAsync();
    
    void handleRefreshStatus();
    
    void handleCharacterChangedEvent(const ::XIFriendList::App::Events::CharacterChanged& event);
    void handleZoneChangedEvent(const ::XIFriendList::App::Events::ZoneChanged& event);
    
    void handleSendFriendRequest(const std::string& friendName);
    void handleAcceptFriendRequest(const std::string& requestId);
    void handleRejectFriendRequest(const std::string& requestId);
    void handleCancelFriendRequest(const std::string& requestId);
    void handleGetFriendRequests();
    
    void handleRemoveFriend(const std::string& friendName);
    void handleRemoveFriendVisibility(const std::string& friendName);
    void handleRefreshAltVisibility();
    void handleAddFriendVisibility(const std::string& friendName);
    void handleToggleFriendVisibility(const std::string& commandData);  // Format: "friendAccountId|friendName|true/false"
    
    void handleApplyTheme(int themeIndex);
    void handleSetCustomTheme(const std::string& themeName);
    void handleSetThemePreset(const std::string& presetName);
    void handleUpdateThemeColors();  // Update theme colors immediately (for per-color changes)
    void handleSetBackgroundAlpha(float alpha);  // Update in memory (no save)
    void handleSetTextAlpha(float alpha);  // Update in memory (no save)
    void handleSaveThemeAlpha();  // Save theme alpha to disk (debounced)
    void handleSaveCustomTheme(const std::string& themeName);
    void handleDeleteCustomTheme(const std::string& themeName);
    void handleSetCustomThemeByName(const std::string& themeName);
    void pushThemeStyles();  // Push theme styles to prevent affecting other addons
    void popThemeStyles();    // Pop theme styles to restore original style
    void saveCurrentImGuiStyle();  // Save current ImGui style before applying theme
    void restoreImGuiStyle();      // Restore saved ImGui style after rendering
    void updateThemesViewModel();
    
    void handleLoadPreferences();
    void handleUpdatePreference(const std::string& valueJson);
    void handleUpdateWindowLock(const std::string& valueJson);
    void handleSavePreferences();
    void handleResetPreferences();
    void updateOptionsViewModel();
    void updateFriendListViewModelsFromPreferences();
    void handleToggleColumnVisibility(const std::string& jsonData);

    void startPreferencesSyncFromServerAsync(const std::string& apiKey, const std::string& characterName);
    
    // Automatic settings save (debounced)
    void scheduleAutoSave();
    void performAutoSave();
    
    void scheduleStatusUpdate(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous);
    void performStatusUpdate();
    void performStatusUpdateImmediate(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous);
    
    
    void handleSaveNote(const std::string& friendName);
    void handleDeleteNote(const std::string& friendName);
    void updateNotesViewModel();
    
    // Server selection
    void checkServerSelectionGate();
    void showServerSelectionWindow();
    void handleSaveServerSelection(const std::string& serverId);
    void handleRefreshServerList();
    void detectServerFromRealm();
    bool shouldBlockNetworkOperation() const;
    void rerouteToServerSelectionIfNeeded();
    
    ::XIFriendList::Core::ServerList serverList_;  // Cached server list
    
    IAshitaCore* ashitaCore_;
    ILogManager* logManager_;
    IGuiManager* guiManager_;
    IChatManager* chatManager_; 
    uint32_t pluginId_;
    
    // Platform implementations
    std::unique_ptr<AshitaNetClient> netClient_;
    std::unique_ptr<AshitaClock> clock_;
    std::unique_ptr<AshitaLogger> logger_;
    ::XIFriendList::App::State::ApiKeyState apiKeyState_;  // API key state (replaces IApiKeyStore interface)
    ::XIFriendList::App::State::ThemeState themeState_;  // Theme state
    ::XIFriendList::App::State::ServerSelectionState serverSelectionState_;  // Server selection state
    std::unique_ptr<AshitaPreferencesStore> preferencesStore_;
    ::XIFriendList::App::State::NotesState notesState_;  // Notes state (replaces INotesStore interface)
    std::unique_ptr<AshitaUiRenderer> uiRenderer_;
    std::unique_ptr<AshitaSoundPlayer> soundPlayer_;
    std::unique_ptr<::XIFriendList::App::NotificationSoundService> soundService_;
    
    // App layer use cases
    std::unique_ptr<::XIFriendList::App::UseCases::ConnectUseCase> connectUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::SyncFriendListUseCase> syncUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::UpdatePresenceUseCase> presenceUseCase_;  // For parsing statuses from responses
    std::unique_ptr<::XIFriendList::App::UseCases::UpdateMyStatusUseCase> updateMyStatusUseCase_;  // C1: Status flags sync
    std::unique_ptr<::XIFriendList::App::UseCases::SendFriendRequestUseCase> sendRequestUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::AcceptFriendRequestUseCase> acceptRequestUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::RejectFriendRequestUseCase> rejectRequestUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::CancelFriendRequestUseCase> cancelRequestUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::GetFriendRequestsUseCase> getRequestsUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::RemoveFriendUseCase> removeFriendUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::RemoveFriendVisibilityUseCase> removeFriendVisibilityUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::GetAltVisibilityUseCase> getAltVisibilityUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::ThemeUseCase> themeUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::PreferencesUseCase> preferencesUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::NotificationUseCase> notificationUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::GetNotesUseCase> getNotesUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::SaveNoteUseCase> saveNoteUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::DeleteNoteUseCase> deleteNoteUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::HandleCharacterChangedUseCase> handleCharacterChangedUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::HandleZoneChangedUseCase> handleZoneChangedUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::TestRunnerUseCase> testRunnerUseCase_;
    std::unique_ptr<::XIFriendList::App::UseCases::FetchServerListUseCase> fetchServerListUseCase_;
    
    // Event queue for character/zone change events
    std::unique_ptr<AshitaEventQueue> eventQueue_;
    
    // UI layer
    std::unique_ptr<::XIFriendList::UI::WindowManager> windowManager_;
    std::unique_ptr<::XIFriendList::UI::FriendListViewModel> viewModel_;  // Owned by WindowManager but we keep reference
    std::unique_ptr<::XIFriendList::UI::FriendListViewModel> quickOnlineViewModel_;  // Online-only list, separate column prefs
    std::unique_ptr<::XIFriendList::UI::ThemesViewModel> themesViewModel_;
    std::unique_ptr<::XIFriendList::UI::OptionsViewModel> optionsViewModel_;
    std::unique_ptr<::XIFriendList::UI::NotesViewModel> notesViewModel_;
    std::unique_ptr<::XIFriendList::UI::AltVisibilityViewModel> altVisibilityViewModel_;
    std::unique_ptr<::XIFriendList::Platform::Ashita::IconManager> iconManager_;
    
    std::unique_ptr<FriendListMenuDetector> friendListMenuDetector_;
    
    // Customizable close key handling (defaults to ESC)
    std::unique_ptr<KeyEdgeDetector> escKeyDetector_;
    std::unique_ptr<KeyEdgeDetector> backspaceKeyDetector_;
    std::unique_ptr<KeyEdgeDetector> customKeyDetector_;
    std::unique_ptr<::XIFriendList::UI::WindowClosePolicy> windowClosePolicy_;
    
    // Key capture state for custom key selection
    bool capturingCustomKey_;
    int capturedKeyCode_;
    
    // Cached preferences for notification view model (avoids dangling pointer)
    ::XIFriendList::Core::Preferences lastPreferences_;
    
    void handleEscapeKey();
    
    // Start capturing a custom key (called from UI)
    void startCapturingCustomKey();
    
    void processKeyCapture();
    
    // buttonCode: XInput button code (0x2000 = B button, 0 = disabled)
    bool checkControllerInput(int buttonCode);
    
    bool isGameMenuOpen();
    
    // State
    std::string apiKey_;
    std::string characterName_;
    int accountId_;  // Current account ID (0 = not set)
    std::string lastDetectedCharacterName_;  // Track character name changes
    bool initialized_;
    uint32_t initializationTime_{ 0 };  // Timestamp when initialization completed (for notification cooldown)
    static constexpr uint32_t NOTIFICATION_RENDERING_COOLDOWN_MS = 500;  // Skip notification rendering for 500ms after init
    bool deferredInitPending_;  // Deferred initialization flag (load themes/prefs after first render)
    bool autoConnectAttempted_;  // Track if auto-connect was attempted for current character
    bool autoConnectInProgress_;  // Track if auto-connect is currently running in worker thread
    bool autoConnectCompleted_;  // Track if auto-connect completed (main thread processes result)
    ::XIFriendList::App::UseCases::ConnectResult pendingConnectResult_;  // Result from worker thread (processed by main thread)

    bool preferencesSyncInProgress_{ false };
    bool preferencesSyncCompleted_{ false };
    ::XIFriendList::App::UseCases::PreferencesResult pendingPreferencesSyncResult_{};

    bool friendRequestsSyncInProgress_{ false };
    bool friendRequestsSyncCompleted_{ false };
    ::XIFriendList::App::UseCases::GetFriendRequestsResult pendingFriendRequestsResult_{};

    // Pending chat echo error (set by worker thread, processed by main thread for thread-safe chat output)
    std::string pendingChatEchoError_;

    bool characterChangedInProgress_{ false };
    bool characterChangedCompleted_{ false };
    ::XIFriendList::App::Events::CharacterChanged pendingCharacterChangedEvent_{ "", "", 0 };
    ::XIFriendList::App::UseCases::CharacterChangeResult pendingCharacterChangedResult_{};
    static constexpr uint64_t POLL_INTERVAL_PRESENCE_MS = 20000;  // 20 seconds - HEARTBEAT (alive only; no full player data)
    static constexpr uint64_t POLL_INTERVAL_REFRESH_MS = 60000;  // 60 seconds - full data refresh
    static constexpr uint64_t POLL_INTERVAL_PLAYER_DATA_CHECK_MS = 5000;  // 5 seconds - local player data change detection
    
    uint64_t lastPresenceUpdate_;
    uint64_t lastFullRefresh_;
    uint64_t lastPlayerDataCheck_;
    uint64_t initializationTimeMs_;  // Timestamp when initialization completed (for update check delay)

    // Heartbeat event cursors (server returns monotonic timestamps)
    uint64_t lastHeartbeatEventTimestamp_;
    uint64_t lastHeartbeatRequestEventTimestamp_;
    bool hasReportedVersion_;
    
    bool presenceUpdateInFlight_; // Heartbeat in-flight (kept name for minimal churn)
    bool fullRefreshInFlight_;
    bool friendListSyncInFlight_; // Friend list sync in-flight (prevents duplicate /api/friends calls)
    mutable std::mutex pollingMutex_;  // Protects in-flight flags and timestamps
    
    uint64_t friendListSyncRequestId_;  // Monotonic counter for request IDs
    std::string lastFriendListSyncCallsite_;  // Last callsite that triggered sync
    uint64_t lastFriendListSyncTimestamp_;  // Timestamp of last sync trigger

    // Cached data for heartbeat-driven UI refresh (heartbeat returns statuses, not friend list)
    ::XIFriendList::Core::FriendList cachedFriendList_;
    std::vector<::XIFriendList::Protocol::FriendRequestPayload> cachedOutgoingRequests_;
    std::vector<::XIFriendList::Protocol::FriendRequestPayload> cachedIncomingRequests_;
    std::vector<::XIFriendList::Core::FriendStatus> cachedFriendStatuses_;
    
    // Automatic settings save debouncing (max 2 seconds as per requirement)
    uint64_t lastPreferenceChangeTime_;
    static constexpr uint64_t PREFERENCES_AUTO_SAVE_DELAY_MS = 2000;  // 5 seconds max debounce
    std::unique_ptr<std::thread> autoSaveThread_;
    std::mutex autoSaveMutex_;
    bool autoSavePending_;
    bool autoSaveThreadShouldExit_;
    
    uint64_t lastStatusChangeTime_;
    static constexpr uint64_t STATUS_UPDATE_DELAY_MS = 0;  // Immediate status flags sync (privacy/visibility requires immediate POST)
    std::unique_ptr<std::thread> statusUpdateThread_;
    std::mutex statusUpdateMutex_;
    bool statusUpdatePending_;
    bool statusUpdateThreadShouldExit_;
    bool pendingShowOnlineStatus_;
    bool pendingShareLocation_;
    bool pendingIsAnonymous_;
    bool pendingShareJobWhenAnonymous_;
    bool hasPendingStatusUpdate_;
    
    // Mutex for thread-safe state updates from worker threads
    mutable std::mutex stateMutex_;
    
    // Zone cache (for efficient zone name lookups)
    mutable std::mutex zoneCacheMutex_;
    mutable uint16_t cachedZoneId_;
    mutable std::string cachedZoneName_;
    
    bool initialStatusScanComplete_;
    std::map<std::string, bool> previousOnlineStatus_;  // friend name (lowercase) -> was online
    mutable std::mutex statusChangeMutex_;
    
    // Track processed friend request IDs to avoid duplicate notifications
    std::set<std::string> processedRequestIds_;
    mutable std::mutex processedRequestIdsMutex_;
    

    // Local disk cache warmup state.
    // We use these to prevent expensive disk reads/parses (notes cache)
    // from running on the render thread, and to gate operations until warmup completes.
    std::atomic<bool> localCacheWarmupInProgress_{ false };
    std::atomic<bool> localCacheWarmupCompleted_{ true };

    // Debug: log a single status field dump to confirm decoding/mapping (guarded by preferences.debugMode).
    std::atomic<bool> statusFieldDumpLogged_{ false };

    // Debug gating:
    // - Enabled in Debug builds
    // - Enabled when local config has debugMode=true
    std::atomic<bool> debugEnabled_{ false };
    
    // Test mode: background work control
    std::atomic<bool> backgroundPausedForTests_{ false };
    std::atomic<int> activeJobs_{ 0 };  // Track active async jobs for idle detection
    std::mutex idleWaitMutex_;  // Protects idle condition variable
    std::condition_variable idleWaitCondition_;  // Signaled when activeJobs_ changes
    
    // ImGui default style storage (PIMPL - instance-owned, type hidden in .cpp)
    struct ImGuiStyleStorage;
    std::unique_ptr<ImGuiStyleStorage> defaultStyleStorage_;
    
    // ImGui style management (implementation in .cpp only)
    void storeDefaultImGuiStyle();
    void resetImGuiStyleToDefaults();
    
    // Helper methods for formatting game data
    std::string formatJobString(uint8_t mainJob, uint8_t mainJobLevel, uint8_t subJob, uint8_t subJobLevel) const;
    std::string getZoneNameFromId(uint16_t zoneId) const;
    
    bool isDebugEnabled() const;
    void syncDebugEnabledFromPreferences();

    void pushDebugLog(const std::string& message);

    // Push message to in-game chat.
    // This should only be used for intentional, user-facing chat messages (not debug spam).
    void pushToGameEcho(const std::string& message);
    
    // Called after friend list/status updates
    void checkForStatusChanges(const std::vector<::XIFriendList::Core::FriendStatus>& currentStatuses);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_ADAPTER_H

