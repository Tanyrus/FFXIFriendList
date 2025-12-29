
#ifndef PLATFORM_ASHITA_ASHITA_THEME_HELPER_H
#define PLATFORM_ASHITA_ASHITA_THEME_HELPER_H

#include "App/Theming/ThemeTokens.h"
#include <cassert>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaThemeHelper {
public:
    static bool pushThemeStyles(const ::XIFriendList::App::Theming::ThemeTokens& theme);
    static void popThemeStyles();
    static void validateStackBalance();
    static void resetStackCounters();

private:
    static int styleColorPushCount_;
    static int styleVarPushCount_;
};

class ScopedThemeGuard {
public:
    explicit ScopedThemeGuard(const ::XIFriendList::App::Theming::ThemeTokens& theme);
    ~ScopedThemeGuard();
    ScopedThemeGuard(const ScopedThemeGuard&) = delete;
    ScopedThemeGuard& operator=(const ScopedThemeGuard&) = delete;
    ScopedThemeGuard(ScopedThemeGuard&&) = delete;
    ScopedThemeGuard& operator=(ScopedThemeGuard&&) = delete;

private:
    bool stylesPushed_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_ASHITA_THEME_HELPER_H

