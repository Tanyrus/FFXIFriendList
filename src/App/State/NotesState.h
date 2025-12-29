
#ifndef APP_NOTES_STATE_H
#define APP_NOTES_STATE_H

#include "Core/NotesCore.h"
#include "Core/MemoryStats.h"
#include <string>
#include <map>
#include <optional>

namespace XIFriendList {
namespace App {
namespace State {

struct NotesState {
    std::map<std::string, XIFriendList::Core::Note> notes;
    int accountId = 0;
    bool dirty = false;
    
    NotesState() = default;
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;
};

} // namespace State
} // namespace App
} // namespace XIFriendList

#endif // APP_NOTES_STATE_H

