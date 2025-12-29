#include "Platform/Ashita/ImGuiBridge.h"
#include <Ashita.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

static IGuiManager* s_GuiManager = nullptr;
static bool s_Initialized = false;

bool ImGuiBridge::initialize() {
    s_Initialized = true;
    return true;
}

void ImGuiBridge::shutdown() {
    s_GuiManager = nullptr;
    s_Initialized = false;
}

void ImGuiBridge::setGuiManager(IGuiManager* guiManager) {
    s_GuiManager = guiManager;
}

void ImGuiBridge::beginFrame() {
    if (!s_Initialized || !s_GuiManager) {
        return;
    }
}

void ImGuiBridge::endFrame() {
    if (!s_Initialized || !s_GuiManager) {
        return;
    }
}

bool ImGuiBridge::isAvailable() {
    return s_Initialized && s_GuiManager != nullptr;
}

IGuiManager* ImGuiBridge::getGuiManager() {
    return s_GuiManager;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

