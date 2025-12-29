
#ifndef CORE_UTILITIES_CORE_H
#define CORE_UTILITIES_CORE_H

#include <string>
#include <cstdint>
#include <unordered_map>

namespace XIFriendList {
namespace Core {

struct ValidationResult {
    bool valid;
    std::string error;
    std::string sanitized;
    
    ValidationResult() : valid(false), error(""), sanitized("") {}
    ValidationResult(bool v, const std::string& e, const std::string& s) 
        : valid(v), error(e), sanitized(s) {}
};

namespace Limits {
    constexpr size_t CHARACTER_NAME_MAX = 16;
    constexpr size_t FRIEND_NAME_MAX = 16;
    constexpr size_t NOTE_MAX = 8192;
    constexpr size_t MAIL_SUBJECT_MAX = 100;
    constexpr size_t MAIL_BODY_MAX = 2000;
    constexpr size_t ZONE_MAX = 100;
    constexpr size_t JOB_MAX = 50;
    constexpr size_t RANK_MAX = 50;
}

class Sanitize {
public:
    static std::string removeControlChars(const std::string& str, bool allowNewlines = false);
    static std::string trim(const std::string& str);
    static std::string collapseWhitespace(const std::string& str);
    static std::string sanitizeForLogging(const std::string& str);
    static ValidationResult validateCharacterName(const std::string& name, size_t maxLength = Limits::CHARACTER_NAME_MAX);
    static ValidationResult validateFriendName(const std::string& name, size_t maxLength = Limits::FRIEND_NAME_MAX);
    static ValidationResult validateNoteText(const std::string& noteText, size_t maxLength = Limits::NOTE_MAX);
    static ValidationResult validateMailSubject(const std::string& subject, size_t maxLength = Limits::MAIL_SUBJECT_MAX);
    static ValidationResult validateMailBody(const std::string& body, size_t maxLength = Limits::MAIL_BODY_MAX);
    static ValidationResult validateZone(const std::string& zone, size_t maxLength = Limits::ZONE_MAX);
    static ValidationResult validateJob(const std::string& job, size_t maxLength = Limits::JOB_MAX);
    static ValidationResult validateRank(const std::string& rank, size_t maxLength = Limits::RANK_MAX);
    static std::string normalizeNameTitleCase(const std::string& name);
    static bool isValidCharacterNameChar(char c);
    
private:
    static bool isValidZoneChar(char c);
    static bool isValidJobRankChar(char c);
};

enum class NotificationSoundType {
    FriendOnline,
    FriendRequest,
    Unknown
};

class NotificationSoundPolicy {
public:
    NotificationSoundPolicy();
    bool shouldPlay(NotificationSoundType type, uint64_t currentTimeMs);
    void reset();
    uint32_t getSuppressedCount(NotificationSoundType type) const;

private:
    struct SoundState {
        uint64_t lastPlayTimeMs;
        uint32_t suppressedCount;
        
        SoundState() : lastPlayTimeMs(0), suppressedCount(0) {}
    };
    
    std::unordered_map<NotificationSoundType, SoundState> soundStates_;
    
    static constexpr uint64_t COOLDOWN_FRIEND_ONLINE_MS = 10000;
    static constexpr uint64_t COOLDOWN_FRIEND_REQUEST_MS = 5000;
    
    uint64_t getCooldownMs(NotificationSoundType type) const;
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_UTILITIES_CORE_H

