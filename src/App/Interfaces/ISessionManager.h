
#ifndef APP_ISESSION_MANAGER_H
#define APP_ISESSION_MANAGER_H

#include <string>
#include <cstdint>

namespace XIFriendList {
namespace App {

class ISessionManager {
public:
    virtual ~ISessionManager() = default;
    virtual std::string getSessionId() const = 0;
    virtual int64_t getAccountId() const = 0;
    virtual void setAccountId(int64_t accountId) = 0;
    virtual int64_t getCharacterId() const = 0;
    virtual void setCharacterId(int64_t characterId) = 0;
    virtual int64_t getActiveCharacterId() const = 0;
    virtual void setActiveCharacterId(int64_t activeCharacterId) = 0;
    virtual void clearSession() = 0;
    virtual bool isAuthenticated() const = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_ISESSION_MANAGER_H

