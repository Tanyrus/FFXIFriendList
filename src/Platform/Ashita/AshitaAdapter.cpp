#include "Platform/Ashita/AshitaAdapter.h"
#include "Platform/Ashita/PathUtils.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Platform/Ashita/FriendListMenuDetector.h"
#include "Platform/Ashita/KeyEdgeDetector.h"
#include "UI/Windows/WindowClosePolicy.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Core/FriendsCore.h"
#include "Core/ModelsCore.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Protocol/JsonUtils.h"
#include "PluginVersion.h"
#include "App/Interfaces/ILogger.h"
#include "App/Interfaces/IClock.h"
#include "UI/Notifications/ToastManager.h"
#include "App/NotificationConstants.h"
#include "Debug/DebugLog.h"
#include "Debug/Perf.h"
#include "Platform/Ashita/IconManager.h"
#include "Core/MemoryStats.h"
#ifndef BUILDING_TESTS
#include <windows.h>
#endif
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <map>
#include <set>
#include <thread>
#include <memory>
#include <optional>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <filesystem>

class JobScope {
public:
    JobScope(std::atomic<int>& activeJobs, std::mutex& mutex, std::condition_variable& condition)
        : activeJobs_(activeJobs), mutex_(mutex), condition_(condition) {
        activeJobs_.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            condition_.notify_all();
        }
    }
    
    ~JobScope() {
        activeJobs_.fetch_sub(1);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            condition_.notify_all();
        }
    }
    
private:
    std::atomic<int>& activeJobs_;
    std::mutex& mutex_;
    std::condition_variable& condition_;
};

class TestRunGuard {
public:
    TestRunGuard(::XIFriendList::Platform::Ashita::AshitaAdapter& adapter, ::XIFriendList::App::ILogger& logger, ::XIFriendList::App::IClock& clock)
        : adapter_(adapter), logger_(logger), clock_(clock) {
        logger_.info("TestRunGuard: Pausing background work for tests");
        adapter_.pauseBackgroundForTests();
        
        uint64_t startTime = clock_.nowMs();
        bool idle = adapter_.waitForIdleForTests(2000);
        uint64_t elapsed = clock_.nowMs() - startTime;
        
        if (idle) {
            logger_.info("TestRunGuard: Background work is idle (waited " + std::to_string(elapsed) + "ms)");
        } else {
            int activeJobs = adapter_.getActiveJobsCount();
            logger_.warning("TestRunGuard: Background work did not become idle within timeout. Active jobs: " + 
                          std::to_string(activeJobs) + " (waited " + std::to_string(elapsed) + "ms)");
        }
        
        std::string baseUrl = adapter_.getServerBaseUrl();
        bool isPaused = adapter_.isBackgroundPausedForTests();
        int activeJobs = adapter_.getActiveJobsCount();
        
        logger_.info("TestRunGuard: Starting tests - server: " + baseUrl + 
                    ", backgroundPausedForTests: " + std::string(isPaused ? "true" : "false") +
                    ", activeJobs: " + std::to_string(activeJobs));
        
        if (!isPaused) {
            logger_.error("TestRunGuard: CRITICAL - Background work is NOT paused! Tests may crash.");
        }
    }
    
    ~TestRunGuard() {
        logger_.info("TestRunGuard: Resuming background work after tests");
        adapter_.resumeBackgroundAfterTests();
        
        logger_.info("TestRunGuard: Tests completed - activeJobs: " + std::to_string(adapter_.getActiveJobsCount()));
    }
    
private:
    ::XIFriendList::Platform::Ashita::AshitaAdapter& adapter_;
    ::XIFriendList::App::ILogger& logger_;
    ::XIFriendList::App::IClock& clock_;
};

#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

#ifndef BUILDING_TESTS
struct AshitaAdapter::ImGuiStyleStorage {
    std::optional<ImGuiStyle> defaultStyle;
    std::optional<ImGuiStyle> savedStyle;
    
    ImGuiStyleStorage() = default;
    ~ImGuiStyleStorage() = default;
    ImGuiStyleStorage(const ImGuiStyleStorage&) = delete;
    ImGuiStyleStorage& operator=(const ImGuiStyleStorage&) = delete;
    ImGuiStyleStorage(ImGuiStyleStorage&&) = delete;
    ImGuiStyleStorage& operator=(ImGuiStyleStorage&&) = delete;
};
#else
struct AshitaAdapter::ImGuiStyleStorage {
    ImGuiStyleStorage() = default;
    ~ImGuiStyleStorage() = default;
    ImGuiStyleStorage(const ImGuiStyleStorage&) = delete;
    ImGuiStyleStorage& operator=(const ImGuiStyleStorage&) = delete;
    ImGuiStyleStorage(ImGuiStyleStorage&&) = delete;
    ImGuiStyleStorage& operator=(ImGuiStyleStorage&&) = delete;
};
#endif

AshitaAdapter::AshitaAdapter()
    : ashitaCore_(nullptr)
    , logManager_(nullptr)
    , guiManager_(nullptr)
    , chatManager_(nullptr)
    , pluginId_(0)
    , initialized_(false)
    , autoConnectAttempted_(false)
    , autoConnectInProgress_(false)
    , autoConnectCompleted_(false)
    , lastPresenceUpdate_(0)
    , lastFullRefresh_(0)
    , lastPlayerDataCheck_(0)
    , initializationTimeMs_(0)
    , lastHeartbeatEventTimestamp_(0)
    , lastHeartbeatRequestEventTimestamp_(0)
    , hasReportedVersion_(false)
    , presenceUpdateInFlight_(false)
    , fullRefreshInFlight_(false)
    , friendListSyncInFlight_(false)
    , friendListSyncRequestId_(0)
    , cachedZoneId_(0)
    , lastPreferenceChangeTime_(0)
    , autoSavePending_(false)
    , autoSaveThreadShouldExit_(false)
    , lastStatusChangeTime_(0)  // C1: Status update debouncing
    , statusUpdatePending_(false)
    , statusUpdateThreadShouldExit_(false)
    , pendingShowOnlineStatus_(false)
    , pendingShareLocation_(false)
    , pendingIsAnonymous_(false)
    , pendingShareJobWhenAnonymous_(false)
    , accountId_(0)
    , hasPendingStatusUpdate_(false)
    , initialStatusScanComplete_(false)
    , deferredInitPending_(false)
    , defaultStyleStorage_(std::make_unique<ImGuiStyleStorage>())
{
    netClient_ = std::make_unique<AshitaNetClient>();
    clock_ = std::make_unique<AshitaClock>();
    logger_ = std::make_unique<AshitaLogger>();
    
    std::string mainJsonPath = PathUtils::getDefaultMainJsonPath();
    if (mainJsonPath.empty()) {
        char dllPath[MAX_PATH] = {0};
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    mainJsonPath = gameDir + "config\\FFXIFriendList\\ffxifriendlist.json";
                }
            }
        }
        if (mainJsonPath.empty()) {
            mainJsonPath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.json";
        }
    }
    
    std::string cacheJsonPath = PathUtils::getDefaultCachePath();
    if (cacheJsonPath.empty()) {
        char dllPath[MAX_PATH] = {0};
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    cacheJsonPath = gameDir + "config\\FFXIFriendList\\cache.json";
                }
            }
        }
        if (cacheJsonPath.empty()) {
            cacheJsonPath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\cache.json";
        }
    }
    
    std::string iniPath = PathUtils::getDefaultIniPath();
    if (iniPath.empty()) {
        char dllPath[MAX_PATH] = {0};
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    iniPath = gameDir + "config\\FFXIFriendList\\ffxifriendlist.ini";
                }
            }
        }
        if (iniPath.empty()) {
            iniPath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.ini";
        }
    }
    
    std::string settingsJsonPath = PathUtils::getDefaultConfigPath("settings.json");
    if (settingsJsonPath.empty()) {
        char dllPath[MAX_PATH] = {0};
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (hModule != nullptr && GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    settingsJsonPath = gameDir + "config\\FFXIFriendList\\settings.json";
                }
            }
        }
        if (settingsJsonPath.empty()) {
            settingsJsonPath = "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\settings.json";
        }
    }
    
    std::filesystem::path oldConfigDir;
    std::filesystem::path newConfigDir;
    char dllPathForMigration[MAX_PATH] = {0};
    HMODULE hModuleForMigration = GetModuleHandleA(nullptr);
    if (hModuleForMigration != nullptr && GetModuleFileNameA(hModuleForMigration, dllPathForMigration, MAX_PATH) > 0) {
        std::string path(dllPathForMigration);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string gameDir = path.substr(0, lastSlash);
            lastSlash = gameDir.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                gameDir = gameDir.substr(0, lastSlash + 1);
                oldConfigDir = std::filesystem::path(gameDir) / "config" / "XIFriendList";
                newConfigDir = std::filesystem::path(gameDir) / "config" / "FFXIFriendList";
            }
        }
    }
    if (oldConfigDir.empty() || newConfigDir.empty()) {
        oldConfigDir = std::filesystem::path("C:\\HorizonXI\\HorizonXI\\Game\\config\\XIFriendList");
        newConfigDir = std::filesystem::path("C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList");
    }
    
    if (CacheMigration::migrateConfigDirectory(oldConfigDir, newConfigDir)) {
        logger_->info("[migration] Migrated config files from XIFriendList to FFXIFriendList directory");
    }
    
    std::string defaultConfigDir = PathUtils::getDefaultConfigDirectory();
    if (!defaultConfigDir.empty()) {
        std::filesystem::path newAppDataDir = std::filesystem::path(defaultConfigDir);
        std::filesystem::path oldAppDataDir = newAppDataDir.parent_path() / "XIFriendList";
        if (std::filesystem::exists(oldAppDataDir) && !std::filesystem::equivalent(oldAppDataDir, newAppDataDir)) {
            if (CacheMigration::migrateConfigDirectory(oldAppDataDir, newAppDataDir)) {
                logger_->info("[migration] Migrated config files from AppData XIFriendList to FFXIFriendList directory");
            }
        }
    }
    
    CacheMigration::migrateCacheAndIniToJson(mainJsonPath, cacheJsonPath, iniPath, settingsJsonPath);
    
    ApiKeyPersistence::loadFromFile(apiKeyState_);
    ThemePersistence::loadFromFile(themeState_);
    ServerSelectionPersistence::loadFromFile(serverSelectionState_);
    if (serverSelectionState_.hasSavedServer()) {
        logger_->info("[server-selection] Loaded saved server ID from cache: " + serverSelectionState_.savedServerId.value());
    } else {
        logger_->info("[server-selection] No saved server found in cache");
    }
    preferencesStore_ = std::make_unique<AshitaPreferencesStore>();
    
    soundPlayer_ = std::make_unique<AshitaSoundPlayer>(*logger_);
    
    std::filesystem::path configDir;
    char dllPath[MAX_PATH] = {0};
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    configDir = std::filesystem::path(gameDir) / "config" / "FFXIFriendList";
                }
            }
        }
    }
    if (configDir.empty()) {
        std::string defaultDir = PathUtils::getDefaultConfigDirectory();
        if (!defaultDir.empty()) {
            configDir = std::filesystem::path(defaultDir);
        } else {
            configDir = std::filesystem::path("C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList");
        }
    }
    
    soundService_ = std::make_unique<App::NotificationSoundService>(
        *soundPlayer_,
        *clock_,
        *logger_,
        configDir
    );
    
    connectUseCase_ = std::make_unique<App::UseCases::ConnectUseCase>(*netClient_, *clock_, *logger_, &apiKeyState_);
    syncUseCase_ = std::make_unique<App::UseCases::SyncFriendListUseCase>(*netClient_, *clock_, *logger_);
    presenceUseCase_ = std::make_unique<App::UseCases::UpdatePresenceUseCase>(*netClient_, *clock_, *logger_);
    updateMyStatusUseCase_ = std::make_unique<App::UseCases::UpdateMyStatusUseCase>(*netClient_, *clock_, *logger_);  // C1: Status flags sync
    sendRequestUseCase_ = std::make_unique<App::UseCases::SendFriendRequestUseCase>(*netClient_, *clock_, *logger_);
    sendRequestUseCase_->setRetryConfig(1, 500);
    acceptRequestUseCase_ = std::make_unique<App::UseCases::AcceptFriendRequestUseCase>(*netClient_, *clock_, *logger_);
    rejectRequestUseCase_ = std::make_unique<App::UseCases::RejectFriendRequestUseCase>(*netClient_, *clock_, *logger_);
    cancelRequestUseCase_ = std::make_unique<App::UseCases::CancelFriendRequestUseCase>(*netClient_, *clock_, *logger_);
    getRequestsUseCase_ = std::make_unique<App::UseCases::GetFriendRequestsUseCase>(*netClient_, *clock_, *logger_);
    removeFriendUseCase_ = std::make_unique<App::UseCases::RemoveFriendUseCase>(*netClient_, *clock_, *logger_);
    removeFriendVisibilityUseCase_ = std::make_unique<App::UseCases::RemoveFriendVisibilityUseCase>(*netClient_, *clock_, *logger_);
    getAltVisibilityUseCase_ = std::make_unique<App::UseCases::GetAltVisibilityUseCase>(*netClient_, *clock_, *logger_);
    themeUseCase_ = std::make_unique<App::UseCases::ThemeUseCase>(themeState_);
    preferencesUseCase_ = std::make_unique<App::UseCases::PreferencesUseCase>(*netClient_, *clock_, *logger_, preferencesStore_.get());
    getNotesUseCase_ = std::make_unique<App::UseCases::GetNotesUseCase>(*netClient_, notesState_, *clock_, *logger_);
    saveNoteUseCase_ = std::make_unique<App::UseCases::SaveNoteUseCase>(*netClient_, notesState_, *clock_, *logger_);
    deleteNoteUseCase_ = std::make_unique<App::UseCases::DeleteNoteUseCase>(*netClient_, notesState_, *clock_, *logger_);
    handleCharacterChangedUseCase_ = std::make_unique<App::UseCases::HandleCharacterChangedUseCase>(*netClient_, *clock_, *logger_, &apiKeyState_);
    handleZoneChangedUseCase_ = std::make_unique<App::UseCases::HandleZoneChangedUseCase>(*clock_, *logger_);
    testRunnerUseCase_ = std::make_unique<App::UseCases::TestRunnerUseCase>(*netClient_, *clock_, *logger_, apiKeyState_);
    fetchServerListUseCase_ = std::make_unique<App::UseCases::FetchServerListUseCase>(*netClient_, *logger_);
    
    eventQueue_ = std::make_unique<AshitaEventQueue>();
    
    auto* eventQueue = static_cast<AshitaEventQueue*>(eventQueue_.get());
    eventQueue->setCharacterChangedHandler([this](const App::Events::CharacterChanged& event) {
        handleCharacterChangedEvent(event);
    });
    eventQueue->setZoneChangedHandler([this](const App::Events::ZoneChanged& event) {
        handleZoneChangedEvent(event);
    });
    
    viewModel_ = std::make_unique<UI::FriendListViewModel>();
    quickOnlineViewModel_ = std::make_unique<UI::FriendListViewModel>();
    quickOnlineViewModel_->setShowFriendedAsColumn(false);
    quickOnlineViewModel_->setShowJobColumn(false);
    quickOnlineViewModel_->setShowZoneColumn(false);
    quickOnlineViewModel_->setShowNationColumn(false);
    quickOnlineViewModel_->setShowRankColumn(false);
    quickOnlineViewModel_->setShowLastSeenColumn(false);
    themesViewModel_ = std::make_unique<UI::ThemesViewModel>();
    optionsViewModel_ = std::make_unique<UI::OptionsViewModel>();
    notesViewModel_ = std::make_unique<UI::NotesViewModel>();
    altVisibilityViewModel_ = std::make_unique<UI::AltVisibilityViewModel>();
    
    notificationUseCase_ = std::make_unique<App::UseCases::NotificationUseCase>();
    
    iconManager_ = std::make_unique<IconManager>();
    windowManager_ = std::make_unique<UI::WindowManager>();
    windowManager_->setViewModel(viewModel_.get());  // Pass pointer, WindowManager stores pointer
    windowManager_->setQuickOnlineViewModel(quickOnlineViewModel_.get());
    windowManager_->setOptionsViewModel(optionsViewModel_.get());
    windowManager_->setThemesViewModel(themesViewModel_.get());
    windowManager_->setThemesViewModelForOptions(themesViewModel_.get());  // Also set for Options window
    logger_->debug("[theme] ViewModel initialized");
    windowManager_->setNotesViewModel(notesViewModel_.get());
    windowManager_->setAltVisibilityViewModel(altVisibilityViewModel_.get());
    windowManager_->setIconManager(iconManager_.get());
    windowManager_->setCommandHandler(this);
    
    windowManager_->getMainWindow().setPluginInfo(getName(), getAuthor(), getVersionString());
    
    // Load local preferences synchronously so notification position is set immediately
    // This prevents notifications from starting at default position and then shifting
    if (preferencesUseCase_ && preferencesStore_) {
        preferencesUseCase_->loadPreferences("", "");  // Load local preferences only (empty apiKey/characterName)
        
        // Set notification position from loaded preferences immediately
        Core::Preferences prefs = preferencesUseCase_->getPreferences();
        float posX = (prefs.notificationPositionX < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : prefs.notificationPositionX;
        float posY = (prefs.notificationPositionY < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : prefs.notificationPositionY;
        UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
    }
    
    deferredInitPending_ = true;
}

AshitaAdapter::~AshitaAdapter() {
    release();
}

bool AshitaAdapter::initialize(IAshitaCore* core, ILogManager* logger, uint32_t pluginId) {
    if (initialized_) {
        return true;
    }

    PERF_SCOPE("AshitaAdapter::initialize (total)");
    
    ashitaCore_ = core;
    logManager_ = logger;
    pluginId_ = pluginId;
    
    netClient_->setAshitaCore(core);
    logger_->setLogManager(logger);
    
    std::ostringstream sessionId;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    uint32_t a = dist(gen);
    uint16_t b = static_cast<uint16_t>(dist(gen) & 0xFFFF);
    uint16_t c = static_cast<uint16_t>((dist(gen) & 0x0FFF) | 0x4000);  // Version 4
    uint16_t d = static_cast<uint16_t>((dist(gen) & 0x3FFF) | 0x8000);  // Variant 1
    uint32_t e1 = dist(gen);
    uint16_t e2 = static_cast<uint16_t>(dist(gen) & 0xFFFF);
    sessionId << std::hex << std::setfill('0')
              << std::setw(8) << a << "-"
              << std::setw(4) << b << "-"
              << std::setw(4) << c << "-"
              << std::setw(4) << d << "-"
              << std::setw(8) << e1
              << std::setw(4) << e2;
    netClient_->setSessionId(sessionId.str());
    
    std::string serverUrl = netClient_->getBaseUrl();
    logger_->info("[init] Server URL: " + serverUrl);
    #ifdef USE_TEST_SERVER
    logger_->warning("[init] USE_TEST_SERVER is defined (Debug build)");
    if (serverUrl.find("api-test") == std::string::npos && serverUrl.find("localhost") == std::string::npos) {
        logger_->warning("[init] USE_TEST_SERVER defined but server URL is not test server");
    }
    #else
    logger_->debug("[init] USE_TEST_SERVER not defined (Release build)");
    if (serverUrl.find("api-test") != std::string::npos) {
        logger_->error("[init] Release build using test server - check config");
    }
    #endif
    
#ifdef BUILDING_TESTS
    guiManager_ = nullptr;
    chatManager_ = nullptr;
#else
    if (core) {
        guiManager_ = core->GetGuiManager();
        chatManager_ = core->GetChatManager();
    } else {
        guiManager_ = nullptr;
        chatManager_ = nullptr;
    }
#endif
    
    if (!ImGuiBridge::initialize()) {
        logger_->error("[init] Failed to initialize ImGui bridge");
    } else {
        ImGuiBridge::setGuiManager(guiManager_);
        if (guiManager_) {
            PERF_SCOPE("AshitaAdapter::initialize create AshitaUiRenderer + storeDefaultImGuiStyle");
            uiRenderer_ = std::make_unique<AshitaUiRenderer>(guiManager_);
            UI::SetUiRenderer(uiRenderer_.get());
            
            storeDefaultImGuiStyle();
        }
    }
    
    friendListMenuDetector_ = std::make_unique<FriendListMenuDetector>();
    PERF_SCOPE("AshitaAdapter::initialize FriendListMenuDetector::initialize");
    if (!friendListMenuDetector_->initialize(
        ashitaCore_,
        logManager_,
        clock_.get(),
        []() {}
    )) {
        logger_->warning("[init] Failed to initialize friendlist menu detector");
    }
    
    escKeyDetector_ = std::make_unique<KeyEdgeDetector>();
    backspaceKeyDetector_ = std::make_unique<KeyEdgeDetector>();
    customKeyDetector_ = std::make_unique<KeyEdgeDetector>();
    windowClosePolicy_ = std::make_unique<UI::WindowClosePolicy>(windowManager_.get());
    capturingCustomKey_ = false;
    capturedKeyCode_ = 0;
    
    detectServerFromRealm();
    
    if (serverSelectionState_.hasSavedServer()) {
        std::string savedServerId = serverSelectionState_.savedServerId.value();
        
        if (serverSelectionState_.savedServerBaseUrl.has_value() && !serverSelectionState_.savedServerBaseUrl->empty()) {
            netClient_->setBaseUrl(serverSelectionState_.savedServerBaseUrl.value());
            logger_->info("[server-selection] Loaded saved server URL from cache: " + serverSelectionState_.savedServerBaseUrl.value());
        }
        
        netClient_->setRealmId(serverSelectionState_.savedServerId.value());
        
        handleRefreshServerList();
        
        Core::ServerInfo* savedServer = nullptr;
        for (auto& server : serverList_.servers) {
            if (server.id == savedServerId) {
                savedServer = &server;
                break;
            }
        }
        
        if (savedServer) {
            if (savedServer->baseUrl != serverSelectionState_.savedServerBaseUrl.value_or("")) {
                serverSelectionState_.savedServerBaseUrl = savedServer->baseUrl;
                netClient_->setBaseUrl(savedServer->baseUrl);
                ServerSelectionPersistence::saveToFile(serverSelectionState_);
            }
            netClient_->setRealmId(savedServerId);
            logger_->info("[server-selection] Verified saved server: " + savedServer->name + " (" + savedServer->baseUrl + "), realm: " + savedServerId);
        } else {
            logger_->warning("[server-selection] Saved server ID not found in server list: " + savedServerId + ", using cached URL");
        }
    } else {
        handleRefreshServerList();
    }
    
    logger_->info("[init] Initialized");
    
    initialized_ = true;
    initializationTime_ = GetTickCount();  // Record initialization time for notification cooldown
    initializationTimeMs_ = clock_->nowMs();  // Record initialization time in milliseconds for update check delay
    return true;
}

void AshitaAdapter::release() {
    if (!initialized_) {
        return;
    }
    
    logger_->debug("[init] Releasing");
    
    {
        std::lock_guard<std::mutex> lock(autoSaveMutex_);
        autoSaveThreadShouldExit_ = true;
        autoSavePending_ = false;
    }
    if (autoSaveThread_ && autoSaveThread_->joinable()) {
        autoSaveThread_->detach();  // Detach instead of join to avoid blocking
        autoSaveThread_.reset();
    }
    
    defaultStyleStorage_.reset();
    
    if (friendListMenuDetector_) {
        friendListMenuDetector_->shutdown();
        friendListMenuDetector_.reset();
    }
    
    ImGuiBridge::shutdown();
    
    if (connectUseCase_->isConnected()) {
        viewModel_->setConnectionState(App::ConnectionState::Disconnected);
        connectUseCase_->disconnect();
    }
    
    ashitaCore_ = nullptr;
    logManager_ = nullptr;
    guiManager_ = nullptr;
    chatManager_ = nullptr;
    
    initialized_ = false;
}

void AshitaAdapter::render() {
    if (!initialized_ || !ImGuiBridge::isAvailable()) {
        return;
    }
    
    if (!guiManager_) {
        static bool loggedOnce = false;
        if (logger_ && !loggedOnce) {
            logger_->debug("[ui] GUI manager not available, skipping render");
            loggedOnce = true;
        }
        return;
    }
    
    bool hasVisibleWindows = windowManager_ && windowManager_->hasAnyVisibleWindow();
    
    bool hasNotifications = false;
    if (UI::Notifications::ToastManager::getInstance().getToastCount() > 0) {
        uint32_t tickNow = GetTickCount();
        uint32_t timeSinceInit = tickNow - initializationTime_;
        if (timeSinceInit >= NOTIFICATION_RENDERING_COOLDOWN_MS) {
            hasNotifications = true;
        }
    }
    
    if (deferredInitPending_ && (hasVisibleWindows || hasNotifications)) {
        deferredInitPending_ = false;
        
        std::thread([this]() {
            ThemePersistence::loadFromFile(themeState_);
            themeUseCase_->loadThemes();
            
            updateThemesViewModel();
            
            preferencesUseCase_->loadPreferences("", "");
            
            updateOptionsViewModel();
            updateFriendListViewModelsFromPreferences();
            
            // Set notification position from loaded preferences
            if (preferencesUseCase_) {
                Core::Preferences prefs = preferencesUseCase_->getPreferences();
                // Convert -1 (old default) to default position before setting position
                float posX = (prefs.notificationPositionX < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : prefs.notificationPositionX;
                float posY = (prefs.notificationPositionY < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : prefs.notificationPositionY;
                UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
            }
        }).detach();
    }
    
    static bool initialThemeApplied = false;
    if (!initialThemeApplied && guiManager_ && themeUseCase_ && (hasVisibleWindows || hasNotifications)) {
        if (defaultStyleStorage_ && !defaultStyleStorage_->defaultStyle.has_value()) {
            storeDefaultImGuiStyle();
        }
        initialThemeApplied = true;
        if (logger_) {
            logger_->debug("[theme] Initial setup complete");
        }
    }
    
    // Always update and render notifications if they exist
    // Note: This must happen BEFORE the early return check, so notifications render even when no windows are visible
    if (clock_ && guiManager_) {
        int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
        size_t toastCountBefore = UI::Notifications::ToastManager::getInstance().getToastCount();
        UI::Notifications::ToastManager::getInstance().update(currentTime);
        size_t toastCountAfter = UI::Notifications::ToastManager::getInstance().getToastCount();
        
        // Render notifications if they exist
        if (toastCountAfter > 0) {
            UI::Notifications::ToastManager::getInstance().render();
        }
        
        // Debug: log if toasts are being removed unexpectedly
        static size_t lastLoggedCount = 0;
        if (toastCountBefore != toastCountAfter && logger_ && isDebugEnabled()) {
            logger_->debug("[Notifications] Toast count changed: " + std::to_string(toastCountBefore) + 
                " -> " + std::to_string(toastCountAfter) + " at time " + std::to_string(currentTime));
            lastLoggedCount = toastCountAfter;
        }
    }
    
    if (!hasVisibleWindows && !hasNotifications) {
        return;
    }
    
    handleEscapeKey();
    
    if (windowManager_) {
        windowManager_->render();
    }
    
    if (shouldBlockNetworkOperation() && !windowManager_->getServerSelectionWindow().isVisible() && !serverSelectionState_.hasSavedServer()) {
        showServerSelectionWindow();
    }
}

void AshitaAdapter::update() {
    if (!initialized_) {
        return;
    }

    if (iconManager_) {
        iconManager_->processPendingCreates(1);
    }
    
    std::string currentName = getCharacterName();
    
    if (currentName.empty() && characterName_.empty() && lastDetectedCharacterName_.empty()) {
        static uint64_t lastLogTime = 0;
        uint64_t now = clock_->nowMs();
        if (now - lastLogTime > 10000) {  // Log every 10 seconds max
            logger_->debug("[char] Waiting for character detection");
            lastLogTime = now;
        }
        return;
    }
    
    if (eventQueue_) {
        eventQueue_->processEvents();
    }
    
    if (!currentName.empty() && currentName != lastDetectedCharacterName_) {
        std::string previousName = lastDetectedCharacterName_;
        lastDetectedCharacterName_ = currentName;
        characterName_ = currentName;
        hasReportedVersion_ = false;
        
        logger_->info("[char] Detected: " + currentName);
        
        if (eventQueue_) {
            uint64_t timestamp = clock_->nowMs();
            App::Events::CharacterChanged event(currentName, previousName, timestamp);
            eventQueue_->pushCharacterChanged(event);
        }
        
        if (autoConnectAttempted_ && !connectUseCase_->isConnected()) {
            autoConnectAttempted_ = false;
            autoConnectInProgress_ = false;
            logger_->info("[char] Changed, resetting auto-connect");
        }
        
        if (!connectUseCase_->isConnected() && !autoConnectAttempted_ && !autoConnectInProgress_ && !backgroundPausedForTests_.load()) {
            logger_->info("[char] Auto-connecting: " + currentName);
            attemptAutoConnectAsync();
        } else {
            if (connectUseCase_->isConnected()) {
                logger_->debug("[char] Already connected, skipping auto-connect");
            } else if (autoConnectAttempted_) {
                logger_->debug("[char] Auto-connect already attempted, skipping");
            } else if (autoConnectInProgress_) {
                logger_->debug("[char] Auto-connect in progress, skipping");
            }
        }
    }
    
    bool shouldStartPrefSync = false;
    std::string prefSyncApiKey;
    std::string prefSyncCharacterName;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (autoConnectCompleted_) {
            autoConnectCompleted_ = false;
            
            if (pendingConnectResult_.success) {
                apiKey_ = pendingConnectResult_.apiKey;
                characterName_ = pendingConnectResult_.username;
                hasReportedVersion_ = false;
                viewModel_->setConnectionState(App::ConnectionState::Connected);
                logger_->info("[char] Auto-connected: " + characterName_);
                
                {
                    std::lock_guard<std::mutex> pollingLock(pollingMutex_);
                    lastFriendListSyncCallsite_ = "AutoConnect";
                }
                handleSyncFriendListAsync();

                shouldStartPrefSync = true;
                prefSyncApiKey = apiKey_;
                prefSyncCharacterName = characterName_;
            } else {
                viewModel_->setConnectionState(App::ConnectionState::Failed);
                viewModel_->setErrorMessage(pendingConnectResult_.error);
                logger_->error("AshitaAdapter: Auto-connection failed: " + pendingConnectResult_.error);
            }
        }
    }

    if (shouldStartPrefSync) {
        startPreferencesSyncFromServerAsync(prefSyncApiKey, prefSyncCharacterName);
    }

    {
        bool shouldProcess = false;
        App::UseCases::PreferencesResult result;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (preferencesSyncCompleted_) {
                preferencesSyncCompleted_ = false;
                shouldProcess = true;
                result = pendingPreferencesSyncResult_;
            }
        }
        if (shouldProcess) {
            if (result.success) {
                updateOptionsViewModel();
                updateFriendListViewModelsFromPreferences();
                logger_->info("AshitaAdapter: Preferences synced from server");
            } else {
                logger_->warning("AshitaAdapter: Failed to sync preferences from server: " + result.error);
            }
        }
    }

    {
        bool shouldProcess = false;
        App::UseCases::GetFriendRequestsResult result;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (friendRequestsSyncCompleted_) {
                friendRequestsSyncCompleted_ = false;
                friendRequestsSyncInProgress_ = false;
                shouldProcess = true;
                result = pendingFriendRequestsResult_;
            }
        }
        if (shouldProcess) {
            if (result.success) {
                if (viewModel_) {
                    viewModel_->updatePendingRequests(result.incoming, result.outgoing);
                }
                logger_->debug("AshitaAdapter: Friend requests updated: " +
                              std::to_string(result.incoming.size()) + " incoming, " +
                              std::to_string(result.outgoing.size()) + " outgoing");

                {
                    std::lock_guard<std::mutex> lock(processedRequestIdsMutex_);
                    for (const auto& request : result.incoming) {
                        if (processedRequestIds_.find(request.requestId) == processedRequestIds_.end()) {
                            processedRequestIds_.insert(request.requestId);

                            std::string displayName = request.fromCharacterName;
                            if (!displayName.empty()) {
                                displayName[0] = static_cast<char>(std::toupper(displayName[0]));
                                for (size_t i = 1; i < displayName.length(); ++i) {
                                    if (displayName[i - 1] == ' ') {
                                        displayName[i] = static_cast<char>(std::toupper(displayName[i]));
                                    } else {
                                        displayName[i] = static_cast<char>(std::tolower(displayName[i]));
                                    }
                                }
                            }

                            if (notificationUseCase_ && clock_) {
                                int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                                auto toast = notificationUseCase_->createFriendRequestReceivedToast(displayName, currentTime);
                                UI::Notifications::ToastManager::getInstance().addToast(toast);
                                
                                if (isDebugEnabled()) {
                                    pushDebugLog("[Notifications] Friend request received from " + displayName + " - toast created");
                                    logger_->debug("[Notifications] Friend request received: " + displayName + ", toast count: " + 
                                        std::to_string(UI::Notifications::ToastManager::getInstance().getToastCount()));
                                }
                            }

                            if (isDebugEnabled()) {
                                pushDebugLog("Friend request received from " + displayName);
                            }
                        }
                    }
                }
            } else {
                logger_->warning("AshitaAdapter: Failed to get friend requests: " + result.error);
            }
        }
    }

    {
        std::string errorToEcho;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (!pendingChatEchoError_.empty()) {
                errorToEcho = std::move(pendingChatEchoError_);
                pendingChatEchoError_.clear();
            }
        }
        if (!errorToEcho.empty()) {
            pushToGameEcho(errorToEcho);
        }
    }

    {
        bool shouldProcess = false;
        App::Events::CharacterChanged event("", "", 0);
        App::UseCases::CharacterChangeResult result{};
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (characterChangedCompleted_) {
                characterChangedCompleted_ = false;
                characterChangedInProgress_ = false;
                shouldProcess = true;
                event = pendingCharacterChangedEvent_;
                result = pendingCharacterChangedResult_;
            }
        }

        if (shouldProcess) {
            if (result.success) {
                if (result.apiKey.empty()) {
                    logger_->warning("AshitaAdapter: Server did not return API key for " + event.newCharacterName + ", attempting recovery");
                    
                    std::string recoveryUrl = netClient_->getBaseUrl() + "/api/auth/ensure";
                    std::string realmId = serverSelectionState_.savedServerId.has_value() ? serverSelectionState_.savedServerId.value() : netClient_->getRealmId();
                    std::ostringstream recoveryBody;
                    recoveryBody << "{\"characterName\":\"" << event.newCharacterName << "\",\"realmId\":\"" << realmId << "\"}";
                    
                    App::HttpResponse recoveryResponse = netClient_->post(recoveryUrl, "", event.newCharacterName, recoveryBody.str());
                    
                    if (recoveryResponse.isSuccess() && recoveryResponse.statusCode == 200) {
                        std::string recoveredApiKey;
                        Protocol::JsonUtils::extractStringField(recoveryResponse.body, "apiKey", recoveredApiKey);
                        if (!recoveredApiKey.empty()) {
                            result.apiKey = recoveredApiKey;
                            logger_->info("[char] Recovered API key for " + event.newCharacterName);
                        } else {
                            logger_->error("AshitaAdapter: Failed to recover API key for " + event.newCharacterName + " - cannot proceed");
                            return;
                        }
                    } else {
                        logger_->error("AshitaAdapter: Failed to recover API key for " + event.newCharacterName + " (HTTP " + std::to_string(recoveryResponse.statusCode) + ") - cannot proceed");
                        return;
                    }
                }
                
                apiKey_ = result.apiKey;
                logger_->info("[char] Ensured, API key updated for " + event.newCharacterName);

                characterName_ = event.newCharacterName;
                hasReportedVersion_ = false;
                
                if (result.accountId > 0) {
                    int previousAccountId = accountId_;
                    accountId_ = result.accountId;
                    
                    if (previousAccountId > 0 && notesState_.dirty) {
                        NotesPersistence::saveToFile(notesState_, previousAccountId);
                    }
                    
                    notesState_.accountId = accountId_;
                    NotesPersistence::loadFromFile(notesState_, accountId_);
                }

                if (viewModel_) {
                    viewModel_->setCurrentCharacterName(event.newCharacterName);
                }

                startPreferencesSyncFromServerAsync(apiKey_, characterName_);

                {
                    std::lock_guard<std::mutex> lock(pollingMutex_);
                    lastFriendListSyncCallsite_ = "CharacterChange";
                }
                handleSyncFriendListAsync();
            } else {
                logger_->error("AshitaAdapter: Character change handling failed: " + result.error);
            }
        }
    }
    
    if (connectUseCase_->isConnected() && localCacheWarmupCompleted_.load() && !backgroundPausedForTests_.load()) {
        uint64_t now = clock_->nowMs();
        
        bool shouldTriggerPresence = false;
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            if (!presenceUpdateInFlight_ && (now - lastPresenceUpdate_ >= POLL_INTERVAL_PRESENCE_MS)) {
                uint64_t elapsed = now - lastPresenceUpdate_;
                logger_->debug("AshitaAdapter: Presence tick (elapsed: " + std::to_string(elapsed) + "ms, interval: " + 
                              std::to_string(POLL_INTERVAL_PRESENCE_MS) + "ms)");
                presenceUpdateInFlight_ = true;
                lastPresenceUpdate_ = now;
                shouldTriggerPresence = true;
            }
        }
        if (shouldTriggerPresence) {
            presenceHeartbeatTick();
        }
        
        bool shouldTriggerFullRefresh = false;
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            uint64_t timeSinceLastRefresh = now - lastFullRefresh_;
            if (!fullRefreshInFlight_ && !friendListSyncInFlight_ && (timeSinceLastRefresh >= POLL_INTERVAL_REFRESH_MS)) {
                uint64_t elapsed = timeSinceLastRefresh;
                logger_->debug("[sync] Refresh tick (elapsed: " + std::to_string(elapsed) + "ms, interval: " + 
                              std::to_string(POLL_INTERVAL_REFRESH_MS) + "ms)");
                fullRefreshInFlight_ = true;
                lastFullRefresh_ = now;  // Update timestamp BEFORE calling fullRefreshTick to prevent race condition
                shouldTriggerFullRefresh = true;
            } else if (friendListSyncInFlight_) {
                logger_->debug("[sync] Refresh skipped - friend list sync in-flight");
            } else if (fullRefreshInFlight_) {
                logger_->debug("[sync] Refresh skipped - already in-flight");
            } else {
                logger_->debug("[sync] Refresh skipped - interval not elapsed (elapsed: " + 
                              std::to_string(timeSinceLastRefresh) + "ms, need: " + 
                              std::to_string(POLL_INTERVAL_REFRESH_MS) + "ms)");
            }
        }
        if (shouldTriggerFullRefresh) {
            fullRefreshTick();
        }
        
        bool shouldTriggerPlayerDataCheck = false;
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            if (now - lastPlayerDataCheck_ >= POLL_INTERVAL_PLAYER_DATA_CHECK_MS) {
                lastPlayerDataCheck_ = now;
                shouldTriggerPlayerDataCheck = true;
            }
        }
        if (shouldTriggerPlayerDataCheck) {
            playerDataChangeDetectionTick();
        }
        
    }
    
    if (friendListMenuDetector_ && !backgroundPausedForTests_.load()) {
        friendListMenuDetector_->update();
    }
}

