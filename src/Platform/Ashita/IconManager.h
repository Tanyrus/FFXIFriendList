
#ifndef PLATFORM_ASHITA_ICON_MANAGER_H
#define PLATFORM_ASHITA_ICON_MANAGER_H

#include "Core/MemoryStats.h"
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

struct IDirect3DDevice8;

namespace XIFriendList {
namespace Platform {
namespace Ashita {

using IconHandle = void*;

enum class IconType {
    Online,
    Offline,
    FriendRequest,
    Pending,
    Discord,
    GitHub,
    Heart,
    NationSandy,
    NationBastok,
    NationWindurst,
    NationJeuno,
    Lock,
    Unlock
};

class IconManager {
public:
    IconManager();
    ~IconManager();
    bool initialize(IDirect3DDevice8* device);
    IconHandle getIcon(IconType type);
    void processPendingCreates(uint32_t maxIconsToCreate = 1);

    bool isIconAvailable(IconType type);
    
    // Release all loaded icons and textures
    void release();
    
    ::XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    IDirect3DDevice8* device_;
    IconHandle onlineIcon_;
    IconHandle offlineIcon_;
    IconHandle friendRequestIcon_;
    IconHandle pendingIcon_;
    IconHandle discordIcon_;
    IconHandle githubIcon_;
    IconHandle heartIcon_;
    IconHandle nationSandyIcon_;
    IconHandle nationBastokIcon_;
    IconHandle nationWindurstIcon_;
    IconHandle nationJeunoIcon_;
    IconHandle lockIcon_;
    IconHandle unlockIcon_;
    bool initialized_;

    struct DecodedIcon {
        IconType type;
        int width;
        int height;
        std::vector<uint8_t> bgra; // BGRA bytes, row-major, width*height*4
    };

    std::atomic<bool> shutdownRequested_;
    std::thread decodeThread_;
    std::atomic<bool> decodeThreadStarted_;
    mutable std::mutex decodedMutex_;
    std::vector<DecodedIcon> decodedQueue_;
    IconHandle loadIconFromMemory(const unsigned char* data, int dataSize, IconType type);
    void decodeIconToQueue(const unsigned char* data, int dataSize, IconType type);
    IconHandle createTextureFromBgra(int width, int height, const std::vector<uint8_t>& bgra);
    void releaseTexture(IconHandle& handle);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_ICON_MANAGER_H

