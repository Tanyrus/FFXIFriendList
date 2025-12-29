
#ifndef CORE_SERVER_LIST_CORE_H
#define CORE_SERVER_LIST_CORE_H

#include <string>
#include <vector>

namespace XIFriendList {
namespace Core {

struct ServerInfo {
    std::string id;
    std::string name;
    std::string baseUrl;
    std::string realmId;
    bool isFromServer;
    
    ServerInfo()
        : isFromServer(false)
    {}
    
    ServerInfo(const std::string& id, const std::string& name, const std::string& baseUrl, const std::string& realmId = "", bool isFromServer = false)
        : id(id)
        , name(name)
        , baseUrl(baseUrl)
        , realmId(realmId)
        , isFromServer(isFromServer)
    {}
};

struct ServerList {
    std::vector<ServerInfo> servers;
    bool loaded;
    std::string error;
    
    ServerList()
        : loaded(false)
    {}
};

} // namespace Core
} // namespace XIFriendList

#endif // CORE_SERVER_LIST_CORE_H

