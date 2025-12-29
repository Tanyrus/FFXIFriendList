#include "Platform/Ashita/FriendListMenuDetector.h"
#include "Platform/Ashita/AshitaLogger.h"
#include "Platform/Ashita/AshitaClock.h"
#include <Ashita.h>
#include <string>
#include <algorithm>
#include <windows.h>
#include <cstring>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

static FriendListMenuDetector* g_Instance = nullptr;

FriendListMenuDetector::FriendListMenuDetector()
    : originalFunctionAddress_(0)
    , hookTrampoline_(nullptr)
    , hookInstalled_(false)
    , lastMenuOpenState_(false)
    , lastMenuStateCheck_(0)
    , lastMenuOpenTriggerTime_(0)
    , ashitaCore_(nullptr)
    , logger_(nullptr)
    , clock_(nullptr)
    , detectionMethod_(MenuDetectionMethod::Polling)
{
}

FriendListMenuDetector::~FriendListMenuDetector() {
    shutdown();
}

bool FriendListMenuDetector::initialize(IAshitaCore* core, ILogManager* logger, 
                                        ::XIFriendList::Platform::Ashita::AshitaClock* clock,
                                        std::function<void()> callback) {
    if (!core || !logger || !clock) {
        return false;
    }
    
    ashitaCore_ = core;
    logger_ = logger;
    clock_ = clock;
    onMenuOpenedCallback_ = callback;
    
    g_Instance = this;
    
    {
        std::lock_guard<std::mutex> lock(methodMutex_);
        std::string methodStr = (detectionMethod_ == MenuDetectionMethod::FunctionHook ? "FunctionHook" :
                                 detectionMethod_ == MenuDetectionMethod::Polling ? "Polling" : "Disabled");
        logInfo("FriendListMenuDetector: Initialized with method: " + methodStr);
        
        if (detectionMethod_ == MenuDetectionMethod::FunctionHook) {
            uintptr_t addr = findFunctionAddress();
            if (addr != 0) {
                if (installHook()) {
                    logInfo("FriendListMenuDetector: Function hook installed successfully");
                } else {
                    logWarning("FriendListMenuDetector: Function hook installation failed, falling back to polling");
                    detectionMethod_ = MenuDetectionMethod::Polling;
                }
            } else {
                logWarning("FriendListMenuDetector: Function address not found, falling back to polling");
                detectionMethod_ = MenuDetectionMethod::Polling;
            }
        }
    }
    
    return true;
}

void FriendListMenuDetector::shutdown() {
    {
        std::lock_guard<std::mutex> lock(methodMutex_);
        if (detectionMethod_ == MenuDetectionMethod::FunctionHook) {
            uninstallHook();
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        onMenuOpenedCallback_ = nullptr;
    }
    
    g_Instance = nullptr;
    ashitaCore_ = nullptr;
    logger_ = nullptr;
    clock_ = nullptr;
}

void FriendListMenuDetector::update() {
    if (!ashitaCore_ || detectionMethod_ == MenuDetectionMethod::Disabled) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(methodMutex_);
        if (detectionMethod_ == MenuDetectionMethod::Polling) {
            checkMenuStatePolling();
        }
        // Function hook method doesn't need polling - hook callback handles it
    }
}

void FriendListMenuDetector::setDetectionMethod(MenuDetectionMethod method) {
    std::lock_guard<std::mutex> lock(methodMutex_);
    
    // Uninstall hook if switching away from function hook
    if (detectionMethod_ == MenuDetectionMethod::FunctionHook && 
        method != MenuDetectionMethod::FunctionHook) {
        uninstallHook();
    }
    
    detectionMethod_ = method;
    
    // Try to install hook if switching to function hook
    if (method == MenuDetectionMethod::FunctionHook && ashitaCore_) {
        if (findFunctionAddress() != 0) {
            if (installHook()) {
                logInfo("FriendListMenuDetector: Function hook installed after method change");
            } else {
                logWarning("FriendListMenuDetector: Function hook installation failed, using polling");
                detectionMethod_ = MenuDetectionMethod::Polling;
            }
        } else {
            logWarning("FriendListMenuDetector: Function address not found, using polling");
            detectionMethod_ = MenuDetectionMethod::Polling;
        }
    }
}

