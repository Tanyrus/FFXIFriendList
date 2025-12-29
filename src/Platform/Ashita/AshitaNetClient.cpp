#include "Platform/Ashita/AshitaNetClient.h"
#include "Platform/Ashita/PathUtils.h"
#include "Protocol/HttpHeaders.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>
#include <fstream>
#include <cctype>

#ifndef BUILDING_TESTS
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif


namespace XIFriendList {
namespace Platform {
namespace Ashita {

struct ParsedUrl {
    std::string protocol;
    std::string hostname;
    int port;
    std::string path;
    bool isValid;
    
    ParsedUrl() : port(0), isValid(false) {}
};

static bool isValidHostname(const std::string& hostname) {
    if (hostname.empty() || hostname.length() > 253) {
        return false;
    }
    
    if (hostname == "localhost" || hostname == "127.0.0.1" || hostname == "::1") {
        return true;
    }
    
    bool hasDot = false;
    for (size_t i = 0; i < hostname.length(); ++i) {
        char c = hostname[i];
        if (c == '.') {
            hasDot = true;
            if (i == 0 || i == hostname.length() - 1) {
                return false;
            }
            if (i > 0 && hostname[i - 1] == '.') {
                return false;
            }
        } else if (!((c >= 'a' && c <= 'z') || 
                     (c >= 'A' && c <= 'Z') || 
                     (c >= '0' && c <= '9') || 
                     c == '-')) {
            return false;
        }
        if (c == '-' && (i == 0 || i == hostname.length() - 1)) {
            return false;
        }
    }
    
    return hasDot || hostname == "localhost";
}

static ParsedUrl parseUrl(const std::string& url) {
    ParsedUrl parsed;
    
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        return parsed;
    }
    
    parsed.protocol = url.substr(0, protocolEnd);
    std::string rest = url.substr(protocolEnd + 3);
    
    size_t pathStart = rest.find('/');
    std::string hostAndPort = (pathStart != std::string::npos) ? rest.substr(0, pathStart) : rest;
    parsed.path = (pathStart != std::string::npos) ? rest.substr(pathStart) : "/";
    
    size_t portStart = hostAndPort.find(':');
    parsed.hostname = (portStart != std::string::npos) ? hostAndPort.substr(0, portStart) : hostAndPort;
    
    if (portStart != std::string::npos) {
        std::string portStr = hostAndPort.substr(portStart + 1);
        try {
            parsed.port = std::stoi(portStr);
            if (parsed.port < 1 || parsed.port > 65535) {
                parsed.isValid = false;
                return parsed;
            }
        } catch (...) {
            parsed.isValid = false;
            return parsed;
        }
    } else {
#ifdef BUILDING_TESTS
        parsed.port = (parsed.protocol == "https") ? 443 : 80;
#else
        parsed.port = (parsed.protocol == "https") ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
#endif
    }
    
    if (parsed.protocol != "http" && parsed.protocol != "https") {
        parsed.isValid = false;
        return parsed;
    }
    
    parsed.isValid = !parsed.hostname.empty() && isValidHostname(parsed.hostname);
    return parsed;
}

AshitaNetClient::AshitaNetClient()
    : ashitaCore_(nullptr)
#ifdef USE_TEST_SERVER
    , baseUrl_("https://api-test.horizonfriendlist.com")
#else
    , baseUrl_("https://api.horizonfriendlist.com")
#endif
{
    std::string originalUrl = baseUrl_;
    loadServerUrlFromConfig();
}

void AshitaNetClient::setAshitaCore(IAshitaCore* core) {
    ashitaCore_ = core;
}

::XIFriendList::App::HttpResponse AshitaNetClient::get(const std::string& url,
                                       const std::string& apiKey,
                                       const std::string& characterName) {
#ifdef BUILDING_TESTS
    ::XIFriendList::App::HttpResponse response;
    response.statusCode = 0;
    response.error = "HTTP not available in test builds";
    return response;
#else
    ::XIFriendList::App::HttpResponse response;
    
    ParsedUrl parsed = parseUrl(url);
    if (!parsed.isValid) {
        response.statusCode = 0;
        response.error = "Invalid URL format";
        return response;
    }
    
    HINTERNET hSession = WinHttpOpen(L"XIFriendList/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
        response.statusCode = 0;
        response.error = "Failed to create WinHTTP session";
        return response;
    }
    
    WinHttpSetTimeouts(hSession, 30000, 30000, 30000, 30000);
    
    std::wstring wHost(parsed.hostname.begin(), parsed.hostname.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(parsed.port), 0);
    if (hConnect == nullptr) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to " << parsed.hostname << ":" << parsed.port << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    std::wstring wPath(parsed.path.begin(), parsed.path.end());
    DWORD flags = (parsed.protocol == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to create request";
        return response;
    }
    
    std::string headers = buildHeaders(apiKey, characterName);
    std::wstring wHeaders(headers.begin(), headers.end());
    WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    
    BOOL bResult = WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to send request to " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to receive response from " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode, &statusCodeSize, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to query status code";
        return response;
    }
    
