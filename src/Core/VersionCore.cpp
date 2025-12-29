
#include "VersionCore.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace XIFriendList {
namespace Core {

bool Version::parse(const std::string& versionStr, Version& out) {
    if (versionStr.empty()) {
        return false;
    }
    
    out = Version();
    
    std::string lower = versionStr;
    std::transform(lower.begin(), lower.end(), lower.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "dev" || lower == "unknown" || lower == "0.0.0-dev") {
        return false;
    }
    
    std::string str = versionStr;
    if (str.length() > 0 && (str[0] == 'v' || str[0] == 'V')) {
        str = str.substr(1);
    }
    
    std::string versionPart = str;
    std::string buildPart = "";
    size_t plusPos = str.find('+');
    if (plusPos != std::string::npos) {
        versionPart = str.substr(0, plusPos);
        buildPart = str.substr(plusPos + 1);
    }
    
    std::string baseVersion = versionPart;
    std::string prereleasePart = "";
    size_t dashPos = versionPart.find('-');
    if (dashPos != std::string::npos) {
        baseVersion = versionPart.substr(0, dashPos);
        prereleasePart = versionPart.substr(dashPos + 1);
    }
    
    std::istringstream iss(baseVersion);
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
    
    if (!std::getline(iss, token)) {
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
    
    out.prerelease = prereleasePart;
    out.build = buildPart;
    
    return true;
}

std::string Version::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) {
        oss << "-" << prerelease;
    }
    if (!build.empty()) {
        oss << "+" << build;
    }
    return oss.str();
}

bool Version::isValid() const {
    return major >= 0 && minor >= 0 && patch >= 0;
}

bool Version::isDevVersion() const {
    return !isValid() || (!prerelease.empty() && prerelease.find("dev") != std::string::npos);
}

bool Version::operator==(const Version& other) const {
    return major == other.major && 
           minor == other.minor && 
           patch == other.patch &&
           prerelease == other.prerelease;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

bool Version::operator<(const Version& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;
    
    if (prerelease.empty() && !other.prerelease.empty()) {
        return false;
    }
    if (!prerelease.empty() && other.prerelease.empty()) {
        return true;
    }
    if (prerelease.empty() && other.prerelease.empty()) {
        return false;
    }
    
    return prerelease < other.prerelease;
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

bool Version::isOutdated(const Version& latest) const {
    return *this < latest;
}

Version parseVersion(const std::string& versionStr) {
    Version v;
    if (!Version::parse(versionStr, v)) {
        throw std::invalid_argument("Invalid version string: " + versionStr);
    }
    return v;
}

bool isValidVersionString(const std::string& versionStr) {
    Version v;
    return Version::parse(versionStr, v);
}

} // namespace Core
} // namespace XIFriendList

