#include "AshitaSessionManager.h"
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <random>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaSessionManager::AshitaSessionManager()
    : sessionId_(generateSessionId())
    , accountId_(0)
    , characterId_(0)
    , activeCharacterId_(0)
{
}

std::string AshitaSessionManager::getSessionId() const {
    return sessionId_;
}

int64_t AshitaSessionManager::getAccountId() const {
    return accountId_;
}

void AshitaSessionManager::setAccountId(int64_t accountId) {
    accountId_ = accountId;
}

int64_t AshitaSessionManager::getCharacterId() const {
    return characterId_;
}

void AshitaSessionManager::setCharacterId(int64_t characterId) {
    characterId_ = characterId;
}

int64_t AshitaSessionManager::getActiveCharacterId() const {
    return activeCharacterId_;
}

void AshitaSessionManager::setActiveCharacterId(int64_t activeCharacterId) {
    activeCharacterId_ = activeCharacterId;
}

void AshitaSessionManager::clearSession() {
    accountId_ = 0;
    characterId_ = 0;
    activeCharacterId_ = 0;
}

bool AshitaSessionManager::isAuthenticated() const {
    return accountId_ != 0 && characterId_ != 0;
}

std::string AshitaSessionManager::generateSessionId() const {
    std::ostringstream ss;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    uint32_t a = dist(gen);
    uint16_t b = static_cast<uint16_t>(dist(gen) & 0xFFFF);
    uint16_t c = static_cast<uint16_t>((dist(gen) & 0x0FFF) | 0x4000);
    uint16_t d = static_cast<uint16_t>((dist(gen) & 0x3FFF) | 0x8000);
    uint32_t e1 = dist(gen);
    uint16_t e2 = static_cast<uint16_t>(dist(gen) & 0xFFFF);
    ss << std::hex << std::setfill('0')
       << std::setw(8) << a << "-"
       << std::setw(4) << b << "-"
       << std::setw(4) << c << "-"
       << std::setw(4) << d << "-"
       << std::setw(8) << e1
       << std::setw(4) << e2;
    return ss.str();
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

