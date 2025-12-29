
#ifndef PROTOCOL_VERSION_H
#define PROTOCOL_VERSION_H

#include <string>

namespace XIFriendList {
namespace Protocol {

constexpr const char* PROTOCOL_VERSION = "2.0.0";

struct Version {
    int major;
    int minor;
    int patch;
    
    Version() : major(1), minor(0), patch(0) {}
    Version(int mj, int mn, int pt) : major(mj), minor(mn), patch(pt) {}
    static bool parse(const std::string& versionStr, Version& out);
    std::string toString() const;
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const;
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator<=(const Version& other) const;
    bool operator>=(const Version& other) const;
    bool isCompatibleWith(const Version& other) const;
};

Version getCurrentVersion();
bool isValidVersion(const std::string& versionStr);

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_VERSION_H

