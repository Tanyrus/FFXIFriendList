#include "Platform/Ashita/AshitaMailStore.h"
#include "Platform/Ashita/PathUtils.h"
#include "Core/MemoryStats.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <windows.h>

#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaMailStore::AshitaMailStore()
    : characterName_("")
    , dirty_(false)
{
    messages_[::XIFriendList::Core::MailFolder::Inbox] = std::map<std::string, ::XIFriendList::Core::MailMessage>();
    messages_[::XIFriendList::Core::MailFolder::Sent] = std::map<std::string, ::XIFriendList::Core::MailMessage>();
}

void AshitaMailStore::setCharacterName(const std::string& characterName) {
    std::string previousCharacterName;
    bool wasDirty = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previousCharacterName = characterName_;
        wasDirty = dirty_;
        characterName_ = normalizeCharacterName(characterName);
    }
    
    if (wasDirty && !previousCharacterName.empty()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            characterName_ = previousCharacterName;
        }
        saveToDisk();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            characterName_ = normalizeCharacterName(characterName);
        }
    }
    
    loadFromDisk();
}

std::string AshitaMailStore::getCacheFilePath() const {
    char dllPath[MAX_PATH] = {0};
    
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\MailCache_" + characterName_ + ".json";
                }
            }
        }
    }
    
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultConfigPath("MailCache_" + characterName_ + ".json");
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\MailCache_" + characterName_ + ".json" : defaultPath;
}

void AshitaMailStore::ensureCacheDirectory(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

std::string AshitaMailStore::normalizeCharacterName(const std::string& name) const {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}

std::string AshitaMailStore::mailboxTypeToString(::XIFriendList::Core::MailFolder type) const {
    switch (type) {
        case ::XIFriendList::Core::MailFolder::Inbox: return "inbox";
        case ::XIFriendList::Core::MailFolder::Sent: return "sent";
        default: return "inbox";
    }
}

::XIFriendList::Core::MailFolder AshitaMailStore::stringToMailboxType(const std::string& str) const {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "sent") {
        return ::XIFriendList::Core::MailFolder::Sent;
    }
    return ::XIFriendList::Core::MailFolder::Inbox;
}

bool AshitaMailStore::loadFromDisk() {
    if (characterName_.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear existing data
    messages_[::XIFriendList::Core::MailFolder::Inbox].clear();
    messages_[::XIFriendList::Core::MailFolder::Sent].clear();
    
    std::string filePath = getCacheFilePath();
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // File doesn't exist yet - that's okay, start with empty cache
        dirty_ = false;
        return true;
    }
    
    try {
        // Read entire file
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();
        file.close();
        
        if (json.empty()) {
            dirty_ = false;
            return true;
        }
        
        std::string inboxArray;
        if (::XIFriendList::Protocol::JsonUtils::extractField(json, "inbox", inboxArray)) {
            parseMessageArray(inboxArray, ::XIFriendList::Core::MailFolder::Inbox);
        }
        
        std::string sentArray;
        if (::XIFriendList::Protocol::JsonUtils::extractField(json, "sent", sentArray)) {
            parseMessageArray(sentArray, ::XIFriendList::Core::MailFolder::Sent);
        }
        
        dirty_ = false;
        return true;
    } catch (...) {
        // On error, start with empty cache
        messages_[::XIFriendList::Core::MailFolder::Inbox].clear();
        messages_[::XIFriendList::Core::MailFolder::Sent].clear();
        dirty_ = false;
        return false;
    }
}