    response.statusCode = static_cast<int>(statusCode);
    
    char buffer[4096];
    DWORD bytesRead = 0;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response.body += std::string(buffer, bytesRead);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
#endif
}

::XIFriendList::App::HttpResponse AshitaNetClient::getPublic(const std::string& url) {
#ifdef BUILDING_TESTS
    ::XIFriendList::App::HttpResponse response;
    response.statusCode = 0;
    response.error = "HTTP not available in test builds";
    return response;
#else
    ::XIFriendList::App::HttpResponse response;
    
    ParsedUrl parsed = parseUrl(url);
    if (!parsed.isValid) {
        response.statusCode = 0;
        response.error = "Invalid URL format";
        return response;
    }
    
    HINTERNET hSession = WinHttpOpen(L"XIFriendList/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
        response.statusCode = 0;
        response.error = "Failed to create WinHTTP session";
        return response;
    }
    
    WinHttpSetTimeouts(hSession, 30000, 30000, 30000, 30000);
    
    std::wstring wHost(parsed.hostname.begin(), parsed.hostname.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(parsed.port), 0);
    if (hConnect == nullptr) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to " << parsed.hostname << ":" << parsed.port << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    std::wstring wPath(parsed.path.begin(), parsed.path.end());
    DWORD flags = (parsed.protocol == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to create request";
        return response;
    }
    
    std::string headers = "User-Agent: XIFriendList/1.0\r\nAccept: application/json\r\n";
    std::wstring wHeaders(headers.begin(), headers.end());
    WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    
    BOOL bResult = WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to send request to " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to receive response from " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode, &statusCodeSize, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to query status code";
        return response;
    }
    
    response.statusCode = static_cast<int>(statusCode);
    
    std::string body;
    DWORD bytesAvailable = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        if (bytesAvailable == 0) {
            break;
        }
        
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            break;
        }
        body.append(buffer.data(), bytesRead);
    } while (bytesAvailable > 0);
    
    response.body = body;
    
    if (response.statusCode < 200 || response.statusCode >= 300) {
        response.error = "HTTP " + std::to_string(response.statusCode);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
#endif
}

::XIFriendList::App::HttpResponse AshitaNetClient::post(const std::string& url,
                                        const std::string& apiKey,
                                        const std::string& characterName,
                                        const std::string& body) {
#ifdef BUILDING_TESTS
    ::XIFriendList::App::HttpResponse response;
    response.statusCode = 0;
    response.error = "HTTP not available in test builds";
    return response;
#else
    ::XIFriendList::App::HttpResponse response;
    
    ParsedUrl parsed = parseUrl(url);
    if (!parsed.isValid) {
        response.statusCode = 0;
        response.error = "Invalid URL format";
        return response;
    }
    
    HINTERNET hSession = WinHttpOpen(L"XIFriendList/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
        response.statusCode = 0;
        response.error = "Failed to create WinHTTP session";
        return response;
    }
    
    WinHttpSetTimeouts(hSession, 30000, 30000, 30000, 30000);
    
    std::wstring wHost(parsed.hostname.begin(), parsed.hostname.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(parsed.port), 0);
    if (hConnect == nullptr) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to " << parsed.hostname << ":" << parsed.port << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    std::wstring wPath(parsed.path.begin(), parsed.path.end());
    DWORD flags = (parsed.protocol == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to create request";
        return response;
    }
    
    std::string headers = buildHeaders(apiKey, characterName);
    std::wstring wHeaders(headers.begin(), headers.end());
    WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    
    DWORD bodySize = static_cast<DWORD>(body.size());
    BOOL bResult = WinHttpSendRequest(hRequest, nullptr, 0, const_cast<char*>(body.c_str()), bodySize, bodySize, 0);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to send request to " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to receive response from " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode, &statusCodeSize, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to query status code";
        return response;
    }
    
    response.statusCode = static_cast<int>(statusCode);
    
    char buffer[4096];
    DWORD bytesRead = 0;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response.body += std::string(buffer, bytesRead);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
