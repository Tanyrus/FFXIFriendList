// Shared close-gating for all plugin windows.

#ifndef UI_UI_CLOSE_COORDINATOR_H
#define UI_UI_CLOSE_COORDINATOR_H

#include "UI/Interfaces/IUiRenderer.h"
#include <string>

namespace XIFriendList {
namespace UI {

// Returns true if it is safe to close plugin windows this frame.
bool isUiMenuCleanForClose(IUiRenderer* renderer);

// Handles a close request (eg. window X clicked) with menu-open gating.
// - If menus are open, sets pendingClose and keeps visible.
// - If menus are clean, allows close immediately.
void applyWindowCloseGating(IUiRenderer* renderer,
                            const std::string& windowId,
                            bool closeRequested,
                            bool& visible,
                            bool& pendingClose);

} // namespace UI
} // namespace XIFriendList

#endif // UI_UI_CLOSE_COORDINATOR_H


