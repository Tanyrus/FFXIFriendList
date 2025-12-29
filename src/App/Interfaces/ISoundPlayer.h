
#ifndef APP_ISOUND_PLAYER_H
#define APP_ISOUND_PLAYER_H

#include <cstdint>
#include <span>
#include <filesystem>

namespace XIFriendList {
namespace App {

class ISoundPlayer {
public:
    virtual ~ISoundPlayer() = default;
    virtual bool playWavBytes(std::span<const uint8_t> data, float volume) = 0;
    virtual bool playWavFile(const std::filesystem::path& path, float volume) = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_ISOUND_PLAYER_H

