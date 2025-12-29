// Helper functions for common window operations

#ifndef UI_HELPERS_WINDOW_HELPER_H
#define UI_HELPERS_WINDOW_HELPER_H

#include "UI/Interfaces/IUiRenderer.h"
#include "UI/Commands/WindowCommands.h"
#include <string>

namespace XIFriendList {
namespace UI {

// Calculate the reserve height needed for the lock button at the bottom of a window
// Returns: height to reserve (lock button height + spacing)
float calculateLockButtonReserve();

// Render the lock button at the bottom-left of the window
// Must be called after EndChild() for the main content area
// Parameters:
//   - renderer: UI renderer instance
//   - windowId: Window identifier for lock state persistence
//   - locked: Current locked state (will be updated if user clicks)
//   - iconManager: Icon manager for lock/unlock icons (can be nullptr)
//   - commandHandler: Command handler for emitting lock update commands (can be nullptr)
void renderLockButton(IUiRenderer* renderer, const std::string& windowId, bool& locked, void* iconManager, IWindowCommandHandler* commandHandler);

} // namespace UI
} // namespace XIFriendList

#endif // UI_HELPERS_WINDOW_HELPER_H

