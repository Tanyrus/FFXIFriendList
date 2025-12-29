
#ifndef CORE_NOTES_CORE_H
#define CORE_NOTES_CORE_H

#include <string>
#include <cstdint>

namespace XIFriendList {
namespace Core {

struct Note {
    std::string friendName;
    std::string note;
    uint64_t updatedAt;
    
    Note()
        : updatedAt(0)
    {}
    
    Note(const std::string& friendName, const std::string& note, uint64_t updatedAt)
        : friendName(friendName)
        , note(note)
        , updatedAt(updatedAt)
    {}
    
    bool operator==(const Note& other) const {
        return friendName == other.friendName;
    }
    
    bool operator!=(const Note& other) const {
        return !(*this == other);
    }
    
    bool isEmpty() const {
        return note.empty();
    }
};


/**
 * Utility for merging local and server notes.
 * 
 * Merge semantics:
 * - If one is empty, use the other
 * - If identical, use either
 * - Otherwise, concatenate with divider and timestamps
 * - Detect and avoid infinite duplication of merge markers
 */
class NoteMerger {
public:
    static constexpr const char* MERGE_DIVIDER = "\n\n--- Merged Notes ---\n\n";
    
    /**
     * Merge two notes into a single canonical note.
     * 
     * @param localNote The local note content
     * @param localTimestamp Timestamp when local note was last updated
     * @param serverNote The server note content
     * @param serverTimestamp Timestamp when server note was last updated
     * @return Merged note content
     */
    static std::string merge(
        const std::string& localNote, 
        uint64_t localTimestamp,
        const std::string& serverNote, 
        uint64_t serverTimestamp);
    
    /**
     * Check if a note contains merge markers (already merged)
     */
    static bool containsMergeMarker(const std::string& note);
    
    /**
     * Check if two notes are effectively equal (ignoring trailing whitespace)
     */
    static bool areNotesEqual(const std::string& note1, const std::string& note2);
    
    /**
     * Format a timestamp for display in merge header
     */
    static std::string formatTimestamp(uint64_t timestamp);

private:
    static std::string trim(const std::string& str);
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_NOTES_CORE_H

