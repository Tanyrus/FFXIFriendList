
#ifndef APP_THEMING_THEME_TOKENS_H
#define APP_THEMING_THEME_TOKENS_H

#include "Core/ModelsCore.h"
#include <string>

namespace XIFriendList {
namespace App {
namespace Theming {

struct Vec2 {
    float x;
    float y;
    
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct ThemeTokens {
    XIFriendList::Core::Color windowBgColor;
    XIFriendList::Core::Color childBgColor;
    XIFriendList::Core::Color frameBgColor;
    XIFriendList::Core::Color frameBgHovered;
    XIFriendList::Core::Color frameBgActive;
    XIFriendList::Core::Color titleBg;
    XIFriendList::Core::Color titleBgActive;
    XIFriendList::Core::Color titleBgCollapsed;
    XIFriendList::Core::Color buttonColor;
    XIFriendList::Core::Color buttonHoverColor;
    XIFriendList::Core::Color buttonActiveColor;
    XIFriendList::Core::Color separatorColor;
    XIFriendList::Core::Color separatorHovered;
    XIFriendList::Core::Color separatorActive;
    XIFriendList::Core::Color scrollbarBg;
    XIFriendList::Core::Color scrollbarGrab;
    XIFriendList::Core::Color scrollbarGrabHovered;
    XIFriendList::Core::Color scrollbarGrabActive;
    XIFriendList::Core::Color checkMark;
    XIFriendList::Core::Color sliderGrab;
    XIFriendList::Core::Color sliderGrabActive;
    XIFriendList::Core::Color header;
    XIFriendList::Core::Color headerHovered;
    XIFriendList::Core::Color headerActive;
    XIFriendList::Core::Color textColor;
    XIFriendList::Core::Color textDisabled;
    XIFriendList::Core::Color borderColor;
    
    Vec2 windowPadding;
    float windowRounding;
    Vec2 framePadding;
    float frameRounding;
    Vec2 itemSpacing;
    Vec2 itemInnerSpacing;
    float scrollbarSize;
    float scrollbarRounding;
    float grabRounding;
    
    float backgroundAlpha;
    float textAlpha;

    std::string presetName;
    
    ThemeTokens() 
        : windowPadding(12.0f, 12.0f)
        , windowRounding(6.0f)
        , framePadding(6.0f, 3.0f)
        , frameRounding(3.0f)
        , itemSpacing(6.0f, 4.0f)
        , itemInnerSpacing(4.0f, 3.0f)
        , scrollbarSize(12.0f)
        , scrollbarRounding(3.0f)
        , grabRounding(3.0f)
        , backgroundAlpha(0.95f)
        , textAlpha(1.0f)
        , presetName("")
    {
    }
};

} // namespace Theming
} // namespace App
} // namespace XIFriendList

#endif // APP_THEMING_THEME_TOKENS_H

