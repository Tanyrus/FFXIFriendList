#ifndef CORE_MEMORY_STATS_H
#define CORE_MEMORY_STATS_H

#include <cstdint>
#include <string>

namespace XIFriendList {
namespace Core {

struct MemoryStats {
    size_t entryCount = 0;
    size_t estimatedBytes = 0;
    std::string category;
    
    MemoryStats() = default;
    MemoryStats(size_t count, size_t bytes, const std::string& cat)
        : entryCount(count), estimatedBytes(bytes), category(cat) {}
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_MEMORY_STATS_H

