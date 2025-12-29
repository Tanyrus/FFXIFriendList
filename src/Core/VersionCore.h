
#ifndef CORE_VERSION_CORE_H
#define CORE_VERSION_CORE_H

#include <string>

namespace XIFriendList {
namespace Core {

struct Version {
    int major;
    int minor;
    int patch;
    std::string prerelease;
    std::string build;
    
    Version() : major(0), minor(0), patch(0) {}
    Version(int mj, int mn, int pt) : major(mj), minor(mn), patch(pt) {}
    Version(int mj, int mn, int pt, const std::string& pre, const std::string& bld)
        : major(mj), minor(mn), patch(pt), prerelease(pre), build(bld) {}
    
    static bool parse(const std::string& versionStr, Version& out);
    std::string toString() const;
    bool isValid() const;
    bool isDevVersion() const;
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const;
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator<=(const Version& other) const;
    bool operator>=(const Version& other) const;
    bool isOutdated(const Version& latest) const;
};

Version parseVersion(const std::string& versionStr);
bool isValidVersionString(const std::string& versionStr);

} // namespace Core
} // namespace XIFriendList

#endif // CORE_VERSION_CORE_H

