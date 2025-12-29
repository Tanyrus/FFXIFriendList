#include "Platform/Ashita/IconManager.h"
#include "Core/MemoryStats.h"
#include <string>
#include <cstdint>
#include <cstring>
#include <utility>
#include "Debug/Perf.h"

#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#ifndef BUILDING_TESTS
#include "EmbeddedResources.h"
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

namespace {

std::vector<uint8_t> toGrayscaleBgra(const std::vector<uint8_t>& bgra) {
    std::vector<uint8_t> out = bgra;
    const size_t pixelCount = out.size() / 4;
    for (size_t i = 0; i < pixelCount; ++i) {
        const size_t o = i * 4;
        const uint8_t b = out[o + 0];
        const uint8_t g = out[o + 1];
        const uint8_t r = out[o + 2];

        const float y = (0.114f * static_cast<float>(b)) +
                        (0.587f * static_cast<float>(g)) +
                        (0.299f * static_cast<float>(r));
        const uint8_t gray = static_cast<uint8_t>(std::clamp(y, 0.0f, 255.0f));

        out[o + 0] = gray;
        out[o + 1] = gray;
        out[o + 2] = gray;
    }
    return out;
}

} // namespace

IconManager::IconManager()
    : device_(nullptr)
    , onlineIcon_(nullptr)
    , offlineIcon_(nullptr)
    , friendRequestIcon_(nullptr)
    , pendingIcon_(nullptr)
    , discordIcon_(nullptr)
    , githubIcon_(nullptr)
    , heartIcon_(nullptr)
    , nationSandyIcon_(nullptr)
    , nationBastokIcon_(nullptr)
    , nationWindurstIcon_(nullptr)
    , nationJeunoIcon_(nullptr)
    , lockIcon_(nullptr)
    , unlockIcon_(nullptr)
    , initialized_(false)
    , shutdownRequested_(false)
    , decodeThreadStarted_(false)
{
}

IconManager::~IconManager() {
    release();
}

bool IconManager::initialize(IDirect3DDevice8* device) {
    if (initialized_) {
        return true;
    }
    
    device_ = device;
    shutdownRequested_.store(false);

#ifndef BUILDING_TESTS
    if (!decodeThreadStarted_.exchange(true)) {
        decodeThread_ = std::thread([this]() {
            decodeIconToQueue(g_OnlineImageData, g_OnlineImageData_size, IconType::Online);
            decodeIconToQueue(g_FriendRequestImageData, g_FriendRequestImageData_size, IconType::FriendRequest);
            decodeIconToQueue(g_DiscordImageData, g_DiscordImageData_size, IconType::Discord);
            decodeIconToQueue(g_GitHubImageData, g_GitHubImageData_size, IconType::GitHub);
            decodeIconToQueue(g_HeartImageData, g_HeartImageData_size, IconType::Heart);
            decodeIconToQueue(g_SandyIconImageData, g_SandyIconImageData_size, IconType::NationSandy);
            decodeIconToQueue(g_BastokIconImageData, g_BastokIconImageData_size, IconType::NationBastok);
            decodeIconToQueue(g_WindurstIconImageData, g_WindurstIconImageData_size, IconType::NationWindurst);
            decodeIconToQueue(g_LockIconImageData, g_LockIconImageData_size, IconType::Lock);
            decodeIconToQueue(g_UnlockIconImageData, g_UnlockIconImageData_size, IconType::Unlock);
        });
    }
#endif

    initialized_ = true;
    return true;
}

IconHandle IconManager::getIcon(IconType type) {
    if (!initialized_ || !device_) {
        return nullptr;
    }
    switch (type) {
        case IconType::Online:
            return onlineIcon_;
        case IconType::Offline:
            // Prefer a dedicated offline texture (grayscale). Fall back to online if not ready yet.
            return offlineIcon_ ? offlineIcon_ : onlineIcon_;
        case IconType::FriendRequest:
            return friendRequestIcon_;
        case IconType::Pending:
            if (!pendingIcon_ && friendRequestIcon_) {
                pendingIcon_ = friendRequestIcon_;  // Same icon
            }
            return pendingIcon_;
        case IconType::Discord:
            return discordIcon_;
        case IconType::GitHub:
            return githubIcon_;
        case IconType::Heart:
            return heartIcon_;
        case IconType::NationSandy:
            return nationSandyIcon_;
        case IconType::NationBastok:
            return nationBastokIcon_;
        case IconType::NationWindurst:
            return nationWindurstIcon_;
        case IconType::NationJeuno:
            return nationJeunoIcon_;
        case IconType::Lock:
            return lockIcon_;
        case IconType::Unlock:
            return unlockIcon_;
        default:
            return nullptr;
    }
}

