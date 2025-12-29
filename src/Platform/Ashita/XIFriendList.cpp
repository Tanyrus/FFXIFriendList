#include "XIFriendList.h"
#include "AshitaAdapter.h"
#include "CommandHandlerHook.h"
#include "MenuHook.h"
#include "UI/Widgets/Inputs.h"
#include "Debug/DebugLog.h"
#include "Debug/Perf.h"
#include "PluginVersion.h"
#include <memory>
#include <sstream>
#include <windows.h>  // For GetTickCount()
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <windows.h>

#include <Ashita.h>
#include <cstdint>


namespace XIFriendList {
namespace Platform {
namespace Ashita {

XIFriendList::XIFriendList()
    : adapter_(nullptr)
    , initialized_(false)
    , menuHook_(std::make_unique<MenuHook>())
{
    PERF_SCOPE("XIFriendList::XIFriendList construct AshitaAdapter");
    adapter_ = std::make_unique<AshitaAdapter>();
}

XIFriendList::~XIFriendList()
{
    if (initialized_) {
        Release();
    }
}

bool XIFriendList::Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id)
{
    if (initialized_) {
        return true;
    }

    PERF_SCOPE("XIFriendList::Initialize (total)");
    
    if (logger) {
        logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "FFXIFriendList", "Initialize called");
    }
    
    {
        PERF_SCOPE("XIFriendList::Initialize adapter_->initialize");
        if (!adapter_->initialize(core, logger, id)) {
            if (logger) {
                logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Error), "FFXIFriendList", "Failed to initialize adapter");
            }
            return false;
        }
    }
    
    if (core && logger) {
        PERF_SCOPE("XIFriendList::Initialize menuHook_->initialize");
        menuHook_->initialize(core, logger, adapter_.get());
    }
    
    commandHandlerHook_ = std::make_unique<CommandHandlerHook>(
        [this](int32_t mode, const char* command, bool injected) {
            return this->handleCommandImpl(mode, command, injected);
        }
    );
    
    if (logger) {
        commandHandlerHook_->addPostHook(
            [logger](int32_t /*mode*/, const char* command, bool /*injected*/, bool wasHandled) {
                if (wasHandled && command) {
                    std::ostringstream msg;
                    msg << "Command handled: " << command;
                    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "FFXIFriendList", msg.str().c_str());
                }
                return true;  // Continue processing
            }
        );
    }
    
    
    initialized_ = true;
    initializationTime_ = GetTickCount();  // Record initialization time for packet cooldown
    
    if (logger) {
        logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "FFXIFriendList", "Initialized successfully");
    }

    ::XIFriendList::Debug::PrintSummaryOnce("Initialize", 10);
    
    return true;
}

void XIFriendList::Release()
{
    if (!initialized_) {
        return;
    }
    
    // Shutdown menu hooks
    if (menuHook_) {
        menuHook_->shutdown();
    }
    
    commandHandlerHook_.reset();
    
    if (adapter_) {
        adapter_->release();
    }
    
    initialized_ = false;
}

bool XIFriendList::Direct3DInitialize(IDirect3DDevice8* device)
{
    if (!initialized_ || !adapter_) {
        return false;
    }

    PERF_SCOPE("XIFriendList::Direct3DInitialize");
    
    d3dDevice_ = device;
    
    if (adapter_ && device) {
        adapter_->initializeIconManager(device);
    }
    
    return true;
}

void XIFriendList::Direct3DBeginScene(bool isRenderingBackBuffer)
{
    (void)isRenderingBackBuffer;  // Not used for ImGui rendering
    
    if (!initialized_ || !adapter_) {
        return;
    }
    
}

void XIFriendList::Direct3DEndScene(bool isRenderingBackBuffer)
{
    (void)isRenderingBackBuffer;  // Not used for ImGui rendering
    
    if (!initialized_ || !adapter_) {
        return;
    }
    
}

void XIFriendList::Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    (void)pSourceRect;
    (void)pDestRect;
    (void)hDestWindowOverride;
    (void)pDirtyRegion;
    
    if (!initialized_ || !adapter_) {
        return;
    }

    adapter_->render();
    adapter_->update();
}

bool XIFriendList::HandleCommand(int32_t mode, const char* command, bool injected)
{
    if (!initialized_ || !commandHandlerHook_ || !command) {
        return false;
    }
    
    return commandHandlerHook_->execute(mode, command, injected);
}

