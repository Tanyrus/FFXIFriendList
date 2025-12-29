
#ifndef APP_API_KEY_STATE_H
#define APP_API_KEY_STATE_H

#include <string>
#include <map>

namespace XIFriendList {
namespace App {
namespace State {

struct ApiKeyState {
    std::map<std::string, std::string> apiKeys;  // characterName (normalized) -> API key
    
    ApiKeyState() = default;
};

} // namespace State
} // namespace App
} // namespace XIFriendList

#endif // APP_API_KEY_STATE_H