MenuDetectionMethod FriendListMenuDetector::getDetectionMethod() const {
    std::lock_guard<std::mutex> lock(methodMutex_);
    return detectionMethod_;
}

bool FriendListMenuDetector::isHookInstalled() const {
    std::lock_guard<std::mutex> lock(hookMutex_);
    return hookInstalled_;
}

uintptr_t FriendListMenuDetector::findFunctionAddress() {
    if (!ashitaCore_) {
        return 0;
    }
    
    // Try IOffsetManager first
    IOffsetManager* offsetMgr = ashitaCore_->GetOffsetManager();
    if (offsetMgr) {
        // Try different section/key combinations
        const char* sections[] = {"FriendList", "Menu", "YkWndFriendMain", "Functions"};
        const char* keys[] = {"FUNC_YkWndFriendMain_OpenFriend", "OpenFriend", "YkWndFriendMain_OpenFriend"};
        
        for (const char* section : sections) {
            for (const char* key : keys) {
                int32_t offset = offsetMgr->Get(section, key);
                if (offset != 0) {
                    uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("pol.exe");
                    if (moduleBase == 0) {
                        moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
                    }
                    
                    if (moduleBase != 0) {
                        uintptr_t address = moduleBase + offset;
                        logInfo("FriendListMenuDetector: Found function address via offset [" + 
                               std::string(section) + "/" + std::string(key) + "]: 0x" + 
                               std::to_string(address));
                        return address;
                    }
                }
            }
        }
    }
    
    // Try pattern scanning as fallback
    // Common function prologue patterns for __thiscall functions
    uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("pol.exe");
    if (moduleBase == 0) {
        moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
    }
    
    if (moduleBase != 0) {
        uintptr_t moduleSize = ::Ashita::Memory::GetModuleSize("pol.exe");
        if (moduleSize == 0) {
            moduleSize = ::Ashita::Memory::GetModuleSize("FFXiMain.dll");
        }
        
        if (moduleSize > 0) {
            // Common __thiscall prologues: 55 8B EC (push ebp; mov ebp, esp)
            // Try to find function - this is a placeholder, would need actual pattern
            logDebug("FriendListMenuDetector: Pattern scanning not implemented - need pattern from devs");
        }
    }
    
    logWarning("FriendListMenuDetector: Function address not found - check IOffsetManager configuration");
    return 0;
}

