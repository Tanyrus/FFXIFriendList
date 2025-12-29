
#include "UI/ViewModels/MailViewModel.h"
#include "Core/MemoryStats.h"
#include <algorithm>
#include <sstream>
#include <ctime>

namespace XIFriendList {
namespace UI {

MailViewModel::MailViewModel()
    : currentFolder_(XIFriendList::Core::MailFolder::Inbox)
    , unreadCount_(0)
    , isLoading_(false)
    , needsRefresh_(false)
    , cachedInboxCount_(0)
    , cachedSentCount_(0)
{
}

void MailViewModel::updateMailList(const std::vector<XIFriendList::Core::MailMessage>& messages, XIFriendList::Core::MailFolder folder) {
    messages_ = messages;
    currentFolder_ = folder;
    
    mailRows_.clear();
    mailRows_.reserve(messages.size());
    
    for (const auto& msg : messages) {
        mailRows_.push_back(createRowData(msg, folder));
    }
    
    // Sort by timestamp (newest first) - need to find messages to compare
    std::sort(mailRows_.begin(), mailRows_.end(), [this](const MailRowData& a, const MailRowData& b) {
        // Find corresponding messages to compare timestamps
        uint64_t timestampA = 0;
        uint64_t timestampB = 0;
        for (const auto& msg : messages_) {
            if (msg.messageId == a.messageId) {
                timestampA = msg.createdAt;
            }
            if (msg.messageId == b.messageId) {
                timestampB = msg.createdAt;
            }
        }
        return timestampA > timestampB;  // Reverse order for newest first
    });
    
    if (!selectedMessageId_.empty()) {
        bool found = false;
        for (const auto& msg : messages_) {
            if (msg.messageId == selectedMessageId_) {
                found = true;
                break;
            }
        }
        if (!found) {
            selectedMessageId_.clear();
        }
    }
}

MailRowData MailViewModel::createRowData(const XIFriendList::Core::MailMessage& msg, XIFriendList::Core::MailFolder folder) {
    MailRowData row;
    row.messageId = msg.messageId;
    row.isUnread = msg.isUnread();
    row.isSelected = (msg.messageId == selectedMessageId_);
    
    // Format from/to text based on folder
    if (folder == XIFriendList::Core::MailFolder::Inbox) {
        row.fromText = "From: " + msg.fromUserId;
    } else {
        row.fromText = "To: " + msg.toUserId;
    }
    
    row.subject = msg.subject;
    row.timestampText = formatTimestamp(msg.createdAt);
    
    return row;
}

void MailViewModel::setSelectedMessageId(const std::string& messageId) {
    selectedMessageId_ = messageId;
    
    for (auto& row : mailRows_) {
        row.isSelected = (row.messageId == messageId);
    }
}

const XIFriendList::Core::MailMessage* MailViewModel::getSelectedMessage() const {
    if (selectedMessageId_.empty()) {
        return nullptr;
    }
    
    for (const auto& msg : messages_) {
        if (msg.messageId == selectedMessageId_) {
            return &msg;
        }
    }
    
    return nullptr;
}

std::string MailViewModel::formatTimestamp(uint64_t timestamp) {
    if (timestamp == 0) {
        return "Unknown";
    }
    
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr)) * 1000;  // Convert to milliseconds
    
    if (timestamp > currentTime) {
        return "Just now";
    }
    
    uint64_t diffMs = currentTime - timestamp;
    uint64_t diffSeconds = diffMs / 1000;
    uint64_t diffMinutes = diffSeconds / 60;
    uint64_t diffHours = diffMinutes / 60;
    uint64_t diffDays = diffHours / 24;
    
    if (diffSeconds < 60) {
        return "Just now";
    } else if (diffMinutes < 60) {
        return std::to_string(diffMinutes) + (diffMinutes == 1 ? " minute ago" : " minutes ago");
    } else if (diffHours < 24) {
        return std::to_string(diffHours) + (diffHours == 1 ? " hour ago" : " hours ago");
    } else if (diffDays < 7) {
        return std::to_string(diffDays) + (diffDays == 1 ? " day ago" : " days ago");
    } else {
        // Format as date
        std::time_t timeT = static_cast<std::time_t>(timestamp / 1000);
        std::tm tm;
        #ifdef _WIN32
        if (localtime_s(&tm, &timeT) == 0) {
            std::ostringstream oss;
            oss << (tm.tm_mon + 1) << "/" << tm.tm_mday << "/" << (tm.tm_year + 1900);
            return oss.str();
        }
        #else
        std::tm* tmPtr = localtime_r(&timeT, &tm);
        if (tmPtr != nullptr) {
            std::ostringstream oss;
            oss << (tm.tm_mon + 1) << "/" << tm.tm_mday << "/" << (tm.tm_year + 1900);
            return oss.str();
        }
        #endif
        return "Long ago";
    }
}

XIFriendList::Core::MemoryStats MailViewModel::getMemoryStats() const {
    size_t bytes = sizeof(MailViewModel);
    
    for (const auto& msg : messages_) {
        bytes += sizeof(XIFriendList::Core::MailMessage);
        bytes += msg.messageId.capacity();
        bytes += msg.fromUserId.capacity();
        bytes += msg.toUserId.capacity();
        bytes += msg.subject.capacity();
        bytes += msg.body.capacity();
    }
    bytes += messages_.capacity() * sizeof(XIFriendList::Core::MailMessage);
    
    for (const auto& row : mailRows_) {
        bytes += sizeof(MailRowData);
        bytes += row.messageId.capacity();
        bytes += row.fromText.capacity();
        bytes += row.subject.capacity();
        bytes += row.timestampText.capacity();
    }
    bytes += mailRows_.capacity() * sizeof(MailRowData);
    
    bytes += selectedMessageId_.capacity();
    bytes += errorMessage_.capacity();
    
    size_t count = messages_.size() + mailRows_.size();
    
    return XIFriendList::Core::MemoryStats(count, bytes, "Mail ViewModel");
}

} // namespace UI
} // namespace XIFriendList