void IconManager::processPendingCreates(uint32_t maxIconsToCreate) {
#ifndef BUILDING_TESTS
    if (!initialized_ || !device_) {
        return;
    }

    uint32_t created = 0;
    while (created < maxIconsToCreate) {
        DecodedIcon next;
        bool hasNext = false;
        {
            std::lock_guard<std::mutex> lock(decodedMutex_);
            if (!decodedQueue_.empty()) {
                next = std::move(decodedQueue_.back());
                decodedQueue_.pop_back();
                hasNext = true;
            }
        }

        if (!hasNext) {
            break;
        }

        if (next.type == IconType::Online && onlineIcon_) {
            continue;
        }
        if (next.type == IconType::FriendRequest && friendRequestIcon_) {
            continue;
        }
        if (next.type == IconType::Discord && discordIcon_) {
            continue;
        }
        if (next.type == IconType::GitHub && githubIcon_) {
            continue;
        }
        if (next.type == IconType::Heart && heartIcon_) {
            continue;
        }
        if (next.type == IconType::NationSandy && nationSandyIcon_) {
            continue;
        }
        if (next.type == IconType::NationBastok && nationBastokIcon_) {
            continue;
        }
        if (next.type == IconType::NationWindurst && nationWindurstIcon_) {
            continue;
        }
        if (next.type == IconType::NationJeuno && nationJeunoIcon_) {
            continue;
        }
        if (next.type == IconType::Lock && lockIcon_) {
            continue;
        }
        if (next.type == IconType::Unlock && unlockIcon_) {
            continue;
        }

        IconHandle tex = createTextureFromBgra(next.width, next.height, next.bgra);
        if (!tex) {
            continue;
        }

        if (next.type == IconType::Online) {
            onlineIcon_ = tex;
            if (!offlineIcon_) {
                const std::vector<uint8_t> offlineBgra = toGrayscaleBgra(next.bgra);
                offlineIcon_ = createTextureFromBgra(next.width, next.height, offlineBgra);
            }
        } else if (next.type == IconType::FriendRequest) {
            friendRequestIcon_ = tex;
            // Aliases
            if (!pendingIcon_) {
                pendingIcon_ = friendRequestIcon_;
            }
        } else if (next.type == IconType::Discord) {
            discordIcon_ = tex;
        } else if (next.type == IconType::GitHub) {
            githubIcon_ = tex;
        } else if (next.type == IconType::Heart) {
            heartIcon_ = tex;
        } else if (next.type == IconType::NationSandy) {
            nationSandyIcon_ = tex;
        } else if (next.type == IconType::NationBastok) {
            nationBastokIcon_ = tex;
        } else if (next.type == IconType::NationWindurst) {
            nationWindurstIcon_ = tex;
        } else if (next.type == IconType::NationJeuno) {
            nationJeunoIcon_ = tex;
        } else if (next.type == IconType::Lock) {
            lockIcon_ = tex;
        } else if (next.type == IconType::Unlock) {
            unlockIcon_ = tex;
        }

        created++;
    }
#else
    (void)maxIconsToCreate;
#endif
}

bool IconManager::isIconAvailable(IconType type) {
    return getIcon(type) != nullptr;
}

