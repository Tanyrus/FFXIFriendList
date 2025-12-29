
#ifndef PLATFORM_ASHITA_FRIENDLIST_MENU_DETECTOR_H
#define PLATFORM_ASHITA_FRIENDLIST_MENU_DETECTOR_H

#include <functional>
#include <mutex>
#include <cstdint>
#include <memory>
#include <string>

struct IAshitaCore;
struct ILogManager;
struct IMemoryManager;
struct ITarget;

namespace XIFriendList {
namespace Platform {
namespace Ashita {
    class AshitaClock;
}
}
}

namespace XIFriendList {
namespace Platform {
namespace Ashita {

enum class MenuDetectionMethod {
    FunctionHook,
    Polling,
    Disabled
};

class FriendListMenuDetector {
public:
    FriendListMenuDetector();
    ~FriendListMenuDetector();
    bool initialize(IAshitaCore* core, ILogManager* logger, 
                    ::XIFriendList::Platform::Ashita::AshitaClock* clock,
                    std::function<void()> callback);
    void shutdown();
    
    // Polls menu state if using polling method
    void update();
    
    // Returns true if any game menu is currently open, false otherwise
    bool isMenuOpen() const;
    
    void setDetectionMethod(MenuDetectionMethod method);
    MenuDetectionMethod getDetectionMethod() const;
    bool isHookInstalled() const;
    uintptr_t findFunctionAddress();
    bool installHook();
    void uninstallHook();

private:
    uintptr_t originalFunctionAddress_;
    void* hookTrampoline_;
    bool hookInstalled_;
    uint8_t originalBytes_[5];
    mutable std::mutex hookMutex_;
    bool lastMenuOpenState_;
    uint64_t lastMenuStateCheck_;
    static constexpr uint64_t MENU_POLL_INTERVAL_MS = 100;
    static constexpr uint64_t MENU_OPEN_DEBOUNCE_MS = 2000;
    uint64_t lastMenuOpenTriggerTime_;
    mutable std::mutex pollingMutex_;
    IAshitaCore* ashitaCore_;
    ILogManager* logger_;
    ::XIFriendList::Platform::Ashita::AshitaClock* clock_;
    MenuDetectionMethod detectionMethod_;
    mutable std::mutex methodMutex_;
    std::function<void()> onMenuOpenedCallback_;
    mutable std::mutex callbackMutex_;
    void checkMenuStatePolling();
    // TODO: Implement when function address is available
    static void __thiscall HookedFriendListOpen(void* thisPtr, int32_t param);
    
    // TODO: Implement thread-local storage or singleton pattern
    static FriendListMenuDetector* getInstance();
    
    // Helper: Get ITarget interface
    ITarget* getTarget() const;
    
    void logDebug(const std::string& message) const;
    void logInfo(const std::string& message) const;
    void logWarning(const std::string& message) const;
    void logError(const std::string& message) const;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_FRIENDLIST_MENU_DETECTOR_H

