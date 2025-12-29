
#ifndef PLATFORM_ASHITA_PATTERN_SCAN_FINGERPRINT_H
#define PLATFORM_ASHITA_PATTERN_SCAN_FINGERPRINT_H

#include <cstdint>
#include <string>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

struct ModuleFingerprint {
    uintptr_t moduleBase{ 0 };
    uintptr_t moduleSize{ 0 };
    uint32_t peTimeDateStamp{ 0 };
    uint32_t sizeOfImage{ 0 };
    uint32_t entryPointRva{ 0 };
    uint64_t headHash64{ 0 };
    uint64_t fingerprint64{ 0 };
    size_t hashedBytes{ 0 };
};

ModuleFingerprint computeModuleFingerprint(uintptr_t moduleBase, uintptr_t moduleSize, size_t maxHashBytes = 64 * 1024);
std::string formatModuleFingerprint(const ModuleFingerprint& fp);

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_PATTERN_SCAN_FINGERPRINT_H


