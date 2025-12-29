#ifndef PLATFORM_ASHITA_COMMAND_HANDLER_HOOK_H
#define PLATFORM_ASHITA_COMMAND_HANDLER_HOOK_H

#include <functional>
#include <vector>
#include <memory>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

/**
 * Safe Plugin-Level Command Handler Hook System
 * 
 * This implements a legitimate C++ hooking pattern that operates entirely within
 * our plugin code. It does NOT modify game memory, call internal game functions,
 * or use any form of memory patching or detouring.
 * 
 * Pattern: Function pointer wrapping with std::function
 * - Original command handler is stored as a std::function
 * - Hooks can be registered to execute before/after the original handler
 * - All interception occurs within plugin-owned code paths
 * 
 * Why this approach:
 * - No memory modification of external code
 * - No dependency on game-internal symbols or calling conventions
 * - Thread-safe (hooks execute on same thread as original)
 * - Testable (can verify hooks fire and original behavior preserved)
 * - Extensible (easy to add/remove hooks at runtime)
 */

// Command handler function signature
// Returns true if command was handled, false otherwise
using CommandHandlerFunc = std::function<bool(int32_t mode, const char* command, bool injected)>;

// Hook callback function signature
// Called before/after command handler execution
// Parameters: (mode, command, injected, wasHandled)
// Returns: true to continue, false to stop further processing
using CommandHookFunc = std::function<bool(int32_t mode, const char* command, bool injected, bool wasHandled)>;

/**
 * Command handler hook manager
 * 
 * Wraps a command handler function and allows hooks to be registered
 * that execute before and/or after the original handler.
 */
class CommandHandlerHook {
public:
    explicit CommandHandlerHook(CommandHandlerFunc originalHandler);
    bool execute(int32_t mode, const char* command, bool injected);
    size_t addPreHook(CommandHookFunc hook);
    size_t addPostHook(CommandHookFunc hook);
    void removeHook(size_t hookId);
    void clearHooks();
    const CommandHandlerFunc& getOriginalHandler() const { return originalHandler_; }

private:
    CommandHandlerFunc originalHandler_;
    
    struct HookEntry {
        size_t id;
        CommandHookFunc func;
        bool isPreHook;
    };
    
    std::vector<HookEntry> hooks_;
    size_t nextHookId_;
    
    size_t addHook(CommandHookFunc hook, bool isPreHook);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_COMMAND_HANDLER_HOOK_H

