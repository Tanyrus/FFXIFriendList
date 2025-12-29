
#ifndef APP_SERVER_SELECTION_GATE_H
#define APP_SERVER_SELECTION_GATE_H

#include "App/State/ServerSelectionState.h"
#include <string>

namespace XIFriendList {
namespace App {

enum class NetworkBlockReason {
    Allowed,
    BlockedByServerSelection
};

class ServerSelectionGate {
public:
    ServerSelectionGate(const State::ServerSelectionState& state)
        : state_(state)
    {}
    
    NetworkBlockReason checkNetworkAccess() const {
        if (state_.isBlocked()) {
            return NetworkBlockReason::BlockedByServerSelection;
        }
        return NetworkBlockReason::Allowed;
    }
    
    bool isBlocked() const {
        return state_.isBlocked();
    }
    
    bool isAllowed() const {
        return !state_.isBlocked();
    }
    
    std::string getBlockReason() const {
        if (state_.isBlocked()) {
            return "Server selection required";
        }
        return "";
    }

private:
    const State::ServerSelectionState& state_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_SERVER_SELECTION_GATE_H

