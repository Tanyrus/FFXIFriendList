// ViewModel for Mail window (UI layer)

#ifndef UI_MAIL_VIEW_MODEL_H
#define UI_MAIL_VIEW_MODEL_H

#include "Core/ModelsCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <vector>
#include <cstdint>

namespace XIFriendList {
namespace UI {

struct MailRowData {
    std::string messageId;
    std::string fromText;      // "From: CharacterName" or "To: CharacterName"
    std::string subject;
    std::string timestampText;  // Preformatted timestamp (e.g., "2 hours ago")
    bool isUnread;
    bool isSelected;
    
    MailRowData()
        : isUnread(false)
        , isSelected(false)
    {}
};

// ViewModel for Mail window
// Holds UI state and provides formatted strings/flags for rendering
class MailViewModel {
public:
    MailViewModel();
    ~MailViewModel() = default;
    
    // Folder selection
    XIFriendList::Core::MailFolder getCurrentFolder() const { return currentFolder_; }
    void setCurrentFolder(XIFriendList::Core::MailFolder folder) { currentFolder_ = folder; }
    
    void updateMailList(const std::vector<XIFriendList::Core::MailMessage>& messages, XIFriendList::Core::MailFolder folder);
    
    const std::vector<MailRowData>& getMailRows() const {
        return mailRows_;
    }
    
    // Selected message
    const std::string& getSelectedMessageId() const { return selectedMessageId_; }
    void setSelectedMessageId(const std::string& messageId);
    
    const XIFriendList::Core::MailMessage* getSelectedMessage() const;
    bool hasSelectedMessage() const { return !selectedMessageId_.empty(); }
    
    // Unread count
    int getUnreadCount() const { return unreadCount_; }
    void setUnreadCount(int count) { unreadCount_ = count; }
    
    // Loading state
    bool isLoading() const { return isLoading_; }
    void setLoading(bool loading) { isLoading_ = loading; }
    
    // Error state
    bool hasError() const { return !errorMessage_.empty(); }
    const std::string& getError() const { return errorMessage_; }
    void setError(const std::string& message) { errorMessage_ = message; }
    void clearError() { errorMessage_.clear(); }
    
    // Format timestamp for display
    static std::string formatTimestamp(uint64_t timestamp);
    
    // Refresh flag (to trigger reload)
    bool needsRefresh() const { return needsRefresh_; }
    void setNeedsRefresh(bool refresh) { needsRefresh_ = refresh; }
    
    // Cache info
    int getCachedInboxCount() const { return cachedInboxCount_; }
    int getCachedSentCount() const { return cachedSentCount_; }
    void setCachedInboxCount(int count) { cachedInboxCount_ = count; }
    void setCachedSentCount(int count) { cachedSentCount_ = count; }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    XIFriendList::Core::MailFolder currentFolder_;
    std::vector<XIFriendList::Core::MailMessage> messages_;  // Current folder's messages
    std::vector<MailRowData> mailRows_;         // Preformatted rows for display
    std::string selectedMessageId_;
    int unreadCount_;
    bool isLoading_;
    std::string errorMessage_;
    bool needsRefresh_;
    int cachedInboxCount_;
    int cachedSentCount_;
    
    // Helper to create row data from message
    MailRowData createRowData(const XIFriendList::Core::MailMessage& msg, XIFriendList::Core::MailFolder folder);
};

} // namespace UI
} // namespace XIFriendList
#endif // UI_MAIL_VIEW_MODEL_H