void AshitaAdapter::startPreferencesSyncFromServerAsync(const std::string& apiKey, const std::string& characterName) {
    if (!preferencesUseCase_ || apiKey.empty() || characterName.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (preferencesSyncInProgress_) {
            return;
        }
        preferencesSyncInProgress_ = true;
        preferencesSyncCompleted_ = false;
    }

    std::thread([this, apiKey, characterName]() {
        PERF_SCOPE("PreferencesUseCase::syncFromServer (bg)");
        App::UseCases::PreferencesResult result = preferencesUseCase_->syncFromServer(apiKey, characterName);
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            pendingPreferencesSyncResult_ = result;
            preferencesSyncCompleted_ = true;
            preferencesSyncInProgress_ = false;
        }
    }).detach();
}

void AshitaAdapter::toggleWindow() {
    if (!initialized_ || !windowManager_) {
        if (logger_) {
            logger_->warning("AshitaAdapter: toggleWindow called but not initialized or windowManager is null");
        }
        return;
    }
    
    rerouteToServerSelectionIfNeeded();
    if (shouldBlockNetworkOperation()) {
        return;
    }
    
    UI::MainWindow& window = windowManager_->getMainWindow();
    bool wasVisible = window.isVisible();
    window.setVisible(!wasVisible);
    
    if (logger_) {
        logger_->info("AshitaAdapter: Main window toggled from " + 
                     std::string(wasVisible ? "visible" : "hidden") + " to " +
                     std::string(window.isVisible() ? "visible" : "hidden"));
    }
}

bool AshitaAdapter::isWindowVisible() const {
    if (!initialized_ || !windowManager_) {
        return false;
    }
    
    return windowManager_->getMainWindow().isVisible();
}

void AshitaAdapter::openQuickOnlineWindow() {
    if (!initialized_ || !windowManager_) {
        return;
    }
    
    rerouteToServerSelectionIfNeeded();
    if (shouldBlockNetworkOperation()) {
        return;
    }
    
    windowManager_->getQuickOnlineWindow().setVisible(true);
}

void AshitaAdapter::closeQuickOnlineWindow() {
    if (!initialized_ || !windowManager_) {
        return;
    }
    windowManager_->getQuickOnlineWindow().setVisible(false);
}

bool AshitaAdapter::isQuickOnlineWindowVisible() const {
    if (!initialized_ || !windowManager_) {
        return false;
    }
    return windowManager_->getQuickOnlineWindow().isVisible();
}


void AshitaAdapter::toggleDebugWindow() {
    if (!initialized_ || !windowManager_) {
        if (logger_) {
            logger_->warning("AshitaAdapter: toggleDebugWindow called but not initialized or windowManager is null");
        }
        return;
    }
    if (!isDebugEnabled()) {
        windowManager_->getDebugLogWindow().setVisible(false);
        return;
    }
    
    UI::DebugLogWindow& window = windowManager_->getDebugLogWindow();
    bool wasVisible = window.isVisible();
    window.setVisible(!wasVisible);
    
    if (logger_) {
        logger_->info("AshitaAdapter: Debug log window toggled from " + 
                     std::string(wasVisible ? "visible" : "hidden") + " to " +
                     std::string(window.isVisible() ? "visible" : "hidden"));
    }
}

void AshitaAdapter::triggerTestNotification() {
    if (!notificationUseCase_ || !clock_) {
        pushToGameEcho("[FriendList] Cannot create notification: missing dependencies");
        if (logger_) {
            logger_->warning("[Notifications] Cannot create test notification: notificationUseCase_=" + 
                std::string(notificationUseCase_ ? "ok" : "null") + ", clock_=" + 
                std::string(clock_ ? "ok" : "null"));
        }
        return;
    }
    
    int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
    auto toast = notificationUseCase_->createFriendOnlineToast("TestFriend", currentTime);
    
    // Ensure all required fields are set
    toast.alpha = 0.0f;
    toast.offsetX = 0.0f;
    toast.dismissed = false;
    
    UI::Notifications::ToastManager::getInstance().addToast(toast);
    
    size_t toastCount = UI::Notifications::ToastManager::getInstance().getToastCount();
    
    std::ostringstream msg;
    msg << "[FriendList] Test notification added (count: " << toastCount 
        << ", duration: " << toast.duration << "ms, state: ENTERING)";
    pushToGameEcho(msg.str());
    
    if (logger_) {
        logger_->info("[Notifications] Test notification triggered - toast count: " + 
            std::to_string(toastCount) + ", createdAt: " + std::to_string(toast.createdAt) +
            ", duration: " + std::to_string(toast.duration) + ", type: FriendOnline");
    }
}

void AshitaAdapter::triggerRefreshOnOpen() {
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        return;
    }
    
    logger_->debug("[ui] Window opened, triggering refresh");
    handleRefreshStatus();
}

void AshitaAdapter::handleTestList() {
    if (!testRunnerUseCase_) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Test runner not available");
        }
        return;
    }
    
    std::string characterName = getCharacterName();
    if (characterName.empty()) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Character name not available");
        }
        return;
    }
    
    std::thread([this, characterName]() {
        try {
            std::vector<App::UseCases::TestScenario> scenarios = testRunnerUseCase_->getScenarios();
            
            if (scenarios.empty()) {
                if (chatManager_) {
                    chatManager_->Write(200, false, "[FriendList] No test scenarios available - check debug log for details");
                }
                if (logger_) {
                    logger_->error("AshitaAdapter: Test scenarios list is empty - check TestRunnerUseCase logs");
                }
                return;
            }
            
            if (chatManager_) {
                std::ostringstream msg;
                msg << "Available test scenarios (" << scenarios.size() << "):";
                chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
                
                for (const auto& scenario : scenarios) {
                    std::ostringstream scenarioMsg;
                    scenarioMsg << "  " << scenario.id << ": " << scenario.name;
                    chatManager_->Write(200, false, ("[FriendList] " + scenarioMsg.str()).c_str());
                }
            }
        } catch (const std::exception& e) {
            if (chatManager_) {
                std::ostringstream msg;
                msg << "Error getting test scenarios: " << e.what();
                chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
            }
        }
    }).detach();
}

void AshitaAdapter::handleTestRun(const std::string& scenarioId) {
    if (!testRunnerUseCase_) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Test runner not available");
        }
        return;
    }
    
    std::string characterName = getCharacterName();
    if (characterName.empty()) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Character name not available");
        }
        return;
    }
    
    std::thread([this, characterName, scenarioId]() {
        try {
            TestRunGuard guard(*this, *logger_, *clock_);
            
            if (chatManager_) {
                chatManager_->Write(200, false, "[FriendList] Running tests...");
            }
            
            if (scenarioId.empty()) {
                App::UseCases::TestRunSummary summary = testRunnerUseCase_->runAllTests(characterName);
                
                if (chatManager_) {
                    for (const auto& result : summary.results) {
                        std::ostringstream msg;
                        msg << (result.passed ? "[PASS]" : "[FAIL]") << " " 
                            << result.scenarioName;
                        if (!result.details.empty()) {
                            msg << " — " << result.details;
                        }
                        if (!result.error.empty()) {
                            msg << " — " << result.error;
                        }
                        chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
                    }
                    
                    std::ostringstream summaryMsg;
                    summaryMsg << "Summary: Total=" << summary.total 
                               << ", Passed=" << summary.passed 
                               << ", Failed=" << summary.failed
                               << ", Duration=" << (summary.durationMs / 1000.0) << "s";
                    chatManager_->Write(200, false, ("[FriendList] " + summaryMsg.str()).c_str());
                }
            } else {
                std::vector<App::UseCases::TestScenario> scenarios = testRunnerUseCase_->getScenarios();
                App::UseCases::TestScenario* targetScenario = nullptr;
                
                for (auto& scenario : scenarios) {
                    if (scenario.id == scenarioId) {
                        targetScenario = &scenario;
                        break;
                    }
                }
                
                if (!targetScenario) {
                    if (chatManager_) {
                        std::ostringstream msg;
                        msg << "Test scenario not found: " << scenarioId;
                        chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
                    }
                    return;
                }
                
                App::UseCases::TestResult result = testRunnerUseCase_->runScenario(*targetScenario, characterName);
                
                if (chatManager_) {
                    std::ostringstream msg;
                    msg << (result.passed ? "[PASS]" : "[FAIL]") << " " 
                        << result.scenarioName;
                    if (!result.details.empty()) {
                        msg << " — " << result.details;
                    }
                    if (!result.error.empty()) {
                        msg << " — " << result.error;
                    }
                    chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
                }
            }
        } catch (const std::exception& e) {
            if (chatManager_) {
                std::ostringstream msg;
                msg << "Error running tests: " << e.what();
                chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
            }
        }
    }).detach();
}

