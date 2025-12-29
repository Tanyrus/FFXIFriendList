
#include "ModelsCore.h"

namespace XIFriendList {
namespace Core {

bool Presence::hasChanged(const Presence& other) const {
    return characterName != other.characterName
        || job != other.job
        || rank != other.rank
        || nation != other.nation
        || zone != other.zone
        || isAnonymous != other.isAnonymous;
}

bool Presence::isValid() const {
    return !characterName.empty();
}

} // namespace Core
} // namespace XIFriendList

