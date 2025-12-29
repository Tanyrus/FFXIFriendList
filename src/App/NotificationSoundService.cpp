#include "App/NotificationSoundService.h"
#include "UI/Notifications/Notification.h"

namespace XIFriendList {
namespace App {

NotificationSoundService::NotificationSoundService(ISoundPlayer& soundPlayer,
                                                   IClock& clock,
                                                   ILogger& logger,
                                                   const std::filesystem::path& configDir)
    : soundPlayer_(soundPlayer)
    , clock_(clock)
    , logger_(logger)
    , resolver_(configDir)
{
}

void NotificationSoundService::maybePlaySound(const XIFriendList::UI::Notification& notification, const XIFriendList::Core::Preferences& prefs) {
    if (!prefs.notificationSoundsEnabled) {
        return;
    }
    
    auto soundType = getSoundType(notification);
    
    if (soundType == XIFriendList::Core::NotificationSoundType::Unknown) {
        return;
    }
    
    if (soundType == XIFriendList::Core::NotificationSoundType::FriendOnline && !prefs.soundOnFriendOnline) {
        return;
    }
    if (soundType == XIFriendList::Core::NotificationSoundType::FriendRequest && !prefs.soundOnFriendRequest) {
        return;
    }
    
    uint64_t currentTime = clock_.nowMs();
    if (!policy_.shouldPlay(soundType, currentTime)) {
        return;
    }
    
    std::string soundKey = getSoundKey(soundType);
    auto resolution = resolver_.resolve(soundKey);
    if (!resolution) {
        logger_.warning("Notification sound not found: " + soundKey);
        return;
    }
    
    float volume = prefs.notificationSoundVolume;
    bool success = false;
    
    if (resolution->source == SoundResolution::Source::File) {
        success = soundPlayer_.playWavFile(resolution->filePath, volume);
    } else {
        success = soundPlayer_.playWavBytes(resolution->embeddedData, volume);
    }
    
    if (!success) {
        logger_.warning("Failed to play notification sound: " + soundKey);
    }
}

XIFriendList::Core::NotificationSoundType NotificationSoundService::getSoundType(const XIFriendList::UI::Notification& notification) const {
    const std::string& message = notification.message;
    std::string lowerMessage = message;
    std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    if (lowerMessage.find("come online") != std::string::npos || 
        lowerMessage.find("has come online") != std::string::npos) {
        return XIFriendList::Core::NotificationSoundType::FriendOnline;
    }
    
    if (lowerMessage.find("friend request") != std::string::npos ||
        (lowerMessage.find("accept") != std::string::npos && lowerMessage.find("friend request") != std::string::npos)) {
        return XIFriendList::Core::NotificationSoundType::FriendRequest;
    }
    
    return XIFriendList::Core::NotificationSoundType::Unknown;
}

std::string NotificationSoundService::getSoundKey(XIFriendList::Core::NotificationSoundType soundType) const {
    switch (soundType) {
        case XIFriendList::Core::NotificationSoundType::FriendOnline:
            return "online";
        case XIFriendList::Core::NotificationSoundType::FriendRequest:
            return "friend-request";
        case XIFriendList::Core::NotificationSoundType::Unknown:
        default:
            return "";
    }
}

} // namespace App
} // namespace XIFriendList

