
#ifndef PLATFORM_ASHITA_SESSION_MANAGER_H
#define PLATFORM_ASHITA_SESSION_MANAGER_H

#include "App/Interfaces/ISessionManager.h"
#include <string>
#include <cstdint>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaSessionManager : public ::XIFriendList::App::ISessionManager {
public:
    AshitaSessionManager();
    ~AshitaSessionManager() = default;
    
    std::string getSessionId() const override;
    int64_t getAccountId() const override;
    void setAccountId(int64_t accountId) override;
    int64_t getCharacterId() const override;
    void setCharacterId(int64_t characterId) override;
    int64_t getActiveCharacterId() const override;
    void setActiveCharacterId(int64_t activeCharacterId) override;
    void clearSession() override;
    bool isAuthenticated() const override;

private:
    std::string sessionId_;
    int64_t accountId_;
    int64_t characterId_;
    int64_t activeCharacterId_;
    
    std::string generateSessionId() const;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_SESSION_MANAGER_H