#endif
}

::XIFriendList::App::HttpResponse AshitaNetClient::patch(const std::string& url,
                                        const std::string& apiKey,
                                        const std::string& characterName,
                                        const std::string& body) {
#ifdef BUILDING_TESTS
    ::XIFriendList::App::HttpResponse response;
    response.statusCode = 0;
    response.error = "HTTP not available in test builds";
    return response;
#else
    ::XIFriendList::App::HttpResponse response;
    
    ParsedUrl parsed = parseUrl(url);
    if (!parsed.isValid) {
        response.statusCode = 0;
        response.error = "Invalid URL format";
        return response;
    }
    
    HINTERNET hSession = WinHttpOpen(L"XIFriendList/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
        response.statusCode = 0;
        response.error = "Failed to create WinHTTP session";
        return response;
    }
    
    WinHttpSetTimeouts(hSession, 30000, 30000, 30000, 30000);
    
    std::wstring wHost(parsed.hostname.begin(), parsed.hostname.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(parsed.port), 0);
    if (hConnect == nullptr) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to " << parsed.hostname << ":" << parsed.port << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    std::wstring wPath(parsed.path.begin(), parsed.path.end());
    DWORD flags = (parsed.protocol == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"PATCH", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to create request";
        return response;
    }
    
    std::string headers = buildHeaders(apiKey, characterName);
    std::wstring wHeaders(headers.begin(), headers.end());
    WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    
    DWORD bodySize = static_cast<DWORD>(body.size());
    BOOL bResult = WinHttpSendRequest(hRequest, nullptr, 0, const_cast<char*>(body.c_str()), bodySize, bodySize, 0);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to send request to " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to receive response from " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    response.statusCode = static_cast<int>(statusCode);
    
    DWORD size = 0;
    DWORD downloaded = 0;
    std::string responseBody;
    
    do {
        size = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &size)) {
            break;
        }
        
        if (size == 0) {
            break;
        }
        
        std::vector<char> buffer(size + 1);
        if (!WinHttpReadData(hRequest, buffer.data(), size, &downloaded)) {
            break;
        }
        
        buffer[downloaded] = '\0';
        responseBody += buffer.data();
    } while (size > 0);
    
    response.body = responseBody;
    
    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
#endif
}

::XIFriendList::App::HttpResponse AshitaNetClient::del(const std::string& url,
                                       const std::string& apiKey,
                                       const std::string& characterName,
                                       const std::string& body) {
#ifdef BUILDING_TESTS
    ::XIFriendList::App::HttpResponse response;
    response.statusCode = 0;
    response.error = "HTTP not available in test builds";
    return response;
#else
    ::XIFriendList::App::HttpResponse response;
    
    ParsedUrl parsed = parseUrl(url);
    if (!parsed.isValid) {
        response.statusCode = 0;
        response.error = "Invalid URL format";
        return response;
    }
    
    HINTERNET hSession = WinHttpOpen(L"XIFriendList/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == nullptr) {
        response.statusCode = 0;
        response.error = "Failed to create WinHTTP session";
        return response;
    }
    
    WinHttpSetTimeouts(hSession, 30000, 30000, 30000, 30000);
    
    std::wstring wHost(parsed.hostname.begin(), parsed.hostname.end());
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(parsed.port), 0);
    if (hConnect == nullptr) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to connect to " << parsed.hostname << ":" << parsed.port << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    std::wstring wPath(parsed.path.begin(), parsed.path.end());
    DWORD flags = (parsed.protocol == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"DELETE", wPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest == nullptr) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to create request";
        return response;
    }
    
    std::string headers = buildHeaders(apiKey, characterName);
    std::wstring wHeaders(headers.begin(), headers.end());
    WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    
    DWORD bodySize = static_cast<DWORD>(body.size());
    BOOL bResult = WinHttpSendRequest(hRequest, nullptr, 0, bodySize > 0 ? const_cast<char*>(body.c_str()) : nullptr, bodySize, bodySize, 0);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to send request to " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    bResult = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResult) {
        DWORD errorCode = GetLastError();
        std::ostringstream errorMsg;
        errorMsg << "Failed to receive response from " << url << " (WinHTTP error: " << errorCode << ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = errorMsg.str();
        return response;
    }
    
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode, &statusCodeSize, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        response.statusCode = 0;
        response.error = "Failed to query status code";
        return response;
    }
    
    response.statusCode = static_cast<int>(statusCode);
    
    char buffer[4096];
    DWORD bytesRead = 0;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response.body += std::string(buffer, bytesRead);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
