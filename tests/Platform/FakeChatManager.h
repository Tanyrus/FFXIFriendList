// FakeChatManager.h
// Fake implementation of IChatManager for testing

#ifndef PLATFORM_FAKE_CHAT_MANAGER_H
#define PLATFORM_FAKE_CHAT_MANAGER_H

#include <vector>
#include <string>

// Forward declaration (IChatManager is from Ashita SDK)
struct IChatManager;

namespace Platform {
namespace Ashita {
namespace Test {

// Chat message entry for verification
struct ChatMessage {
    uint32_t color;
    bool isSystem;
    std::string message;
};

// Fake chat manager for testing
class FakeChatManager {
public:
    FakeChatManager() {}
    
    // Get all messages sent via Write()
    const std::vector<ChatMessage>& getMessages() const { return messages_; }
    
    // Clear all messages
    void clear() { messages_.clear(); }
    
    // Check if contains message
    bool contains(const std::string& message) const {
        for (const auto& msg : messages_) {
            if (msg.message.find(message) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // Get last message
    std::string getLastMessage() const {
        if (messages_.empty()) {
            return "";
        }
        return messages_.back().message;
    }
    
    // Simulate IChatManager::Write()
    void Write(uint32_t color, bool isSystem, const char* message) {
        messages_.push_back({ color, isSystem, std::string(message) });
    }
    
    // Get IChatManager pointer (for use in tests that need the interface)
    // Note: This is a workaround - in real tests, we'd need to properly mock IChatManager
    // For now, we'll test the echo functionality through the adapter's debugEcho method
    IChatManager* getInterface() {
        // This would need proper implementation if we want to pass to adapter
        // For now, tests will verify through captured messages
        return nullptr;
    }

private:
    std::vector<ChatMessage> messages_;
};

} // namespace Test
} // namespace Ashita
} // namespace Platform

#endif // PLATFORM_FAKE_CHAT_MANAGER_H