void AshitaAdapter::handleTestReset() {
    if (!testRunnerUseCase_) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Test runner not available");
        }
        return;
    }
    
    std::string characterName = getCharacterName();
    if (characterName.empty()) {
        if (chatManager_) {
            chatManager_->Write(200, false, "[FriendList] Character name not available");
        }
        return;
    }
    
    std::thread([this, characterName]() {
        try {
            if (chatManager_) {
                chatManager_->Write(200, false, "[FriendList] Resetting test database...");
            }
            
            bool success = testRunnerUseCase_->resetTestDatabase(characterName);
            
            if (chatManager_) {
                if (success) {
                    chatManager_->Write(200, false, "[FriendList] Test database reset successfully");
                } else {
                    chatManager_->Write(200, false, "[FriendList] Failed to reset test database");
                }
            }
        } catch (const std::exception& e) {
            if (chatManager_) {
                std::ostringstream msg;
                msg << "Error resetting test database: " << e.what();
                chatManager_->Write(200, false, ("[FriendList] " + msg.str()).c_str());
            }
        }
    }).detach();
}

void AshitaAdapter::sendFriendRequestFromCommand(const std::string& friendName) {
    if (friendName.empty()) {
        if (logger_) {
            logger_->warning("AshitaAdapter: /befriend command called with empty name");
        }
        return;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        if (logger_) {
            logger_->warning("AshitaAdapter: /befriend command called but not connected");
        }
        if (chatManager_) {
            std::string message = "[FriendList] Cannot send friend request: not connected to server";
            chatManager_->Write(200, false, message.c_str());
        }
        return;
    }
    
    logger_->info("[friend] /befriend command: sending request to " + friendName);
    
    std::string normalizedName = friendName;
    std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    handleSendFriendRequest(normalizedName);
    
    pushToGameEcho("Friend request sent to " + friendName);
}

void AshitaAdapter::handleCommand(const UI::WindowCommand& command) {
    if (!initialized_) {
        return;
    }
    
    switch (command.type) {
        case UI::WindowCommandType::SyncFriendList:
            {
                std::lock_guard<std::mutex> lock(pollingMutex_);
                lastFriendListSyncCallsite_ = "ManualRefresh";
            }
            handleSyncFriendListAsync();
            break;
        case UI::WindowCommandType::RefreshStatus:
            handleRefreshStatus();
            break;
        case UI::WindowCommandType::SendFriendRequest:
            handleSendFriendRequest(command.data);
            break;
        case UI::WindowCommandType::AcceptFriendRequest:
            handleAcceptFriendRequest(command.data);
            break;
        case UI::WindowCommandType::RejectFriendRequest:
            handleRejectFriendRequest(command.data);
            break;
        case UI::WindowCommandType::CancelFriendRequest:
            handleCancelFriendRequest(command.data);
            break;
        case UI::WindowCommandType::RemoveFriend:
            handleRemoveFriend(command.data);
            break;
        case UI::WindowCommandType::RemoveFriendVisibility:
            handleRemoveFriendVisibility(command.data);
            break;
        case UI::WindowCommandType::OpenAltVisibility:
            if (windowManager_) {
                windowManager_->getMainWindow().setVisible(true);
                windowManager_->getMainWindow().requestExpandAltVisibilitySection();
                
                handleRefreshAltVisibility();
                
                logger_->debug("AshitaAdapter: Options window opened with Alt Visibility section expanded");
            }
            break;
        case UI::WindowCommandType::RefreshAltVisibility:
            handleRefreshAltVisibility();
            break;
        case UI::WindowCommandType::AddFriendVisibility:
            if (!command.data.empty()) {
                handleAddFriendVisibility(command.data);
            }
            break;
        case UI::WindowCommandType::ToggleFriendVisibility:
            if (!command.data.empty()) {
                handleToggleFriendVisibility(command.data);
            }
            break;
        case UI::WindowCommandType::OpenOptions:
            if (windowManager_) {
                bool currentVisible = windowManager_->getMainWindow().isVisible();
                windowManager_->getMainWindow().setVisible(!currentVisible);
                
                if (!currentVisible && preferencesUseCase_ && optionsViewModel_) {
                    handleLoadPreferences();
                }
                
                if (!currentVisible && themeUseCase_) {
                    logger_->debug("[ui] Opening Options window");
                    themeUseCase_->loadThemes();
                    updateThemesViewModel();
                    logger_->debug("[theme] ViewModel refreshed");
                }
                
                logger_->debug("AshitaAdapter: Options window toggled: " + std::string(currentVisible ? "closed" : "opened"));
            }
            break;
        case UI::WindowCommandType::OpenThemes:
            if (windowManager_) {
                windowManager_->getMainWindow().setVisible(true);
            }
            break;
        case UI::WindowCommandType::ViewFriendDetails:
            if (windowManager_ && !command.data.empty()) {
                windowManager_->getMainWindow().setSelectedFriendForDetails(command.data);
                windowManager_->getQuickOnlineWindow().setSelectedFriendForDetails(command.data);
                logger_->debug("AshitaAdapter: View friend details for " + command.data);
            }
            break;
        case UI::WindowCommandType::OpenNoteEditor:
            if (windowManager_ && !command.data.empty()) {
                UI::NoteEditorWindow& noteEditor = windowManager_->getNoteEditorWindow();
                if (noteEditor.isVisible() && noteEditor.getFriendName() == command.data) {
                    noteEditor.setVisible(false);
                    if (notesViewModel_) {
                        notesViewModel_->closeEditor();
                    }
                    logger_->debug("AshitaAdapter: Note editor closed for " + command.data);
                } else {
                    noteEditor.setFriendName(command.data);
                    noteEditor.setVisible(true);
                    logger_->debug("AshitaAdapter: Note editor opened for " + command.data);
                    
                    if (getNotesUseCase_ && notesViewModel_) {
                        notesViewModel_->setServerMode(false);
                        
                        std::thread([this, friendName = command.data]() {
                            if (notesViewModel_) {
                                notesViewModel_->setLoading(true);
                            }
                            
                            App::UseCases::GetNotesResult result = getNotesUseCase_->getNote(apiKey_, characterName_, friendName, false);
                            
                            if (result.success && !result.notes.empty()) {
                                auto it = result.notes.find(friendName);
                                if (it != result.notes.end()) {
                                    if (notesViewModel_) {
                                        notesViewModel_->loadNote(it->second);
                                        notesViewModel_->setLoading(false);
                                    }
                                } else {
                                    if (notesViewModel_) {
                                        notesViewModel_->clearCurrentNote();
                                        notesViewModel_->setLoading(false);
                                    }
                                }
                            } else {
                                if (notesViewModel_) {
                                    notesViewModel_->clearCurrentNote();
                                    if (!result.error.empty()) {
                                        notesViewModel_->setError(result.error);
                                    }
                                    notesViewModel_->setLoading(false);
                                }
                            }
                        }).detach();
                    }
                }
            }
            break;
        case UI::WindowCommandType::ToggleColumnVisibility:
            handleToggleColumnVisibility(command.data);
            break;
        case UI::WindowCommandType::ApplyTheme:
            try {
                int themeIndex = std::stoi(command.data);
                handleApplyTheme(themeIndex);
            } catch (...) {
                logger_->error("AshitaAdapter: Invalid theme index: " + command.data);
            }
            break;
        case UI::WindowCommandType::SetCustomTheme:
            if (!command.data.empty()) {
                handleSetCustomTheme(command.data);
            } else {
                logger_->error("[theme] SetCustomTheme command missing theme name");
            }
            break;
        case UI::WindowCommandType::UpdateThemeColors:
            handleUpdateThemeColors();
            break;
        case UI::WindowCommandType::UpdateQuickOnlineThemeColors:
            break;
        case UI::WindowCommandType::UpdateNotificationThemeColors:
            break;
        case UI::WindowCommandType::SetBackgroundAlpha:
            try {
                float alpha = std::stof(command.data);
                handleSetBackgroundAlpha(alpha);
            } catch (...) {
                logger_->error("AshitaAdapter: Invalid background alpha: " + command.data);
            }
            break;
        case UI::WindowCommandType::SetTextAlpha:
            try {
                float alpha = std::stof(command.data);
                handleSetTextAlpha(alpha);
            } catch (...) {
                logger_->error("AshitaAdapter: Invalid text alpha: " + command.data);
            }
            break;
        case UI::WindowCommandType::SaveThemeAlpha:
            handleSaveThemeAlpha();
            break;
        case UI::WindowCommandType::SaveCustomTheme:
            handleSaveCustomTheme(command.data);
            break;
        case UI::WindowCommandType::DeleteCustomTheme:
            handleDeleteCustomTheme(command.data);
            break;
        case UI::WindowCommandType::SetCustomThemeByName:
            handleSetCustomThemeByName(command.data);
            break;
        case UI::WindowCommandType::RefreshThemesList:
            logger_->info("AshitaAdapter: RefreshThemesList command received");
            if (themeUseCase_) {
                logger_->info("AshitaAdapter: themeUseCase_ is valid, loading themes");
                themeUseCase_->loadThemes();
                logger_->info("AshitaAdapter: Themes loaded, updating ViewModel");
                updateThemesViewModel();
                logger_->info("AshitaAdapter: Themes list refreshed");
            } else {
                logger_->error("AshitaAdapter: RefreshThemesList called but themeUseCase_ is null");
            }
            break;
        case UI::WindowCommandType::SetThemePreset:
            if (!command.data.empty()) {
                handleSetThemePreset(command.data);
            } else {
                logger_->error("AshitaAdapter: SetThemePreset command missing preset name");
            }
            break;
        case UI::WindowCommandType::LoadPreferences:
            handleLoadPreferences();
            break;
        case UI::WindowCommandType::UpdatePreference:
            handleUpdatePreference(command.data);
            break;
        case UI::WindowCommandType::UpdateWindowLock:
            handleUpdateWindowLock(command.data);
            break;
        case UI::WindowCommandType::SavePreferences:
            handleSavePreferences();
            break;
        case UI::WindowCommandType::ResetPreferences:
            handleResetPreferences();
            break;
        case UI::WindowCommandType::StartCapturingCustomKey:
            startCapturingCustomKey();
            break;
        case UI::WindowCommandType::SaveNote:
            handleSaveNote(command.data);
            break;
        case UI::WindowCommandType::DeleteNote:
            handleDeleteNote(command.data);
            break;
        case UI::WindowCommandType::ToggleDebugWindow:
            {
                bool show = (command.data == "true");
                if (!isDebugEnabled()) {
                    show = false;
                }
                windowManager_->getDebugLogWindow().setVisible(show);
            }
            break;
        case UI::WindowCommandType::SaveServerSelection:
            if (!command.data.empty()) {
                handleSaveServerSelection(command.data);
            }
            break;
        case UI::WindowCommandType::RefreshServerList:
            handleRefreshServerList();
            break;
        default:
            if (logger_) {
                logger_->warning("AshitaAdapter: Unknown command type: " + 
                               std::to_string(static_cast<int>(command.type)));
            }
            break;
    }
}

std::optional<App::Theming::ThemeTokens> AshitaAdapter::getCurrentThemeTokens() const {
    if (!themeUseCase_) {
        return std::nullopt;
    }
    
    return themeUseCase_->getCurrentThemeTokens();
}

Core::CustomTheme AshitaAdapter::getQuickOnlineTheme() const {
    if (!themeUseCase_) {
        return Core::CustomTheme();
    }
    return themeUseCase_->getQuickOnlineTheme();
}

Core::CustomTheme AshitaAdapter::getNotificationTheme() const {
    if (!themeUseCase_) {
        return Core::CustomTheme();
    }
    return themeUseCase_->getNotificationTheme();
}

void AshitaAdapter::updateQuickOnlineThemeColors(const Core::CustomTheme& colors) {
    if (!themeUseCase_) {
        return;
    }
    themeUseCase_->updateQuickOnlineThemeColors(colors);
}

void AshitaAdapter::updateNotificationThemeColors(const Core::CustomTheme& colors) {
    if (!themeUseCase_) {
        return;
    }
    themeUseCase_->updateNotificationThemeColors(colors);
}

uint32_t AshitaAdapter::getFlags() const {
#ifdef BUILDING_TESTS
    return 0;  // Not used in tests
#else
    return static_cast<uint32_t>(::Ashita::PluginFlags::UseDirect3D);
#endif
}

Core::Presence AshitaAdapter::queryPlayerPresence() const {
    Core::Presence presence;
    presence.characterName = getCharacterName();
    
#ifdef BUILDING_TESTS
    return presence;
#else
    if (ashitaCore_ == nullptr) {
        return presence;
    }
    
    IMemoryManager* memoryMgr = ashitaCore_->GetMemoryManager();
    if (memoryMgr == nullptr) {
        return presence;
    }
    
    IParty* party = memoryMgr->GetParty();
    bool isAnonymous = false;
    
    IPlayer* player = memoryMgr->GetPlayer();
    
    if (party != nullptr) {
        uint8_t partyMainJob = party->GetMemberMainJob(0);
        uint8_t partyMainJobLevel = party->GetMemberMainJobLevel(0);
        
        if (partyMainJob == 0 || partyMainJobLevel == 0) {
            isAnonymous = true;
        }
    }
    
    if (player == nullptr) {
        if (party != nullptr) {
            uint8_t mainJob = party->GetMemberMainJob(0);
            uint8_t mainJobLevel = party->GetMemberMainJobLevel(0);
            uint8_t subJob = party->GetMemberSubJob(0);
            uint8_t subJobLevel = party->GetMemberSubJobLevel(0);
            uint16_t zoneId = party->GetMemberZone(0);
            
            presence.job = formatJobString(mainJob, mainJobLevel, subJob, subJobLevel);
            presence.zone = getZoneNameFromId(zoneId);
        }
        presence.isAnonymous = isAnonymous;
        return presence;
    }
    
    ::Ashita::FFXI::player_t* playerData = player->GetRawStructure();
    if (playerData != nullptr) {
        uint8_t mainJob = playerData->MainJob;
        uint8_t mainJobLevel = playerData->MainJobLevel;
        uint8_t subJob = playerData->SubJob;
        uint8_t subJobLevel = playerData->SubJobLevel;
        presence.job = formatJobString(mainJob, mainJobLevel, subJob, subJobLevel);
        
        uint16_t playerRank = playerData->Rank;
        if (playerRank > 0) {
            presence.rank = "Rank " + std::to_string(playerRank);
        }
        presence.nation = static_cast<int>(playerData->Nation);
        
        if (!isAnonymous && (mainJob == 0 || mainJobLevel == 0)) {
            isAnonymous = true;
        }
    }
    
    presence.isAnonymous = isAnonymous;
    
    {
        std::lock_guard<std::mutex> lock(zoneCacheMutex_);
        if (!cachedZoneName_.empty()) {
            presence.zone = cachedZoneName_;
        } else {
            if (party != nullptr) {
                uint16_t zoneId = party->GetMemberZone(0);
                if (zoneId > 0) {
                    presence.zone = getZoneNameFromId(zoneId);
                    cachedZoneId_ = zoneId;
                    cachedZoneName_ = presence.zone;
                }
            }
        }
    }
    
    presence.timestamp = clock_->nowMs();
    
    return presence;
#endif
}

std::string AshitaAdapter::getCharacterName() const {
#ifdef BUILDING_TESTS
    return characterName_;
#else
    if (ashitaCore_ == nullptr) {
        return characterName_;
    }
    
    IMemoryManager* memoryMgr = ashitaCore_->GetMemoryManager();
    if (memoryMgr == nullptr) {
        return characterName_;
    }
    
    IParty* party = memoryMgr->GetParty();
    if (party != nullptr) {
        const char* playerName = party->GetMemberName(0);
        if (playerName != nullptr && strlen(playerName) > 0) {
            std::string name(playerName);
            std::transform(name.begin(), name.end(), name.begin(), 
                          [](unsigned char c) { return static_cast<char>(::tolower(c)); });
            return name;
        }
    }
    
    IEntity* entityMgr = memoryMgr->GetEntity();
    if (entityMgr != nullptr) {
        IResourceManager* resourceMgr = ashitaCore_->GetResourceManager();
        if (resourceMgr != nullptr) {
            uint32_t entityCount = resourceMgr->GetEntityCount();
            for (uint32_t i = 0; i < entityCount; i++) {
                uint8_t entityType = entityMgr->GetType(i);
                const char* namePtr = entityMgr->GetName(i);
                if (entityType == 0 && namePtr != nullptr && strlen(namePtr) > 0) {
                    std::string name(namePtr);
                    std::transform(name.begin(), name.end(), name.begin(),
                                  [](unsigned char c) { return static_cast<char>(::tolower(c)); });
                    return name;
                }
            }
        }
    }
    
    return characterName_;
#endif
}

void AshitaAdapter::showErrorNotification(const std::string& message, const std::string& context) {
    std::string logMessage = context.empty() 
        ? "Error: " + message 
        : "AshitaAdapter: " + context + " - " + message;
    
    if (logger_) {
        logger_->error(logMessage);
    }
    
    if (notificationUseCase_ && clock_) {
        int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
        auto toast = notificationUseCase_->createErrorToast(message, currentTime);
        UI::Notifications::ToastManager::getInstance().addToast(toast);
    }
}

void AshitaAdapter::updatePresenceAsync() {
    if (backgroundPausedForTests_.load()) {
        return;
    }
    
    if (shouldBlockNetworkOperation()) {
        return;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        return;
    }
    
    JobScope jobScope(activeJobs_, idleWaitMutex_, idleWaitCondition_);
    
    Core::Presence presence = queryPlayerPresence();
    if (presence.characterName.empty()) {
        return;
    }
    
    if (preferencesUseCase_) {
        bool gameIsAnonymous = presence.isAnonymous;
        bool shareJobWhenAnonymous = preferencesUseCase_->getPreferences().shareJobWhenAnonymous;
        presence.isAnonymous = gameIsAnonymous && !shareJobWhenAnonymous;
    }
    
    std::string requestJson = Protocol::RequestEncoder::encodeUpdatePresence(presence);
    std::string url = netClient_->getBaseUrl() + "/api/characters/state";
    
    netClient_->postAsync(url, apiKey_, presence.characterName, requestJson,
        [this](const App::HttpResponse& response) {
            if (!response.isSuccess()) {
                logger_->error("AshitaAdapter: Failed to update presence: " + response.error);
            }
        });
}

void AshitaAdapter::presenceHeartbeatTick() {
    if (backgroundPausedForTests_.load()) {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        presenceUpdateInFlight_ = false;
        return;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty() || characterName_.empty()) {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        presenceUpdateInFlight_ = false;
        return;
    }

    JobScope jobScope(activeJobs_, idleWaitMutex_, idleWaitCondition_);

    const uint64_t lastEventTs = lastHeartbeatEventTimestamp_;
    const uint64_t lastReqTs = lastHeartbeatRequestEventTimestamp_;
    std::string pluginVersion = hasReportedVersion_ ? "" : std::string(Plugin::PLUGIN_VERSION_STRING);
    std::string requestJson = Protocol::RequestEncoder::encodeGetHeartbeat(characterName_, lastEventTs, lastReqTs, pluginVersion);
    if (!hasReportedVersion_) {
        hasReportedVersion_ = true;
    }
    std::string url = netClient_->getBaseUrl() + "/api/heartbeat";

    netClient_->postAsync(url, apiKey_, characterName_, requestJson,
        [this](const App::HttpResponse& response) {
            if (!response.isSuccess()) {
                logger_->warning("AshitaAdapter: Heartbeat failed: " + (response.error.empty() ? "HTTP " + std::to_string(response.statusCode) : response.error));
                std::lock_guard<std::mutex> lock(pollingMutex_);
                presenceUpdateInFlight_ = false;
                return;
            }

            Protocol::ResponseMessage msg;
            Protocol::DecodeResult decodeResult = Protocol::ResponseDecoder::decode(response.body, msg);
            if (decodeResult != Protocol::DecodeResult::Success || !msg.success || msg.type != Protocol::ResponseType::Heartbeat) {
                logger_->warning("AshitaAdapter: Heartbeat decode failed");
                std::lock_guard<std::mutex> lock(pollingMutex_);
                presenceUpdateInFlight_ = false;
                return;
            }

            Protocol::HeartbeatResponsePayload payload;
            Protocol::DecodeResult payloadResult = Protocol::ResponseDecoder::decodeHeartbeatPayload(msg.payload, payload);
            if (payloadResult != Protocol::DecodeResult::Success) {
                logger_->warning("AshitaAdapter: Heartbeat payload decode failed");
                std::lock_guard<std::mutex> lock(pollingMutex_);
                presenceUpdateInFlight_ = false;
                return;
            }

            std::string isOutdatedStr;
            std::string latestVersion;
            std::string releaseUrl;
            Protocol::JsonUtils::extractField(response.body, "is_outdated", isOutdatedStr);
            Protocol::JsonUtils::extractField(response.body, "latest_version", latestVersion);
            Protocol::JsonUtils::extractField(response.body, "release_url", releaseUrl);
            
            bool isOutdated = false;
            if (!isOutdatedStr.empty()) {
                isOutdated = (isOutdatedStr == "true" || isOutdatedStr == "1");
            }
            
            if (isOutdated && !latestVersion.empty()) {
                App::UseCases::HeartbeatResult heartbeatResult;
                heartbeatResult.isOutdated = isOutdated;
                heartbeatResult.latestVersion = latestVersion;
                heartbeatResult.releaseUrl = releaseUrl;
                
                std::string warningMessage;
                if (presenceUseCase_->shouldShowOutdatedWarning(heartbeatResult, warningMessage)) {
                    if (chatManager_) {
                        chatManager_->Write(200, false, warningMessage.c_str());
                    }
                }
            }

            std::vector<Core::FriendStatus> statuses;
            statuses.reserve(payload.statuses.size());
            for (const auto& statusData : payload.statuses) {
                Core::FriendStatus status;
                status.characterName = statusData.characterName;
                status.displayName = statusData.displayName.empty() ? statusData.characterName : statusData.displayName;
                status.isOnline = statusData.isOnline;
                status.job = statusData.job;
                status.rank = statusData.rank;
                status.nation = statusData.nation;
                status.zone = statusData.zone;
                status.lastSeenAt = statusData.lastSeenAt;
                status.showOnlineStatus = statusData.showOnlineStatus;
                status.isLinkedCharacter = statusData.isLinkedCharacter;
                status.isOnAltCharacter = statusData.isOnAltCharacter;
                status.altCharacterName = statusData.altCharacterName;
                status.friendedAs = statusData.friendedAs;
                status.linkedCharacters = statusData.linkedCharacters;
                statuses.push_back(status);
            }

            if (logger_ && !statuses.empty() && isDebugEnabled()) {
                bool expected = false;
                if (statusFieldDumpLogged_.compare_exchange_strong(expected, true)) {
                    const auto& s = statuses.front();
                    logger_->debug("[FriendList][DecodeStatus] char=" + s.characterName +
                        " display=" + s.displayName +
                        " online=" + std::string(s.isOnline ? "true" : "false") +
                        " showOnline=" + std::string(s.showOnlineStatus ? "true" : "false") +
                        " job='" + s.job + "'" +
                        " rank='" + s.rank + "'" +
                        " nation=" + std::to_string(s.nation) +
                        " zone='" + s.zone + "'" +
                        " lastSeenAt=" + std::to_string(static_cast<unsigned long long>(s.lastSeenAt)));
                }
            }

            lastHeartbeatEventTimestamp_ = payload.lastEventTimestamp;
            lastHeartbeatRequestEventTimestamp_ = payload.lastRequestEventTimestamp;

            const uint64_t currentTime = clock_ ? clock_->nowMs() : 0;
            std::vector<Core::FriendStatus> mergedStatusesForNotifications;
            {
                std::lock_guard<std::mutex> lock(stateMutex_);
                if (!cachedFriendList_.empty() && !statuses.empty()) {
                    auto toLowerCopy = [](const std::string& input) -> std::string {
                        std::string out = input;
                        std::transform(out.begin(), out.end(), out.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        return out;
                    };

                    auto mergeStatus = [](Core::FriendStatus& dst, const Core::FriendStatus& src) {
                        dst.isOnline = src.isOnline;
                        dst.showOnlineStatus = src.showOnlineStatus;
                        if (src.lastSeenAt != 0) {
                            dst.lastSeenAt = src.lastSeenAt;
                        }
                        if (!src.displayName.empty()) {
                            dst.displayName = src.displayName;
                        }

                        if (!src.job.empty()) {
                            dst.job = src.job;
                        }
                        if (!src.rank.empty()) {
                            dst.rank = src.rank;
                        }
                        if (!src.zone.empty()) {
                            dst.zone = src.zone;
                        }
                        if (src.nation != 0) {
                            dst.nation = src.nation;
                        }

                        dst.isLinkedCharacter = src.isLinkedCharacter;
                        dst.isOnAltCharacter = src.isOnAltCharacter;
                        if (!src.altCharacterName.empty()) {
                            dst.altCharacterName = src.altCharacterName;
                        }
                        if (!src.friendedAs.empty()) {
                            dst.friendedAs = src.friendedAs;
                        }
                        if (!src.linkedCharacters.empty()) {
                            dst.linkedCharacters = src.linkedCharacters;
                        }
                    };

                    for (const auto& hb : statuses) {
                        const std::string hbKey = toLowerCopy(hb.characterName);
                        Core::FriendStatus* existing = nullptr;
                        for (auto& s : cachedFriendStatuses_) {
                            if (toLowerCopy(s.characterName) == hbKey) {
                                existing = &s;
                                break;
                            }
                        }
                        if (existing != nullptr) {
                            mergeStatus(*existing, hb);
                        } else {
                            cachedFriendStatuses_.push_back(hb);
                        }
                    }

                    if (viewModel_) {
                        viewModel_->updateWithRequests(cachedFriendList_, cachedFriendStatuses_, cachedOutgoingRequests_, currentTime);
                    }

                    if (quickOnlineViewModel_) {
                        const auto friendNames = cachedFriendList_.getFriendNames();
                        const auto onlineNames = Core::FriendListFilter::filterOnline(friendNames, cachedFriendStatuses_);
                        std::set<std::string> onlineSet;
                        for (const auto& n : onlineNames) {
                            onlineSet.insert(toLowerCopy(n));
                        }

                        Core::FriendList onlineList;
                        for (const auto& f : cachedFriendList_.getFriends()) {
                            if (onlineSet.find(toLowerCopy(f.name)) != onlineSet.end()) {
                                onlineList.addFriend(f);
                            }
                        }
                        quickOnlineViewModel_->updateWithRequests(onlineList, cachedFriendStatuses_, cachedOutgoingRequests_, cachedIncomingRequests_, currentTime);
                    }

                    mergedStatusesForNotifications = cachedFriendStatuses_;
                }
            }

            checkForStatusChanges(mergedStatusesForNotifications);

            std::lock_guard<std::mutex> lock(pollingMutex_);
            presenceUpdateInFlight_ = false;
        });
}

void AshitaAdapter::fullRefreshTick() {
    if (backgroundPausedForTests_.load()) {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        fullRefreshInFlight_ = false;
        return;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        fullRefreshInFlight_ = false;
        return;
    }
    
    logger_->debug("AshitaAdapter: Full refresh tick triggered");
    updatePresenceAsync();
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        lastFriendListSyncCallsite_ = "PollingTimer";
    }
    handleSyncFriendListAsync();
    handleGetFriendRequests();
    
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        fullRefreshInFlight_ = false;
    }
}

