#include "CommandHandlerHook.h"
#include <algorithm>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

CommandHandlerHook::CommandHandlerHook(CommandHandlerFunc originalHandler)
    : originalHandler_(std::move(originalHandler))
    , nextHookId_(1)
{
}

bool CommandHandlerHook::execute(int32_t mode, const char* command, bool injected) {
    for (const auto& hook : hooks_) {
        if (hook.isPreHook) {
            if (!hook.func(mode, command, injected, false)) {
                return false;
            }
        }
    }
    
    bool wasHandled = originalHandler_(mode, command, injected);
    
    for (const auto& hook : hooks_) {
        if (!hook.isPreHook) {
            if (!hook.func(mode, command, injected, wasHandled)) {
                break;
            }
        }
    }
    
    return wasHandled;
}

size_t CommandHandlerHook::addPreHook(CommandHookFunc hook) {
    return addHook(std::move(hook), true);
}

size_t CommandHandlerHook::addPostHook(CommandHookFunc hook) {
    return addHook(std::move(hook), false);
}

void CommandHandlerHook::removeHook(size_t hookId) {
    hooks_.erase(
        std::remove_if(hooks_.begin(), hooks_.end(),
            [hookId](const HookEntry& entry) { return entry.id == hookId; }),
        hooks_.end()
    );
}

void CommandHandlerHook::clearHooks() {
    hooks_.clear();
}

size_t CommandHandlerHook::addHook(CommandHookFunc hook, bool isPreHook) {
    size_t id = nextHookId_++;
    hooks_.push_back({id, std::move(hook), isPreHook});
    return id;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

