
#ifndef PLATFORM_ASHITA_NET_CLIENT_H
#define PLATFORM_ASHITA_NET_CLIENT_H

#include "App/Interfaces/INetClient.h"
#include <string>

struct IAshitaCore;

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaNetClient : public ::XIFriendList::App::INetClient {
public:
    AshitaNetClient();
    ~AshitaNetClient() = default;
    void setAshitaCore(IAshitaCore* core);
    ::XIFriendList::App::HttpResponse get(const std::string& url,
                         const std::string& apiKey,
                         const std::string& characterName) override;
    
    ::XIFriendList::App::HttpResponse getPublic(const std::string& url) override;
    
    ::XIFriendList::App::HttpResponse post(const std::string& url,
                          const std::string& apiKey,
                          const std::string& characterName,
                          const std::string& body) override;
    
    ::XIFriendList::App::HttpResponse patch(const std::string& url,
                           const std::string& apiKey,
                           const std::string& characterName,
                           const std::string& body) override;
    
    ::XIFriendList::App::HttpResponse del(const std::string& url,
                         const std::string& apiKey,
                         const std::string& characterName,
                         const std::string& body = "") override;
    
    void getAsync(const std::string& url,
                 const std::string& apiKey,
                 const std::string& characterName,
                 ::XIFriendList::App::ResponseCallback callback) override;
    
    void postAsync(const std::string& url,
                  const std::string& apiKey,
                  const std::string& characterName,
                  const std::string& body,
                  ::XIFriendList::App::ResponseCallback callback) override;
    
    bool isAvailable() const override;
    
    std::string getBaseUrl() const override;
    
    void setBaseUrl(const std::string& url) override;
    
    void setRealmId(const std::string& realmId) override;
    
    std::string getRealmId() const override;
    
    void setSessionId(const std::string& sessionId) override;
    
    std::string getSessionId() const override;

private:
    IAshitaCore* ashitaCore_;
    std::string baseUrl_;
    std::string realmId_;
    std::string sessionId_;
    
    std::string buildHeaders(const std::string& apiKey, const std::string& characterName) const;
    
    void loadServerUrlFromConfig();
    
    std::string getConfigPath() const;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_NET_CLIENT_H

