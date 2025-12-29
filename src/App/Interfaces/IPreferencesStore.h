
#ifndef APP_I_PREFERENCES_STORE_H
#define APP_I_PREFERENCES_STORE_H

#include "Core/ModelsCore.h"
#include <string>

namespace XIFriendList {
namespace App {

class IPreferencesStore {
public:
    virtual ~IPreferencesStore() = default;
    virtual XIFriendList::Core::Preferences loadServerPreferences() = 0;
    virtual bool saveServerPreferences(const XIFriendList::Core::Preferences& prefs) = 0;
    virtual XIFriendList::Core::Preferences loadLocalPreferences() = 0;
    virtual bool saveLocalPreferences(const XIFriendList::Core::Preferences& prefs) = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_I_PREFERENCES_STORE_H

