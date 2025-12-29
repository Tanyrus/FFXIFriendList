
#ifndef APP_SOUND_RESOLVER_H
#define APP_SOUND_RESOLVER_H

#include <string>
#include <optional>
#include <span>
#include <cstdint>
#include <filesystem>

namespace XIFriendList {
namespace App {

struct SoundResolution {
    enum class Source {
        Embedded,
        File
    };
    
    Source source;
    std::span<const uint8_t> embeddedData;
    std::filesystem::path filePath;
    
    SoundResolution(Source s) : source(s) {}
};

class SoundResolver {
public:
    SoundResolver(const std::filesystem::path& configDir);
    std::optional<SoundResolution> resolve(const std::string& soundKey) const;

private:
    std::filesystem::path configDir_;
    void initializeSoundsDirectory() const;
    std::optional<std::span<const uint8_t>> getEmbeddedSound(const std::string& soundKey) const;
    std::optional<std::filesystem::path> getUserOverridePath(const std::string& soundKey) const;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_SOUND_RESOLVER_H

