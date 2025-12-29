
#ifndef APP_IREALM_DETECTOR_H
#define APP_IREALM_DETECTOR_H

#include <string>

namespace XIFriendList {
namespace App {

class IRealmDetector {
public:
    virtual ~IRealmDetector() = default;
    virtual std::string detectRealm() const = 0;
    virtual std::string getRealmId() const = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_IREALM_DETECTOR_H

