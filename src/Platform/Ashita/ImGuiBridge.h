// Bridge for ImGui rendering

#ifndef PLATFORM_ASHITA_IMGUI_BRIDGE_H
#define PLATFORM_ASHITA_IMGUI_BRIDGE_H

struct IGuiManager;

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class ImGuiBridge {
public:
    static bool initialize();
    static void shutdown();
    static void setGuiManager(IGuiManager* guiManager);
    static void beginFrame();
    static void endFrame();
    static bool isAvailable();
    static IGuiManager* getGuiManager();
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_IMGUI_BRIDGE_H

