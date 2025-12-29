
#include "UtilitiesCore.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace XIFriendList {
namespace Core {

bool Sanitize::isValidCharacterNameChar(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || 
           uc == 0x20 ||
           uc == 0x2D ||
           uc == 0x5F ||
           uc == 0x27;
}

bool Sanitize::isValidZoneChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || 
           c == ' ' || c == '-' || c == '_' || c == '\'' || c == '.';
}

bool Sanitize::isValidJobRankChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || 
           c == ' ' || c == '-' || c == '_' || c == '\'';
}

std::string Sanitize::removeControlChars(const std::string& str, bool allowNewlines) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        unsigned char code = static_cast<unsigned char>(c);
        
        if (code >= 0x20 || code == 0x09) {
            result += c;
        }
        else if (allowNewlines && (code == 0x0A || code == 0x0D)) {
            result += c;
        }
    }
    
    return result;
}

std::string Sanitize::trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

std::string Sanitize::collapseWhitespace(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    bool lastWasSpace = false;
    
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    
    return result;
}

std::string Sanitize::sanitizeForLogging(const std::string& str) {
    std::string result;
    result.reserve(str.length() + 10); // Reserve extra for escape sequences
    
    for (char c : str) {
        if (c == '\n') {
            result += "\\n";
        } else if (c == '\r') {
            result += "\\n";
        } else if (c == '\t') {
            result += "\\t";
        } else if (static_cast<unsigned char>(c) < 0x20) {
            continue;
        } else {
            result += c;
        }
    }
    
    return result;
}

ValidationResult Sanitize::validateCharacterName(const std::string& name, size_t maxLength) {
    if (name.empty()) {
        return ValidationResult(false, "Character name is required", "");
    }
    
    std::string sanitized = removeControlChars(name, false);
    sanitized = trim(sanitized);
    
    if (sanitized.empty()) {
        return ValidationResult(false, "Character name cannot be empty", "");
    }
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Character name must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    for (char c : sanitized) {
        if (!isValidCharacterNameChar(c)) {
            return ValidationResult(false, 
                "Character name contains invalid characters. Only letters, numbers, spaces, hyphens, underscores, and apostrophes are allowed.", "");
        }
    }
    
    std::string lower;
    lower.reserve(sanitized.length());
    for (char c : sanitized) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    return ValidationResult(true, "", lower);
}

ValidationResult Sanitize::validateFriendName(const std::string& name, size_t maxLength) {
    return validateCharacterName(name, maxLength);
}

ValidationResult Sanitize::validateNoteText(const std::string& noteText, size_t maxLength) {
    if (noteText.empty()) {
        return ValidationResult(false, "Note text is required", "");
    }
    
    std::string sanitized = removeControlChars(noteText, true);
    sanitized = trim(sanitized);
    
    if (sanitized.empty()) {
        return ValidationResult(false, "Note text cannot be empty or whitespace-only", "");
    }
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Note text must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    return ValidationResult(true, "", sanitized);
}

ValidationResult Sanitize::validateMailSubject(const std::string& subject, size_t maxLength) {
    if (subject.empty()) {
        return ValidationResult(false, "Mail subject is required", "");
    }
    
    std::string temp;
    temp.reserve(subject.length());
    for (char c : subject) {
        unsigned char code = static_cast<unsigned char>(c);
        if (code == 0x0A || code == 0x0D) {
            temp += ' ';
        } else if (code >= 0x20 || code == 0x09) {
            temp += c;
        }
    }
    
    std::string sanitized = collapseWhitespace(trim(temp));
    
    if (sanitized.empty()) {
        return ValidationResult(false, "Mail subject cannot be empty", "");
    }
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Mail subject must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    return ValidationResult(true, "", sanitized);
}

ValidationResult Sanitize::validateMailBody(const std::string& body, size_t maxLength) {
    if (body.empty()) {
        return ValidationResult(false, "Mail body is required", "");
    }
    
    std::string sanitized = removeControlChars(body, true);
    sanitized = trim(sanitized);
    
    if (sanitized.empty()) {
        return ValidationResult(false, "Mail body cannot be empty", "");
    }
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Mail body must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    return ValidationResult(true, "", sanitized);
}

