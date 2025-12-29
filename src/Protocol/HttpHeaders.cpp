// Pure header building implementation

#include "HttpHeaders.h"
#include <sstream>

namespace XIFriendList {
namespace Protocol {

HeaderList HttpHeaders::buildHeaderList(const RequestContext& ctx) {
    HeaderList headers;
    
    if (!ctx.contentType.empty()) {
        headers.emplace_back(HEADER_CONTENT_TYPE, ctx.contentType);
    }
    
    if (!ctx.apiKey.empty()) {
        headers.emplace_back(HEADER_API_KEY, ctx.apiKey);
    }
    
    if (!ctx.characterName.empty()) {
        headers.emplace_back(HEADER_CHARACTER_NAME, ctx.characterName);
    }
    
    if (!ctx.realmId.empty()) {
        headers.emplace_back(HEADER_REALM_ID, ctx.realmId);
    }
    
    headers.emplace_back(HEADER_PROTOCOL_VERSION, PROTOCOL_VERSION);
    
    if (!ctx.sessionId.empty()) {
        headers.emplace_back(HEADER_SESSION_ID, ctx.sessionId);
    }
    
    return headers;
}

std::string HttpHeaders::serializeForWinHttp(const HeaderList& headers) {
    std::ostringstream oss;
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    return oss.str();
}

std::string HttpHeaders::build(const RequestContext& ctx) {
    return serializeForWinHttp(buildHeaderList(ctx));
}

bool HttpHeaders::hasRequiredHeaders(const HeaderList& headers) {
    bool hasContentType = false;
    bool hasProtocolVersion = false;
    
    for (const auto& header : headers) {
        if (header.first == HEADER_CONTENT_TYPE) hasContentType = true;
        if (header.first == HEADER_PROTOCOL_VERSION) hasProtocolVersion = true;
    }
    
    return hasContentType && hasProtocolVersion;
}

} // namespace Protocol
} // namespace XIFriendList

