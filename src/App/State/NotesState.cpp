#include "App/State/NotesState.h"

namespace XIFriendList {
namespace App {
namespace State {

XIFriendList::Core::MemoryStats NotesState::getMemoryStats() const {
    size_t noteBytes = 0;
    for (const auto& pair : notes) {
        noteBytes += sizeof(XIFriendList::Core::Note);
        noteBytes += pair.first.capacity();
        noteBytes += pair.second.friendName.capacity();
        noteBytes += pair.second.note.capacity();
    }
    noteBytes += notes.size() * (sizeof(std::string) + sizeof(XIFriendList::Core::Note));
    
    return XIFriendList::Core::MemoryStats(notes.size(), noteBytes, "Notes");
}

} // namespace State
} // namespace App
} // namespace XIFriendList

