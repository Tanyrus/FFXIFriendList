
#ifndef PLATFORM_ASHITA_NOTIFIED_MAIL_STORE_H
#define PLATFORM_ASHITA_NOTIFIED_MAIL_STORE_H

#include "App/Interfaces/INotifiedMailStore.h"
#include <string>
#include <set>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaNotifiedMailStore : public ::XIFriendList::App::INotifiedMailStore {
public:
    AshitaNotifiedMailStore();
    ~AshitaNotifiedMailStore() = default;
    std::set<std::string> loadNotifiedMailIds(const std::string& characterName) override;
    bool saveNotifiedMailId(const std::string& characterName, const std::string& messageId) override;
    bool saveNotifiedMailIds(const std::string& characterName, const std::set<std::string>& messageIds) override;

private:
    std::string getCacheJsonPath() const;
    std::string getConfigPath() const;
    void ensureConfigDirectory(const std::string& filePath) const;
    std::string normalizeCharacterName(const std::string& name) const;
    void trimString(std::string& str) const;
    std::set<std::string> splitCommaSeparated(const std::string& str) const;
    std::string joinCommaSeparated(const std::set<std::string>& ids) const;
    std::set<std::string> loadNotifiedMailIdsFromJson(const std::string& characterName) const;
    bool saveNotifiedMailIdsToJson(const std::string& characterName, const std::set<std::string>& messageIds);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_NOTIFIED_MAIL_STORE_H

