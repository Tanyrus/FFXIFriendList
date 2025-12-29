
#ifndef PLATFORM_ASHITA_NOTES_PERSISTENCE_H
#define PLATFORM_ASHITA_NOTES_PERSISTENCE_H

#include "App/State/NotesState.h"
#include "Core/NotesCore.h"
#include <string>
#include <map>
#include <optional>
#include <mutex>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class NotesPersistence {
public:
    static bool loadFromFile(::XIFriendList::App::State::NotesState& state, int accountId);
    static bool saveToFile(const ::XIFriendList::App::State::NotesState& state, int accountId);

private:
    static std::string getNotesFilePath(int accountId);
    static void ensureNotesDirectory(const std::string& filePath);
    static std::string normalizeCharacterName(const std::string& name);
    static void parseNotesArray(const std::string& arrayJson, ::XIFriendList::App::State::NotesState& state);
    static bool parseNoteObject(const std::string& objJson, ::XIFriendList::Core::Note& out);
    static void writeNotesArray(std::ofstream& file, const ::XIFriendList::App::State::NotesState& state);
    
    // Write single note object to JSON
    static void writeNoteObject(std::ofstream& file, const ::XIFriendList::Core::Note& note);
    
    // Mutex for thread-safe file I/O
    static std::mutex ioMutex_;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_NOTES_PERSISTENCE_H

