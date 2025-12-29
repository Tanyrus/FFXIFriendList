// FakeLogger.h
// Fake implementation of ILogger for testing

#ifndef APP_FAKE_LOGGER_H
#define APP_FAKE_LOGGER_H

#include "App/Interfaces/ILogger.h"
#include <vector>
#include <string>

namespace XIFriendList {
namespace App {

// Log entry
struct LogEntry {
    LogLevel level;
    std::string module;
    std::string message;
};

// Fake logger for testing
class FakeLogger : public ILogger {
public:
    // Get log entries
    const std::vector<LogEntry>& getEntries() const { return entries_; }
    
    // Clear log entries
    void clear() { entries_.clear(); }
    
    // Check if contains message
    bool contains(const std::string& message) const {
        for (const auto& entry : entries_) {
            if (entry.message.find(message) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // ILogger interface
    void debug(const std::string& message) override {
        entries_.push_back({ LogLevel::Debug, "", message });
    }
    
    void info(const std::string& message) override {
        entries_.push_back({ LogLevel::Info, "", message });
    }
    
    void warning(const std::string& message) override {
        entries_.push_back({ LogLevel::Warning, "", message });
    }
    
    void error(const std::string& message) override {
        entries_.push_back({ LogLevel::Error, "", message });
    }
    
    void log(LogLevel level, 
             const std::string& module,
             const std::string& message) override {
        entries_.push_back({ level, module, message });
    }

private:
    std::vector<LogEntry> entries_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_FAKE_LOGGER_H

