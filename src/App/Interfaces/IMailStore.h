
#ifndef APP_IMAIL_STORE_H
#define APP_IMAIL_STORE_H

#include "Core/ModelsCore.h"
#include <string>
#include <vector>
#include <optional>

namespace XIFriendList {
namespace App {

class IMailStore {
public:
    virtual ~IMailStore() = default;
    virtual bool upsertMessage(XIFriendList::Core::MailFolder mailboxType, const XIFriendList::Core::MailMessage& message) = 0;
    virtual bool hasMessage(XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) = 0;
    virtual std::optional<XIFriendList::Core::MailMessage> getMessage(XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) = 0;
    virtual std::vector<XIFriendList::Core::MailMessage> getAllMessages(XIFriendList::Core::MailFolder mailboxType) = 0;
    virtual bool markRead(XIFriendList::Core::MailFolder mailboxType, const std::string& messageId, bool isRead, uint64_t readAt) = 0;
    virtual bool deleteMessage(XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) = 0;
    virtual int pruneOld(XIFriendList::Core::MailFolder mailboxType, int maxMessages) = 0;
    virtual bool clear(XIFriendList::Core::MailFolder mailboxType) = 0;
    virtual int getMessageCount(XIFriendList::Core::MailFolder mailboxType) = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_IMAIL_STORE_H

