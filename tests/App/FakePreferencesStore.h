#ifndef APP_FAKE_PREFERENCES_STORE_H
#define APP_FAKE_PREFERENCES_STORE_H

#include "App/Interfaces/IPreferencesStore.h"
#include "Core/ModelsCore.h"

namespace XIFriendList {
namespace App {

class FakePreferencesStore : public IPreferencesStore {
public:
    FakePreferencesStore() {}
    
    Core::Preferences loadServerPreferences() override {
        return serverPreferences_;
    }
    
    bool saveServerPreferences(const Core::Preferences& prefs) override {
        serverPreferences_ = prefs;
        return true;
    }
    
    Core::Preferences loadLocalPreferences() override {
        return localPreferences_;
    }
    
    bool saveLocalPreferences(const Core::Preferences& prefs) override {
        localPreferences_ = prefs;
        return true;
    }
    
    Core::Preferences serverPreferences_;
    Core::Preferences localPreferences_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_FAKE_PREFERENCES_STORE_H

