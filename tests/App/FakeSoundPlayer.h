#ifndef APP_FAKE_SOUND_PLAYER_H
#define APP_FAKE_SOUND_PLAYER_H

#include "App/Interfaces/ISoundPlayer.h"
#include <vector>
#include <filesystem>

namespace XIFriendList {
namespace App {

class FakeSoundPlayer : public ISoundPlayer {
public:
    FakeSoundPlayer() : playWavBytesCalled_(false), playWavFileCalled_(false) {}
    
    bool playWavBytes(std::span<const uint8_t> data, float volume) override {
        playWavBytesCalled_ = true;
        lastWavBytesData_.assign(data.begin(), data.end());
        lastVolume_ = volume;
        return playWavBytesResult_;
    }
    
    bool playWavFile(const std::filesystem::path& path, float volume) override {
        playWavFileCalled_ = true;
        lastWavFilePath_ = path;
        lastVolume_ = volume;
        return playWavFileResult_;
    }
    
    bool playWavBytesCalled_;
    bool playWavFileCalled_;
    std::vector<uint8_t> lastWavBytesData_;
    std::filesystem::path lastWavFilePath_;
    float lastVolume_;
    bool playWavBytesResult_ = true;
    bool playWavFileResult_ = true;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_FAKE_SOUND_PLAYER_H