std::string AshitaAdapter::getVersionString() const {
    return std::string(Plugin::PLUGIN_VERSION_STRING);
}

void AshitaAdapter::playerDataChangeDetectionTick() {
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        return;
    }
    
    Core::Presence currentPresence = queryPlayerPresence();
    if (currentPresence.characterName.empty()) {
        return;
    }
    
    
    {
        std::lock_guard<std::mutex> lock(zoneCacheMutex_);
    }
}

void AshitaAdapter::attemptAutoConnectAsync() {
    if (autoConnectAttempted_ || autoConnectInProgress_) {
        logger_->debug("AshitaAdapter: Auto-connect already attempted or in progress, skipping");
        return;  // Already attempted or in progress
    }
    
    std::string name = getCharacterName();
    if (name.empty()) {
        logger_->debug("AshitaAdapter: Character name not available yet, will retry in next update");
        return;
    }
    
    autoConnectInProgress_ = true;
    autoConnectAttempted_ = true;
    
    viewModel_->setConnectionState(App::ConnectionState::Connecting);
    logger_->info("AshitaAdapter: Starting async auto-connect for " + name + " (server: " + netClient_->getBaseUrl() + ")");
    
    std::thread([this, name]() {
        attemptAutoConnectWorker(name);
    }).detach();
}

// Flow:
//   1. Attempt auto-connect (loads API key from store, attempts login/register)
//   2. If successful: set warmup flags, warm zone name table, load notified mail IDs, initialize mail/notes stores
//   3. Store result for main thread to process (mutex-protected)
//   4. Main thread updates ViewModel and triggers friend list sync
// Invariants:
//   - Runs in worker thread (no Ashita/ImGui APIs)
//   - Warmup state set BEFORE autoConnectCompleted_ flag (main thread gates on warmup)
//   - All disk I/O happens here (not on render thread)
//   - Uses server-provided username as canonical character name for caches
// Edge cases:
//   - Best-effort warmup (failures don't block connection)
//   - Notes store initialization deferred until account ID available
//   - Zone name table warmup prevents first-query hitch (static table initialization)
void AshitaAdapter::attemptAutoConnectWorker(const std::string& characterName) {
    JobScope jobScope(activeJobs_, idleWaitMutex_, idleWaitCondition_);
    
    PERF_SCOPE("AutoConnectWorker::connectUseCase_->autoConnect (bg)");
    App::UseCases::ConnectResult result = connectUseCase_->autoConnect(characterName);
    
    ApiKeyPersistence::saveToFile(apiKeyState_);

    if (result.success) {
        localCacheWarmupInProgress_.store(true);
        localCacheWarmupCompleted_.store(false);
    }
    
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        pendingConnectResult_ = result;
        autoConnectCompleted_ = true;
        autoConnectInProgress_ = false;
    }

    if (result.success) {
        const std::string cacheCharacterName = result.username;

        {
            PERF_SCOPE("AutoConnectWorker::warmup getZoneNameFromId static table");
            try {
                (void)getZoneNameFromId(0);
            } catch (...) {
            }
        }

        localCacheWarmupInProgress_.store(false);
        localCacheWarmupCompleted_.store(true);
    } else {
        localCacheWarmupInProgress_.store(false);
        localCacheWarmupCompleted_.store(true);
    }
    
    if (result.success) {
        logger_->info("[char] Auto-connect completed: " + characterName);
    } else {
        logger_->error("AshitaAdapter: Auto-connect worker failed for " + characterName + ": " + result.error);
    }
}

// Flow:
//   1. Check if already in-flight (prevent duplicates)
//   2. Validate connection state
//   3. Launch worker thread to fetch friend list with statuses
//   4. Worker thread: fetch friend list, get requests, update ViewModels (mutex-protected), check status changes
//   5. Clear in-flight flag on completion
// Invariants:
//   - Guarded by friendListSyncInFlight_ flag to prevent concurrent requests
//   - ViewModel updates happen in worker thread with mutex protection
//   - Quick Online ViewModel filtered to online-only friends (case-insensitive matching)
//   - Status change notifications fired after ViewModel updates
// Edge cases:
//   - Handles both incoming and outgoing requests in friend list display
//   - Background sync errors don't show notifications (only user actions show errors)
//   - Request ID tracking for instrumentation/debugging
//   - Notes upload to server disabled (always uses local)
void AshitaAdapter::handleSyncFriendListAsync() {
    if (backgroundPausedForTests_.load()) {
        return;
    }
    
    if (shouldBlockNetworkOperation()) {
        return;
    }
    
    uint64_t requestId;
    std::string callsite;
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        if (friendListSyncInFlight_) {
            logger_->debug("AshitaAdapter: Friend list sync already in-flight (callsite: " + lastFriendListSyncCallsite_ + 
                          ", requestId: " + std::to_string(friendListSyncRequestId_) + "), skipping duplicate");
            return;
        }
        friendListSyncInFlight_ = true;
        friendListSyncRequestId_++;
        requestId = friendListSyncRequestId_;
        lastFriendListSyncTimestamp_ = clock_->nowMs();
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty() || characterName_.empty()) {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        friendListSyncInFlight_ = false;
        return;
    }
    
    std::thread([this, requestId]() {
        JobScope jobScope(activeJobs_, idleWaitMutex_, idleWaitCondition_);
        std::string apiKey = apiKey_;  // Copy to avoid race conditions
        std::string charName = characterName_;  // Copy to avoid race conditions
        
        logger_->debug("AshitaAdapter: [FriendListSync] requestId=" + std::to_string(requestId) + 
                      " callsite=" + lastFriendListSyncCallsite_ + 
                      " timestamp=" + std::to_string(lastFriendListSyncTimestamp_));
        
        auto result = syncUseCase_->getFriendListWithStatuses(apiKey, charName, *presenceUseCase_);
        
        if (result.success) {
            std::vector<Core::FriendStatus> statuses = result.statuses;

            if (logger_ && !statuses.empty() && isDebugEnabled()) {
                bool expected = false;
                if (statusFieldDumpLogged_.compare_exchange_strong(expected, true)) {
                    const auto& s = statuses.front();
                    logger_->debug("[FriendList][DecodeStatus] char=" + s.characterName +
                        " display=" + s.displayName +
                        " online=" + std::string(s.isOnline ? "true" : "false") +
                        " showOnline=" + std::string(s.showOnlineStatus ? "true" : "false") +
                        " job='" + s.job + "'" +
                        " rank='" + s.rank + "'" +
                        " nation=" + std::to_string(s.nation) +
                        " zone='" + s.zone + "'" +
                        " lastSeenAt=" + std::to_string(static_cast<unsigned long long>(s.lastSeenAt)));
                }
            }
            
            auto requestsResult = getRequestsUseCase_->getRequests(apiKey, charName);
            std::vector<Protocol::FriendRequestPayload> outgoingRequests = requestsResult.success ? requestsResult.outgoing : std::vector<Protocol::FriendRequestPayload>();
            std::vector<Protocol::FriendRequestPayload> incomingRequests = requestsResult.success ? requestsResult.incoming : std::vector<Protocol::FriendRequestPayload>();
            
            {
                std::lock_guard<std::mutex> lock(stateMutex_);
                cachedFriendList_ = result.friendList;
                cachedOutgoingRequests_ = outgoingRequests;
                cachedIncomingRequests_ = incomingRequests;
                cachedFriendStatuses_ = statuses;
                
                if (viewModel_) {
                    uint64_t currentTime = clock_ ? clock_->nowMs() : 0;
                    viewModel_->updateWithRequests(result.friendList, statuses, outgoingRequests, currentTime);
                }

                if (quickOnlineViewModel_) {
                    uint64_t currentTime = clock_ ? clock_->nowMs() : 0;

                    const auto friendNames = result.friendList.getFriendNames();
                    const auto onlineNames = Core::FriendListFilter::filterOnline(friendNames, statuses);
                    std::set<std::string> onlineSet;
                    for (const auto& n : onlineNames) {
                        std::string lower = n;
                        std::transform(lower.begin(), lower.end(), lower.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        onlineSet.insert(lower);
                    }

                    Core::FriendList onlineList;
                    for (const auto& f : result.friendList.getFriends()) {
                        std::string lower = f.name;
                        std::transform(lower.begin(), lower.end(), lower.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        if (onlineSet.find(lower) != onlineSet.end()) {
                            onlineList.addFriend(f);
                        }
                    }

                    std::vector<Protocol::FriendRequestPayload> incomingRequestsForQuickOnline = requestsResult.success ? requestsResult.incoming : std::vector<Protocol::FriendRequestPayload>();
                    std::vector<Protocol::FriendRequestPayload> outgoingRequestsForQuickOnline = requestsResult.success ? requestsResult.outgoing : std::vector<Protocol::FriendRequestPayload>();
                    quickOnlineViewModel_->updateWithRequests(onlineList, statuses, outgoingRequestsForQuickOnline, incomingRequestsForQuickOnline, currentTime);
                }
                
                if (requestsResult.success) {
                    viewModel_->updatePendingRequests(requestsResult.incoming, requestsResult.outgoing);
                }
            }
            
            checkForStatusChanges(statuses);
            
            logger_->info("AshitaAdapter: Friend list synced");
            
            //         // Load all notes in background (non-blocking)
        } else {
            std::lock_guard<std::mutex> lock(stateMutex_);
            viewModel_->setErrorMessage(result.error);
            logger_->error("AshitaAdapter: Failed to sync friend list: " + result.error);
            
        }
        
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            friendListSyncInFlight_ = false;
            logger_->debug("AshitaAdapter: [FriendListSync] requestId=" + std::to_string(requestId) + 
                          " completed (success=" + (result.success ? "true" : "false") + ")");
        }
    }).detach();
}


void AshitaAdapter::handleRefreshStatus() {
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        return;
    }
    
    logger_->info("[sync] Full refresh triggered");
    
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        if (fullRefreshInFlight_) {
            logger_->debug("[sync] Refresh already in-flight, skipping");
            return;
        }
        fullRefreshInFlight_ = true;
        lastFullRefresh_ = clock_->nowMs();
    }
    
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        lastFriendListSyncCallsite_ = "RefreshButton";
    }
    handleSyncFriendListAsync();
    handleGetFriendRequests();
    updatePresenceAsync();
    
    std::lock_guard<std::mutex> lock(pollingMutex_);
    fullRefreshInFlight_ = false;
}

// Flow:
//   1. Parse friend name and optional note text from command data
//   2. Validate connection state and inputs
//   3. Launch worker thread to send request (non-blocking)
//   4. Worker thread: send request, save note if provided, update ViewModel, show notifications
// Invariants:
//   - Must be connected with valid API key before sending
//   - ViewModel updates happen on worker thread with mutex protection
//   - Notifications shown outside mutex (ToastManager is thread-safe)
// Edge cases:
//   - Handles different action types (PENDING_ACCEPT, ALREADY_VISIBLE, etc.) with appropriate messages
//   - Removes optimistic pending request on failure
//   - Note saving happens after successful request (local-only, useServerNotes is false)
//   - Error notifications shown for failures, success messages via ViewModel status
void AshitaAdapter::handleSendFriendRequest(const std::string& commandData) {
    if (shouldBlockNetworkOperation()) {
        rerouteToServerSelectionIfNeeded();
        return;
    }
    
    std::string friendName;
    std::string noteText;
    
    size_t delimiterPos = commandData.find('|');
    if (delimiterPos != std::string::npos) {
        friendName = commandData.substr(0, delimiterPos);
        noteText = commandData.substr(delimiterPos + 1);
    } else {
        friendName = commandData;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty() || friendName.empty()) {
        logger_->warning("[friend] Cannot send request - not connected or invalid name");
        return;
    }
    
    logger_->info("[friend] Sending request to " + friendName);
    
    bool debugMode = isDebugEnabled();
    
    try {
        std::thread([this, friendName, noteText, debugMode]() {
        std::string apiKey = apiKey_;
        std::string charName = characterName_;
        
        auto result = sendRequestUseCase_->sendRequest(apiKey, charName, friendName);
        
        uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
        
        if (result.success) {
            if (!result.debugMessage.empty()) {
                logger_->info("AshitaAdapter: " + result.debugMessage);
            }
            logger_->info("[friend] Request sent to " + friendName);
        } else {
            if (!result.debugMessage.empty()) {
                logger_->error("[friend] " + result.debugMessage);
            }
            logger_->error("[friend] Failed to send request: " + result.userMessage);
        }
        
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            
            if (result.success) {
                if (!noteText.empty() && saveNoteUseCase_) {
                    logger_->debug("[friend] Saving note for " + friendName);
                    App::UseCases::SaveNoteResult noteResult = saveNoteUseCase_->saveNote(apiKey, charName, friendName, noteText, false);
                    if (!noteResult.success) {
                        logger_->warning("[friend] Failed to save note: " + noteResult.error);
                    }
                }
                
                if (viewModel_) {
                    std::string successMessage;
                    if (!result.action.empty()) {
                        if (result.action == "PENDING_ACCEPT") {
                            successMessage = "Friend request sent to " + friendName + ".";
                        } else if (result.action == "ALREADY_VISIBLE") {
                            successMessage = "Already friends with " + friendName + ".";
                        } else if (result.action == "VISIBILITY_GRANTED") {
                            successMessage = "Visibility granted for " + friendName + ".";
                        } else if (result.action == "VISIBILITY_REQUEST_SENT") {
                            successMessage = "Visibility request sent to " + friendName + ".";
                        } else {
                            successMessage = !result.message.empty() ? result.message : "Request sent to " + friendName + ".";
                        }
                    } else {
                        successMessage = !result.message.empty() ? result.message : "Request sent to " + friendName + ".";
                    }
                    viewModel_->setActionStatusSuccess(successMessage, timestampMs);
                }
            } else {
                if (viewModel_) {
                    viewModel_->removeOptimisticPendingRequest(friendName);
                    viewModel_->setActionStatusError(result.userMessage, result.errorCode, timestampMs);
                }
            }
        }
        
        if (result.success) {
            // Debug echo (thread-safe debug log only, use cached value)
            if (debugMode) {
                std::string fullMessage = "[FriendList] Friend request sent to " + friendName;
                Debug::DebugLog::getInstance().push(fullMessage);
                logger_->info(fullMessage);
            }
        } else {
            showErrorNotification("Failed to send request: " + result.userMessage, "SendFriendRequest");
            
            pendingChatEchoError_ = "Failed to send friend request to " + friendName + ": " + result.userMessage;
            
            // Debug echo for errors (thread-safe debug log only, use cached value)
            if (debugMode) {
                std::string fullMessage = "[FriendList] Failed to send friend request: " + result.userMessage;
                Debug::DebugLog::getInstance().push(fullMessage);
                logger_->error(fullMessage);
            }
        }
        }).detach();
    } catch (const std::exception& e) {
        logger_->error("[friend] Exception creating thread: " + std::string(e.what()));
    } catch (...) {
        logger_->error("[friend] Unknown exception creating thread");
    }
}

void AshitaAdapter::handleAcceptFriendRequest(const std::string& requestId) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || requestId.empty()) {
        return;
    }
    
    logger_->info("[friend] Accepting request " + requestId);
    
    std::string friendName;
    if (viewModel_) {
        const auto& incomingRequests = viewModel_->getIncomingRequests();
        for (const auto& request : incomingRequests) {
            if (request.requestId == requestId) {
                friendName = request.fromCharacterName;
                break;
            }
        }
    }
    
    auto result = acceptRequestUseCase_->acceptRequest(apiKey_, characterName_, requestId);
    uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
    
    if (result.success) {
        if (!result.debugMessage.empty()) {
            logger_->info("AshitaAdapter: " + result.debugMessage);
        }
        logger_->info("[friend] Request accepted");
        
        if (isDebugEnabled()) {
            std::string displayName = friendName;
            if (displayName.empty()) {
                displayName = result.friendName;
                if (displayName.empty()) {
                    displayName = result.userMessage;
                    size_t pos = displayName.find(" is now your friend");
                    if (pos != std::string::npos) {
                        displayName = displayName.substr(0, pos);
                    }
                }
            }
            pushDebugLog("Added friend " + displayName);
        }
        
        
        {
            std::string displayName = friendName;
            if (displayName.empty()) {
                displayName = result.friendName;
                if (displayName.empty()) {
                    displayName = result.userMessage;
                    size_t pos = displayName.find(" is now your friend");
                    if (pos != std::string::npos) {
                        displayName = displayName.substr(0, pos);
                    }
                }
            }
            if (!displayName.empty()) {
                displayName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(displayName[0])));
            }
            
            if (notificationUseCase_ && clock_) {
                int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                auto toast = notificationUseCase_->createFriendRequestAcceptedToast(displayName, currentTime);
                UI::Notifications::ToastManager::getInstance().addToast(toast);
                
                if (isDebugEnabled()) {
                    pushDebugLog("[Notifications] Friend request accepted for " + displayName + " - toast created");
                    logger_->debug("[Notifications] Friend request accepted: " + displayName + ", toast count: " + 
                        std::to_string(UI::Notifications::ToastManager::getInstance().getToastCount()));
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            lastFriendListSyncCallsite_ = "AcceptFriendRequest";
        }
        handleSyncFriendListAsync();
        handleGetFriendRequests();
        
        if (windowManager_ && windowManager_->getMainWindow().isVisible()) {
            handleRefreshAltVisibility();
        }
        
        if (isDebugEnabled()) {
            pushDebugLog("Friend request accepted - " + result.userMessage);
        }
    } else {
        if (!result.debugMessage.empty()) {
            logger_->error("AshitaAdapter: " + result.debugMessage);
        }
        logger_->error("[friend] Failed to accept request: " + result.userMessage);
        
        if (viewModel_) {
            viewModel_->setActionStatusError(result.userMessage, result.errorCode, timestampMs);
        }
        
        showErrorNotification("Failed to accept request: " + result.userMessage, "AcceptFriendRequest");
    }
}

void AshitaAdapter::handleRejectFriendRequest(const std::string& requestId) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || requestId.empty()) {
        return;
    }
    
    logger_->info("[friend] Rejecting request " + requestId);
    
    auto result = rejectRequestUseCase_->rejectRequest(apiKey_, characterName_, requestId);
    uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
    
    if (result.success) {
        if (!result.debugMessage.empty()) {
            logger_->info("AshitaAdapter: " + result.debugMessage);
        }
        logger_->info("[friend] Request rejected");
        
        if (viewModel_) {
            viewModel_->setActionStatusSuccess(result.userMessage, timestampMs);
        }
        
        if (notificationUseCase_ && clock_) {
            int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
            auto toast = notificationUseCase_->createInfoToast(
                std::string(App::Notifications::Constants::TITLE_FRIEND_REQUEST),
                std::string(App::Notifications::Constants::MESSAGE_FRIEND_REQUEST_REJECTED),
                currentTime);
            UI::Notifications::ToastManager::getInstance().addToast(toast);
        }
        
        handleGetFriendRequests();
        
        if (isDebugEnabled()) {
            pushDebugLog("Friend request rejected");
        }
    } else {
        if (!result.debugMessage.empty()) {
            logger_->error("AshitaAdapter: " + result.debugMessage);
        }
        logger_->error("[friend] Failed to reject request: " + result.userMessage);
        
        if (viewModel_) {
            viewModel_->setActionStatusError(result.userMessage, result.errorCode, timestampMs);
        }
        
        showErrorNotification("Failed to reject request: " + result.userMessage, "RejectFriendRequest");
    }
}

void AshitaAdapter::handleCancelFriendRequest(const std::string& requestId) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || requestId.empty()) {
        return;
    }
    
    logger_->info("[friend] Canceling request " + requestId);
    
    auto result = cancelRequestUseCase_->cancelRequest(apiKey_, characterName_, requestId);
    uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
    
    if (result.success) {
        if (!result.debugMessage.empty()) {
            logger_->info("AshitaAdapter: " + result.debugMessage);
        }
        logger_->info("[friend] Request canceled");
        
        if (viewModel_) {
            viewModel_->setActionStatusSuccess(result.userMessage, timestampMs);
        }
        
        if (notificationUseCase_ && clock_) {
            int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
            auto toast = notificationUseCase_->createInfoToast(
                std::string(App::Notifications::Constants::TITLE_FRIEND_REQUEST),
                std::string(App::Notifications::Constants::MESSAGE_FRIEND_REQUEST_CANCELED),
                currentTime);
            UI::Notifications::ToastManager::getInstance().addToast(toast);
        }
        
        {
            std::lock_guard<std::mutex> lock(pollingMutex_);
            lastFriendListSyncCallsite_ = "CancelFriendRequest";
        }
        handleSyncFriendListAsync();
        handleGetFriendRequests();
        
        if (isDebugEnabled()) {
            pushDebugLog("Friend request cancelled");
        }
    } else {
        if (!result.debugMessage.empty()) {
            logger_->error("AshitaAdapter: " + result.debugMessage);
        }
        logger_->error("[friend] Failed to cancel request: " + result.userMessage);
        
        if (viewModel_) {
            viewModel_->setActionStatusError(result.userMessage, result.errorCode, timestampMs);
        }
        
        showErrorNotification("Failed to cancel request: " + result.userMessage, "CancelFriendRequest");
    }
}