void AshitaMailStore::parseMessageArray(const std::string& arrayJson, ::XIFriendList::Core::MailFolder mailboxType) {
    // Simple array parser: find objects between [ and ]
    // Each object is: { "messageId": "...", "fromUserId": "...", ... }
    size_t pos = 0;
    
    while (pos < arrayJson.length() && arrayJson[pos] != '[') {
        ++pos;
    }
    if (pos >= arrayJson.length()) return;
    ++pos; // Skip [
    
    while (pos < arrayJson.length() && std::isspace(static_cast<unsigned char>(arrayJson[pos]))) {
        ++pos;
    }
    
    while (pos < arrayJson.length() && arrayJson[pos] != ']') {
        // Find object boundaries
        if (arrayJson[pos] == '{') {
            size_t objStart = pos;
            size_t depth = 0;
            size_t objEnd = pos;
            
            // Find matching }
            for (size_t i = pos; i < arrayJson.length(); ++i) {
                if (arrayJson[i] == '{') {
                    ++depth;
                } else if (arrayJson[i] == '}') {
                    --depth;
                    if (depth == 0) {
                        objEnd = i + 1;
                        break;
                    }
                }
            }
            
            if (objEnd > objStart) {
                std::string objJson = arrayJson.substr(objStart, objEnd - objStart);
                ::XIFriendList::Core::MailMessage msg;
                if (parseMessageObject(objJson, msg)) {
                    messages_[mailboxType][msg.messageId] = msg;
                }
            }
            
            pos = objEnd;
        } else {
            ++pos;
        }
        
        while (pos < arrayJson.length() && 
               (std::isspace(static_cast<unsigned char>(arrayJson[pos])) || arrayJson[pos] == ',')) {
            ++pos;
        }
    }
}

bool AshitaMailStore::parseMessageObject(const std::string& objJson, ::XIFriendList::Core::MailMessage& out) {
    out = ::XIFriendList::Core::MailMessage();
    
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "messageId", out.messageId)) {
        return false;
    }
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "fromUserId", out.fromUserId)) {
        return false;
    }
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "toUserId", out.toUserId)) {
        return false;
    }
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "subject", out.subject)) {
        return false;
    }
    
    // Body is optional (may be missing in meta mode)
    ::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "body", out.body);
    
    if (!::XIFriendList::Protocol::JsonUtils::extractNumberField(objJson, "createdAt", out.createdAt)) {
        return false;
    }
    
    uint64_t readAt = 0;
    ::XIFriendList::Protocol::JsonUtils::extractNumberField(objJson, "readAt", readAt);
    out.readAt = readAt;
    
    ::XIFriendList::Protocol::JsonUtils::extractBooleanField(objJson, "isRead", out.isRead);
    
    return true;
}

bool AshitaMailStore::saveToDisk() {
    if (characterName_.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string filePath = getCacheFilePath();
    ensureCacheDirectory(filePath);
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        // Write JSON structure
        file << "{\n";
        file << "  \"version\": 1,\n";
        file << "  \"characterName\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(characterName_) << ",\n";
        
        // Write inbox array
        file << "  \"inbox\": ";
        writeMessageArray(file, ::XIFriendList::Core::MailFolder::Inbox);
        file << ",\n";
        
        // Write sent array
        file << "  \"sent\": ";
        writeMessageArray(file, ::XIFriendList::Core::MailFolder::Sent);
        file << "\n";
        
        file << "}\n";
        const std::streampos bytesWritten = file.tellp();
        file.close();
        
        dirty_ = false;
        return true;
    } catch (...) {
        return false;
    }
}

void AshitaMailStore::writeMessageArray(std::ofstream& file, ::XIFriendList::Core::MailFolder mailboxType) {
    file << "[\n";
    
    const auto& mailbox = messages_[mailboxType];
    size_t index = 0;
    for (const auto& pair : mailbox) {
        if (index > 0) {
            file << ",\n";
        }
        writeMessageObject(file, pair.second);
        ++index;
    }
    
    file << "\n  ]";
}

void AshitaMailStore::writeMessageObject(std::ofstream& file, const ::XIFriendList::Core::MailMessage& msg) {
    file << "    {\n";
    file << "      \"messageId\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(msg.messageId) << ",\n";
    file << "      \"fromUserId\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(msg.fromUserId) << ",\n";
    file << "      \"toUserId\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(msg.toUserId) << ",\n";
    file << "      \"subject\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(msg.subject) << ",\n";
    file << "      \"body\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(msg.body) << ",\n";
    file << "      \"createdAt\": " << ::XIFriendList::Protocol::JsonUtils::encodeNumber(msg.createdAt) << ",\n";
    file << "      \"readAt\": " << ::XIFriendList::Protocol::JsonUtils::encodeNumber(msg.readAt) << ",\n";
    file << "      \"isRead\": " << ::XIFriendList::Protocol::JsonUtils::encodeBoolean(msg.isRead) << "\n";
    file << "    }";
}

