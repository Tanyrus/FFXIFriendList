
#ifndef PROTOCOL_HTTP_HEADERS_H
#define PROTOCOL_HTTP_HEADERS_H

#include <string>
#include <vector>
#include <utility>
#include "ProtocolVersion.h"

namespace XIFriendList {
namespace Protocol {

using Header = std::pair<std::string, std::string>;
using HeaderList = std::vector<Header>;

struct RequestContext {
    std::string apiKey;
    std::string characterName;
    std::string realmId;
    std::string sessionId;
    std::string contentType = "application/json";
};

class HttpHeaders {
public:
    static HeaderList buildHeaderList(const RequestContext& ctx);
    static std::string serializeForWinHttp(const HeaderList& headers);
    static std::string build(const RequestContext& ctx);
    static bool hasRequiredHeaders(const HeaderList& headers);
    static constexpr const char* HEADER_API_KEY = "X-API-Key";
    static constexpr const char* HEADER_CHARACTER_NAME = "characterName";
    static constexpr const char* HEADER_REALM_ID = "X-Realm-Id";
    static constexpr const char* HEADER_PROTOCOL_VERSION = "X-Protocol-Version";
    static constexpr const char* HEADER_SESSION_ID = "X-Session-Id";
    static constexpr const char* HEADER_CONTENT_TYPE = "Content-Type";
};

} // namespace Protocol
} // namespace XIFriendList

#endif // PROTOCOL_HTTP_HEADERS_H