void AshitaAdapter::handleGetFriendRequests() {
    if (backgroundPausedForTests_.load()) {
        return;
    }
    
    if (!connectUseCase_->isConnected() || apiKey_.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (friendRequestsSyncInProgress_) {
            return;
        }
        friendRequestsSyncInProgress_ = true;
        friendRequestsSyncCompleted_ = false;
    }

    const std::string apiKeyCopy = apiKey_;
    const std::string characterNameCopy = characterName_;
    std::thread([this, apiKeyCopy, characterNameCopy]() {
        JobScope jobScope(activeJobs_, idleWaitMutex_, idleWaitCondition_);
        App::UseCases::GetFriendRequestsResult result = getRequestsUseCase_->getRequests(apiKeyCopy, characterNameCopy);
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            pendingFriendRequestsResult_ = result;
            friendRequestsSyncCompleted_ = true;
        }
    }).detach();
}

void AshitaAdapter::handleRemoveFriend(const std::string& friendName) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || friendName.empty()) {
        return;
    }
    
    logger_->info("AshitaAdapter: Removing friend " + friendName);
    
    if (!removeFriendUseCase_) {
        viewModel_->setErrorMessage("Remove friend use case not initialized");
        logger_->error("AshitaAdapter: RemoveFriendUseCase not initialized");
        return;
    }
    
    removeFriendUseCase_->removeFriend(apiKey_, characterName_, friendName,
        [this, friendName](App::UseCases::RemoveFriendResult result) {
            std::lock_guard<std::mutex> stateLock(stateMutex_);
            if (!result.success) {
                viewModel_->setErrorMessage("Failed to remove friend: " + result.error);
                logger_->error("AshitaAdapter: Failed to remove friend: " + result.error);
            } else {
                logger_->info("AshitaAdapter: Friend " + friendName + " removed successfully");
                
                if (viewModel_) {
                }
                
                if (notificationUseCase_ && clock_) {
                    int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                    auto toast = notificationUseCase_->createInfoToast(
                        std::string(App::Notifications::Constants::TITLE_FRIEND_REMOVED),
                        "Friend " + friendName + " removed",  // Dynamic content
                        currentTime);
                    UI::Notifications::ToastManager::getInstance().addToast(toast);
                }
                
                {
                    std::lock_guard<std::mutex> lock(pollingMutex_);
                    lastFriendListSyncCallsite_ = "RemoveFriend";
                }
                handleSyncFriendListAsync();
            }
        });
}

void AshitaAdapter::handleRemoveFriendVisibility(const std::string& friendName) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || friendName.empty()) {
        return;
    }
    
    logger_->info("AshitaAdapter: Removing friend visibility for " + friendName);
    
    if (!removeFriendVisibilityUseCase_) {
        viewModel_->setErrorMessage("Remove friend visibility use case not initialized");
        logger_->error("AshitaAdapter: RemoveFriendVisibilityUseCase not initialized");
        return;
    }
    
    removeFriendVisibilityUseCase_->removeFriendVisibility(apiKey_, characterName_, friendName,
        [this, friendName](App::UseCases::RemoveFriendVisibilityResult result) {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (!result.success) {
                viewModel_->setErrorMessage("Failed to remove friend visibility: " + result.error);
                logger_->error("AshitaAdapter: Failed to remove friend visibility: " + result.error);
                showErrorNotification("Failed to remove friend visibility: " + result.error, "RemoveFriendVisibility");
            } else {
                logger_->info("AshitaAdapter: Friend visibility for " + friendName + " removed successfully");
                
                if (notificationUseCase_ && clock_) {
                    int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                    auto toast = notificationUseCase_->createInfoToast(
                        std::string(App::Notifications::Constants::TITLE_FRIEND_VISIBILITY),
                        "Friend " + friendName + " removed from this character's view",  // Dynamic content
                        currentTime);
                    UI::Notifications::ToastManager::getInstance().addToast(toast);
                }
                
                {
                    std::lock_guard<std::mutex> pollingLock(pollingMutex_);
                    lastFriendListSyncCallsite_ = "RemoveFriendVisibility";
                }
                handleSyncFriendListAsync();
            }
        });
}

void AshitaAdapter::handleRefreshAltVisibility() {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || characterName_.empty()) {
        return;
    }
    
    logger_->info("AshitaAdapter: Refreshing alt visibility data");
    
    if (!getAltVisibilityUseCase_ || !altVisibilityViewModel_) {
        logger_->error("AshitaAdapter: GetAltVisibilityUseCase or AltVisibilityViewModel not initialized");
        return;
    }
    
    if (altVisibilityViewModel_) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        altVisibilityViewModel_->setLoading(true);
        altVisibilityViewModel_->clearError();
    }
    
    std::thread([this]() {
        App::UseCases::GetAltVisibilityResult result = getAltVisibilityUseCase_->getVisibility(apiKey_, characterName_);
        
        if (result.success) {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (altVisibilityViewModel_) {
                altVisibilityViewModel_->updateFromResult(result.friends, result.characters);
                altVisibilityViewModel_->setLastUpdateTime(result.serverTime);
                altVisibilityViewModel_->setLoading(false);
                logger_->info("AshitaAdapter: Alt visibility data refreshed successfully");
            }
        } else {
            logger_->error("AshitaAdapter: Failed to refresh alt visibility: " + result.error);
            if (altVisibilityViewModel_) {
                std::lock_guard<std::mutex> lock(stateMutex_);
                altVisibilityViewModel_->setError(result.error);
                altVisibilityViewModel_->setLoading(false);
            }
        }
    }).detach();
}

void AshitaAdapter::handleAddFriendVisibility(const std::string& friendName) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || friendName.empty()) {
        return;
    }
    
    logger_->info("AshitaAdapter: Adding friend visibility for " + friendName);
    
    handleSendFriendRequest(friendName);
    
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        handleRefreshAltVisibility();
    }).detach();
}

void AshitaAdapter::handleToggleFriendVisibility(const std::string& commandData) {
    if (!connectUseCase_->isConnected() || apiKey_.empty() || commandData.empty()) {
        return;
    }
    
    size_t pipe1 = commandData.find('|');
    if (pipe1 == std::string::npos) {
        logger_->error("AshitaAdapter: Invalid ToggleFriendVisibility command data format");
        return;
    }
    
    size_t pipe2 = commandData.find('|', pipe1 + 1);
    if (pipe2 == std::string::npos) {
        logger_->error("AshitaAdapter: Invalid ToggleFriendVisibility command data format");
        return;
    }
    
    size_t pipe3 = commandData.find('|', pipe2 + 1);
    if (pipe3 == std::string::npos) {
        logger_->error("AshitaAdapter: Invalid ToggleFriendVisibility command data format");
        return;
    }
    
    std::string friendAccountIdStr = commandData.substr(0, pipe1);
    std::string characterIdStr = commandData.substr(pipe1 + 1, pipe2 - pipe1 - 1);
    std::string friendName = commandData.substr(pipe2 + 1, pipe3 - pipe2 - 1);
    std::string desiredVisibleStr = commandData.substr(pipe3 + 1);
    
    int friendAccountId = 0;
    int characterId = 0;
    try {
        friendAccountId = std::stoi(friendAccountIdStr);
        characterId = std::stoi(characterIdStr);
    } catch (...) {
        logger_->error("AshitaAdapter: Invalid friendAccountId or characterId in ToggleFriendVisibility command");
        return;
    }
    
    bool desiredVisible = (desiredVisibleStr == "true");
    
    logger_->info("AshitaAdapter: Toggling friend visibility for " + friendName + " (accountId: " + friendAccountIdStr + ", characterId: " + characterIdStr + ", visible: " + desiredVisibleStr + ")");
    
    if (!altVisibilityViewModel_) {
        logger_->error("AshitaAdapter: AltVisibilityViewModel not initialized");
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        altVisibilityViewModel_->setBusy(friendAccountId, characterId, true);
        altVisibilityViewModel_->setVisibility(friendAccountId, characterId, desiredVisible);
    }
    
    int currentCharacterId = 0;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (altVisibilityViewModel_) {
            const auto& characters = altVisibilityViewModel_->getCharacters();
            for (const auto& charInfo : characters) {
                if (charInfo.isActive) {
                    currentCharacterId = charInfo.characterId;
                    break;
                }
            }
        }
    }
    
    bool useNewEndpoint = (currentCharacterId != 0 && currentCharacterId != characterId);
    
    std::thread([this, friendName, friendAccountId, characterId, desiredVisible, useNewEndpoint, currentCharacterId]() {
        std::string apiKey = apiKey_;
        std::string charName = characterName_;
        
        bool success = false;
        std::string errorMessage;
        std::string action;  // "VISIBILITY_GRANTED", "VISIBILITY_REQUEST_SENT", etc.
        
        if (useNewEndpoint) {
            std::string url = netClient_->getBaseUrl() + "/api/friends/visibility";
            std::ostringstream body;
            body << "{\"friendName\":\"" << friendName << "\",\"forCharacterId\":" << characterId << ",\"desiredVisible\":" << (desiredVisible ? "true" : "false") << "}";
            
            App::HttpResponse response = netClient_->post(url, apiKey, charName, body.str());
            
            if (response.isSuccess() && response.statusCode == 200) {
                bool responseSuccess = false;
                Protocol::JsonUtils::extractBooleanField(response.body, "success", responseSuccess);
                
                if (responseSuccess) {
                    success = true;
                    Protocol::JsonUtils::extractStringField(response.body, "action", action);
                    if (action.empty()) {
                        action = desiredVisible ? "VISIBILITY_GRANTED" : "VISIBILITY_REMOVED";
                    }
                    logger_->info("AshitaAdapter: Visibility updated successfully for " + friendName + " (characterId: " + std::to_string(characterId) + ", action: " + action + ")");
                } else {
                    Protocol::JsonUtils::extractStringField(response.body, "error", errorMessage);
                    if (errorMessage.empty()) {
                        errorMessage = "Server returned success=false";
                    }
                    logger_->error("AshitaAdapter: Failed to update visibility: " + errorMessage);
                }
            } else {
                success = false;
                errorMessage = response.error.empty() ? 
                    "HTTP " + std::to_string(response.statusCode) : response.error;
                logger_->error("AshitaAdapter: Failed to update visibility: " + errorMessage);
            }
        } else if (desiredVisible) {
            if (!sendRequestUseCase_) {
                errorMessage = "SendFriendRequestUseCase not initialized";
            } else {
                auto result = sendRequestUseCase_->sendRequest(apiKey, charName, friendName);
                success = result.success;
                errorMessage = result.userMessage.empty() ? result.errorCode : result.userMessage;
                action = result.action;
                
                if (success) {
                    logger_->info("AshitaAdapter: Visibility request sent successfully for " + friendName + " (action: " + action + ")");
                } else {
                    logger_->error("AshitaAdapter: Failed to send visibility request: " + errorMessage);
                }
            }
        } else {
            if (!removeFriendVisibilityUseCase_) {
                errorMessage = "RemoveFriendVisibilityUseCase not initialized";
            } else {
                std::mutex callbackMutex;
                std::condition_variable callbackCv;
                bool callbackReceived = false;
                App::UseCases::RemoveFriendVisibilityResult callbackResult;
                
                removeFriendVisibilityUseCase_->removeFriendVisibility(apiKey, charName, friendName,
                    [&callbackResult, &callbackMutex, &callbackCv, &callbackReceived](App::UseCases::RemoveFriendVisibilityResult result) {
                        std::lock_guard<std::mutex> lock(callbackMutex);
                        callbackResult = result;
                        callbackReceived = true;
                        callbackCv.notify_one();
                    });
                
                std::unique_lock<std::mutex> lock(callbackMutex);
                if (!callbackCv.wait_for(lock, std::chrono::seconds(5), [&callbackReceived]() { return callbackReceived; })) {
                    success = false;
                    errorMessage = "Timeout waiting for remove visibility response";
                    logger_->error("AshitaAdapter: Timeout waiting for remove visibility callback");
                } else {
                    success = callbackResult.success;
                    errorMessage = callbackResult.userMessage.empty() ? callbackResult.error : callbackResult.userMessage;
                    
                    if (success) {
                        logger_->info("AshitaAdapter: Visibility removed successfully for " + friendName);
                    } else {
                        logger_->error("AshitaAdapter: Failed to remove visibility: " + errorMessage);
                    }
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> stateLock(stateMutex_);
            altVisibilityViewModel_->setBusy(friendAccountId, characterId, false);
            
            if (success) {
            } else {
                std::lock_guard<std::mutex> stateLock2(stateMutex_);
                if (altVisibilityViewModel_) {
                    altVisibilityViewModel_->setBusy(friendAccountId, characterId, false);
                }
                logger_->warning("AshitaAdapter: Visibility toggle failed, refreshing to get correct state");
            }
        }
        
        if (notificationUseCase_ && clock_) {
            int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
            if (success) {
                if (desiredVisible) {
                    if (action == "VISIBILITY_GRANTED") {
                        auto toast = notificationUseCase_->createSuccessToast(
                            std::string(App::Notifications::Constants::TITLE_FRIEND_VISIBILITY),
                            "Visibility granted for " + friendName,  // Dynamic content
                            currentTime);
                        UI::Notifications::ToastManager::getInstance().addToast(toast);
                    } else if (action == "VISIBILITY_REQUEST_SENT") {
                        auto toast = notificationUseCase_->createInfoToast(
                            std::string(App::Notifications::Constants::TITLE_FRIEND_VISIBILITY),
                            "Visibility request sent to " + friendName,  // Dynamic content
                            currentTime);
                        UI::Notifications::ToastManager::getInstance().addToast(toast);
                    } else {
                        auto toast = notificationUseCase_->createInfoToast(
                            std::string(App::Notifications::Constants::TITLE_FRIEND_VISIBILITY),
                            "Visibility updated for " + friendName,  // Dynamic content
                            currentTime);
                        UI::Notifications::ToastManager::getInstance().addToast(toast);
                    }
                } else {
                    auto toast = notificationUseCase_->createInfoToast(
                        std::string(App::Notifications::Constants::TITLE_FRIEND_VISIBILITY),
                        "Visibility removed for " + friendName,  // Dynamic content
                        currentTime);
                    UI::Notifications::ToastManager::getInstance().addToast(toast);
                }
            } else {
                auto toast = notificationUseCase_->createErrorToast(
                    "Failed to update visibility: " + errorMessage,  // Dynamic content
                    currentTime);
                UI::Notifications::ToastManager::getInstance().addToast(toast);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        handleRefreshAltVisibility();
    }).detach();
}

std::string AshitaAdapter::formatJobString(uint8_t mainJob, uint8_t mainJobLevel, uint8_t subJob, uint8_t subJobLevel) const {
    if (mainJob == 0 || mainJobLevel == 0) {
        return "";
    }
    
    static const char* jobNames[] = {
        "NON", "WAR", "MNK", "WHM", "BLM", "RDM", "THF", "PLD", "DRK", "BST",
        "BRD", "RNG", "SAM", "NIN", "DRG", "SMN", "BLU", "COR", "PUP", "DNC",
        "SCH", "GEO", "RUN"
    };
    
    if (mainJob >= 23) {
        return "";
    }
    
    std::string job = std::string(jobNames[mainJob]) + " " + std::to_string(mainJobLevel);
    
    if (subJob > 0 && subJobLevel > 0 && subJob < 23) {
        job += "/" + std::string(jobNames[subJob]) + " " + std::to_string(subJobLevel);
    }
    
    return job;
}

std::string AshitaAdapter::getZoneNameFromId(uint16_t zoneId) const {
    static const std::map<uint16_t, std::string> zoneIdToName = {
        {static_cast<uint16_t>(0), "unknown"},
        {static_cast<uint16_t>(1), "Phanauet Channel"},
        {static_cast<uint16_t>(2), "Carpenters' Landing"},
        {static_cast<uint16_t>(3), "Manaclipper"},
        {static_cast<uint16_t>(4), "Bibiki Bay"},
        {static_cast<uint16_t>(5), "Uleguerand Range"},
        {static_cast<uint16_t>(6), "Bearclaw Pinnacle"},
        {static_cast<uint16_t>(7), "Attohwa Chasm"},
        {static_cast<uint16_t>(8), "Boneyard Gully"},
        {static_cast<uint16_t>(9), "Pso'Xja"},
        {static_cast<uint16_t>(10), "The Shrouded Maw"},
        {static_cast<uint16_t>(11), "Oldton Movalpolos"},
        {static_cast<uint16_t>(12), "Newton Movalpolos"},
        {static_cast<uint16_t>(13), "Mine Shaft #2716"},
        {static_cast<uint16_t>(14), "Hall of Transference"},
        {static_cast<uint16_t>(16), "Promyvion - Holla"},
        {static_cast<uint16_t>(17), "Spire of Holla"},
        {static_cast<uint16_t>(18), "Promyvion - Dem"},
        {static_cast<uint16_t>(19), "Spire of Dem"},
        {static_cast<uint16_t>(20), "Promyvion - Mea"},
        {static_cast<uint16_t>(21), "Spire of Mea"},
        {static_cast<uint16_t>(22), "Promyvion - Vahzl"},
        {static_cast<uint16_t>(23), "Spire of Vahzl"},
        {static_cast<uint16_t>(24), "Lufaise Meadows"},
        {static_cast<uint16_t>(25), "Misareaux Coast"},
        {static_cast<uint16_t>(26), "Tavnazian Safehold"},
        {static_cast<uint16_t>(27), "Phomiuna Aqueducts"},
        {static_cast<uint16_t>(28), "Sacrarium"},
        {static_cast<uint16_t>(29), "Riverne - Site #B01"},
        {static_cast<uint16_t>(30), "Riverne - Site #A01"},
        {static_cast<uint16_t>(31), "Monarch Linn"},
        {static_cast<uint16_t>(32), "Sealion's Den"},
        {static_cast<uint16_t>(33), "Al'Taieu"},
        {static_cast<uint16_t>(34), "Grand Palace of Hu'Xzoi"},
        {static_cast<uint16_t>(35), "The Garden of Ru'Hmet"},
        {static_cast<uint16_t>(36), "Empyreal Paradox"},
        {static_cast<uint16_t>(37), "Temenos"},
        {static_cast<uint16_t>(38), "Apollyon"},
        {static_cast<uint16_t>(39), "Dynamis - Valkurm"},
        {static_cast<uint16_t>(40), "Dynamis - Buburimu"},
        {static_cast<uint16_t>(41), "Dynamis - Qufim"},
        {static_cast<uint16_t>(42), "Dynamis - Tavnazia"},
        {static_cast<uint16_t>(44), "Bibiki Bay - Purgonorgo Isle"},
        {static_cast<uint16_t>(46), "Open sea route to Al Zahbi"},
        {static_cast<uint16_t>(47), "Open sea route to Mhaura"},
        {static_cast<uint16_t>(48), "Al Zahbi"},
        {static_cast<uint16_t>(50), "Aht Urhgan Whitegate"},
        {static_cast<uint16_t>(51), "Wajaom Woodlands"},
        {static_cast<uint16_t>(52), "Bhaflau Thickets"},
        {static_cast<uint16_t>(53), "Nashmau"},
        {static_cast<uint16_t>(54), "Arrapago Reef"},
        {static_cast<uint16_t>(55), "Ilrusi Atoll"},
        {static_cast<uint16_t>(56), "Periqia"},
        {static_cast<uint16_t>(57), "Talacca Cove"},
        {static_cast<uint16_t>(58), "Silver Sea route to Nashmau"},
        {static_cast<uint16_t>(59), "Silver Sea route to Al Zahbi"},
        {static_cast<uint16_t>(60), "The Ashu Talif"},
        {static_cast<uint16_t>(61), "Mount Zhayolm"},
        {static_cast<uint16_t>(62), "Halvung"},
        {static_cast<uint16_t>(63), "Lebros Cavern"},
        {static_cast<uint16_t>(64), "Navukgo Execution Chamber"},
        {static_cast<uint16_t>(65), "Mamook"},
        {static_cast<uint16_t>(66), "Mamool Ja Training Grounds"},
        {static_cast<uint16_t>(67), "Jade Sepulcher"},
        {static_cast<uint16_t>(68), "Aydeewa Subterrane"},
        {static_cast<uint16_t>(69), "Leujaoam Sanctum"},
        {static_cast<uint16_t>(70), "Chocobo Circuit"},
        {static_cast<uint16_t>(71), "The Colosseum"},
        {static_cast<uint16_t>(72), "Alzadaal Undersea Ruins"},
        {static_cast<uint16_t>(73), "Zhayolm Remnants"},
        {static_cast<uint16_t>(74), "Arrapago Remnants"},
        {static_cast<uint16_t>(75), "Bhaflau Remnants"},
        {static_cast<uint16_t>(76), "Silver Sea Remnants"},
        {static_cast<uint16_t>(77), "Nyzul Isle"},
        {static_cast<uint16_t>(78), "Hazhalm Testing Grounds"},
        {static_cast<uint16_t>(79), "Caedarva Mire"},
        {static_cast<uint16_t>(100), "West Ronfaure"},
        {static_cast<uint16_t>(101), "East Ronfaure"},
        {static_cast<uint16_t>(102), "La Theine Plateau"},
        {static_cast<uint16_t>(103), "Valkurm Dunes"},
        {static_cast<uint16_t>(104), "Jugner Forest"},
        {static_cast<uint16_t>(105), "Batallia Downs"},
        {static_cast<uint16_t>(106), "North Gustaberg"},
        {static_cast<uint16_t>(107), "South Gustaberg"},
        {static_cast<uint16_t>(108), "Konschtat Highlands"},
        {static_cast<uint16_t>(109), "Pashhow Marshlands"},
        {static_cast<uint16_t>(110), "Rolanberry Fields"},
        {static_cast<uint16_t>(111), "Beaucedine Glacier"},
        {static_cast<uint16_t>(112), "Xarcabard"},
        {static_cast<uint16_t>(113), "Cape Teriggan"},
        {static_cast<uint16_t>(114), "Eastern Altepa Desert"},
        {static_cast<uint16_t>(115), "West Sarutabaruta"},
        {static_cast<uint16_t>(116), "East Sarutabaruta"},
        {static_cast<uint16_t>(117), "Tahrongi Canyon"},
        {static_cast<uint16_t>(118), "Buburimu Peninsula"},
        {static_cast<uint16_t>(119), "Meriphataud Mountains"},
        {static_cast<uint16_t>(120), "Sauromugue Champaign"},
        {static_cast<uint16_t>(121), "The Sanctuary of Zi'Tah"},
        {static_cast<uint16_t>(122), "Ro'Maeve"},
        {static_cast<uint16_t>(123), "Yuhtunga Jungle"},
        {static_cast<uint16_t>(124), "Yhoator Jungle"},
        {static_cast<uint16_t>(125), "Western Altepa Desert"},
        {static_cast<uint16_t>(126), "Qufim Island"},
        {static_cast<uint16_t>(127), "Behemoth's Dominion"},
        {static_cast<uint16_t>(128), "Valley of Sorrows"},
        {static_cast<uint16_t>(130), "Ru'Aun Gardens"},
        {static_cast<uint16_t>(131), "Mordion Gaol"},
        {static_cast<uint16_t>(134), "Dynamis - Beaucedine"},
        {static_cast<uint16_t>(135), "Dynamis - Xarcabard"},
        {static_cast<uint16_t>(139), "Horlais Peak"},
        {static_cast<uint16_t>(140), "Ghelsba Outpost"},
        {static_cast<uint16_t>(141), "Fort Ghelsba"},
        {static_cast<uint16_t>(142), "Yughott Grotto"},
        {static_cast<uint16_t>(143), "Palborough Mines"},
        {static_cast<uint16_t>(144), "Waughroon Shrine"},
        {static_cast<uint16_t>(145), "Giddeus"},
        {static_cast<uint16_t>(146), "Balga's Dais"},
        {static_cast<uint16_t>(147), "Beadeaux"},
        {static_cast<uint16_t>(148), "Qulun Dome"},
        {static_cast<uint16_t>(149), "Davoi"},
        {static_cast<uint16_t>(150), "Monastic Cavern"},
        {static_cast<uint16_t>(151), "Castle Oztroja"},
        {static_cast<uint16_t>(152), "Altar Room"},
        {static_cast<uint16_t>(153), "The Boyahda Tree"},
        {static_cast<uint16_t>(154), "Dragon's Aery"},
        {static_cast<uint16_t>(157), "Middle Delkfutt's Tower"},
        {static_cast<uint16_t>(158), "Upper Delkfutt's Tower"},
        {static_cast<uint16_t>(159), "Temple of Uggalepih"},
        {static_cast<uint16_t>(160), "Den of Rancor"},
        {static_cast<uint16_t>(161), "Castle Zvahl Baileys"},
        {static_cast<uint16_t>(162), "Castle Zvahl Keep"},
        {static_cast<uint16_t>(163), "Sacrificial Chamber"},
        {static_cast<uint16_t>(165), "Throne Room"},
        {static_cast<uint16_t>(166), "Ranguemont Pass"},
        {static_cast<uint16_t>(167), "Bostaunieux Oubliette"},
        {static_cast<uint16_t>(168), "Chamber of Oracles"},
        {static_cast<uint16_t>(169), "Toraimarai Canal"},
        {static_cast<uint16_t>(170), "Full Moon Fountain"},
        {static_cast<uint16_t>(172), "Zeruhn Mines"},
        {static_cast<uint16_t>(173), "Korroloka Tunnel"},
        {static_cast<uint16_t>(174), "Kuftal Tunnel"},
        {static_cast<uint16_t>(176), "Sea Serpent Grotto"},
        {static_cast<uint16_t>(177), "Ve'Lugannon Palace"},
        {static_cast<uint16_t>(178), "The Shrine of Ru'Avitau"},
        {static_cast<uint16_t>(179), "Stellar Fulcrum"},
        {static_cast<uint16_t>(180), "La'Loff Amphitheater"},
        {static_cast<uint16_t>(181), "The Celestial Nexus"},
        {static_cast<uint16_t>(184), "Lower Delkfutt's Tower"},
        {static_cast<uint16_t>(185), "Dynamis - San d'Oria"},
        {static_cast<uint16_t>(186), "Dynamis - Bastok"},
        {static_cast<uint16_t>(187), "Dynamis - Windurst"},
        {static_cast<uint16_t>(188), "Dynamis - Jeuno"},
        {static_cast<uint16_t>(190), "King Ranperre's Tomb"},
        {static_cast<uint16_t>(191), "Dangruf Wadi"},
        {static_cast<uint16_t>(192), "Inner Horutoto Ruins"},
        {static_cast<uint16_t>(193), "Ordelle's Caves"},
        {static_cast<uint16_t>(194), "Outer Horutoto Ruins"},
        {static_cast<uint16_t>(195), "The Eldieme Necropolis"},
        {static_cast<uint16_t>(196), "Gusgen Mines"},
        {static_cast<uint16_t>(197), "Crawlers' Nest"},
        {static_cast<uint16_t>(198), "Maze of Shakhrami"},
        {static_cast<uint16_t>(200), "Garlaige Citadel"},
        {static_cast<uint16_t>(201), "Cloister of Gales"},
        {static_cast<uint16_t>(202), "Cloister of Storms"},
        {static_cast<uint16_t>(203), "Cloister of Frost"},
        {static_cast<uint16_t>(204), "Fei'Yin"},
        {static_cast<uint16_t>(205), "Ifrit's Cauldron"},
        {static_cast<uint16_t>(206), "Qu'Bia Arena"},
        {static_cast<uint16_t>(207), "Cloister of Flames"},
        {static_cast<uint16_t>(208), "Quicksand Caves"},
        {static_cast<uint16_t>(209), "Cloister of Tremors"},
        {static_cast<uint16_t>(211), "Cloister of Tides"},
        {static_cast<uint16_t>(212), "Gustav Tunnel"},
        {static_cast<uint16_t>(213), "Labyrinth of Onzozo"},
        {static_cast<uint16_t>(220), "Ship bound for Selbina"},
        {static_cast<uint16_t>(221), "Ship bound for Mhaura"},
        {static_cast<uint16_t>(223), "San d'Oria-Jeuno Airship"},
        {static_cast<uint16_t>(224), "Bastok-Jeuno Airship"},
        {static_cast<uint16_t>(225), "Windurst-Jeuno Airship"},
        {static_cast<uint16_t>(226), "Kazham-Jeuno Airship"},
        {static_cast<uint16_t>(227), "Ship bound for Selbina"},
        {static_cast<uint16_t>(228), "Ship bound for Mhaura"},
        {static_cast<uint16_t>(230), "Southern San d'Oria"},
        {static_cast<uint16_t>(231), "Northern San d'Oria"},
        {static_cast<uint16_t>(232), "Port San d'Oria"},
        {static_cast<uint16_t>(233), "Chateau d'Oraguille"},
        {static_cast<uint16_t>(234), "Bastok Mines"},
        {static_cast<uint16_t>(235), "Bastok Markets"},
        {static_cast<uint16_t>(236), "Port Bastok"},
        {static_cast<uint16_t>(237), "Metalworks"},
        {static_cast<uint16_t>(238), "Windurst Waters"},
        {static_cast<uint16_t>(239), "Windurst Walls"},
        {static_cast<uint16_t>(240), "Port Windurst"},
        {static_cast<uint16_t>(241), "Windurst Woods"},
        {static_cast<uint16_t>(242), "Heavens Tower"},
        {static_cast<uint16_t>(243), "Ru'Lude Gardens"},
        {static_cast<uint16_t>(244), "Upper Jeuno"},
        {static_cast<uint16_t>(245), "Lower Jeuno"},
        {static_cast<uint16_t>(246), "Port Jeuno"},
        {static_cast<uint16_t>(247), "Rabao"},
        {static_cast<uint16_t>(248), "Selbina"},
        {static_cast<uint16_t>(249), "Mhaura"},
        {static_cast<uint16_t>(250), "Kazham"},
        {static_cast<uint16_t>(251), "Hall of the Gods"},
        {static_cast<uint16_t>(252), "Norg"}
    };
    
    auto it = zoneIdToName.find(zoneId);
    if (it != zoneIdToName.end()) {
        return it->second;
    }
    
    return "Zone " + std::to_string(zoneId);
}

void AshitaAdapter::updateThemesViewModel() {
    logger_->info("AshitaAdapter::updateThemesViewModel: Called");
    if (!themesViewModel_) {
        logger_->error("AshitaAdapter::updateThemesViewModel: themesViewModel_ is null!");
        return;
    }
    if (!themeUseCase_) {
        logger_->error("AshitaAdapter::updateThemesViewModel: themeUseCase_ is null!");
        return;
    }
    
    int currentIndex = themeUseCase_->getCurrentThemeIndex();
    themesViewModel_->setCurrentThemeIndex(currentIndex);
    themesViewModel_->setCustomThemes(themeUseCase_->getCustomThemes());
    themesViewModel_->setBackgroundAlpha(themeUseCase_->getBackgroundAlpha());
    themesViewModel_->setTextAlpha(themeUseCase_->getTextAlpha());
    
    std::string currentPresetName = themeUseCase_->getCurrentPresetName();
    std::vector<std::string> availablePresets = themeUseCase_->getAvailablePresets();
    
    logger_->info("AshitaAdapter::updateThemesViewModel: BEFORE set - currentPresetName='" + currentPresetName + 
                   "', availablePresets.size()=" + std::to_string(availablePresets.size()));
    for (size_t i = 0; i < availablePresets.size(); ++i) {
        logger_->info("AshitaAdapter::updateThemesViewModel: preset[" + std::to_string(i) + "]='" + availablePresets[i] + "'");
    }
    
    if (availablePresets.empty()) {
        logger_->error("AshitaAdapter::updateThemesViewModel: WARNING - availablePresets is EMPTY! This should never happen!");
        availablePresets = {"XIUI Default", "Classic"};
        logger_->info("AshitaAdapter::updateThemesViewModel: Using fallback presets");
    }
    
    const auto& existingPresets = themesViewModel_->getAvailablePresets();
    if (!existingPresets.empty() && existingPresets.size() == availablePresets.size()) {
        bool presetsMatch = true;
        for (size_t i = 0; i < availablePresets.size(); ++i) {
            if (i >= existingPresets.size() || existingPresets[i] != availablePresets[i]) {
                presetsMatch = false;
                break;
            }
        }
        if (presetsMatch) {
            logger_->info("AshitaAdapter::updateThemesViewModel: Presets already set correctly, skipping preset update");
        } else {
            logger_->info("AshitaAdapter::updateThemesViewModel: Presets differ, updating");
            themesViewModel_->setAvailablePresets(availablePresets);
        }
    } else {
        logger_->info("AshitaAdapter::updateThemesViewModel: Setting presets (existing.size()=" + 
                     std::to_string(existingPresets.size()) + ", new.size()=" + std::to_string(availablePresets.size()) + ")");
        themesViewModel_->setAvailablePresets(availablePresets);
    }
    
    themesViewModel_->setCurrentPresetName(currentPresetName);
    
    logger_->info("AshitaAdapter::updateThemesViewModel: AFTER set - ViewModel presetName='" + themesViewModel_->getCurrentPresetName() + 
                   "', ViewModel presets.size()=" + std::to_string(themesViewModel_->getAvailablePresets().size()));
    
    if (currentIndex == -1) {
        std::string currentCustomThemeName = themeUseCase_->getCurrentCustomThemeName();
        themesViewModel_->setCurrentCustomThemeName(currentCustomThemeName);
    } else {
        themesViewModel_->setCurrentCustomThemeName("");  // Clear if not using custom theme
    }
    
    Core::CustomTheme currentTheme = themeUseCase_->getCurrentCustomTheme();
    themesViewModel_->setCurrentThemeColors(currentTheme);
}

void AshitaAdapter::handleApplyTheme(int themeIndex) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->setTheme(themeIndex);
    if (result.success) {
        ThemePersistence::saveToFile(themeState_);
        updateThemesViewModel();
        logger_->info("[theme] Applied theme: " + std::to_string(themeIndex));
    } else {
        logger_->error("[theme] Failed to apply theme: " + result.error);
    }
}

void AshitaAdapter::handleSetCustomTheme(const std::string& themeName) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->setCustomTheme(themeName);
    if (result.success) {
        ThemePersistence::saveToFile(themeState_);
        updateThemesViewModel();
        logger_->info("[theme] Applied custom theme: " + themeName);
    } else {
        logger_->error("[theme] Failed to apply custom theme: " + result.error);
    }
}

void AshitaAdapter::handleSetCustomThemeByName(const std::string& themeName) {
    handleSetCustomTheme(themeName);
}

void AshitaAdapter::handleSetThemePreset(const std::string& presetName) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->setThemePreset(presetName);
    if (!result.success) {
        logger_->error("[theme] Failed to set preset: " + result.error);
        return;
    }
    
    ThemePersistence::saveToFile(themeState_);
    
    updateThemesViewModel();
}


