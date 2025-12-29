// Shared close-gating for all plugin windows.

#include "UI/Windows/UiCloseCoordinator.h"

#include "Debug/DebugLog.h"

namespace XIFriendList {
namespace UI {

bool isUiMenuCleanForClose(IUiRenderer* renderer) {
    if (!renderer) {
        return true;
    }
    return !renderer->IsAnyPopupOpen();
}

void applyWindowCloseGating(IUiRenderer* renderer,
                            const std::string& windowId,
                            bool closeRequested,
                            bool& visible,
                            bool& pendingClose) {
    if (!closeRequested) {
        return;
    }

    if (!isUiMenuCleanForClose(renderer)) {
        // Defer close until menus/popups are closed.
        pendingClose = true;
        visible = true;
        Debug::DebugLog::getInstance().push("[UI] Close deferred: menu open (" + windowId + ")");
        return;
    }

    pendingClose = false;
    visible = false;
}

} // namespace UI
} // namespace XIFriendList


