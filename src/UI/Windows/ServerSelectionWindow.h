
#ifndef UI_SERVER_SELECTION_WINDOW_H
#define UI_SERVER_SELECTION_WINDOW_H

#include "Core/ServerListCore.h"
#include "Core/MemoryStats.h"
#include "App/State/ServerSelectionState.h"
#include "UI/Commands/WindowCommands.h"
#include <string>
#include <vector>
#include <optional>

namespace XIFriendList {
namespace UI {

class ServerSelectionWindow {
public:
    ServerSelectionWindow();
    ~ServerSelectionWindow() = default;
    
    void setCommandHandler(IWindowCommandHandler* handler) {
        commandHandler_ = handler;
    }
    
    void render();
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    void setServerList(const XIFriendList::Core::ServerList& serverList);
    void setServerSelectionState(const XIFriendList::App::State::ServerSelectionState& state);
    
    void setDetectedServerSuggestion(const std::string& serverId, const std::string& serverName);
    void clearDetectedServerSuggestion();
    
    std::string getDraftSelectedServerId() const;
    void setDraftSelectedServerId(const std::string& serverId);
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    void emitCommand(WindowCommandType type, const std::string& data = "");
    void renderServerList();
    void renderServerSelection();
    
    IWindowCommandHandler* commandHandler_;
    bool visible_;
    std::string windowId_;
    bool locked_;
    bool pendingClose_;
    
    XIFriendList::Core::ServerList serverList_;
    XIFriendList::App::State::ServerSelectionState state_;
    std::optional<std::string> detectedServerId_;
    std::optional<std::string> detectedServerName_;
    std::string draftSelectedServerId_;
    
    std::vector<XIFriendList::Core::ServerInfo> combinedServerList_;
    void rebuildCombinedServerList();
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_SERVER_SELECTION_WINDOW_H

