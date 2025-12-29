
#ifndef PLATFORM_ASHITA_REALM_DETECTOR_H
#define PLATFORM_ASHITA_REALM_DETECTOR_H

#include "App/Interfaces/IRealmDetector.h"
#include <string>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaRealmDetector : public ::XIFriendList::App::IRealmDetector {
public:
    AshitaRealmDetector();
    ~AshitaRealmDetector() = default;
    
    std::string detectRealm() const override;
    std::string getRealmId() const override;

private:
    std::string cachedRealmId_;
    
    std::string getConfigDirectory() const;
    bool fileExists(const std::string& path) const;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_REALM_DETECTOR_H

