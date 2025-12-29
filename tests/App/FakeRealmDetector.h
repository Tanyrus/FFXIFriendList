// FakeRealmDetector.h
// Fake implementation of IRealmDetector for testing

#ifndef TESTS_APP_FAKE_REALM_DETECTOR_H
#define TESTS_APP_FAKE_REALM_DETECTOR_H

#include "App/Interfaces/IRealmDetector.h"
#include <string>
#include <set>

namespace XIFriendList {
namespace App {

/**
 * Fake realm detector for testing.
 * Allows tests to configure which sentinel files "exist".
 */
class FakeRealmDetector : public IRealmDetector {
public:
    FakeRealmDetector() : cachedRealmId_("horizon") {}
    
    // Set which sentinel files exist (simulate file system)
    void setSentinelFiles(const std::set<std::string>& files) {
        sentinelFiles_ = files;
        // Re-detect realm based on new sentinel files
        cachedRealmId_ = detectRealm();
    }
    
    // Clear all sentinel files
    void clearSentinelFiles() {
        sentinelFiles_.clear();
        cachedRealmId_ = "horizon";  // Default
    }
    
    std::string detectRealm() const override {
        // Check for sentinel files in order of priority
        // Allowed realms: Nasomi, Eden, Catseye, Horizon, Gaia, LevelDown99
        if (sentinelFiles_.count("Nasomi") > 0) {
            return "nasomi";
        }
        if (sentinelFiles_.count("Eden") > 0) {
            return "eden";
        }
        if (sentinelFiles_.count("Catseye") > 0) {
            return "catseye";
        }
        if (sentinelFiles_.count("Horizon") > 0) {
            return "horizon";
        }
        if (sentinelFiles_.count("Gaia") > 0) {
            return "gaia";
        }
        if (sentinelFiles_.count("LevelDown99") > 0) {
            return "leveldown99";
        }
        // Default to Horizon if no sentinel file found
        return "horizon";
    }
    
    std::string getRealmId() const override {
        return cachedRealmId_;
    }

private:
    std::set<std::string> sentinelFiles_;
    std::string cachedRealmId_;
};

} // namespace App
} // namespace XIFriendList

#endif // TESTS_APP_FAKE_REALM_DETECTOR_H