void IconManager::release() {
#ifndef BUILDING_TESTS
    shutdownRequested_.store(true);
    if (decodeThread_.joinable()) {
        decodeThread_.join();
    }
    decodeThreadStarted_.store(false);
    {
        std::lock_guard<std::mutex> lock(decodedMutex_);
        decodedQueue_.clear();
    }
#endif

#ifndef BUILDING_TESTS
    // Release D3D8 textures
    if (offlineIcon_ == onlineIcon_) {
        // Defensive: avoid double-release if offline was aliased to online by older builds.
        offlineIcon_ = nullptr;
    }
    releaseTexture(onlineIcon_);
    releaseTexture(offlineIcon_);
    releaseTexture(friendRequestIcon_);
    releaseTexture(pendingIcon_);
    releaseTexture(discordIcon_);
    releaseTexture(githubIcon_);
    releaseTexture(heartIcon_);
    releaseTexture(nationSandyIcon_);
    releaseTexture(nationBastokIcon_);
    releaseTexture(nationWindurstIcon_);
    releaseTexture(nationJeunoIcon_);
    releaseTexture(lockIcon_);
    releaseTexture(unlockIcon_);
#endif
    
    device_ = nullptr;
    initialized_ = false;
}

IconHandle IconManager::loadIconFromMemory(const unsigned char* data, int dataSize, IconType type) {
#ifndef BUILDING_TESTS
    if (!device_ || !data || dataSize <= 0) {
        return nullptr;
    }
    
    int width = 0, height = 0, channels = 0;
    unsigned char* imageData = stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);
    
    if (!imageData || width <= 0 || height <= 0) {
        if (imageData) {
            stbi_image_free(imageData);
        }
        return nullptr;
    }
    
    // Note: D3D8 types are available through Ashita.h
    IDirect3DTexture8* texture = nullptr;
    HRESULT hr = device_->CreateTexture(
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,  // Mip levels
        0,  // Usage
        D3DFMT_A8R8G8B8,  // Format (32-bit ARGB)
        D3DPOOL_MANAGED,
        &texture
    );
    
    if (FAILED(hr) || !texture) {
        stbi_image_free(imageData);
        return nullptr;
    }
    
    // Lock texture and copy pixel data
    D3DLOCKED_RECT lockedRect;
    hr = texture->LockRect(0, &lockedRect, nullptr, 0);
    if (FAILED(hr)) {
        texture->Release();
        stbi_image_free(imageData);
        return nullptr;
    }
    
    // Copy pixel data (stb_image returns RGBA, D3D8 expects BGRA in memory)
    // D3DFMT_A8R8G8B8 stores bytes in BGRA order (little-endian)
    unsigned char* dest = static_cast<unsigned char*>(lockedRect.pBits);
    const unsigned char* src = imageData;
    for (int y = 0; y < height; ++y) {
        unsigned char* destRow = dest + (y * lockedRect.Pitch);
        const unsigned char* srcRow = src + (y * width * 4);
        
        for (int x = 0; x < width; ++x) {
            const int pixelOffset = x * 4;
            
            // Convert RGBA to BGRA (matches legacy TextureLoader.cpp)
            destRow[pixelOffset + 0] = srcRow[pixelOffset + 2];  // B
            destRow[pixelOffset + 1] = srcRow[pixelOffset + 1];  // G
            destRow[pixelOffset + 2] = srcRow[pixelOffset + 0];  // R
            destRow[pixelOffset + 3] = srcRow[pixelOffset + 3];  // A
        }
    }
    
    texture->UnlockRect(0);
    stbi_image_free(imageData);
    
    // Register texture with ImGui and return ImTextureID
    // For D3D8, ImTextureID is the texture pointer cast to void*
    // ImGui will use this directly when rendering
    return reinterpret_cast<IconHandle>(texture);
#else
    // In tests, return placeholder
    (void)type;
    return reinterpret_cast<IconHandle>(0x1);
#endif
}

void IconManager::decodeIconToQueue(const unsigned char* data, int dataSize, IconType type) {
#ifndef BUILDING_TESTS
    if (!data || dataSize <= 0) {
        return;
    }
    if (shutdownRequested_.load()) {
        return;
    }

    if (type == IconType::Online) {
        PERF_SCOPE("IconManager::decodeIconToQueue Online");
    } else if (type == IconType::FriendRequest) {
        PERF_SCOPE("IconManager::decodeIconToQueue FriendRequest");
    } else {
        PERF_SCOPE("IconManager::decodeIconToQueue");
    }

    int width = 0, height = 0, channels = 0;
    unsigned char* imageData = stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);
    if (!imageData || width <= 0 || height <= 0) {
        if (imageData) {
            stbi_image_free(imageData);
        }
        return;
    }

    // Convert to BGRA once (off-thread) so the main thread can memcpy into the texture.
    std::vector<uint8_t> bgra;
    bgra.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    const uint8_t* src = imageData;
    uint8_t* dst = bgra.data();

    // RGBA -> BGRA
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    for (size_t i = 0; i < pixelCount; ++i) {
        const size_t o = i * 4;
        dst[o + 0] = src[o + 2];
        dst[o + 1] = src[o + 1];
        dst[o + 2] = src[o + 0];
        dst[o + 3] = src[o + 3];
    }

    stbi_image_free(imageData);

    if (shutdownRequested_.load()) {
        return;
    }

    DecodedIcon decoded;
    decoded.type = type;
    decoded.width = width;
    decoded.height = height;
    decoded.bgra = std::move(bgra);

    {
        std::lock_guard<std::mutex> lock(decodedMutex_);
        decodedQueue_.push_back(std::move(decoded));
    }
