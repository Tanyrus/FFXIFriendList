
#ifndef APP_SERVER_SELECTION_STATE_H
#define APP_SERVER_SELECTION_STATE_H

#include <string>
#include <optional>

namespace XIFriendList {
namespace App {
namespace State {

struct ServerSelectionState {
    std::optional<std::string> savedServerId;
    std::optional<std::string> savedServerBaseUrl;
    std::optional<std::string> draftServerId;
    std::optional<std::string> detectedServerSuggestion;
    bool detectedServerShownOnce;
    
    ServerSelectionState()
        : savedServerId(std::nullopt)
        , savedServerBaseUrl(std::nullopt)
        , draftServerId(std::nullopt)
        , detectedServerSuggestion(std::nullopt)
        , detectedServerShownOnce(false)
    {}
    
    bool hasSavedServer() const {
        return savedServerId.has_value() && !savedServerId->empty();
    }
    
    bool isBlocked() const {
        return !hasSavedServer();
    }
};

} // namespace State
} // namespace App
} // namespace XIFriendList

#endif // APP_SERVER_SELECTION_STATE_H

