
#ifndef APP_SERVER_LIST_USE_CASES_H
#define APP_SERVER_LIST_USE_CASES_H

#include "App/Interfaces/INetClient.h"
#include "App/Interfaces/ILogger.h"
#include "Core/ServerListCore.h"
#include <string>

namespace XIFriendList {
namespace App {
namespace UseCases {

struct ServerListResult {
    bool success;
    std::string error;
    XIFriendList::Core::ServerList serverList;
    
    ServerListResult()
        : success(false)
    {}
};

class FetchServerListUseCase {
public:
    FetchServerListUseCase(INetClient& netClient, ILogger& logger)
        : netClient_(netClient)
        , logger_(logger)
    {}
    
    ServerListResult fetchServerList();

private:
    INetClient& netClient_;
    ILogger& logger_;
};

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

#endif // APP_SERVER_LIST_USE_CASES_H

