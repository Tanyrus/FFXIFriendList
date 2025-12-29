
#ifndef DEBUG_DEBUG_LOG_H
#define DEBUG_DEBUG_LOG_H

#include <string>
#include <vector>
#include <mutex>
#include <chrono>

namespace XIFriendList {
namespace Debug {

constexpr size_t MAX_LOG_LINES = 1000;

struct LogEntry {
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    
    LogEntry() = default;
    LogEntry(const std::string& msg) 
        : message(msg)
        , timestamp(std::chrono::system_clock::now())
    {}
};

class DebugLog {
public:
    static DebugLog& getInstance() {
        static DebugLog instance;
        return instance;
    }
    
    void push(const std::string& message);
    
    std::vector<LogEntry> snapshot() const;
    
    void clear();
    
    size_t size() const;
    
    size_t maxLines() const { return MAX_LOG_LINES; }

private:
    DebugLog();
    ~DebugLog() = default;
    DebugLog(const DebugLog&) = delete;
    DebugLog& operator=(const DebugLog&) = delete;
    
    mutable std::mutex mutex_;
    std::vector<LogEntry> entries_;
    size_t writeIndex_;  // Current write position in ring buffer
    bool isFull_;        // Whether buffer has wrapped around
};

} // namespace Debug
} // namespace XIFriendList

#endif // DEBUG_DEBUG_LOG_H

