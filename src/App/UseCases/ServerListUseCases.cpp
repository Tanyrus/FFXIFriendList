#include "ServerListUseCases.h"
#include "Protocol/JsonUtils.h"
#include <sstream>

namespace XIFriendList {
namespace App {
namespace UseCases {

ServerListResult FetchServerListUseCase::fetchServerList() {
    ServerListResult result;
    result.serverList.loaded = false;
    
    // Always use production API for server list - the list is the same regardless of test/prod
    // This ensures the server list is always available even when testing against test API
    std::string baseUrl = "https://api.horizonfriendlist.com";
    
    std::string url = baseUrl + "/api/servers";
    
    logger_.info("[server-list] Fetching server list from: " + url);
    
    XIFriendList::App::HttpResponse response = netClient_.getPublic(url);
    
    if (!response.isSuccess()) {
        result.error = "Failed to fetch server list: " + response.error;
        logger_.error("[server-list] " + result.error);
        result.serverList.loaded = false;
        return result;
    }
    
    if (response.statusCode != 200) {
        result.error = "Server returned status " + std::to_string(response.statusCode);
        logger_.error("[server-list] " + result.error);
        result.serverList.loaded = false;
        return result;
    }
    
    if (response.body.empty() || !XIFriendList::Protocol::JsonUtils::isValidJson(response.body)) {
        result.error = "Invalid JSON response from server";
        logger_.error("[server-list] " + result.error);
        result.serverList.loaded = false;
        return result;
    }
    
    std::string serversJson;
    if (!XIFriendList::Protocol::JsonUtils::extractField(response.body, "servers", serversJson)) {
        result.error = "Missing 'servers' field in response";
        logger_.error("[server-list] " + result.error);
        result.serverList.loaded = false;
        return result;
    }
    
    result.serverList.servers.clear();
    
    size_t pos = 1;
    while (pos < serversJson.length() && serversJson[pos] != ']') {
        while (pos < serversJson.length() && std::isspace(static_cast<unsigned char>(serversJson[pos]))) {
            ++pos;
        }
        if (pos >= serversJson.length() || serversJson[pos] == ']') break;
        
        if (serversJson[pos] == ',') {
            ++pos;
            while (pos < serversJson.length() && std::isspace(static_cast<unsigned char>(serversJson[pos]))) {
                ++pos;
            }
        }
        
        if (serversJson[pos] != '{') break;
        
        size_t objStart = pos;
        int braceDepth = 0;
        while (pos < serversJson.length()) {
            if (serversJson[pos] == '{') {
                ++braceDepth;
            } else if (serversJson[pos] == '}') {
                --braceDepth;
                if (braceDepth == 0) {
                    ++pos;
                    break;
                }
            }
            ++pos;
        }
        
        if (braceDepth != 0) break;
        
        std::string serverObj = serversJson.substr(objStart, pos - objStart);
        
        XIFriendList::Core::ServerInfo server;
        XIFriendList::Protocol::JsonUtils::extractStringField(serverObj, "id", server.id);
        XIFriendList::Protocol::JsonUtils::extractStringField(serverObj, "name", server.name);
        XIFriendList::Protocol::JsonUtils::extractStringField(serverObj, "baseUrl", server.baseUrl);
        XIFriendList::Protocol::JsonUtils::extractStringField(serverObj, "realmId", server.realmId);
        server.isFromServer = true;
        
        if (!server.id.empty() && !server.name.empty() && !server.baseUrl.empty()) {
            result.serverList.servers.push_back(server);
        }
    }
    
    result.serverList.loaded = true;
    result.success = true;
    logger_.info("[server-list] Loaded " + std::to_string(result.serverList.servers.size()) + " servers");
    
    return result;
}

} // namespace UseCases
} // namespace App
} // namespace XIFriendList

