
#include "SoundResolver.h"
#include "Platform/Ashita/EmbeddedResources.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace XIFriendList {
namespace App {

SoundResolver::SoundResolver(const std::filesystem::path& configDir)
    : configDir_(configDir)
{
    initializeSoundsDirectory();
}

void SoundResolver::initializeSoundsDirectory() const {
    std::filesystem::path soundsDir = configDir_ / "sounds";
    
    try {
        std::filesystem::create_directories(soundsDir);
        
        std::filesystem::path readmePath = soundsDir / "README.txt";
        if (!std::filesystem::exists(readmePath)) {
            std::ofstream readme(readmePath);
            if (readme.is_open()) {
                readme << "XIFriendList Custom Notification Sounds\n";
                readme << "========================================\n\n";
                readme << "This folder allows you to customize notification sounds used by the plugin.\n";
                readme << "Place your custom WAV files here to override the default embedded sounds.\n\n";
                readme << "Available Sounds:\n";
                readme << "------------------\n\n";
                readme << "1. friend-request.wav\n";
                readme << "   - Plays when you receive a friend request\n";
                readme << "   - Sound key: \"friend-request\"\n\n";
                readme << "2. online.wav\n";
                readme << "   - Plays when a friend comes online\n";
                readme << "   - Sound key: \"online\"\n\n";
                readme << "How to Use:\n";
                readme << "-----------\n";
                readme << "1. Create or obtain a WAV format sound file\n";
                readme << "2. Name it exactly as shown above (e.g., \"friend-request.wav\")\n";
                readme << "3. Place it in this folder\n";
                readme << "4. The plugin will automatically use your custom sound\n";
                readme << "5. If the file doesn't exist, the default embedded sound will be used\n\n";
                readme << "Requirements:\n";
                readme << "-------------\n";
                readme << "- File format: WAV (Windows WAV format)\n";
                readme << "- File names are case-sensitive\n";
                readme << "- Files are checked each time a sound is played (no restart needed)\n\n";
                readme << "Note: You can delete this README file if you don't need it.\n";
                readme.close();
            }
        }
    } catch (const std::exception&) {
    }
}

std::optional<SoundResolution> SoundResolver::resolve(const std::string& soundKey) const {
    auto overridePath = getUserOverridePath(soundKey);
    if (overridePath && std::filesystem::exists(*overridePath)) {
        SoundResolution result(SoundResolution::Source::File);
        result.filePath = *overridePath;
        return result;
    }
    
    auto embeddedData = getEmbeddedSound(soundKey);
    if (embeddedData) {
        SoundResolution result(SoundResolution::Source::Embedded);
        result.embeddedData = *embeddedData;
        return result;
    }
    
    return std::nullopt;
}

std::optional<std::filesystem::path> SoundResolver::getUserOverridePath(const std::string& soundKey) const {
    std::filesystem::path overridePath = configDir_;
    overridePath /= "sounds";
    overridePath /= soundKey + ".wav";
    
    return overridePath;
}

std::optional<std::span<const uint8_t>> SoundResolver::getEmbeddedSound(const std::string& soundKey) const {
    if (soundKey == "online") {
        return std::span<const uint8_t>(g_OnlineSoundData, g_OnlineSoundData_size);
    } else if (soundKey == "friend-request") {
        return std::span<const uint8_t>(g_FriendRequestSoundData, g_FriendRequestSoundData_size);
    }
    
    return std::nullopt;
}

} // namespace App
} // namespace XIFriendList