#else
    (void)data;
    (void)dataSize;
    (void)type;
#endif
}

IconHandle IconManager::createTextureFromBgra(int width, int height, const std::vector<uint8_t>& bgra) {
#ifndef BUILDING_TESTS
    if (!device_ || width <= 0 || height <= 0) {
        return nullptr;
    }
    if (bgra.size() < static_cast<size_t>(width) * static_cast<size_t>(height) * 4) {
        return nullptr;
    }

    IDirect3DTexture8* texture = nullptr;
    HRESULT hr = device_->CreateTexture(
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,  // Mip levels
        0,  // Usage
        D3DFMT_A8R8G8B8,  // Format (32-bit ARGB)
        D3DPOOL_MANAGED,
        &texture
    );

    if (FAILED(hr) || !texture) {
        return nullptr;
    }

    D3DLOCKED_RECT lockedRect;
    hr = texture->LockRect(0, &lockedRect, nullptr, 0);
    if (FAILED(hr)) {
        texture->Release();
        return nullptr;
    }

    const uint8_t* src = bgra.data();
    uint8_t* dest = static_cast<uint8_t*>(lockedRect.pBits);

    const size_t rowBytes = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        std::memcpy(dest + (static_cast<size_t>(y) * lockedRect.Pitch),
                    src + (static_cast<size_t>(y) * rowBytes),
                    rowBytes);
    }

    texture->UnlockRect(0);

    return reinterpret_cast<IconHandle>(texture);
#else
    (void)width;
    (void)height;
    (void)bgra;
    return reinterpret_cast<IconHandle>(0x1);
#endif
}

void IconManager::releaseTexture(IconHandle& handle) {
#ifndef BUILDING_TESTS
    if (handle) {
        IDirect3DTexture8* texture = reinterpret_cast<IDirect3DTexture8*>(handle);
        if (texture) {
            texture->Release();
        }
        handle = nullptr;
    }
#else
    (void)handle;
#endif
}

::XIFriendList::Core::MemoryStats IconManager::getMemoryStats() const {
    size_t iconBytes = 0;
    
    std::lock_guard<std::mutex> lock(decodedMutex_);
    
    for (const auto& decoded : decodedQueue_) {
        iconBytes += sizeof(DecodedIcon);
        iconBytes += decoded.bgra.size();
    }
    iconBytes += decodedQueue_.capacity() * sizeof(DecodedIcon);
    
    constexpr size_t ESTIMATED_TEXTURE_SIZE = 16 * 16 * 4;
    size_t loadedIconCount = 0;
    if (onlineIcon_) ++loadedIconCount;
    if (offlineIcon_) ++loadedIconCount;
    if (friendRequestIcon_) ++loadedIconCount;
    if (pendingIcon_) ++loadedIconCount;
    if (discordIcon_) ++loadedIconCount;
    if (githubIcon_) ++loadedIconCount;
    if (heartIcon_) ++loadedIconCount;
    if (nationSandyIcon_) ++loadedIconCount;
    if (nationBastokIcon_) ++loadedIconCount;
    if (nationWindurstIcon_) ++loadedIconCount;
    if (nationJeunoIcon_) ++loadedIconCount;
    
    iconBytes += loadedIconCount * ESTIMATED_TEXTURE_SIZE;
    
    return ::XIFriendList::Core::MemoryStats(loadedIconCount + decodedQueue_.size(), iconBytes, "Icons/Textures");
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

