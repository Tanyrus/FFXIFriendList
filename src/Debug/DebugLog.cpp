
#include "DebugLog.h"
#include <algorithm>

namespace XIFriendList {
namespace Debug {

DebugLog::DebugLog()
    : writeIndex_(0)
    , isFull_(false)
{
}

void DebugLog::push(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LogEntry entry(message);
    
    if (entries_.size() < MAX_LOG_LINES) {
        entries_.push_back(entry);
        writeIndex_ = entries_.size();
        
        if (entries_.size() == MAX_LOG_LINES) {
            isFull_ = true;
            writeIndex_ = 0;  // Next write will wrap to start
        }
    } else {
        entries_[writeIndex_] = entry;
        writeIndex_ = (writeIndex_ + 1) % MAX_LOG_LINES;
    }
}

std::vector<LogEntry> DebugLog::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LogEntry> result;
    
    if (entries_.empty()) {
        return result;
    }
    
    if (!isFull_) {
        result = entries_;
    } else {
        result.reserve(MAX_LOG_LINES);
        
        for (size_t i = writeIndex_; i < MAX_LOG_LINES; ++i) {
            result.push_back(entries_[i]);
        }
        
        for (size_t i = 0; i < writeIndex_; ++i) {
            result.push_back(entries_[i]);
        }
    }
    
    return result;
}

void DebugLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    writeIndex_ = 0;
    isFull_ = false;
}

size_t DebugLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

} // namespace Debug
} // namespace XIFriendList

