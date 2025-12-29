#include "Platform/Ashita/NotesPersistence.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/JsonUtils.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <windows.h>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::mutex NotesPersistence::ioMutex_;

std::string NotesPersistence::getNotesFilePath(int accountId) {
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
                    return gameDir + "config\\FFXIFriendList\\Notes_Account_" + std::to_string(accountId) + ".json";
                }
            }
        }
    }
    
    std::string defaultPath = PathUtils::getDefaultConfigPath("Notes_Account_" + std::to_string(accountId) + ".json");
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\Notes_Account_" + std::to_string(accountId) + ".json" : defaultPath;
}

void NotesPersistence::ensureNotesDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        
        for (size_t i = 0; i < dir.length(); ++i) {
            if (dir[i] == '\\' || dir[i] == '/') {
                std::string subDir = dir.substr(0, i);
                if (!subDir.empty()) {
                    CreateDirectoryA(subDir.c_str(), nullptr);
                }
            }
        }
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

std::string NotesPersistence::normalizeCharacterName(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}

bool NotesPersistence::parseNoteObject(const std::string& objJson, ::XIFriendList::Core::Note& out) {
    out = ::XIFriendList::Core::Note();
    
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "friendName", out.friendName)) {
        return false;
    }
    if (!::XIFriendList::Protocol::JsonUtils::extractStringField(objJson, "note", out.note)) {
        return false;
    }
    if (!::XIFriendList::Protocol::JsonUtils::extractNumberField(objJson, "updatedAt", out.updatedAt)) {
        return false;
    }
    
    return true;
}

void NotesPersistence::parseNotesArray(const std::string& arrayJson, ::XIFriendList::App::State::NotesState& state) {
    size_t pos = 0;
    
    while (pos < arrayJson.length() && arrayJson[pos] != '[') {
        ++pos;
    }
    if (pos >= arrayJson.length()) return;
    ++pos;
    
    while (pos < arrayJson.length() && std::isspace(static_cast<unsigned char>(arrayJson[pos]))) {
        ++pos;
    }
    
    while (pos < arrayJson.length() && arrayJson[pos] != ']') {
        if (arrayJson[pos] == '{') {
            size_t objStart = pos;
            size_t depth = 0;
            size_t objEnd = pos;
            
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
                ::XIFriendList::Core::Note note;
                if (parseNoteObject(objJson, note)) {
                    std::string normalizedFriendName = normalizeCharacterName(note.friendName);
                    note.friendName = normalizedFriendName;
                    state.notes[normalizedFriendName] = note;
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

void NotesPersistence::writeNoteObject(std::ofstream& file, const ::XIFriendList::Core::Note& note) {
    file << "    {\n";
    file << "      \"friendName\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(note.friendName) << ",\n";
    file << "      \"note\": " << ::XIFriendList::Protocol::JsonUtils::encodeString(note.note) << ",\n";
    file << "      \"updatedAt\": " << ::XIFriendList::Protocol::JsonUtils::encodeNumber(note.updatedAt) << "\n";
    file << "    }";
}

void NotesPersistence::writeNotesArray(std::ofstream& file, const ::XIFriendList::App::State::NotesState& state) {
    file << "[\n";
    
    size_t index = 0;
    for (const auto& pair : state.notes) {
        if (index > 0) {
            file << ",\n";
        }
        writeNoteObject(file, pair.second);
        ++index;
    }
    
    file << "\n  ]";
}

bool NotesPersistence::loadFromFile(::XIFriendList::App::State::NotesState& state, int accountId) {
    if (accountId == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    state.notes.clear();
    
    std::string filePath = getNotesFilePath(accountId);
    std::ifstream file(filePath);
    if (!file.is_open()) {
        state.dirty = false;
        return true;
    }
    
    try {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();
        file.close();
        
        if (json.empty()) {
            state.dirty = false;
            return true;
        }
        
        std::string notesArray;
        if (::XIFriendList::Protocol::JsonUtils::extractField(json, "notes", notesArray)) {
            parseNotesArray(notesArray, state);
        }
        
        state.accountId = accountId;
        state.dirty = false;
        return true;
    } catch (...) {
        state.notes.clear();
        state.accountId = accountId;
        state.dirty = false;
        return false;
    }
}

bool NotesPersistence::saveToFile(const ::XIFriendList::App::State::NotesState& state, int accountId) {
    if (accountId == 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    std::string filePath = getNotesFilePath(accountId);
    ensureNotesDirectory(filePath);
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << "{\n";
        file << "  \"version\": 1,\n";
        file << "  \"accountId\": " << accountId << ",\n";
        
        file << "  \"notes\": ";
        writeNotesArray(file, state);
        file << "\n";
        
        file << "}\n";
        file.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

