#ifndef PLATFORM_ASHITA_CACHE_MIGRATION_H
#define PLATFORM_ASHITA_CACHE_MIGRATION_H

#include <string>
#include <filesystem>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class CacheMigration {
public:
    static bool migrateCacheAndIniToJson(const std::string& jsonPath, const std::string& cacheJsonPath, const std::string& iniPath, const std::string& settingsJsonPath);
    static bool hasMigrationCompleted(const std::string& jsonPath);
    static void markMigrationCompleted(const std::string& jsonPath);
    static bool migrateConfigDirectory(const std::filesystem::path& oldConfigDir, const std::filesystem::path& newConfigDir);

private:
    static bool writeIniValue(const std::string& filePath, const std::string& section, const std::string& key, const std::string& value);
    static std::string readIniValue(const std::string& filePath, const std::string& section, const std::string& key);
    static void ensureConfigDirectory(const std::string& filePath);
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_CACHE_MIGRATION_H

