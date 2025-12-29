
#ifndef PLATFORM_ASHITA_MAIL_STORE_H
#define PLATFORM_ASHITA_MAIL_STORE_H

#include "App/Interfaces/IMailStore.h"
#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <map>
#include <set>
#include <mutex>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaMailStore : public ::XIFriendList::App::IMailStore {
public:
    AshitaMailStore();
    ~AshitaMailStore() = default;
    bool upsertMessage(::XIFriendList::Core::MailFolder mailboxType, const ::XIFriendList::Core::MailMessage& message) override;
    bool hasMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) override;
    std::optional<::XIFriendList::Core::MailMessage> getMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) override;
    std::vector<::XIFriendList::Core::MailMessage> getAllMessages(::XIFriendList::Core::MailFolder mailboxType) override;
    bool markRead(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId, bool isRead, uint64_t readAt) override;
    bool deleteMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) override;
    int pruneOld(::XIFriendList::Core::MailFolder mailboxType, int maxMessages) override;
    bool clear(::XIFriendList::Core::MailFolder mailboxType) override;
    int getMessageCount(::XIFriendList::Core::MailFolder mailboxType) override;
    void setCharacterName(const std::string& characterName);
    bool loadFromDisk();
    bool saveToDisk();
    
    ::XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    std::string getCacheFilePath() const;
    
    // Ensure cache directory exists
    void ensureCacheDirectory(const std::string& filePath) const;
    
    std::string normalizeCharacterName(const std::string& name) const;
    
    // Convert mailbox type to string
    std::string mailboxTypeToString(::XIFriendList::Core::MailFolder type) const;
    
    // Convert string to mailbox type
    ::XIFriendList::Core::MailFolder stringToMailboxType(const std::string& str) const;
    
    void parseMessageArray(const std::string& arrayJson, ::XIFriendList::Core::MailFolder mailboxType);
    
    bool parseMessageObject(const std::string& objJson, ::XIFriendList::Core::MailMessage& out);
    
    // Write message array to JSON
    void writeMessageArray(std::ofstream& file, ::XIFriendList::Core::MailFolder mailboxType);
    
    // Write single message object to JSON
    void writeMessageObject(std::ofstream& file, const ::XIFriendList::Core::MailMessage& msg);
    
    // In-memory storage: mailboxType -> (messageId -> message)
    std::map<::XIFriendList::Core::MailFolder, std::map<std::string, ::XIFriendList::Core::MailMessage>> messages_;
    
    std::string characterName_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Dirty flag (needs save)
    bool dirty_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_MAIL_STORE_H