bool AshitaMailStore::upsertMessage(::XIFriendList::Core::MailFolder mailboxType, const ::XIFriendList::Core::MailMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    messages_[mailboxType][message.messageId] = message;
    dirty_ = true;
    
    return true;
}

bool AshitaMailStore::hasMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& mailbox = messages_[mailboxType];
    return mailbox.find(messageId) != mailbox.end();
}

std::optional<::XIFriendList::Core::MailMessage> AshitaMailStore::getMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& mailbox = messages_[mailboxType];
    auto it = mailbox.find(messageId);
    if (it != mailbox.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<::XIFriendList::Core::MailMessage> AshitaMailStore::getAllMessages(::XIFriendList::Core::MailFolder mailboxType) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<::XIFriendList::Core::MailMessage> result;
    const auto& mailbox = messages_[mailboxType];
    
    // Copy to vector and sort by createdAt DESC
    for (const auto& pair : mailbox) {
        result.push_back(pair.second);
    }
    
    std::sort(result.begin(), result.end(),
              [](const ::XIFriendList::Core::MailMessage& a, const ::XIFriendList::Core::MailMessage& b) {
                  return a.createdAt > b.createdAt; // DESC order
              });
    
    return result;
}

bool AshitaMailStore::markRead(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId, bool isRead, uint64_t readAt) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& mailbox = messages_[mailboxType];
    auto it = mailbox.find(messageId);
    if (it != mailbox.end()) {
        it->second.isRead = isRead;
        it->second.readAt = readAt;
        dirty_ = true;
        return true;
    }
    return false;
}

bool AshitaMailStore::deleteMessage(::XIFriendList::Core::MailFolder mailboxType, const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& mailbox = messages_[mailboxType];
    auto it = mailbox.find(messageId);
    if (it != mailbox.end()) {
        mailbox.erase(it);
        dirty_ = true;
        return true;
    }
    return false;
}

int AshitaMailStore::pruneOld(::XIFriendList::Core::MailFolder mailboxType, int maxMessages) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& mailbox = messages_[mailboxType];
    if (static_cast<int>(mailbox.size()) <= maxMessages) {
        return 0;
    }
    
    // Convert to vector and sort by createdAt
    std::vector<std::pair<std::string, ::XIFriendList::Core::MailMessage>> sorted;
    for (const auto& pair : mailbox) {
        sorted.push_back(pair);
    }
    
    std::sort(sorted.begin(), sorted.end(),
              [](const std::pair<std::string, ::XIFriendList::Core::MailMessage>& a,
                 const std::pair<std::string, ::XIFriendList::Core::MailMessage>& b) {
                  return a.second.createdAt > b.second.createdAt; // DESC order
              });
    
    // Remove oldest messages
    int removed = 0;
    for (size_t i = maxMessages; i < sorted.size(); ++i) {
        mailbox.erase(sorted[i].first);
        ++removed;
    }
    
    if (removed > 0) {
        dirty_ = true;
    }
    
    return removed;
}

bool AshitaMailStore::clear(::XIFriendList::Core::MailFolder mailboxType) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    messages_[mailboxType].clear();
    dirty_ = true;
    return true;
}

int AshitaMailStore::getMessageCount(::XIFriendList::Core::MailFolder mailboxType) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return static_cast<int>(messages_[mailboxType].size());
}

::XIFriendList::Core::MemoryStats AshitaMailStore::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t bytes = sizeof(AshitaMailStore);
    size_t count = 0;
    
    bytes += characterName_.capacity();
    
    for (const auto& mailboxPair : messages_) {
        for (const auto& msgPair : mailboxPair.second) {
            bytes += sizeof(::XIFriendList::Core::MailMessage);
            bytes += msgPair.first.capacity();
            bytes += msgPair.second.messageId.capacity();
            bytes += msgPair.second.fromUserId.capacity();
            bytes += msgPair.second.toUserId.capacity();
            bytes += msgPair.second.subject.capacity();
            bytes += msgPair.second.body.capacity();
            ++count;
        }
        bytes += mailboxPair.second.size() * sizeof(std::string);
    }
    
    return ::XIFriendList::Core::MemoryStats(count, bytes, "Mail Cache");
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

