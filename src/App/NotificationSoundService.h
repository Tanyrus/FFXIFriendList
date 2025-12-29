
#ifndef APP_NOTIFICATION_SOUND_SERVICE_H
#define APP_NOTIFICATION_SOUND_SERVICE_H

#include "UI/Notifications/Notification.h"
#include "App/Interfaces/ISoundPlayer.h"
#include "App/Interfaces/IClock.h"
#include "App/Interfaces/ILogger.h"
#include "Core/UtilitiesCore.h"
#include "Core/ModelsCore.h"
#include "App/SoundResolver.h"
#include <memory>

namespace XIFriendList {
namespace App {

class NotificationSoundService {
public:
    NotificationSoundService(ISoundPlayer& soundPlayer,
                           IClock& clock,
                           ILogger& logger,
                           const std::filesystem::path& configDir);
    
    void maybePlaySound(const XIFriendList::UI::Notification& notification, const XIFriendList::Core::Preferences& prefs);
    
    void updatePreferences(const XIFriendList::Core::Preferences& prefs) { prefs_ = prefs; }

private:
    ISoundPlayer& soundPlayer_;
    IClock& clock_;
    ILogger& logger_;
    XIFriendList::Core::NotificationSoundPolicy policy_;
    SoundResolver resolver_;
    XIFriendList::Core::Preferences prefs_;
    
    XIFriendList::Core::NotificationSoundType getSoundType(const XIFriendList::UI::Notification& notification) const;
    
    std::string getSoundKey(XIFriendList::Core::NotificationSoundType soundType) const;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_NOTIFICATION_SOUND_SERVICE_H

