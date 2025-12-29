// UI renderer global access implementation

#include "UI/Interfaces/IUiRenderer.h"

namespace XIFriendList {
namespace UI {

// Global renderer instance (set by Platform layer)
static IUiRenderer* s_Renderer = nullptr;

IUiRenderer* GetUiRenderer() {
    return s_Renderer;
}

void SetUiRenderer(IUiRenderer* renderer) {
    s_Renderer = renderer;
}

} // namespace UI
} // namespace XIFriendList

