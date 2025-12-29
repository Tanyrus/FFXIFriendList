
#ifndef PLATFORM_ASHITA_THEME_PERSISTENCE_H
#define PLATFORM_ASHITA_THEME_PERSISTENCE_H

#include "App/State/ThemeState.h"
#include "Core/ModelsCore.h"
#include <string>
#include <vector>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class ThemePersistence {
public:
    static bool loadFromFile(::XIFriendList::App::State::ThemeState& state);
    static bool saveToFile(const ::XIFriendList::App::State::ThemeState& state);

private:
    static std::string getConfigPath();
    static std::string getCustomThemesPath();
    static void ensureConfigDirectory(const std::string& filePath);
    static ::XIFriendList::Core::Color parseColor(const std::string& colorStr);
    static std::string formatColor(const ::XIFriendList::Core::Color& color);
    static void trimString(std::string& str);
    static std::string readIniValue(const std::string& filePath, const std::string& key);
    static bool writeIniValue(const std::string& filePath, const std::string& key, const std::string& value);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_THEME_PERSISTENCE_H