bool FriendListMenuDetector::installHook() {
    if (originalFunctionAddress_ == 0) {
        originalFunctionAddress_ = findFunctionAddress();
        if (originalFunctionAddress_ == 0) {
            logError("FriendListMenuDetector: Cannot install hook - function address not found");
            return false;
        }
    }
    
    std::lock_guard<std::mutex> lock(hookMutex_);
    
    if (hookInstalled_) {
        logWarning("FriendListMenuDetector: Hook already installed");
        return true;
    }
    
    // Simple inline hook: Replace first 5 bytes with JMP to our function
    memcpy(originalBytes_, (void*)originalFunctionAddress_, 5);
    
    // Unprotect memory
    DWORD oldProtect = 0;
    if (!::VirtualProtect((void*)originalFunctionAddress_, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logError("FriendListMenuDetector: Failed to unprotect memory for hook");
        return false;
    }
    
    // Calculate JMP offset: JMP rel32 = E9 [offset]
    // offset = target - (source + 5)
    uintptr_t hookFunctionAddr = (uintptr_t)HookedFriendListOpen;
    int32_t jmpOffset = (int32_t)(hookFunctionAddr - (originalFunctionAddress_ + 5));
    
    // Write JMP instruction
    uint8_t jmp[5];
    jmp[0] = 0xE9;  // JMP rel32
    memcpy(&jmp[1], &jmpOffset, 4);
    memcpy((void*)originalFunctionAddress_, jmp, 5);
    
    // Restore protection
    DWORD dummy;
    ::VirtualProtect((void*)originalFunctionAddress_, 5, oldProtect, &dummy);
    
    hookInstalled_ = true;
    logInfo("FriendListMenuDetector: Hook installed successfully at address 0x" + 
           std::to_string(originalFunctionAddress_));
    logDebug("FriendListMenuDetector: JMP offset: " + std::to_string(jmpOffset));
    
    return true;
}

void FriendListMenuDetector::uninstallHook() {
    std::lock_guard<std::mutex> lock(hookMutex_);
    
    if (!hookInstalled_ || originalFunctionAddress_ == 0) {
        return;
    }
    
    // Restore original function bytes
    DWORD oldProtect = 0;
    if (::VirtualProtect((void*)originalFunctionAddress_, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy((void*)originalFunctionAddress_, originalBytes_, 5);
        DWORD dummy;
        ::VirtualProtect((void*)originalFunctionAddress_, 5, oldProtect, &dummy);
        logInfo("FriendListMenuDetector: Hook uninstalled - original bytes restored");
    } else {
        logError("FriendListMenuDetector: Failed to unprotect memory for hook removal");
    }
    
    hookInstalled_ = false;
    hookTrampoline_ = nullptr;
    originalFunctionAddress_ = 0;
}

void FriendListMenuDetector::checkMenuStatePolling() {
    if (!ashitaCore_ || !clock_) {
        return;
    }
    
    uint64_t now = clock_->nowMs();
    
    {
        std::lock_guard<std::mutex> lock(pollingMutex_);
        
        if (now - lastMenuStateCheck_ < MENU_POLL_INTERVAL_MS) {
            return;
        }
        
        lastMenuStateCheck_ = now;
        
        ITarget* target = getTarget();
        if (!target) {
            return;
        }
        
        bool menuOpen = (target->GetIsMenuOpen() != 0);
        // Detect transition: closed -> open
        if (menuOpen && !lastMenuOpenState_) {
            // Debounce: Don't trigger if triggered recently
            if (now - lastMenuOpenTriggerTime_ >= MENU_OPEN_DEBOUNCE_MS) {
                lastMenuOpenTriggerTime_ = now;
                
                logInfo("FriendListMenuDetector: Menu opened (detected via polling)");
                logDebug("FriendListMenuDetector: DEBUG - Friendlist menu opened! Triggering refresh...");
                
                // Trigger callback (thread-safe)
                {
                    std::lock_guard<std::mutex> callbackLock(callbackMutex_);
                    if (onMenuOpenedCallback_) {
                        try {
                            onMenuOpenedCallback_();
                            logDebug("FriendListMenuDetector: DEBUG - Refresh callback executed successfully");
                        } catch (const std::exception& e) {
                            logError("FriendListMenuDetector: Exception in callback: " + 
                                    std::string(e.what()));
                        } catch (...) {
                            logError("FriendListMenuDetector: Unknown exception in callback");
                        }
                    } else {
                        logWarning("FriendListMenuDetector: DEBUG - Menu opened but callback is null!");
                    }
                }
            } else {
                logDebug("FriendListMenuDetector: Menu open detected but debounced");
            }
        }
        
        lastMenuOpenState_ = menuOpen;
    }
}

bool FriendListMenuDetector::isMenuOpen() const {
    {
        std::lock_guard<std::mutex> methodLock(methodMutex_);
        if (detectionMethod_ != MenuDetectionMethod::Polling) {
            ITarget* target = getTarget();
            if (target) {
                return (target->GetIsMenuOpen() != 0);
            }
            return false;
        }
    }
    
    std::lock_guard<std::mutex> lock(pollingMutex_);
    return lastMenuOpenState_;
}

void __thiscall FriendListMenuDetector::HookedFriendListOpen(void* thisPtr, int32_t param) {
    FriendListMenuDetector* instance = getInstance();
    if (!instance) {
        return;
    }
    
    instance->logDebug("FriendListMenuDetector: DEBUG - FUNC_YkWndFriendMain_OpenFriend CALLED! thisPtr=0x" + 
                      std::to_string((uintptr_t)thisPtr) + " param=" + std::to_string(param));
    
    // Call original function
    // We need to restore the original bytes temporarily, call the function, then re-hook
    {
        std::lock_guard<std::mutex> lock(instance->hookMutex_);
        if (instance->hookInstalled_ && instance->originalFunctionAddress_ != 0) {
            // Temporarily restore original bytes
            uint8_t savedBytes[5];
            memcpy(savedBytes, (void*)instance->originalFunctionAddress_, 5);
            memcpy((void*)instance->originalFunctionAddress_, instance->originalBytes_, 5);
            
            // Call original function
            typedef void (__thiscall *OriginalFunc)(void*, int32_t);
            OriginalFunc original = (OriginalFunc)instance->originalFunctionAddress_;
            original(thisPtr, param);
            
            // Re-apply hook
            uintptr_t hookFunctionAddr = (uintptr_t)HookedFriendListOpen;
            int32_t jmpOffset = (int32_t)(hookFunctionAddr - (instance->originalFunctionAddress_ + 5));
            uint8_t jmp[5];
            jmp[0] = 0xE9;
            memcpy(&jmp[1], &jmpOffset, 4);
            memcpy((void*)instance->originalFunctionAddress_, jmp, 5);
        }
    }
    
    // Trigger callback
    {
        std::lock_guard<std::mutex> lock(instance->callbackMutex_);
        if (instance->onMenuOpenedCallback_) {
            try {
                instance->onMenuOpenedCallback_();
                instance->logDebug("FriendListMenuDetector: DEBUG - Refresh callback executed successfully");
            } catch (const std::exception& e) {
                instance->logError("FriendListMenuDetector: Exception in hook callback: " + 
                                  std::string(e.what()));
            } catch (...) {
                instance->logError("FriendListMenuDetector: Unknown exception in hook callback");
            }
        } else {
            instance->logWarning("FriendListMenuDetector: DEBUG - Menu opened but callback is null!");
        }
    }
}

FriendListMenuDetector* FriendListMenuDetector::getInstance() {
    return g_Instance;
}

ITarget* FriendListMenuDetector::getTarget() const {
    if (!ashitaCore_) {
        return nullptr;
    }
    
    IMemoryManager* memoryMgr = ashitaCore_->GetMemoryManager();
    if (!memoryMgr) {
        return nullptr;
    }
    
    return memoryMgr->GetTarget();
}

void FriendListMenuDetector::logDebug(const std::string& message) const {
    if (logger_) {
        logger_->Log(static_cast<uint32_t>(::Ashita::LogLevel::Debug), 
                    "FriendListMenuDetector", message.c_str());
    }
}

void FriendListMenuDetector::logInfo(const std::string& message) const {
    if (logger_) {
        logger_->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), 
                    "FriendListMenuDetector", message.c_str());
    }
}

void FriendListMenuDetector::logWarning(const std::string& message) const {
    if (logger_) {
        // Ashita LogLevel enum: Debug=0, Info=1, Warn=2, Error=3
        logger_->Log(2, "FriendListMenuDetector", message.c_str());
    }
}

void FriendListMenuDetector::logError(const std::string& message) const {
    if (logger_) {
        logger_->Log(static_cast<uint32_t>(::Ashita::LogLevel::Error), 
                    "FriendListMenuDetector", message.c_str());
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

