#include "PatternScanFingerprint.h"

#include <sstream>
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

static uint64_t fnv1a64(const uint8_t* data, size_t size) {
    constexpr uint64_t FNV_OFFSET_BASIS = 1469598103934665603ull;
    constexpr uint64_t FNV_PRIME = 1099511628211ull;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

ModuleFingerprint computeModuleFingerprint(uintptr_t moduleBase, uintptr_t moduleSize, size_t maxHashBytes) {
    ModuleFingerprint fp;
    fp.moduleBase = moduleBase;
    fp.moduleSize = moduleSize;

    if (moduleBase == 0 || moduleSize == 0) {
        return fp;
    }

    const uint8_t* basePtr = reinterpret_cast<const uint8_t*>(moduleBase);

    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(basePtr);
    if (dos && dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew > 0) {
        const uint8_t* ntPtr = basePtr + static_cast<size_t>(dos->e_lfanew);
        const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(ntPtr);
        if (nt && nt->Signature == IMAGE_NT_SIGNATURE) {
            fp.peTimeDateStamp = nt->FileHeader.TimeDateStamp;
            fp.sizeOfImage = nt->OptionalHeader.SizeOfImage;
            fp.entryPointRva = nt->OptionalHeader.AddressOfEntryPoint;
        }
    }

    const size_t bytesToHash = (maxHashBytes < static_cast<size_t>(moduleSize))
        ? maxHashBytes
        : static_cast<size_t>(moduleSize);
    fp.hashedBytes = bytesToHash;
    fp.headHash64 = fnv1a64(basePtr, bytesToHash);

    uint64_t combined = 0;
    combined ^= mix64(static_cast<uint64_t>(fp.peTimeDateStamp));
    combined ^= mix64(static_cast<uint64_t>(fp.sizeOfImage) << 1);
    combined ^= mix64(static_cast<uint64_t>(fp.entryPointRva) << 2);
    combined ^= mix64(fp.headHash64);
    combined ^= mix64(static_cast<uint64_t>(fp.moduleSize) << 3);
    fp.fingerprint64 = combined;

    return fp;
}

std::string formatModuleFingerprint(const ModuleFingerprint& fp) {
    std::ostringstream oss;
    oss << "base=0x" << std::hex << static_cast<uintptr_t>(fp.moduleBase)
        << " size=0x" << static_cast<uintptr_t>(fp.moduleSize)
        << " peTimeDateStamp=0x" << static_cast<uint32_t>(fp.peTimeDateStamp)
        << " sizeOfImage=0x" << static_cast<uint32_t>(fp.sizeOfImage)
        << " entryPointRva=0x" << static_cast<uint32_t>(fp.entryPointRva)
        << " headHash64=0x" << static_cast<uint64_t>(fp.headHash64)
        << " hashedBytes=0x" << static_cast<uintptr_t>(fp.hashedBytes)
        << " fingerprint64=0x" << static_cast<uint64_t>(fp.fingerprint64)
        << std::dec;
    return oss.str();
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList


