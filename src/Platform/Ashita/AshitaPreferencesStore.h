
#ifndef PLATFORM_ASHITA_PREFERENCES_STORE_H
#define PLATFORM_ASHITA_PREFERENCES_STORE_H

#include "App/Interfaces/IPreferencesStore.h"
#include "Core/ModelsCore.h"
#include <map>
#include <string>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaPreferencesStore : public ::XIFriendList::App::IPreferencesStore {
public:
    AshitaPreferencesStore();
    ~AshitaPreferencesStore() = default;
    ::XIFriendList::Core::Preferences loadServerPreferences() override;
    bool saveServerPreferences(const ::XIFriendList::Core::Preferences& prefs) override;
    ::XIFriendList::Core::Preferences loadLocalPreferences() override;
    bool saveLocalPreferences(const ::XIFriendList::Core::Preferences& prefs) override;
    static bool loadWindowLockState(const std::string& windowId);
    static bool saveWindowLockState(const std::string& windowId, bool locked);
    static bool loadCollapsibleSectionState(const std::string& windowId, const std::string& sectionId);
    static bool saveCollapsibleSectionState(const std::string& windowId, const std::string& sectionId, bool expanded);

private:
    static std::map<std::string, bool> windowLockCache_;
    static bool windowLockCacheValid_;
    static void loadWindowLockCache();
    static std::map<std::string, bool> collapsibleSectionCache_;
    static bool collapsibleSectionCacheValid_;
    static void loadCollapsibleSectionCache();
    std::string getSettingsJsonPath() const;
    std::string getCacheJsonPath() const;
    std::string getConfigPath() const;
    void ensureConfigDirectory(const std::string& filePath) const;
    ::XIFriendList::Core::Preferences loadPreferencesFromJson(bool serverPrefs);
    bool savePreferencesToJson(const ::XIFriendList::Core::Preferences& prefs, bool serverPrefs);
    static void trimString(std::string& str);
    static bool parseBoolean(const std::string& value);
    static float parseFloat(const std::string& value);
    int loadCustomCloseKeyCodeFromConfig() const;
    int loadDebugModeFromConfig() const;
    static std::string getCacheJsonPathStatic();
    static void ensureConfigDirectoryStatic(const std::string& filePath);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_PREFERENCES_STORE_H