void AshitaAdapter::handleUpdateThemeColors() {
    if (!themeUseCase_ || !themesViewModel_) {
        return;
    }
    
    if (themeUseCase_->isDefaultTheme()) {
        return;
    }
    
    Core::CustomTheme updatedColors = themesViewModel_->getCurrentThemeColors();
    
    App::UseCases::ThemeResult result = themeUseCase_->updateCurrentThemeColors(updatedColors);
    if (result.success) {
    } else {
        logger_->error("[theme] Failed to update colors: " + result.error);
    }
}

void AshitaAdapter::handleSetBackgroundAlpha(float alpha) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->setBackgroundAlpha(alpha);
    if (result.success) {
        updateThemesViewModel();
        logger_->debug("[theme] Background alpha: " + std::to_string(alpha));
    } else {
        logger_->error("[theme] Failed to set background alpha: " + result.error);
    }
}

void AshitaAdapter::handleSetTextAlpha(float alpha) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->setTextAlpha(alpha);
    if (result.success) {
        updateThemesViewModel();
        logger_->debug("AshitaAdapter: Text alpha updated: " + std::to_string(alpha));
    } else {
        logger_->error("[theme] Failed to set text alpha: " + result.error);
    }
}

void AshitaAdapter::handleSaveThemeAlpha() {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult bgResult = themeUseCase_->saveBackgroundAlpha();
    App::UseCases::ThemeResult textResult = themeUseCase_->saveTextAlpha();
    
    if (bgResult.success && textResult.success) {
        ThemePersistence::saveToFile(themeState_);
        logger_->debug("[theme] Alpha saved");
    } else {
        logger_->warning("[theme] Failed to save alpha: " + 
                        (bgResult.success ? "" : bgResult.error) + 
                        (textResult.success ? "" : textResult.error));
    }
}

void AshitaAdapter::handleSaveCustomTheme(const std::string& themeName) {
    if (!themeUseCase_ || !themesViewModel_) {
        return;
    }
    
    if (themeName.empty()) {
        logger_->error("[theme] Cannot save theme with empty name");
        return;
    }
    
    logger_->info("[theme] Saving custom theme: " + themeName);
    
    Core::CustomTheme theme = themesViewModel_->getCurrentThemeColors();
    theme.name = themeName;
    
    int currentIndex = themeUseCase_->getCurrentThemeIndex();
    std::string currentThemeName = themeUseCase_->getCurrentCustomThemeName();
    logger_->info("AshitaAdapter: Current theme index: " + std::to_string(currentIndex) + 
                  ", current theme name: '" + currentThemeName + "'");
    
    bool colorsAppearUninitialized = (theme.windowBgColor.r == 0.0f && theme.windowBgColor.g == 0.0f && 
                                      theme.windowBgColor.b == 0.0f && theme.windowBgColor.a == 1.0f);
    
    if (colorsAppearUninitialized) {
        Core::CustomTheme useCaseTheme = themeUseCase_->getCurrentCustomTheme();
        bool useCaseHasValidColors = !(useCaseTheme.windowBgColor.r == 0.0f && useCaseTheme.windowBgColor.g == 0.0f && 
                                       useCaseTheme.windowBgColor.b == 0.0f && useCaseTheme.windowBgColor.a == 1.0f);
        
        if (useCaseHasValidColors) {
            theme = useCaseTheme;
            theme.name = themeName;
            logger_->warning("AshitaAdapter: ViewModel colors were zero, using ThemeUseCase colors for save");
        } else {
            logger_->error("AshitaAdapter: Cannot save theme - both ViewModel and ThemeUseCase have uninitialized colors");
            return;
        }
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->saveCustomTheme(themeName, theme);
    
    std::string actualThemeName = result.actualName.empty() ? themeName : result.actualName;
    
    if (result.success) {
        bool isUpdatingCurrentTheme = false;
        if (currentIndex == -1) {
            if (!currentThemeName.empty() && currentThemeName == themeName) {
                isUpdatingCurrentTheme = true;
                logger_->info("AshitaAdapter: Updating existing custom theme: " + themeName);
            } else {
                logger_->info("AshitaAdapter: Creating new custom theme: " + themeName + 
                             " (current theme: '" + currentThemeName + "')");
            }
        } else {
            logger_->info("AshitaAdapter: Saving built-in theme as custom: " + themeName);
        }
        
        if (currentIndex >= 0 && currentIndex <= 3) {
            App::UseCases::ThemeResult switchResult = themeUseCase_->setCustomTheme(actualThemeName);
            if (!switchResult.success) {
                logger_->error("AshitaAdapter: Failed to switch to saved custom theme: " + switchResult.error);
            }
        } else if (isUpdatingCurrentTheme && actualThemeName == themeName) {
            themeUseCase_->updateCurrentThemeColors(theme);
        } else {
            App::UseCases::ThemeResult switchResult = themeUseCase_->setCustomTheme(actualThemeName);
            if (!switchResult.success) {
                logger_->error("AshitaAdapter: Failed to switch to saved custom theme: " + switchResult.error + 
                              " (theme may have been saved but not switched)");
            }
        }
        
        ThemePersistence::saveToFile(themeState_);
        
        themesViewModel_->getNewThemeName().clear();
        updateThemesViewModel();
        logger_->info("AshitaAdapter: Custom theme saved: " + actualThemeName + (isUpdatingCurrentTheme && actualThemeName == themeName ? " (updated)" : " (new)"));
    } else {
        logger_->error("AshitaAdapter: Failed to save custom theme: " + result.error);
    }
}

void AshitaAdapter::handleDeleteCustomTheme(const std::string& themeName) {
    if (!themeUseCase_) {
        return;
    }
    
    App::UseCases::ThemeResult result = themeUseCase_->deleteCustomTheme(themeName);
    if (result.success) {
        ThemePersistence::saveToFile(themeState_);
        updateThemesViewModel();
        logger_->info("[theme] Deleted custom theme: " + themeName);
    } else {
        logger_->error("AshitaAdapter: Failed to delete custom theme: " + result.error);
    }
}

void AshitaAdapter::storeDefaultImGuiStyle() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !defaultStyleStorage_ || defaultStyleStorage_->defaultStyle.has_value()) {
        return;
    }
    
    ImGuiStyle& currentStyle = guiManager_->GetStyle();
    defaultStyleStorage_->defaultStyle = currentStyle;  // Copy by value
#endif
}

void AshitaAdapter::resetImGuiStyleToDefaults() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !defaultStyleStorage_ || !defaultStyleStorage_->defaultStyle.has_value()) {
        return;
    }
    
    ImGuiStyle& currentStyle = guiManager_->GetStyle();
    currentStyle = defaultStyleStorage_->defaultStyle.value();  // Restore all style values
#endif
}

void AshitaAdapter::pushThemeStyles() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !themeUseCase_) {
        return;
    }
    
    if (themeUseCase_->isDefaultTheme()) {
        return;
    }
    
    Core::CustomTheme theme = themeUseCase_->getCurrentCustomTheme();
    float backgroundAlpha = themeUseCase_->getBackgroundAlpha();
    float textAlpha = themeUseCase_->getTextAlpha();
    
    ImGuiStyle& style = guiManager_->GetStyle();
    
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 6.0f;
    style.FramePadding = ImVec2(6.0f, 3.0f);
    style.FrameRounding = 3.0f;
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 3.0f);
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(theme.windowBgColor.r, theme.windowBgColor.g, theme.windowBgColor.b, backgroundAlpha);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(theme.childBgColor.r, theme.childBgColor.g, theme.childBgColor.b, theme.childBgColor.a);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(theme.frameBgColor.r, theme.frameBgColor.g, theme.frameBgColor.b, theme.frameBgColor.a);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(theme.frameBgHovered.r, theme.frameBgHovered.g, theme.frameBgHovered.b, theme.frameBgHovered.a);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(theme.frameBgActive.r, theme.frameBgActive.g, theme.frameBgActive.b, theme.frameBgActive.a);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(theme.titleBg.r, theme.titleBg.g, theme.titleBg.b, theme.titleBg.a);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(theme.titleBgActive.r, theme.titleBgActive.g, theme.titleBgActive.b, theme.titleBgActive.a);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(theme.titleBgCollapsed.r, theme.titleBgCollapsed.g, theme.titleBgCollapsed.b, theme.titleBgCollapsed.a);
    style.Colors[ImGuiCol_Button] = ImVec4(theme.buttonColor.r, theme.buttonColor.g, theme.buttonColor.b, backgroundAlpha);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(theme.buttonHoverColor.r, theme.buttonHoverColor.g, theme.buttonHoverColor.b, backgroundAlpha);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(theme.buttonActiveColor.r, theme.buttonActiveColor.g, theme.buttonActiveColor.b, backgroundAlpha);
    style.Colors[ImGuiCol_Separator] = ImVec4(theme.separatorColor.r, theme.separatorColor.g, theme.separatorColor.b, theme.separatorColor.a);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(theme.separatorHovered.r, theme.separatorHovered.g, theme.separatorHovered.b, theme.separatorHovered.a);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(theme.separatorActive.r, theme.separatorActive.g, theme.separatorActive.b, theme.separatorActive.a);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(theme.scrollbarBg.r, theme.scrollbarBg.g, theme.scrollbarBg.b, theme.scrollbarBg.a);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(theme.scrollbarGrab.r, theme.scrollbarGrab.g, theme.scrollbarGrab.b, theme.scrollbarGrab.a);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(theme.scrollbarGrabHovered.r, theme.scrollbarGrabHovered.g, theme.scrollbarGrabHovered.b, theme.scrollbarGrabHovered.a);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(theme.scrollbarGrabActive.r, theme.scrollbarGrabActive.g, theme.scrollbarGrabActive.b, theme.scrollbarGrabActive.a);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(theme.checkMark.r, theme.checkMark.g, theme.checkMark.b, theme.checkMark.a);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(theme.sliderGrab.r, theme.sliderGrab.g, theme.sliderGrab.b, theme.sliderGrab.a);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(theme.sliderGrabActive.r, theme.sliderGrabActive.g, theme.sliderGrabActive.b, theme.sliderGrabActive.a);
    style.Colors[ImGuiCol_Header] = ImVec4(theme.header.r, theme.header.g, theme.header.b, theme.header.a);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(theme.headerHovered.r, theme.headerHovered.g, theme.headerHovered.b, theme.headerHovered.a);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(theme.headerActive.r, theme.headerActive.g, theme.headerActive.b, theme.headerActive.a);
    style.Colors[ImGuiCol_Text] = ImVec4(theme.textColor.r, theme.textColor.g, theme.textColor.b, textAlpha);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(theme.textDisabled.r, theme.textDisabled.g, theme.textDisabled.b, theme.textDisabled.a);
#endif
}

void AshitaAdapter::popThemeStyles() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !themeUseCase_) {
        return;
    }
    
    if (themeUseCase_->isDefaultTheme()) {
        return;
    }
    
    restoreImGuiStyle();
#endif
}

void AshitaAdapter::saveCurrentImGuiStyle() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !defaultStyleStorage_) {
        return;
    }
    
    ImGuiStyle& currentStyle = guiManager_->GetStyle();
    if (!defaultStyleStorage_->savedStyle.has_value()) {
        defaultStyleStorage_->savedStyle = currentStyle;  // Copy by value
    }
#endif
}

void AshitaAdapter::restoreImGuiStyle() {
#ifndef BUILDING_TESTS
    if (!guiManager_ || !defaultStyleStorage_ || !defaultStyleStorage_->savedStyle.has_value()) {
        return;
    }
    
    // Restore saved ImGui style (to prevent affecting other addons)
    ImGuiStyle& currentStyle = guiManager_->GetStyle();
    currentStyle = defaultStyleStorage_->savedStyle.value();  // Restore all style values
    defaultStyleStorage_->savedStyle.reset();  // Clear saved style after restore
#endif
}