ValidationResult Sanitize::validateZone(const std::string& zone, size_t maxLength) {
    if (zone.empty()) {
        return ValidationResult(true, "", "");
    }
    
    std::string sanitized = removeControlChars(zone, false);
    sanitized = collapseWhitespace(trim(sanitized));
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Zone must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    if (!sanitized.empty()) {
        for (char c : sanitized) {
            if (!isValidZoneChar(c)) {
                return ValidationResult(false, "Zone contains invalid characters", "");
            }
        }
    }
    
    return ValidationResult(true, "", sanitized);
}

ValidationResult Sanitize::validateJob(const std::string& job, size_t maxLength) {
    if (job.empty()) {
        return ValidationResult(true, "", "");
    }
    
    std::string sanitized = removeControlChars(job, false);
    sanitized = collapseWhitespace(trim(sanitized));
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Job must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    if (!sanitized.empty()) {
        for (char c : sanitized) {
            if (!isValidJobRankChar(c)) {
                return ValidationResult(false, "Job contains invalid characters", "");
            }
        }
    }
    
    return ValidationResult(true, "", sanitized);
}

ValidationResult Sanitize::validateRank(const std::string& rank, size_t maxLength) {
    if (rank.empty()) {
        return ValidationResult(true, "", "");
    }
    
    std::string sanitized = removeControlChars(rank, false);
    sanitized = collapseWhitespace(trim(sanitized));
    
    if (sanitized.length() > maxLength) {
        return ValidationResult(false, 
            "Rank must be " + std::to_string(maxLength) + " characters or less", "");
    }
    
    if (!sanitized.empty()) {
        for (char c : sanitized) {
            if (!isValidJobRankChar(c)) {
                return ValidationResult(false, "Rank contains invalid characters", "");
            }
        }
    }
    
    return ValidationResult(true, "", sanitized);
}

std::string Sanitize::normalizeNameTitleCase(const std::string& name) {
    if (name.empty()) {
        return name;
    }
    
    std::string result;
    result.reserve(name.length());
    bool capitalizeNext = true;
    
    for (char c : name) {
        if (std::isspace(static_cast<unsigned char>(c)) || 
            c == '-' || c == '_' || c == '\'') {
            result += c;
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalizeNext = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    
    return result;
}

NotificationSoundPolicy::NotificationSoundPolicy() {
    soundStates_[NotificationSoundType::FriendOnline] = SoundState();
    soundStates_[NotificationSoundType::FriendRequest] = SoundState();
}

bool NotificationSoundPolicy::shouldPlay(NotificationSoundType type, uint64_t currentTimeMs) {
    auto& state = soundStates_[type];
    uint64_t cooldown = getCooldownMs(type);
    
    if (state.lastPlayTimeMs == 0 || currentTimeMs - state.lastPlayTimeMs >= cooldown) {
        state.lastPlayTimeMs = currentTimeMs;
        return true;
    }
    
    state.suppressedCount++;
    return false;
}

void NotificationSoundPolicy::reset() {
    for (auto& pair : soundStates_) {
        pair.second.lastPlayTimeMs = 0;
        pair.second.suppressedCount = 0;
    }
}

uint32_t NotificationSoundPolicy::getSuppressedCount(NotificationSoundType type) const {
    auto it = soundStates_.find(type);
    if (it != soundStates_.end()) {
        return it->second.suppressedCount;
    }
    return 0;
}

uint64_t NotificationSoundPolicy::getCooldownMs(NotificationSoundType type) const {
    switch (type) {
        case NotificationSoundType::FriendOnline:
            return COOLDOWN_FRIEND_ONLINE_MS;
        case NotificationSoundType::FriendRequest:
            return COOLDOWN_FRIEND_REQUEST_MS;
        case NotificationSoundType::Unknown:
        default:
            return 0;
    }
}

} // namespace Core
} // namespace XIFriendList

