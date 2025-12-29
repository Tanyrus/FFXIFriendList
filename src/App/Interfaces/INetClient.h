
#ifndef APP_INET_CLIENT_H
#define APP_INET_CLIENT_H

#include <string>
#include <functional>
#include <cstdint>

namespace XIFriendList {
namespace App {

struct HttpResponse {
    int statusCode;
    std::string body;
    std::string error;
    
    bool isSuccess() const {
        return statusCode >= 200 && statusCode < 300 && error.empty();
    }
};

using ResponseCallback = std::function<void(const HttpResponse&)>;

class INetClient {
public:
    virtual ~INetClient() = default;
    virtual HttpResponse get(const std::string& url, 
                           const std::string& apiKey,
                           const std::string& characterName) = 0;
    virtual HttpResponse getPublic(const std::string& url) = 0;
    virtual HttpResponse post(const std::string& url,
                             const std::string& apiKey,
                             const std::string& characterName,
                             const std::string& body) = 0;
    virtual HttpResponse patch(const std::string& url,
                              const std::string& apiKey,
                              const std::string& characterName,
                              const std::string& body) = 0;
    virtual HttpResponse del(const std::string& url,
                             const std::string& apiKey,
                             const std::string& characterName,
                             const std::string& body = "") = 0;
    virtual void getAsync(const std::string& url,
                         const std::string& apiKey,
                         const std::string& characterName,
                         ResponseCallback callback) = 0;
    virtual void postAsync(const std::string& url,
                          const std::string& apiKey,
                          const std::string& characterName,
                          const std::string& body,
                          ResponseCallback callback) = 0;
    virtual bool isAvailable() const = 0;
    virtual std::string getBaseUrl() const = 0;
    virtual void setBaseUrl(const std::string& url) = 0;
    virtual void setRealmId(const std::string& realmId) = 0;
    virtual std::string getRealmId() const = 0;
    virtual void setSessionId(const std::string& sessionId) = 0;
    virtual std::string getSessionId() const = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_INET_CLIENT_H