void AshitaAdapter::handleLoadPreferences() {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    preferencesUseCase_->loadPreferences(apiKey_, characterName_);
    
    updateOptionsViewModel();
    updateFriendListViewModelsFromPreferences();
    
    if (notesViewModel_ && preferencesUseCase_) {
        notesViewModel_->setServerMode(false);
    }
    
    if (preferencesUseCase_) {
        Core::Preferences prefs = preferencesUseCase_->getPreferences();
        // Convert -1 (old default) to default position before setting position
        float posX = (prefs.notificationPositionX < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : prefs.notificationPositionX;
        float posY = (prefs.notificationPositionY < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : prefs.notificationPositionY;
        UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
    }
    
    // (duration is set per-toast based on type, preferences are applied at creation time)

    syncDebugEnabledFromPreferences();
    
    logger_->debug("AshitaAdapter: Preferences loaded");
}

void AshitaAdapter::handleUpdatePreference(const std::string& valueJson) {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    try {
        std::string field;
        if (!Protocol::JsonUtils::extractStringField(valueJson, "field", field)) {
            logger_->error("AshitaAdapter: Invalid preference update JSON - missing 'field'");
            return;
        }

        if (field == "debugMode") {
            return;
        }
        
        bool boolValue;
        if (Protocol::JsonUtils::extractBooleanField(valueJson, "value", boolValue)) {
            bool updatedServerPreference = false;
            App::UseCases::PreferencesResult result = preferencesUseCase_->updateServerPreference(
                field, boolValue, apiKey_, characterName_);
            if (result.success) {
                updatedServerPreference = true;
            } else {
                result = preferencesUseCase_->updateLocalPreference(field, boolValue, apiKey_, characterName_);
            }
            if (!result.success) {
                logger_->error("AshitaAdapter: Failed to update preference: " + result.error);
                optionsViewModel_->setError(result.error);
            } else {
                optionsViewModel_->clearError();
            }
            } else {
                float floatValue;
                if (Protocol::JsonUtils::extractNumberField(valueJson, "value", floatValue)) {
                App::UseCases::PreferencesResult result = preferencesUseCase_->updateLocalPreference(field, floatValue, apiKey_, characterName_);
                if (!result.success) {
                    logger_->error("AshitaAdapter: Failed to update preference: " + result.error);
                    optionsViewModel_->setError(result.error);
                } else {
                    optionsViewModel_->clearError();
                    // (no need to sync to NotificationViewModel - it's been removed)
                }
            } else {
                std::string stringValue;
                if (Protocol::JsonUtils::extractStringField(valueJson, "value", stringValue)) {
                    App::UseCases::PreferencesResult result = preferencesUseCase_->updateServerPreference(
                        field, stringValue, apiKey_, characterName_);
                    if (!result.success) {
                        logger_->error("AshitaAdapter: Failed to update preference: " + result.error);
                        optionsViewModel_->setError(result.error);
                    } else {
                        optionsViewModel_->clearError();
                    }
                } else {
                    logger_->error("AshitaAdapter: Invalid preference update JSON - 'value' must be boolean, float, or string");
                    return;
                }
            }
        }
        
        bool isFriendViewSetting = (field.find("mainFriendView.") == 0) || (field.find("quickOnlineFriendView.") == 0);
        if (isFriendViewSetting && windowManager_) {
            updateFriendListViewModelsFromPreferences();
        }
        
        if (isDebugEnabled()) {
            std::string settingName = field;
            std::string readableName = settingName;
            for (size_t i = 1; i < readableName.length(); i++) {
                if (std::isupper(readableName[i]) && std::islower(readableName[i-1])) {
                    readableName.insert(i, " ");
                }
            }
            std::string valueStr;
            bool boolValueForEcho;
            float floatValueForEcho;
            std::string stringValue;
            if (Protocol::JsonUtils::extractBooleanField(valueJson, "value", boolValueForEcho)) {
                valueStr = boolValueForEcho ? "true" : "false";
            } else if (Protocol::JsonUtils::extractNumberField(valueJson, "value", floatValueForEcho)) {
                valueStr = std::to_string(floatValueForEcho);
            } else if (Protocol::JsonUtils::extractStringField(valueJson, "value", stringValue)) {
                valueStr = stringValue;
            } else {
                valueStr = "unknown";
            }
            pushDebugLog("Setting '" + readableName + "' changed to " + valueStr);
        }
        
        if (field == "showOnlineStatus" || field == "shareLocation" || field == "shareJobWhenAnonymous") {
            if (Protocol::JsonUtils::extractBooleanField(valueJson, "value", boolValue)) {
                Core::Preferences prefs = preferencesUseCase_->getPreferences();
                bool showOnlineStatus = (field == "showOnlineStatus") ? boolValue : prefs.showOnlineStatus;
                bool shareLocation = (field == "shareLocation") ? boolValue : prefs.shareLocation;
                bool shareJobWhenAnonymous = (field == "shareJobWhenAnonymous") ? boolValue : prefs.shareJobWhenAnonymous;
                
                Core::Presence currentPresence = queryPlayerPresence();
                bool gameIsAnonymous = currentPresence.isAnonymous;
                
                bool isAnonymous = gameIsAnonymous && !shareJobWhenAnonymous;
                
                if (updateMyStatusUseCase_ && !apiKey_.empty() && !characterName_.empty()) {
                    std::thread([this, showOnlineStatus, shareLocation, isAnonymous, shareJobWhenAnonymous]() {
                        performStatusUpdateImmediate(showOnlineStatus, shareLocation, isAnonymous, shareJobWhenAnonymous);
                    }).detach();
                }
            }
        }
        
        if (field == "debugMode" || field == "overwriteNotesOnUpload" || 
            field == "overwriteNotesOnDownload" || field == "notificationDuration" ||
            field == "customCloseKeyCode" || field == "controllerCloseButton" || 
            field == "windowsLocked") {
            scheduleAutoSave();
        }
        
        updateOptionsViewModel();
        
        // Update ToastManager position immediately when notification position changes
        if (field == "notificationPositionX" || field == "notificationPositionY") {
            Core::Preferences prefs = preferencesUseCase_->getPreferences();
            // Convert -1 (old default) to default position before setting position
            float posX = (prefs.notificationPositionX < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : prefs.notificationPositionX;
            float posY = (prefs.notificationPositionY < 0.0f) ? App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : prefs.notificationPositionY;
            UI::Notifications::ToastManager::getInstance().setPosition(posX, posY);
        }
        
        if (field == "shareFriendsAcrossAlts") {
            updateFriendListViewModelsFromPreferences();
        }
        
        //         // Update NotesViewModel mode
        //             notesViewModel_->setServerMode(newUseServerNotes);
    } catch (...) {
        logger_->error("AshitaAdapter: Exception while parsing preference update");
        optionsViewModel_->setError("Failed to parse preference update");
    }
}

void AshitaAdapter::handleUpdateWindowLock(const std::string& valueJson) {
    try {
        std::string windowId;
        if (!Protocol::JsonUtils::extractStringField(valueJson, "windowId", windowId)) {
            logger_->error("AshitaAdapter: Invalid window lock update JSON - missing 'windowId'");
            return;
        }
        
        bool locked;
        if (!Protocol::JsonUtils::extractBooleanField(valueJson, "locked", locked)) {
            logger_->error("AshitaAdapter: Invalid window lock update JSON - missing or invalid 'locked'");
            return;
        }
        
        if (preferencesStore_) {
            bool success = Platform::Ashita::AshitaPreferencesStore::saveWindowLockState(windowId, locked);
            if (!success) {
                logger_->error("AshitaAdapter: Failed to save window lock state for: " + windowId);
            } else {
                logger_->debug("AshitaAdapter: Window lock state updated - " + windowId + " = " + (locked ? "locked" : "unlocked"));
            }
        }
    } catch (...) {
        logger_->error("AshitaAdapter: Exception in handleUpdateWindowLock");
    }
}

void AshitaAdapter::handleSavePreferences() {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    Core::Preferences prefs = optionsViewModel_->toPreferences();
    
    App::UseCases::PreferencesResult result = preferencesUseCase_->updateServerPreferences(prefs, apiKey_, characterName_);
    if (!result.success) {
        logger_->error("AshitaAdapter: Failed to save server preferences: " + result.error);
        optionsViewModel_->setError(result.error);
        return;
    }
    
    result = preferencesUseCase_->updateLocalPreferences(prefs);
    if (!result.success) {
        logger_->error("AshitaAdapter: Failed to save local preferences: " + result.error);
        optionsViewModel_->setError(result.error);
        return;
    }
    
    optionsViewModel_->clearDirtyFlags();
    optionsViewModel_->clearError();
    
    updateOptionsViewModel();
    
    if (preferencesUseCase_) {
        Core::Preferences savedPrefs = preferencesUseCase_->getPreferences();
        UI::Notifications::ToastManager::getInstance().setPosition(
            savedPrefs.notificationPositionX,
            savedPrefs.notificationPositionY
        );
    }
    
    logger_->info("AshitaAdapter: Preferences saved successfully");
    
    if (isDebugEnabled()) {
        pushDebugLog("Settings saved successfully");
    }
}

void AshitaAdapter::handleResetPreferences() {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    App::UseCases::PreferencesResult result = preferencesUseCase_->resetPreferences();
    if (!result.success) {
        logger_->error("AshitaAdapter: Failed to reset preferences: " + result.error);
        optionsViewModel_->setError(result.error);
        return;
    }
    
    preferencesUseCase_->savePreferences(apiKey_, characterName_);
    
    updateOptionsViewModel();
    optionsViewModel_->clearDirtyFlags();
    optionsViewModel_->clearError();
    
    logger_->info("AshitaAdapter: Preferences reset to defaults");
    
    if (isDebugEnabled()) {
        pushDebugLog("Settings reset to defaults");
    }
}

void AshitaAdapter::updateOptionsViewModel() {
    if (!optionsViewModel_ || !preferencesUseCase_) {
        return;
    }
    
    Core::Preferences prefs = preferencesUseCase_->getPreferences();
    optionsViewModel_->updateFromPreferences(prefs);
    debugEnabled_.store(prefs.debugMode);
    
    lastPreferences_ = prefs;
    
    if (soundService_) {
        soundService_->updatePreferences(prefs);
    }
    
    if (windowClosePolicy_) {
        windowClosePolicy_->setWindowsLocked(prefs.windowsLocked);
    }
}

void AshitaAdapter::updateFriendListViewModelsFromPreferences() {
    if (!preferencesUseCase_) {
        return;
    }

    const Core::Preferences prefs = preferencesUseCase_->getPreferences();
    const bool debugEnabled = isDebugEnabled();

    if (viewModel_) {
        viewModel_->setDebugEnabled(debugEnabled);
    }

    if (viewModel_) {
        viewModel_->setShowJobColumn(prefs.mainFriendView.showJob);
        viewModel_->setShowZoneColumn(prefs.mainFriendView.showZone);
        viewModel_->setShowNationRankColumn(prefs.mainFriendView.showNationRank);
        viewModel_->setShowLastSeenColumn(prefs.mainFriendView.showLastSeen);
    }

    if (quickOnlineViewModel_) {
        quickOnlineViewModel_->setDebugEnabled(debugEnabled);
        quickOnlineViewModel_->setShowJobColumn(prefs.quickOnlineFriendView.showJob);
        quickOnlineViewModel_->setShowZoneColumn(prefs.quickOnlineFriendView.showZone);
        quickOnlineViewModel_->setShowNationRankColumn(prefs.quickOnlineFriendView.showNationRank);
        quickOnlineViewModel_->setShowLastSeenColumn(prefs.quickOnlineFriendView.showLastSeen);
    }
    
    if (windowManager_) {
        windowManager_->getMainWindow().setShareFriendsAcrossAlts(prefs.shareFriendsAcrossAlts);
        windowManager_->getMainWindow().setFriendViewSettings(prefs.mainFriendView);
        windowManager_->getQuickOnlineWindow().setShareFriendsAcrossAlts(prefs.shareFriendsAcrossAlts);
        windowManager_->getQuickOnlineWindow().setFriendViewSettings(prefs.quickOnlineFriendView);
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    if (cachedFriendList_.empty() || cachedFriendStatuses_.empty()) {
        return;
    }

    const uint64_t now = clock_ ? clock_->nowMs() : 0;

    if (viewModel_) {
        viewModel_->updateWithRequests(cachedFriendList_, cachedFriendStatuses_, cachedOutgoingRequests_, now);
    }

    if (quickOnlineViewModel_) {
        const auto friendNames = cachedFriendList_.getFriendNames();
        const auto onlineNames = Core::FriendListFilter::filterOnline(friendNames, cachedFriendStatuses_);
        std::set<std::string> onlineSet;
        for (const auto& n : onlineNames) {
            std::string lower = n;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            onlineSet.insert(lower);
        }

        Core::FriendList onlineList;
        for (const auto& f : cachedFriendList_.getFriends()) {
            std::string lower = f.name;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (onlineSet.find(lower) != onlineSet.end()) {
                onlineList.addFriend(f);
            }
        }

        quickOnlineViewModel_->updateWithRequests(onlineList, cachedFriendStatuses_, cachedOutgoingRequests_, cachedIncomingRequests_, now);
    }
}

void AshitaAdapter::handleToggleColumnVisibility(const std::string& jsonData) {
    if (!preferencesUseCase_) {
        return;
    }

    std::string scope;
    std::string column;

    const bool hasScope = Protocol::JsonUtils::extractStringField(jsonData, "scope", scope);
    const bool hasColumn = Protocol::JsonUtils::extractStringField(jsonData, "column", column);

    if (!hasColumn) {
        scope = "FriendList";
        column = jsonData;
    } else if (!hasScope) {
        scope = "FriendList";
    }

    UI::FriendListViewModel* targetVm = nullptr;
    if (scope == "QuickOnline") {
        targetVm = quickOnlineViewModel_.get();
    } else {
        targetVm = viewModel_.get();
        scope = "FriendList";
    }

    if (!targetVm) {
        return;
    }

    bool value = false;
    std::string field;

    auto pick = [&](const std::string& colId, const std::string& friendListField, const std::string& quickField, bool currentValue) {
        if (column == colId) {
            field = (scope == "QuickOnline") ? quickField : friendListField;
            value = currentValue;
            return true;
        }
        return false;
    };

    if (pick("friended_as", "showFriendedAsColumn", "quickOnlineShowFriendedAsColumn", targetVm->getShowFriendedAsColumn()) ||
        pick("job", "showJobColumn", "quickOnlineShowJobColumn", targetVm->getShowJobColumn()) ||
        pick("zone", "showZoneColumn", "quickOnlineShowZoneColumn", targetVm->getShowZoneColumn()) ||
        pick("nation", "showNationColumn", "quickOnlineShowNationColumn", targetVm->getShowNationColumn()) ||
        pick("rank", "showRankColumn", "quickOnlineShowRankColumn", targetVm->getShowRankColumn()) ||
        pick("last_seen", "showLastSeenColumn", "quickOnlineShowLastSeenColumn", targetVm->getShowLastSeenColumn())) {
    } else {
        if (logger_) {
            logger_->warning("AshitaAdapter: Unknown column id for ToggleColumnVisibility: " + column);
        }
        return;
    }

    App::UseCases::PreferencesResult result;
    if (scope == "QuickOnline") {
        result = preferencesUseCase_->updateServerPreference(field, value, apiKey_, characterName_);
    } else {
        result = preferencesUseCase_->updateLocalPreference(field, value, apiKey_, characterName_);
    }
    
    if (!result.success) {
        if (logger_) {
            logger_->error("AshitaAdapter: Failed to persist column visibility: " + result.error);
        }
        return;
    }

    updateFriendListViewModelsFromPreferences();
}

void AshitaAdapter::handleSaveNote(const std::string& friendName) {
    if (!saveNoteUseCase_ || friendName.empty()) {
        if (notesViewModel_) {
            notesViewModel_->setError("Not connected or invalid friend name");
        }
        return;
    }
    
    if (!notesViewModel_) {
        logger_->error("AshitaAdapter: NotesViewModel not available");
        return;
    }
    
    std::string noteText = notesViewModel_->getCurrentNoteText();
    
    notesViewModel_->setLoading(true);
    notesViewModel_->clearError();
    
    std::thread([this, friendName, noteText]() {
        App::UseCases::SaveNoteResult result = saveNoteUseCase_->saveNote(apiKey_, characterName_, friendName, noteText, false);
        
        uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
        
        if (result.success) {
            if (notesState_.dirty && accountId_ > 0) {
                NotesPersistence::saveToFile(notesState_, accountId_);
                notesState_.dirty = false;
            }
            
            logger_->info("AshitaAdapter: Note saved for " + friendName);
            
            if (isDebugEnabled()) {
                pushDebugLog("Note saved for " + friendName);
            }
            
            if (notesViewModel_) {
                notesViewModel_->markSaved(result.note.updatedAt);
                notesViewModel_->setActionStatusSuccess("Note saved", timestampMs);
                notesViewModel_->setLoading(false);
            }
        } else {
            logger_->error("AshitaAdapter: Failed to save note: " + result.error);
            if (notesViewModel_) {
                notesViewModel_->setError(result.error);
                notesViewModel_->setActionStatusError(result.error, "", timestampMs);
                notesViewModel_->setLoading(false);
            }
        }
    }).detach();
}

void AshitaAdapter::handleDeleteNote(const std::string& friendName) {
    if (!deleteNoteUseCase_ || friendName.empty()) {
        if (notesViewModel_) {
            notesViewModel_->setError("Not connected or invalid friend name");
        }
        return;
    }
    
    if (notesViewModel_) {
        notesViewModel_->setLoading(true);
        notesViewModel_->clearError();
    }
    
    std::thread([this, friendName]() {
        App::UseCases::DeleteNoteResult result = deleteNoteUseCase_->deleteNote(apiKey_, characterName_, friendName, false);
        
        uint64_t timestampMs = clock_ ? clock_->nowMs() : 0;
        
        if (result.success) {
            if (notesState_.dirty && accountId_ > 0) {
                NotesPersistence::saveToFile(notesState_, accountId_);
                notesState_.dirty = false;
            }
            
            logger_->info("AshitaAdapter: Note deleted for " + friendName);
            
            if (isDebugEnabled()) {
                pushDebugLog("Note deleted for " + friendName);
            }
            
            if (notesViewModel_) {
                notesViewModel_->markDeleted();
                notesViewModel_->setActionStatusSuccess("Note deleted", timestampMs);
                notesViewModel_->setLoading(false);
            }
        } else {
            logger_->error("AshitaAdapter: Failed to delete note: " + result.error);
            if (notesViewModel_) {
                notesViewModel_->setError(result.error);
                notesViewModel_->setActionStatusError(result.error, "", timestampMs);
                notesViewModel_->setLoading(false);
            }
        }
    }).detach();
}

void AshitaAdapter::initializeIconManager(IDirect3DDevice8* device) {
    if (iconManager_) {
        iconManager_->initialize(device);
    }
}

void AshitaAdapter::handleZoneChangePacket() {
    if (!initialized_ || !eventQueue_) {
        return;
    }
    
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!ashitaCore_) {
            return;
        }
        
        IMemoryManager* memoryMgr = ashitaCore_->GetMemoryManager();
        if (memoryMgr == nullptr) {
            return;
        }
        
        IParty* party = memoryMgr->GetParty();
        if (party == nullptr) {
            return;
        }
        
        uint16_t zoneId = party->GetMemberZone(0);
        if (zoneId > 0) {
            std::string zoneName = getZoneNameFromId(zoneId);
            
            uint64_t timestamp = clock_->nowMs();
            App::Events::ZoneChanged event(zoneId, zoneName, timestamp);
            eventQueue_->pushZoneChanged(event);
            
            logger_->debug("AshitaAdapter: Zone change packet detected: Zone ID " + 
                          std::to_string(zoneId) + " (" + zoneName + ")");
        }
    }).detach();
}

void AshitaAdapter::handleCharacterChangedEvent(const App::Events::CharacterChanged& event) {
    if (!handleCharacterChangedUseCase_) {
        return;
    }
    
    logger_->info("AshitaAdapter: Processing character changed event: " + event.newCharacterName);
    
    if (isDebugEnabled()) {
        pushDebugLog("Character changed to " + event.newCharacterName);
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        if (characterChangedInProgress_) {
            return;
        }
        characterChangedInProgress_ = true;
        characterChangedCompleted_ = false;
        pendingCharacterChangedEvent_ = event;
    }

    if (viewModel_) {
        viewModel_->setCurrentCharacterName(event.newCharacterName);
    }

    std::string currentApiKey;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        currentApiKey = apiKey_;  // Use currently connected character's API key
    }
    
    std::thread([this, event, currentApiKey]() {
        App::UseCases::CharacterChangeResult result = handleCharacterChangedUseCase_->handleCharacterChanged(event, currentApiKey);
        
        ApiKeyPersistence::saveToFile(apiKeyState_);

        if (preferencesUseCase_) {
            try {
                preferencesUseCase_->loadPreferences("", event.newCharacterName);
            } catch (...) {
            }
        }

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            pendingCharacterChangedResult_ = result;
            characterChangedCompleted_ = true;
        }
    }).detach();
}

void AshitaAdapter::handleZoneChangedEvent(const App::Events::ZoneChanged& event) {
    if (!handleZoneChangedUseCase_) {
        return;
    }
    
    logger_->debug("AshitaAdapter: Processing zone changed event: " + std::to_string(event.zoneId));
    
    App::UseCases::ZoneChangeResult result = handleZoneChangedUseCase_->handleZoneChanged(event);
    
    if (result.success) {
        {
            std::lock_guard<std::mutex> lock(zoneCacheMutex_);
            cachedZoneId_ = event.zoneId;
            cachedZoneName_ = event.zoneName;
        }
        
        
        if (isDebugEnabled()) {
            pushDebugLog("Zone changed to " + event.zoneName);
        }
    }
}

void AshitaAdapter::scheduleAutoSave() {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    uint64_t now = clock_ ? clock_->nowMs() : 0;
    lastPreferenceChangeTime_ = now;
    
    {
        std::lock_guard<std::mutex> lock(autoSaveMutex_);
        autoSavePending_ = true;
        autoSaveThreadShouldExit_ = false;
    }
    
    if (autoSaveThread_ && autoSaveThread_->joinable()) {
        return;
    }
    
    if (autoSaveThread_) {
        autoSaveThread_->join();
        autoSaveThread_.reset();
    }
    
    autoSaveThread_ = std::make_unique<std::thread>([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Check every 100ms
            
            bool shouldExit = false;
            bool shouldSave = false;
            
            {
                std::lock_guard<std::mutex> lock(autoSaveMutex_);
                if (autoSaveThreadShouldExit_) {
                    shouldExit = true;
                } else if (autoSavePending_) {
                    uint64_t now = clock_ ? clock_->nowMs() : 0;
                    uint64_t timeSinceChange = now - lastPreferenceChangeTime_;
                    
                    if (timeSinceChange >= PREFERENCES_AUTO_SAVE_DELAY_MS) {
                        shouldSave = true;
                        autoSavePending_ = false;
                    }
                }
            }
            
            if (shouldExit) {
                break;
            }
            
            if (shouldSave) {
                performAutoSave();
                break;  // Exit thread after saving
            }
        }
    });
}

void AshitaAdapter::performAutoSave() {
    if (!preferencesUseCase_ || !optionsViewModel_) {
        return;
    }
    
    Core::Preferences prefs = optionsViewModel_->toPreferences();
    
    if (!apiKey_.empty() && !characterName_.empty()) {
        std::thread([this, prefs]() {
            App::UseCases::PreferencesResult result = preferencesUseCase_->updateServerPreferences(prefs, apiKey_, characterName_);
            if (!result.success) {
                logger_->warning("AshitaAdapter: Auto-save server preferences failed: " + result.error);
            }
        }).detach();
    }
    
    App::UseCases::PreferencesResult result = preferencesUseCase_->updateLocalPreferences(prefs);
    if (!result.success) {
        logger_->warning("AshitaAdapter: Auto-save local preferences failed: " + result.error);
    } else {
        logger_->debug("AshitaAdapter: Preferences auto-saved");
        
        if (isDebugEnabled()) {
            pushDebugLog("Settings auto-saved");
        }
    }
    
    optionsViewModel_->clearDirtyFlags();
}

void AshitaAdapter::scheduleStatusUpdate(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous) {
    if (!updateMyStatusUseCase_ || apiKey_.empty() || characterName_.empty()) {
        return;
    }
    
    uint64_t now = clock_ ? clock_->nowMs() : 0;
    lastStatusChangeTime_ = now;
    
    {
        std::lock_guard<std::mutex> lock(statusUpdateMutex_);
        pendingShowOnlineStatus_ = showOnlineStatus;
        pendingShareLocation_ = shareLocation;
        pendingIsAnonymous_ = isAnonymous;
        pendingShareJobWhenAnonymous_ = shareJobWhenAnonymous;
        statusUpdatePending_ = true;
        hasPendingStatusUpdate_ = true;
        statusUpdateThreadShouldExit_ = false;
    }
    
    if (statusUpdateThread_ && statusUpdateThread_->joinable()) {
        return;
    }
    
    if (statusUpdateThread_) {
        statusUpdateThread_->join();
        statusUpdateThread_.reset();
    }
    
    statusUpdateThread_ = std::make_unique<std::thread>([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Check every 100ms
            
            bool shouldExit = false;
            bool shouldUpdate = false;
            bool showOnlineStatus;
            bool shareLocation;
            bool isAnonymous;
            
            {
                std::lock_guard<std::mutex> lock(statusUpdateMutex_);
                if (statusUpdateThreadShouldExit_) {
                    shouldExit = true;
                } else if (statusUpdatePending_ && hasPendingStatusUpdate_) {
                    uint64_t now = clock_ ? clock_->nowMs() : 0;
                    uint64_t timeSinceChange = now - lastStatusChangeTime_;
                    
                    if (timeSinceChange >= STATUS_UPDATE_DELAY_MS) {
                        shouldUpdate = true;
                        statusUpdatePending_ = false;
                        showOnlineStatus = pendingShowOnlineStatus_;
                        shareLocation = pendingShareLocation_;
                        isAnonymous = pendingIsAnonymous_;
                        hasPendingStatusUpdate_ = false;
                    }
                }
            }
            
            if (shouldExit) {
                break;
            }
            
            if (shouldUpdate) {
                performStatusUpdate();
                break;  // Exit thread after updating
            }
        }
    });
}

