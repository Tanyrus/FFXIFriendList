// FakeNetClient.h
// Fake implementation of INetClient for testing

#ifndef APP_FAKE_NET_CLIENT_H
#define APP_FAKE_NET_CLIENT_H

#include "App/Interfaces/INetClient.h"
#include <string>
#include <map>
#include <functional>

namespace XIFriendList {
namespace App {

// Fake network client for testing
class FakeNetClient : public INetClient {
public:
    FakeNetClient() : baseUrl_("http://localhost:3000"), realmId_("horizon") {}
    
    // Set response for a URL
    void setResponse(const std::string& url, const HttpResponse& response) {
        responses_[url] = response;
    }
    
    // Set response callback (for dynamic responses)
    void setResponseCallback(std::function<HttpResponse(const std::string&, const std::string&, const std::string&)> callback) {
        responseCallback_ = callback;
    }
    
    // Get last request
    struct LastRequest {
        std::string url;
        std::string apiKey;
        std::string characterName;
        std::string body;
    };
    
    LastRequest getLastGetRequest() const { return lastGetRequest_; }
    LastRequest getLastPostRequest() const { return lastPostRequest_; }
    LastRequest getLastDelRequest() const { return lastDelRequest_; }
    
    // INetClient interface
    HttpResponse get(const std::string& url, 
                     const std::string& apiKey,
                     const std::string& characterName) override {
        lastGetRequest_ = { url, apiKey, characterName, "" };
        
        if (responseCallback_) {
            return responseCallback_(url, apiKey, characterName);
        }
        
        auto it = responses_.find(url);
        if (it != responses_.end()) {
            return it->second;
        }
        
        return { 404, "", "URL not found in fake responses" };
    }
    
    HttpResponse getPublic(const std::string& url) override {
        // For public requests, use empty strings for apiKey/characterName
        return get(url, "", "");
    }
    
    HttpResponse post(const std::string& url,
                     const std::string& apiKey,
                     const std::string& characterName,
                     const std::string& body) override {
        lastPostRequest_ = { url, apiKey, characterName, body };
        
        if (responseCallback_) {
            return responseCallback_(url, apiKey, characterName);
        }
        
        auto it = responses_.find(url);
        if (it != responses_.end()) {
            return it->second;
        }
        
        return { 404, "", "URL not found in fake responses" };
    }
    
    void getAsync(const std::string& url,
                 const std::string& apiKey,
                 const std::string& characterName,
                 ResponseCallback callback) override {
        HttpResponse response = get(url, apiKey, characterName);
        callback(response);
    }
    
    HttpResponse del(const std::string& url,
                     const std::string& apiKey,
                     const std::string& characterName,
                     const std::string& body = "") override {
        lastDelRequest_ = { url, apiKey, characterName, body };
        
        if (responseCallback_) {
            return responseCallback_(url, apiKey, characterName);
        }
        
        auto it = responses_.find(url);
        if (it != responses_.end()) {
            return it->second;
        }
        
        return { 404, "", "URL not found in fake responses" };
    }
    
    HttpResponse patch(const std::string& url,
                      const std::string& apiKey,
                      const std::string& characterName,
                      const std::string& body) override {
        lastPatchRequest_ = { url, apiKey, characterName, body };
        
        if (responseCallback_) {
            return responseCallback_(url, apiKey, characterName);
        }
        
        auto it = responses_.find(url);
        if (it != responses_.end()) {
            return it->second;
        }
        
        return { 404, "", "URL not found in fake responses" };
    }
    
    void postAsync(const std::string& url,
                  const std::string& apiKey,
                  const std::string& characterName,
                  const std::string& body,
                  ResponseCallback callback) override {
        HttpResponse response = post(url, apiKey, characterName, body);
        callback(response);
    }
    
    bool isAvailable() const override { return true; }
    
    std::string getBaseUrl() const override { return baseUrl_; }
    
    void setBaseUrl(const std::string& url) override { baseUrl_ = url; }
    
    void setRealmId(const std::string& realmId) override { realmId_ = realmId; }
    
    std::string getRealmId() const override { return realmId_; }
    
    void setSessionId(const std::string& sessionId) override { sessionId_ = sessionId; }
    
    std::string getSessionId() const override { return sessionId_; }
    
    // Test helpers
    LastRequest getLastPatchRequest() const { return lastPatchRequest_; }

private:
    std::string baseUrl_;
    std::string realmId_;
    std::string sessionId_;
    std::map<std::string, HttpResponse> responses_;
    std::function<HttpResponse(const std::string&, const std::string&, const std::string&)> responseCallback_;
    LastRequest lastGetRequest_;
    LastRequest lastPostRequest_;
    LastRequest lastDelRequest_;
    LastRequest lastPatchRequest_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_FAKE_NET_CLIENT_H