#endif
}

void AshitaNetClient::getAsync(const std::string& url,
                               const std::string& apiKey,
                               const std::string& characterName,
                               ::XIFriendList::App::ResponseCallback callback) {
    std::thread([this, url, apiKey, characterName, callback]() {
        ::XIFriendList::App::HttpResponse response = get(url, apiKey, characterName);
        callback(response);
    }).detach();
}

void AshitaNetClient::postAsync(const std::string& url,
                                const std::string& apiKey,
                                const std::string& characterName,
                                const std::string& body,
                                ::XIFriendList::App::ResponseCallback callback) {
    std::thread([this, url, apiKey, characterName, body, callback]() {
        ::XIFriendList::App::HttpResponse response = post(url, apiKey, characterName, body);
        callback(response);
    }).detach();
}

bool AshitaNetClient::isAvailable() const {
    return ashitaCore_ != nullptr;
}

std::string AshitaNetClient::getBaseUrl() const {
    return baseUrl_;
}

void AshitaNetClient::setBaseUrl(const std::string& url) {
    baseUrl_ = url;
}

void AshitaNetClient::setRealmId(const std::string& realmId) {
    realmId_ = realmId;
}

std::string AshitaNetClient::getRealmId() const {
    return realmId_;
}

std::string AshitaNetClient::getConfigPath() const {
    char dllPath[MAX_PATH] = {0};
    
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\ffxifriendlist.ini";
                }
            }
        }
    }
    
    std::string defaultPath = PathUtils::getDefaultIniPath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.ini" : defaultPath;
}

void AshitaNetClient::loadServerUrlFromConfig() {
    try {
        std::string filePath = getConfigPath();
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return;
        }
        
        std::string line;
        bool isInSettingsSection = false;
        
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }
            
            if (line[0] == '[' && line.back() == ']') {
                std::string section = line.substr(1, line.length() - 2);
                isInSettingsSection = (section == "Settings");
                continue;
            }
            
            if (!isInSettingsSection) {
                continue;
            }
            
            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }
            
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), 
                          [](unsigned char c) { return static_cast<char>(::tolower(c)); });
            
            if (keyLower == "serverurl" || keyLower == "server_url" || keyLower == "apiurl" || keyLower == "api_url") {
                if (!value.empty()) {
                    ParsedUrl parsed = parseUrl(value);
                    
                    if (!parsed.isValid) {
                        return;
                    }
                    
                    if (parsed.protocol != "https") {
                        if (parsed.protocol != "http" || 
                            (parsed.hostname != "localhost" && 
                             parsed.hostname != "127.0.0.1" && 
                             parsed.hostname != "::1")) {
                            return;
                        }
                    }
                    
                    #ifndef USE_TEST_SERVER
                    if (parsed.hostname != "api.horizonfriendlist.com" && 
                        parsed.hostname != "localhost" && 
                        parsed.hostname != "127.0.0.1" &&
                        parsed.hostname != "::1") {
                        return;
                    }
                    #endif
                    
                    baseUrl_ = value;
                    return;
                }
            }
        }
        
        file.close();
    } catch (...) {
    }
}

std::string AshitaNetClient::buildHeaders(const std::string& apiKey, const std::string& characterName) const {
    ::XIFriendList::Protocol::RequestContext ctx;
    ctx.apiKey = apiKey;
    ctx.characterName = characterName;
    ctx.realmId = realmId_;
    ctx.sessionId = sessionId_;
    ctx.contentType = "application/json";
    
    return ::XIFriendList::Protocol::HttpHeaders::build(ctx);
}

void AshitaNetClient::setSessionId(const std::string& sessionId) {
    sessionId_ = sessionId;
}

std::string AshitaNetClient::getSessionId() const {
    return sessionId_;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

