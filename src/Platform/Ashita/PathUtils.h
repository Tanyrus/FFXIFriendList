
#ifndef PLATFORM_ASHITA_PATH_UTILS_H
#define PLATFORM_ASHITA_PATH_UTILS_H

#include <string>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class PathUtils {
public:
    static std::string getDefaultConfigDirectory();
    static std::string getDefaultConfigPath(const std::string& filename);
    static std::string getDefaultCachePath();
    static std::string getDefaultIniPath();
    static std::string getDefaultMainJsonPath();
    static std::string getDefaultThemesIniPath();
    static std::string getDefaultNotesJsonPath();
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_PATH_UTILS_H

