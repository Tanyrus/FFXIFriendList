
#ifndef UI_THEMES_WINDOW_H
#define UI_THEMES_WINDOW_H

#include "UI/Commands/WindowCommands.h"
#include "UI/ViewModels/ThemesViewModel.h"
#include <string>

namespace XIFriendList {
namespace UI {

class ThemesWindow {
public:
    ThemesWindow();
    ~ThemesWindow() = default;
    
    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
    }
    
    void setViewModel(ThemesViewModel* viewModel) {
        viewModel_ = viewModel;
    }
    
    void render();
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

private:
    void emitCommand(WindowCommandType type, const std::string& data = "");
    
    void renderThemeSelection();
    void renderThemePresetSelector();
    void renderCustomColors();
    void renderThemeManagement();
    
    IWindowCommandHandler* commandHandler_;
    ThemesViewModel* viewModel_;
    bool visible_;
    std::string title_;
    std::string windowId_;  // Unique window identifier for lock state
    bool locked_;  // Per-window lock state
    bool pendingClose_{ false };  // Close requested while a popup/menu is open; defer until clean.
    
    // Color editing buffers (float[4] arrays for each color)
    float windowBgColor_[4];
    float childBgColor_[4];
    float frameBgColor_[4];
    float frameBgHovered_[4];
    float frameBgActive_[4];
    float titleBg_[4];
    float titleBgActive_[4];
    float titleBgCollapsed_[4];
    float buttonColor_[4];
    float buttonHoverColor_[4];
    float buttonActiveColor_[4];
    float separatorColor_[4];
    float separatorHovered_[4];
    float separatorActive_[4];
    float scrollbarBg_[4];
    float scrollbarGrab_[4];
    float scrollbarGrabHovered_[4];
    float scrollbarGrabActive_[4];
    float checkMark_[4];
    float sliderGrab_[4];
    float sliderGrabActive_[4];
    float header_[4];
    float headerHovered_[4];
    float headerActive_[4];
    float textColor_[4];
    float textDisabled_[4];
    
    void syncColorsToBuffers();
    void syncBuffersToColors();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_THEMES_WINDOW_H

