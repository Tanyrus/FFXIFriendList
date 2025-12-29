
#ifndef APP_PREFERENCES_USE_CASES_H
#define APP_PREFERENCES_USE_CASES_H

#include "App/Interfaces/IPreferencesStore.h"
#include "App/Interfaces/INetClient.h"
#include "App/Interfaces/ILogger.h"
#include "App/Interfaces/IClock.h"
#include "Core/ModelsCore.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include <string>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct PreferencesResult {
    bool success;
    std::string error;
    
    PreferencesResult() : success(false) {}
    PreferencesResult(bool success, const std::string& error = "") : success(success), error(error) {}
};

class PreferencesUseCase {
public:
    PreferencesUseCase(INetClient& netClient,
                      IClock& clock,
                      ILogger& logger,
                      IPreferencesStore* preferencesStore);
    
    XIFriendList::Core::Preferences getPreferences() const;
    XIFriendList::Core::Preferences getServerPreferences() const { return serverPreferences_; }
    XIFriendList::Core::Preferences getLocalPreferences() const { return localPreferences_; }
    PreferencesResult updateServerPreference(const std::string& field, bool value,
                                            const std::string& apiKey = "",
                                            const std::string& characterName = "");
    PreferencesResult updateServerPreference(const std::string& field, const std::string& value,
                                            const std::string& apiKey = "",
                                            const std::string& characterName = "");
    PreferencesResult updateLocalPreference(const std::string& field, bool value,
                                            const std::string& apiKey = "",
                                            const std::string& characterName = "");
    PreferencesResult updateLocalPreference(const std::string& field, float value,
                                           const std::string& apiKey = "",
                                           const std::string& characterName = "");
    PreferencesResult updateServerPreferences(const XIFriendList::Core::Preferences& prefs, 
                                             const std::string& apiKey,
                                             const std::string& characterName);
    PreferencesResult updateLocalPreferences(const XIFriendList::Core::Preferences& prefs);
    PreferencesResult resetPreferences();
    void loadPreferences(const std::string& apiKey = "", 
                        const std::string& characterName = "");
    void savePreferences(const std::string& apiKey = "", 
                        const std::string& characterName = "");
    PreferencesResult syncFromServer(const std::string& apiKey, const std::string& characterName);
    bool isLoaded() const { return loaded_; }

private:
    INetClient& netClient_;
    IClock& clock_;
    ILogger& logger_;
    IPreferencesStore* preferencesStore_;
    XIFriendList::Core::Preferences serverPreferences_;
    XIFriendList::Core::Preferences localPreferences_;
    bool loaded_;
    
    XIFriendList::Core::Preferences mergePreferences() const;
    bool syncServerPreferencesToServer(const XIFriendList::Core::Preferences& prefs,
                                      const std::string& apiKey,
                                      const std::string& characterName);
    bool loadServerPreferencesFromServer(const std::string& apiKey,
                                        const std::string& characterName);
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_PREFERENCES_USE_CASES_H

