#include "NotesCore.h"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace XIFriendList {
namespace Core {

std::string NoteMerger::merge(
    const std::string& localNote, 
    uint64_t localTimestamp,
    const std::string& serverNote, 
    uint64_t serverTimestamp) {
    
    std::string local = trim(localNote);
    std::string server = trim(serverNote);
    
    if (local.empty() && server.empty()) {
        return "";
    }
    
    if (local.empty()) {
        return server;
    }
    if (server.empty()) {
        return local;
    }
    
    if (areNotesEqual(local, server)) {
        return local;
    }
    
    bool localHasMarker = containsMergeMarker(local);
    bool serverHasMarker = containsMergeMarker(server);
    
    if (localHasMarker && serverHasMarker) {
        return (localTimestamp >= serverTimestamp) ? local : server;
    }
    
    std::ostringstream merged;
    
    if (localTimestamp >= serverTimestamp) {
        merged << "=== Local Note (" << formatTimestamp(localTimestamp) << ") ===\n";
        merged << local;
        merged << MERGE_DIVIDER;
        merged << "=== Server Note (" << formatTimestamp(serverTimestamp) << ") ===\n";
        merged << server;
    } else {
        merged << "=== Server Note (" << formatTimestamp(serverTimestamp) << ") ===\n";
        merged << server;
        merged << MERGE_DIVIDER;
        merged << "=== Local Note (" << formatTimestamp(localTimestamp) << ") ===\n";
        merged << local;
    }
    
    return merged.str();
}

bool NoteMerger::containsMergeMarker(const std::string& note) {
    if (note.find("--- Merged Notes ---") != std::string::npos) {
        return true;
    }
    if (note.find("=== Local Note (") != std::string::npos ||
        note.find("=== Server Note (") != std::string::npos) {
        return true;
    }
    return false;
}

bool NoteMerger::areNotesEqual(const std::string& note1, const std::string& note2) {
    std::string t1 = trim(note1);
    std::string t2 = trim(note2);
    return t1 == t2;
}

std::string NoteMerger::formatTimestamp(uint64_t timestamp) {
    if (timestamp == 0) {
        return "unknown";
    }
    
    time_t epochSeconds;
    if (timestamp > 1000000000000ULL) {
        epochSeconds = static_cast<time_t>(timestamp / 1000);
    } else {
        epochSeconds = static_cast<time_t>(timestamp);
    }
    
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &epochSeconds);
#else
    localtime_r(&epochSeconds, &timeInfo);
#endif
    
    std::ostringstream ss;
    ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M");
    return ss.str();
}

std::string NoteMerger::trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

} // namespace Core
} // namespace XIFriendList

