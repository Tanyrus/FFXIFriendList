
#ifndef PLATFORM_ASHITA_MENU_HOOK_H
#define PLATFORM_ASHITA_MENU_HOOK_H

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>

// Forward declarations
struct IAshitaCore;
struct ILogManager;

namespace XIFriendList {
namespace Platform {
namespace Ashita {
    class AshitaAdapter;
}
}
}

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class MenuHook {
public:
    MenuHook();
    ~MenuHook();
    bool initialize(IAshitaCore* core, ILogManager* logger, ::XIFriendList::Platform::Ashita::AshitaAdapter* adapter);
    void shutdown();
    bool isInstalled() const { return hookInstalled_; }
    void onFriendListOpen(void* thisPtr, int32_t param);
    void onMessageListOpen(void* thisPtr, int64_t param1, int32_t param2);
    ILogManager* logger_;
    ::XIFriendList::Platform::Ashita::AshitaAdapter* adapter_;

private:
    bool installFriendListHook(IAshitaCore* core, ILogManager* logger, bool allowSlowResolution);
    bool installMessageListHook(IAshitaCore* core, ILogManager* logger, bool allowSlowResolution);
    bool installFriendListHookAtAddress(ILogManager* logger, uintptr_t funcAddr);
    bool installMessageListHookAtAddress(ILogManager* logger, uintptr_t funcAddr);
    std::optional<uintptr_t> tryResolveFriendListByOffset(IAshitaCore* core) const;
    std::optional<uintptr_t> tryResolveMessageListByOffset(IAshitaCore* core) const;
    uintptr_t findFunctionByPattern(const char* moduleName, const uint8_t* pattern, const uint8_t* mask, size_t patternSize, const char* patternId);
    bool hookInstalled_;
    uintptr_t friendListHookAddress_;
    uintptr_t messageListHookAddress_;
    uint8_t friendListOriginalBytes_[5];
    uint8_t messageListOriginalBytes_[5];
    uintptr_t friendListTrampoline_;
    uintptr_t messageListTrampoline_;
};

extern MenuHook* g_MenuHookInstance;

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_MENU_HOOK_H

