#include "AshitaSoundPlayer.h"
#include <windows.h>
#include <mmsystem.h>
#include <chrono>

#pragma comment(lib, "winmm.lib")

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaSoundPlayer::AshitaSoundPlayer(::XIFriendList::App::ILogger& logger)
    : logger_(logger)
{
}

bool AshitaSoundPlayer::playWavBytes(std::span<const uint8_t> data, float volume) {
    if (data.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t previousVolume = setGlobalVolume(volume);
    
    bool success = PlaySoundW(
        reinterpret_cast<LPCWSTR>(data.data()),
        nullptr,
        SND_ASYNC | SND_MEMORY | SND_NODEFAULT
    ) != FALSE;
    
    restoreGlobalVolume(previousVolume);
    
    if (!success) {
        DWORD error = GetLastError();
        if (error != 0) {
            logError("playWavBytes", "PlaySoundW failed with error: " + std::to_string(error));
        }
    }
    
    return success;
}

bool AshitaSoundPlayer::playWavFile(const std::filesystem::path& path, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::wstring widePath = path.wstring();
    
    uint32_t previousVolume = setGlobalVolume(volume);
    
    bool success = PlaySoundW(
        widePath.c_str(),
        nullptr,
        SND_ASYNC | SND_FILENAME | SND_NODEFAULT
    ) != FALSE;
    
    restoreGlobalVolume(previousVolume);
    
    if (!success) {
        DWORD error = GetLastError();
        if (error != 0) {
            logError("playWavFile", "PlaySoundW failed with error: " + std::to_string(error) + " for file: " + path.string());
        }
    }
    
    return success;
}

uint32_t AshitaSoundPlayer::setGlobalVolume(float volume) const {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    DWORD currentVolume = 0;
    HWAVEOUT hWaveOut = nullptr;
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
        waveOutGetVolume(hWaveOut, &currentVolume);
        waveOutClose(hWaveOut);
    }
    
    // Calculate new volume (Windows volume is 16-bit left + 16-bit right)
    // Volume 0.0 = 0x0000, Volume 1.0 = 0xFFFF for each channel
    uint16_t volumeLevel = static_cast<uint16_t>(volume * 0xFFFF);
    uint32_t newVolume = (volumeLevel << 16) | volumeLevel;  // Same volume for left and right
    
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
        waveOutSetVolume(hWaveOut, newVolume);
        waveOutClose(hWaveOut);
    }
    
    return currentVolume;
}

void AshitaSoundPlayer::restoreGlobalVolume(uint32_t previousVolume) const {
    HWAVEOUT hWaveOut = nullptr;
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
        waveOutSetVolume(hWaveOut, previousVolume);
        waveOutClose(hWaveOut);
    }
}

void AshitaSoundPlayer::logError(const std::string& context, const std::string& error) const {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (now - lastErrorLogTime_ >= ERROR_LOG_COOLDOWN_MS) {
        logger_.warning("[AshitaSoundPlayer] " + context + ": " + error);
        lastErrorLogTime_ = now;
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

