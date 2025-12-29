
#include "ProtocolVersion.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace XIFriendList {
namespace Protocol {

bool Version::parse(const std::string& versionStr, Version& out) {
    if (versionStr.empty()) {
        return false;
    }
    
    std::istringstream iss(versionStr);
    std::string token;
    
    if (!std::getline(iss, token, '.')) {
        return false;
    }
    try {
        out.major = std::stoi(token);
    } catch (...) {
        return false;
    }
    
    if (!std::getline(iss, token, '.')) {
        return false;
    }
    try {
        out.minor = std::stoi(token);
    } catch (...) {
        return false;
    }
    
    if (!std::getline(iss, token, '.')) {
        return false;
    }
    try {
        out.patch = std::stoi(token);
    } catch (...) {
        return false;
    }
    
    if (std::getline(iss, token)) {
        return false;
    }
    
    return true;
}

std::string Version::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

bool Version::operator==(const Version& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

bool Version::operator<(const Version& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    return patch < other.patch;
}

bool Version::operator>(const Version& other) const {
    return other < *this;
}

bool Version::operator<=(const Version& other) const {
    return !(other < *this);
}

bool Version::operator>=(const Version& other) const {
    return !(*this < other);
}

bool Version::isCompatibleWith(const Version& other) const {
    return major == other.major;
}

Version getCurrentVersion() {
    Version v;
    Version::parse(PROTOCOL_VERSION, v);
    return v;
}

bool isValidVersion(const std::string& versionStr) {
    Version v;
    return Version::parse(versionStr, v);
}

} // namespace Protocol
} // namespace XIFriendList

