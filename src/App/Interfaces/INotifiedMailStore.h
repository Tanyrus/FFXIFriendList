
#ifndef APP_INOTIFIED_MAIL_STORE_H
#define APP_INOTIFIED_MAIL_STORE_H

#include <string>
#include <set>

namespace XIFriendList {
namespace App {

class INotifiedMailStore {
public:
    virtual ~INotifiedMailStore() = default;
    virtual std::set<std::string> loadNotifiedMailIds(const std::string& characterName) = 0;
    virtual bool saveNotifiedMailId(const std::string& characterName, const std::string& messageId) = 0;
    virtual bool saveNotifiedMailIds(const std::string& characterName, const std::set<std::string>& messageIds) = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_INOTIFIED_MAIL_STORE_H