void AshitaAdapter::performStatusUpdate() {
    if (!updateMyStatusUseCase_ || apiKey_.empty() || characterName_.empty()) {
        return;
    }
    
    bool showOnlineStatus;
    bool shareLocation;
    bool isAnonymous;
    bool shareJobWhenAnonymous;
    
    {
        std::lock_guard<std::mutex> lock(statusUpdateMutex_);
        showOnlineStatus = pendingShowOnlineStatus_;
        shareLocation = pendingShareLocation_;
        isAnonymous = pendingIsAnonymous_;
        shareJobWhenAnonymous = pendingShareJobWhenAnonymous_;
    }
    
    App::UseCases::UpdateMyStatusResult result = updateMyStatusUseCase_->updateStatus(
        apiKey_, characterName_, showOnlineStatus, shareLocation, isAnonymous, shareJobWhenAnonymous);
    
    if (!result.success) {
        logger_->warning("AshitaAdapter: Status update failed: " + result.error);
    } else {
        logger_->debug("AshitaAdapter: Status updated successfully");
        updatePresenceAsync();
    }
}

void AshitaAdapter::performStatusUpdateImmediate(bool showOnlineStatus, bool shareLocation, bool isAnonymous, bool shareJobWhenAnonymous) {
    if (!updateMyStatusUseCase_ || apiKey_.empty() || characterName_.empty()) {
        logger_->warning("AshitaAdapter: Cannot perform immediate status update - missing use case or credentials");
        return;
    }
    
    logger_->info("AshitaAdapter: Performing immediate privacy settings update - showOnlineStatus=" + 
                  (showOnlineStatus ? std::string("true") : std::string("false")) + 
                  ", shareLocation=" + (shareLocation ? std::string("true") : std::string("false")) +
                  ", isAnonymous=" + (isAnonymous ? std::string("true") : std::string("false")) +
                  ", shareJobWhenAnonymous=" + (shareJobWhenAnonymous ? std::string("true") : std::string("false")));
    
    App::UseCases::UpdateMyStatusResult result = updateMyStatusUseCase_->updateStatus(
        apiKey_, characterName_, showOnlineStatus, shareLocation, isAnonymous, shareJobWhenAnonymous);
    
    if (!result.success) {
        logger_->error("AshitaAdapter: Immediate privacy settings update failed: " + result.error);
    } else {
        logger_->info("AshitaAdapter: Privacy settings updated immediately on server");
        updatePresenceAsync();
    }
}

bool AshitaAdapter::isDebugEnabled() const {
#if defined(_DEBUG) || defined(DEBUG)
    return true;
#else
    return debugEnabled_.load();
#endif
}

void AshitaAdapter::syncDebugEnabledFromPreferences() {
    bool enabled = false;
    if (preferencesUseCase_) {
        enabled = preferencesUseCase_->getPreferences().debugMode;
    }
    debugEnabled_.store(enabled);
    if (!isDebugEnabled() && windowManager_) {
        windowManager_->getDebugLogWindow().setVisible(false);
    }
}

namespace {
    std::string ensureFriendListPrefix(const std::string& message) {
        if (message.find("[FriendList]") == 0) {
            return message;
        }
        return "[FriendList] " + message;
    }
}

void AshitaAdapter::pushDebugLog(const std::string& message) {
    if (!isDebugEnabled()) {
        return;
    }
    if (!logger_) {
        return;
    }
    std::string fullMessage = ensureFriendListPrefix(message);
    Debug::DebugLog::getInstance().push(fullMessage);
    logger_->debug(fullMessage);
}

Core::MemoryStats AshitaAdapter::getAdapterCacheStats() const {
    size_t bytes = 0;
    size_t count = 0;
    
    for (const auto& req : cachedOutgoingRequests_) {
        bytes += sizeof(Protocol::FriendRequestPayload);
        bytes += req.requestId.capacity();
        bytes += req.fromCharacterName.capacity();
        bytes += req.toCharacterName.capacity();
        ++count;
    }
    bytes += cachedOutgoingRequests_.capacity() * sizeof(Protocol::FriendRequestPayload);
    
    for (const auto& req : cachedIncomingRequests_) {
        bytes += sizeof(Protocol::FriendRequestPayload);
        bytes += req.requestId.capacity();
        bytes += req.fromCharacterName.capacity();
        bytes += req.toCharacterName.capacity();
        ++count;
    }
    bytes += cachedIncomingRequests_.capacity() * sizeof(Protocol::FriendRequestPayload);
    
    for (const auto& status : cachedFriendStatuses_) {
        bytes += sizeof(Core::FriendStatus);
        bytes += status.characterName.capacity();
        bytes += status.displayName.capacity();
        bytes += status.job.capacity();
        bytes += status.rank.capacity();
        bytes += status.zone.capacity();
        bytes += status.altCharacterName.capacity();
        bytes += status.friendedAs.capacity();
        for (const auto& linked : status.linkedCharacters) {
            bytes += linked.capacity();
        }
        bytes += status.linkedCharacters.capacity() * sizeof(std::string);
        ++count;
    }
    bytes += cachedFriendStatuses_.capacity() * sizeof(Core::FriendStatus);
    
    bytes += cachedZoneName_.capacity();
    
    for (const auto& pair : previousOnlineStatus_) {
        bytes += pair.first.capacity();
        bytes += sizeof(bool);
        ++count;
    }
    bytes += previousOnlineStatus_.size() * sizeof(std::string);
    
    for (const auto& id : processedRequestIds_) {
        bytes += id.capacity();
        ++count;
    }
    bytes += processedRequestIds_.size() * sizeof(std::string);
    
    return Core::MemoryStats(count, bytes, "Adapter Caches");
}

void AshitaAdapter::printMemoryStats() {
    std::vector<Core::MemoryStats> stats;
    
    stats.push_back(cachedFriendList_.getMemoryStats());
    stats.push_back(notesState_.getMemoryStats());
    stats.push_back(themeState_.getMemoryStats());
    stats.push_back(UI::Notifications::ToastManager::getInstance().getMemoryStats());
    if (iconManager_) {
        stats.push_back(iconManager_->getMemoryStats());
    }
    
    if (viewModel_) {
        stats.push_back(viewModel_->getMemoryStats());
    }
    if (quickOnlineViewModel_) {
        auto quickStats = quickOnlineViewModel_->getMemoryStats();
        quickStats.category = "QuickOnline ViewModel";
        stats.push_back(quickStats);
    }
    
#ifndef DISABLE_NOTES
    if (notesViewModel_) {
        stats.push_back(notesViewModel_->getMemoryStats());
    }
#endif
    
    if (altVisibilityViewModel_) {
        stats.push_back(altVisibilityViewModel_->getMemoryStats());
    }
    
    if (themesViewModel_) {
        stats.push_back(themesViewModel_->getMemoryStats());
    }
    
    if (optionsViewModel_) {
        stats.push_back(optionsViewModel_->getMemoryStats());
    }
    
    if (windowManager_) {
        stats.push_back(windowManager_->getMainWindow().getMemoryStats());
        stats.push_back(windowManager_->getQuickOnlineWindow().getMemoryStats());
        stats.push_back(windowManager_->getNoteEditorWindow().getMemoryStats());
    }
    
    stats.push_back(getAdapterCacheStats());
    
    size_t totalBytes = 0;
    size_t totalEntries = 0;
    for (const auto& stat : stats) {
        totalBytes += stat.estimatedBytes;
        totalEntries += stat.entryCount;
    }
    
    pushToGameEcho("=== FFXIFriendList Memory Usage ===");
    pushToGameEcho("");
    
    for (const auto& stat : stats) {
        size_t kb = (stat.estimatedBytes + 512) / 1024;
        pushToGameEcho("  " + stat.category + ": " + std::to_string(stat.entryCount) + 
                      " entries (~" + std::to_string(kb) + " KB)");
    }
    
    pushToGameEcho("");
    size_t totalKb = (totalBytes + 512) / 1024;
    pushToGameEcho("  TOTAL: " + std::to_string(totalEntries) + " entries (~" + std::to_string(totalKb) + " KB)");
    pushToGameEcho("");
    pushToGameEcho("Note: This is an estimate of plugin-owned data structures only.");
    pushToGameEcho("      It does NOT include ImGui, Direct3D, or OS-level allocations.");
}

void AshitaAdapter::pushToGameEcho(const std::string& message) {
#ifndef BUILDING_TESTS
    if (chatManager_ == nullptr) {
        return;
    }
    std::string fullMessage = ensureFriendListPrefix(message);
    try {
        chatManager_->Write(200, false, fullMessage.c_str());
    } catch (...) {
    }
#else
    (void)message;
#endif
}

void AshitaAdapter::checkForStatusChanges(const std::vector<Core::FriendStatus>& currentStatuses) {
    
    std::lock_guard<std::mutex> lock(statusChangeMutex_);
    
    std::map<std::string, bool> currentOnlineStatus;
    std::map<std::string, std::string> displayNames;  // For notifications
    
    for (const auto& status : currentStatuses) {
        std::string friendNameLower = status.characterName;
        std::transform(friendNameLower.begin(), friendNameLower.end(), friendNameLower.begin(),
                      [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        
        bool isOnline = status.isOnline;
        currentOnlineStatus[friendNameLower] = isOnline;
        
        std::string displayName = status.displayName.empty() ? status.characterName : status.displayName;
        if (!displayName.empty()) {
            displayName[0] = static_cast<char>(std::toupper(displayName[0]));
            for (size_t i = 1; i < displayName.length(); ++i) {
                if (displayName[i-1] == ' ') {
                    displayName[i] = static_cast<char>(std::toupper(displayName[i]));
                } else {
                    displayName[i] = static_cast<char>(std::tolower(displayName[i]));
                }
            }
        }
        displayNames[friendNameLower] = displayName;
    }
    
    if (!initialStatusScanComplete_) {
        previousOnlineStatus_ = currentOnlineStatus;
        initialStatusScanComplete_ = true;
        logger_->debug("AshitaAdapter: Initial status scan complete, notifications enabled");
        return;
    }
    
    for (const auto& currentPair : currentOnlineStatus) {
        const std::string& friendName = currentPair.first;
        bool isCurrentlyOnline = currentPair.second;
        
        auto prevIt = previousOnlineStatus_.find(friendName);
        bool wasPreviouslyOnline = (prevIt != previousOnlineStatus_.end() && prevIt->second);
        
        if (!wasPreviouslyOnline && isCurrentlyOnline) {
            std::string displayName = displayNames[friendName];
            if (displayName.empty()) {
                displayName = friendName;  // Fallback
            }
            
            if (notificationUseCase_ && clock_) {
                int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                auto toast = notificationUseCase_->createFriendOnlineToast(displayName, currentTime);
                UI::Notifications::ToastManager::getInstance().addToast(toast);
                
                if (isDebugEnabled()) {
                    pushDebugLog("[Notifications] Friend " + displayName + " came online - toast created");
                    logger_->debug("[Notifications] Friend online: " + displayName + ", toast count: " + 
                        std::to_string(UI::Notifications::ToastManager::getInstance().getToastCount()));
                }
            }
            
            previousOnlineStatus_[friendName] = true;
        }
        
        if (wasPreviouslyOnline && !isCurrentlyOnline) {
            std::string displayName = displayNames[friendName];
            if (displayName.empty()) {
                displayName = friendName;  // Fallback
            }
            
            if (notificationUseCase_ && clock_) {
                int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
                auto toast = notificationUseCase_->createFriendOfflineToast(displayName, currentTime);
                UI::Notifications::ToastManager::getInstance().addToast(toast);
                
                if (isDebugEnabled()) {
                    pushDebugLog("[Notifications] Friend " + displayName + " went offline - toast created");
                    logger_->debug("[Notifications] Friend offline: " + displayName + ", toast count: " + 
                        std::to_string(UI::Notifications::ToastManager::getInstance().getToastCount()));
                }
            }
        }
    }
    
    for (const auto& currentPair : currentOnlineStatus) {
        previousOnlineStatus_[currentPair.first] = currentPair.second;
    }
}


void AshitaAdapter::handleEscapeKey() {
    if (!initialized_ || !windowClosePolicy_ || !escKeyDetector_ || !backspaceKeyDetector_ || !customKeyDetector_) {
        return;
    }
    
    if (capturingCustomKey_) {
        processKeyCapture();
        return;  // Don't process window closing while capturing
    }
    
    if (!preferencesUseCase_) {
        return;
    }
    
    Core::Preferences prefs = preferencesUseCase_->getPreferences();
    
    int keyToCheck = (prefs.customCloseKeyCode > 0 && prefs.customCloseKeyCode < 256) 
                     ? prefs.customCloseKeyCode 
                     : VK_ESCAPE;
    
    bool keyPressed = false;
    if (keyToCheck == VK_ESCAPE) {
        keyPressed = escKeyDetector_->update(VK_ESCAPE);
    } else if (keyToCheck == VK_BACK) {
        keyPressed = backspaceKeyDetector_->update(VK_BACK);
    } else {
        keyPressed = customKeyDetector_->update(keyToCheck);
    }
    
    bool controllerPressed = false;
    if (!keyPressed) {
        controllerPressed = checkControllerInput(prefs.controllerCloseButton);
    }
    
    if (keyPressed || controllerPressed) {
        if (uiRenderer_ && uiRenderer_->IsAnyPopupOpen()) {
            if (logger_) {
                logger_->debug("[FriendList] Close deferred: menu open");
            }
            return;
        }

        if (isGameMenuOpen()) {
            return;
        }
        
        if (windowClosePolicy_->anyWindowOpen()) {
            std::string closedWindow = windowClosePolicy_->closeTopMostWindow();
            if (!closedWindow.empty() && logger_) {
                std::string inputType = controllerPressed ? "Controller" : "Keyboard";
                logger_->debug("[FriendList] " + inputType + ": closing " + closedWindow);
            } else if (windowClosePolicy_->areWindowsLocked() && logger_) {
                logger_->debug("[FriendList] ESC pressed but windows are locked");
            }
        } else if (logger_) {
            logger_->debug("[FriendList] ESC pressed but no plugin windows open to close");
        }
    }
}

void AshitaAdapter::startCapturingCustomKey() {
    capturingCustomKey_ = true;
    capturedKeyCode_ = 0;
    if (customKeyDetector_) {
        customKeyDetector_->reset();  // Reset detector state
    }
    if (logger_) {
        logger_->info("[FriendList] Key capture started - press any key to set custom close key");
    }
    if (notificationUseCase_ && clock_) {
        int64_t currentTime = static_cast<int64_t>(clock_->nowMs());
        auto toast = notificationUseCase_->createInfoToast(
            std::string(App::Notifications::Constants::TITLE_KEY_CAPTURE),
            std::string(App::Notifications::Constants::MESSAGE_PRESS_ANY_KEY),
            currentTime);
        UI::Notifications::ToastManager::getInstance().addToast(toast);
    }
}

void AshitaAdapter::processKeyCapture() {
    if (!capturingCustomKey_) {
        return;
    }
    
    for (int vk = 1; vk < 256; vk++) {
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || 
            vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
            continue;
        }
        
        if ((::GetAsyncKeyState(vk) & 0x8000) != 0) {
            capturedKeyCode_ = vk;
            capturingCustomKey_ = false;
            
            if (preferencesUseCase_) {
                preferencesUseCase_->updateLocalPreference("customCloseKeyCode", static_cast<float>(vk), apiKey_, characterName_);
                scheduleAutoSave();
                if (logger_) {
                    char keyName[256] = {0};
                    if (vk >= 'A' && vk <= 'Z') {
                        keyName[0] = static_cast<char>(vk);
                        keyName[1] = '\0';
                    } else if (vk >= '0' && vk <= '9') {
                        keyName[0] = static_cast<char>(vk);
                        keyName[1] = '\0';
                    } else {
                        sprintf_s(keyName, sizeof(keyName), "VK_%d", vk);
                    }
                    logger_->info("[FriendList] Custom close key set to: " + std::string(keyName) + " (VK_" + std::to_string(vk) + ")");
                }
            }
            
            if (customKeyDetector_) {
                customKeyDetector_->reset();
            }
            
            return;
        }
    }
}

bool AshitaAdapter::checkControllerInput(int buttonCode) {
#ifndef BUILDING_TESTS
    if (buttonCode == 0) {
        return false;
    }
    
    typedef DWORD (WINAPI *XInputGetStateFunc)(DWORD, void*);
    static XInputGetStateFunc xInputGetState = nullptr;
    static HMODULE xInputDll = nullptr;
    
    if (!xInputDll) {
        xInputDll = LoadLibraryA("xinput1_4.dll");
        if (!xInputDll) {
            xInputDll = LoadLibraryA("xinput9_1_0.dll");
        }
        if (xInputDll) {
            xInputGetState = (XInputGetStateFunc)GetProcAddress(xInputDll, "XInputGetState");
        }
    }
    
    if (!xInputGetState) {
        return false;
    }
    
    struct XINPUT_STATE {
        DWORD dwPacketNumber;
        struct {
            WORD wButtons;
            BYTE bLeftTrigger;
            BYTE bRightTrigger;
            SHORT sThumbLX;
            SHORT sThumbLY;
            SHORT sThumbRX;
            SHORT sThumbRY;
        } Gamepad;
    };
    
    static std::map<int, bool> lastButtonStates;
    XINPUT_STATE state = {0};
    
    if (xInputGetState(0, &state) == ERROR_SUCCESS) {
        bool buttonPressed = (state.Gamepad.wButtons & buttonCode) != 0;
        bool& lastButtonState = lastButtonStates[buttonCode];
        bool buttonJustPressed = buttonPressed && !lastButtonState;
        lastButtonState = buttonPressed;
        return buttonJustPressed;
    }
#endif
    return false;
}

bool AshitaAdapter::isGameMenuOpen() {
#ifndef BUILDING_TESTS
    // This avoids duplicate checks and uses the already-polled state
    if (friendListMenuDetector_) {
        return friendListMenuDetector_->isMenuOpen();
    }
    return false;
#else
    return false;
#endif
}

void AshitaAdapter::pauseBackgroundForTests() {
    logger_->info("AshitaAdapter: Pausing background work for tests");
    backgroundPausedForTests_.store(true);
    // Notify any waiting threads
    {
        std::lock_guard<std::mutex> lock(idleWaitMutex_);
        idleWaitCondition_.notify_all();
    }
}

void AshitaAdapter::resumeBackgroundAfterTests() {
    logger_->info("AshitaAdapter: Resuming background work after tests");
    backgroundPausedForTests_.store(false);
    // Notify any waiting threads
    {
        std::lock_guard<std::mutex> lock(idleWaitMutex_);
        idleWaitCondition_.notify_all();
    }
}

bool AshitaAdapter::waitForIdleForTests(uint64_t timeoutMs) {
    uint64_t startTime = clock_->nowMs();
    uint64_t endTime = startTime + timeoutMs;
    
    while (clock_->nowMs() < endTime) {
        int active = activeJobs_.load();
        if (active == 0) {
            logger_->info("AshitaAdapter: Background work is idle (0 active jobs)");
            return true;
        }
        
        // Wait with timeout using condition variable
        std::unique_lock<std::mutex> lock(idleWaitMutex_);
        uint64_t remaining = endTime - clock_->nowMs();
        if (remaining > 0) {
            idleWaitCondition_.wait_for(lock, std::chrono::milliseconds(remaining));
        }
    }
    
    int finalActive = activeJobs_.load();
    logger_->warning("AshitaAdapter: Background work did not become idle within timeout (" + 
                     std::to_string(timeoutMs) + "ms). Active jobs: " + std::to_string(finalActive));
    return false;
}

bool AshitaAdapter::shouldBlockNetworkOperation() const {
    App::ServerSelectionGate gate(serverSelectionState_);
    return gate.isBlocked();
}

void AshitaAdapter::checkServerSelectionGate() {
    if (shouldBlockNetworkOperation() && !serverSelectionState_.hasSavedServer()) {
        showServerSelectionWindow();
    }
}

void AshitaAdapter::showServerSelectionWindow() {
    if (!initialized_ || !windowManager_) {
        return;
    }
    
    if (serverSelectionState_.hasSavedServer()) {
        return;
    }
    
    UI::ServerSelectionWindow& window = windowManager_->getServerSelectionWindow();
    window.setVisible(true);
    window.setServerSelectionState(serverSelectionState_);
    
    if (!serverSelectionState_.detectedServerShownOnce && serverSelectionState_.detectedServerSuggestion.has_value()) {
        std::string serverId = serverSelectionState_.detectedServerSuggestion.value();
        std::map<std::string, std::string> serverNames = {
            {"horizon", "Horizon"},
            {"nasomi", "Nasomi"},
            {"eden", "Eden"},
            {"catseye", "Catseye"},
            {"gaia", "Gaia"},
            {"phoenix", "Phoenix"},
            {"leveldown99", "LevelDown99"}
        };
        std::string serverName = serverNames.find(serverId) != serverNames.end() ? serverNames[serverId] : serverId;
        window.setDetectedServerSuggestion(serverId, serverName);
    }
    
    window.setServerList(serverList_);
    
    if (!serverList_.loaded && serverList_.error.empty()) {
        handleRefreshServerList();
    }
}

void AshitaAdapter::handleSaveServerSelection(const std::string& serverId) {
    if (serverId.empty()) {
        return;
    }
    
    Core::ServerInfo* selectedServer = nullptr;
    for (auto& server : serverList_.servers) {
        if (server.id == serverId) {
            selectedServer = &server;
            break;
        }
    }
    
    if (!selectedServer) {
        logger_->error("[server-selection] Server ID not found in server list: " + serverId);
        return;
    }
    
    serverSelectionState_.savedServerId = serverId;
    serverSelectionState_.savedServerBaseUrl = selectedServer->baseUrl;
    serverSelectionState_.draftServerId = std::nullopt;
    
    if (serverSelectionState_.detectedServerSuggestion.has_value() && 
        serverSelectionState_.detectedServerSuggestion.value() == serverId) {
        serverSelectionState_.detectedServerShownOnce = true;
    }
    
    ServerSelectionPersistence::saveToFile(serverSelectionState_);
    
    netClient_->setBaseUrl(selectedServer->baseUrl);
    netClient_->setRealmId(serverId);
    logger_->info("[server-selection] Server saved: " + selectedServer->name + " (" + selectedServer->baseUrl + "), realm: " + serverId);
    
    if (windowManager_) {
        windowManager_->getServerSelectionWindow().setVisible(false);
    }
}

void AshitaAdapter::handleRefreshServerList() {
    if (!fetchServerListUseCase_) {
        return;
    }
    
    std::thread([this]() {
        App::UseCases::ServerListResult result = fetchServerListUseCase_->fetchServerList();
        
        if (result.success) {
            serverList_ = result.serverList;
            if (windowManager_) {
                windowManager_->getServerSelectionWindow().setServerList(serverList_);
            }
        } else {
            serverList_.loaded = false;
            serverList_.error = result.error;
            if (windowManager_) {
                windowManager_->getServerSelectionWindow().setServerList(serverList_);
            }
        }
    }).detach();
}

void AshitaAdapter::detectServerFromRealm() {
    if (serverSelectionState_.hasSavedServer()) {
        return;
    }
    
    std::unique_ptr<AshitaRealmDetector> realmDetector = std::make_unique<AshitaRealmDetector>();
    std::string realmId = realmDetector->getRealmId();
    
    if (!realmId.empty()) {
        std::map<std::string, std::string> realmToServerId = {
            {"horizon", "horizon"},
            {"nasomi", "nasomi"},
            {"eden", "eden"},
            {"catseye", "catseye"},
            {"gaia", "gaia"},
            {"phoenix", "phoenix"},
            {"leveldown99", "leveldown99"}
        };
        
        auto it = realmToServerId.find(realmId);
        if (it != realmToServerId.end()) {
            serverSelectionState_.detectedServerSuggestion = it->second;
            logger_->info("[server-selection] Detected server from realm: " + realmId + " -> " + it->second);
        }
    }
}

void AshitaAdapter::rerouteToServerSelectionIfNeeded() {
    if (shouldBlockNetworkOperation() && !serverSelectionState_.hasSavedServer()) {
        showServerSelectionWindow();
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

