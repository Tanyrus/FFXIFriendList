#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"

#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif

#include <cassert>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

int AshitaThemeHelper::styleColorPushCount_ = 0;
int AshitaThemeHelper::styleVarPushCount_ = 0;

bool AshitaThemeHelper::pushThemeStyles(const ::XIFriendList::App::Theming::ThemeTokens& theme) {
#ifndef BUILDING_TESTS
    IGuiManager* guiManager = ImGuiBridge::getGuiManager();
    if (!guiManager) {
        return false;
    }
    
    guiManager->PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(theme.windowPadding.x, theme.windowPadding.y));
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_WindowRounding, theme.windowRounding);
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(theme.framePadding.x, theme.framePadding.y));
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_FrameRounding, theme.frameRounding);
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(theme.itemSpacing.x, theme.itemSpacing.y));
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(theme.itemInnerSpacing.x, theme.itemInnerSpacing.y));
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_ScrollbarSize, theme.scrollbarSize);
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_ScrollbarRounding, theme.scrollbarRounding);
    styleVarPushCount_++;
    
    guiManager->PushStyleVar(ImGuiStyleVar_GrabRounding, theme.grabRounding);
    styleVarPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_WindowBg, ImVec4(theme.windowBgColor.r, theme.windowBgColor.g, theme.windowBgColor.b, theme.backgroundAlpha));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ChildBg, ImVec4(theme.childBgColor.r, theme.childBgColor.g, theme.childBgColor.b, theme.childBgColor.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_FrameBg, ImVec4(theme.frameBgColor.r, theme.frameBgColor.g, theme.frameBgColor.b, theme.frameBgColor.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(theme.frameBgHovered.r, theme.frameBgHovered.g, theme.frameBgHovered.b, theme.frameBgHovered.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(theme.frameBgActive.r, theme.frameBgActive.g, theme.frameBgActive.b, theme.frameBgActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_TitleBg, ImVec4(theme.titleBg.r, theme.titleBg.g, theme.titleBg.b, theme.titleBg.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(theme.titleBgActive.r, theme.titleBgActive.g, theme.titleBgActive.b, theme.titleBgActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(theme.titleBgCollapsed.r, theme.titleBgCollapsed.g, theme.titleBgCollapsed.b, theme.titleBgCollapsed.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_Button, ImVec4(theme.buttonColor.r, theme.buttonColor.g, theme.buttonColor.b, theme.backgroundAlpha));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(theme.buttonHoverColor.r, theme.buttonHoverColor.g, theme.buttonHoverColor.b, theme.backgroundAlpha));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ButtonActive, ImVec4(theme.buttonActiveColor.r, theme.buttonActiveColor.g, theme.buttonActiveColor.b, theme.backgroundAlpha));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_Separator, ImVec4(theme.separatorColor.r, theme.separatorColor.g, theme.separatorColor.b, theme.separatorColor.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(theme.separatorHovered.r, theme.separatorHovered.g, theme.separatorHovered.b, theme.separatorHovered.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(theme.separatorActive.r, theme.separatorActive.g, theme.separatorActive.b, theme.separatorActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(theme.scrollbarBg.r, theme.scrollbarBg.g, theme.scrollbarBg.b, theme.scrollbarBg.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(theme.scrollbarGrab.r, theme.scrollbarGrab.g, theme.scrollbarGrab.b, theme.scrollbarGrab.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(theme.scrollbarGrabHovered.r, theme.scrollbarGrabHovered.g, theme.scrollbarGrabHovered.b, theme.scrollbarGrabHovered.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(theme.scrollbarGrabActive.r, theme.scrollbarGrabActive.g, theme.scrollbarGrabActive.b, theme.scrollbarGrabActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_CheckMark, ImVec4(theme.checkMark.r, theme.checkMark.g, theme.checkMark.b, theme.checkMark.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_SliderGrab, ImVec4(theme.sliderGrab.r, theme.sliderGrab.g, theme.sliderGrab.b, theme.sliderGrab.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(theme.sliderGrabActive.r, theme.sliderGrabActive.g, theme.sliderGrabActive.b, theme.sliderGrabActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_Header, ImVec4(theme.header.r, theme.header.g, theme.header.b, theme.header.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(theme.headerHovered.r, theme.headerHovered.g, theme.headerHovered.b, theme.headerHovered.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_HeaderActive, ImVec4(theme.headerActive.r, theme.headerActive.g, theme.headerActive.b, theme.headerActive.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_Text, ImVec4(theme.textColor.r, theme.textColor.g, theme.textColor.b, theme.textAlpha));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_TextDisabled, ImVec4(theme.textDisabled.r, theme.textDisabled.g, theme.textDisabled.b, theme.textDisabled.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_Border, ImVec4(theme.borderColor.r, theme.borderColor.g, theme.borderColor.b, theme.borderColor.a));
    styleColorPushCount_++;
    
    guiManager->PushStyleColor(ImGuiCol_PopupBg, ImVec4(theme.frameBgColor.r, theme.frameBgColor.g, theme.frameBgColor.b, theme.frameBgColor.a));
    styleColorPushCount_++;
    
    return true;
#else
    return false;
#endif
}

void AshitaThemeHelper::popThemeStyles() {
#ifndef BUILDING_TESTS
    IGuiManager* guiManager = ImGuiBridge::getGuiManager();
    if (!guiManager) {
        return;
    }
    
    guiManager->PopStyleColor(28);
    styleColorPushCount_ -= 28;
    
    guiManager->PopStyleVar(9);
    styleVarPushCount_ -= 9;
#endif
}

void AshitaThemeHelper::validateStackBalance() {
#ifdef _DEBUG
    assert(styleColorPushCount_ == 0 && "StyleColor stack is not balanced! Push/pop count mismatch.");
    assert(styleVarPushCount_ == 0 && "StyleVar stack is not balanced! Push/pop count mismatch.");
#endif
}

void AshitaThemeHelper::resetStackCounters() {
    styleColorPushCount_ = 0;
    styleVarPushCount_ = 0;
}

ScopedThemeGuard::ScopedThemeGuard(const ::XIFriendList::App::Theming::ThemeTokens& theme)
    : stylesPushed_(false)
{
#ifndef BUILDING_TESTS
    stylesPushed_ = AshitaThemeHelper::pushThemeStyles(theme);
#else
    stylesPushed_ = false;
#endif
}

ScopedThemeGuard::~ScopedThemeGuard() {
#ifndef BUILDING_TESTS
    if (stylesPushed_) {
        AshitaThemeHelper::popThemeStyles();
    }
#endif
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

