// Platform implementation of ISoundPlayer using Windows PlaySound API

#ifndef PLATFORM_ASHITA_ASHITA_SOUND_PLAYER_H
#define PLATFORM_ASHITA_ASHITA_SOUND_PLAYER_H

#include "App/Interfaces/ISoundPlayer.h"
#include "App/Interfaces/ILogger.h"
#include <string>
#include <mutex>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaSoundPlayer : public ::XIFriendList::App::ISoundPlayer {
public:
    AshitaSoundPlayer(::XIFriendList::App::ILogger& logger);
    ~AshitaSoundPlayer() = default;
    bool playWavBytes(std::span<const uint8_t> data, float volume) override;
    bool playWavFile(const std::filesystem::path& path, float volume) override;

private:
    ::XIFriendList::App::ILogger& logger_;
    mutable std::mutex mutex_;
    bool playWavData(const void* data, size_t size, float volume);
    bool playWavFileInternal(const std::wstring& filePath, float volume);
    uint32_t setGlobalVolume(float volume) const;
    void restoreGlobalVolume(uint32_t previousVolume) const;
    void logError(const std::string& context, const std::string& error) const;
    mutable uint64_t lastErrorLogTime_ = 0;
    static constexpr uint64_t ERROR_LOG_COOLDOWN_MS = 5000;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_ASHITA_SOUND_PLAYER_H