bool XIFriendList::handleCommandImpl(int32_t mode, const char* command, bool injected)
{
    (void)mode;  // Mode not used
    (void)injected;  // Injected flag not used
    
    if (!initialized_ || !adapter_ || !command) {
        return false;
    }
    
    std::string cmdStr(command);
    std::string trimmed = cmdStr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    if (trimmed.empty() && ::XIFriendList::UI::IsAnyInputActive()) {
        return true;
    }
    
    std::string cmd(cmdStr);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), 
                   [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    
    if (cmd == "/fl" || cmd.find("/fl ") == 0) {
        std::istringstream iss(cmdStr);
        std::string first, second, third;
        iss >> first >> second >> third;
        
        if (second == "perf" || second == "Perf") {
            ::XIFriendList::Debug::PrintSummary("Manual", 10);
            return true;
        }

        if (second == "debug" || second == "Debug") {
            if (third == "memory" || third == "Memory") {
                adapter_->printMemoryStats();
                return true;
            }
            adapter_->toggleDebugWindow();
            return true;
        }
        
        if (second == "stats" || second == "Stats") {
            adapter_->printMemoryStats();
            return true;
        }
        
        if (second == "notify" || second == "Notify") {
            adapter_->triggerTestNotification();
            return true;
        }
        
        if (second == "test" || second == "Test") {
            std::istringstream iss2(cmdStr);
            std::string first2, second2, third2;
            iss2 >> first2 >> second2 >> third2;
            
            if (third2 == "list" || third2 == "List" || third2.empty()) {
                adapter_->handleTestList();
                return true;
            }
            
            if (third2 == "run" || third2 == "Run") {
                std::istringstream iss3(cmdStr);
                std::string first3, second3, third3, fourth3;
                iss3 >> first3 >> second3 >> third3 >> fourth3;
                adapter_->handleTestRun(fourth3);  // Empty if no scenario ID provided
                return true;
            }
            
            if (third2 == "reset" || third2 == "Reset") {
                adapter_->handleTestReset();
                return true;
            }
            
            return true;
        }
        
        bool wasOpen = adapter_->isWindowVisible();
        adapter_->toggleWindow();
        
        if (!wasOpen && adapter_->isWindowVisible()) {
            ::XIFriendList::Debug::MarkFirstInteractive();
            adapter_->triggerRefreshOnOpen();
        }
        return true;
    }
    
    if (cmd.find("/befriend ") == 0 || cmd.find("/befriend\t") == 0) {
        std::string friendName;
        if (cmd.find("/befriend ") == 0) {
            friendName = cmdStr.substr(10);  // "/befriend " is 10 chars
        } else {
            friendName = cmdStr.substr(9);  // "/befriend" is 9 chars, tab is 1 char
        }
        
        size_t firstNonWhitespace = friendName.find_first_not_of(" \t");
        if (firstNonWhitespace != std::string::npos) {
            friendName = friendName.substr(firstNonWhitespace);
        } else {
            friendName.clear();
        }
        
        if (!friendName.empty()) {
            adapter_->sendFriendRequestFromCommand(friendName);
        }
        return true;  // Block further processing
    }
    
    // Command not handled by this plugin
    return false;
}

const char* XIFriendList::GetName() const
{
    return "FFXIFriendList";
}

const char* XIFriendList::GetAuthor() const
{
    return "Tanyrus";
}

const char* XIFriendList::GetDescription() const
{
    return "A Friendlist Management Plugin";
}

const char* XIFriendList::GetLink() const
{
    return "";
}

double XIFriendList::GetVersion() const
{
    return Plugin::PLUGIN_VERSION;
}

uint32_t XIFriendList::GetFlags() const
{
    // Request Direct3D access for ImGui rendering, Commands for /fl toggle, and Packets for zone detection
    // PluginFlags is in the global Ashita namespace (not Platform::Ashita)
    return static_cast<uint32_t>(::Ashita::PluginFlags::UseDirect3D | 
                                  ::Ashita::PluginFlags::UseCommands | 
                                  ::Ashita::PluginFlags::UsePackets);
}

bool XIFriendList::HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
    (void)modified;
    (void)sizeChunk;
    (void)dataChunk;
    (void)injected;
    (void)blocked;
    
    // FIX: Skip packet processing during initialization to prevent stutter from backlog
    // Even lightweight packet handlers can cause stutter if called hundreds of times
    if (!initialized_ || !adapter_ || !data || size == 0) {
        return false;
    }
    
    // FIX: Skip packet processing for a short cooldown period after initialization
    // This prevents processing the backlog of queued packets that causes stutter
    uint32_t currentTime = GetTickCount();
    uint32_t timeSinceInit = currentTime - initializationTime_;
    if (timeSinceInit < PACKET_PROCESSING_COOLDOWN_MS) {
        return false;
    }
    
    // Packet 0x0A indicates zone change
    // Note: handleZoneChangePacket() spawns a thread, so this is non-blocking
    if (id == 0x000A && size >= 4) {
        adapter_->handleZoneChangePacket();
    }
    
    // Don't modify packets
    return false;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

