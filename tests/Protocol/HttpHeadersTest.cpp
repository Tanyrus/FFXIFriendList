// HttpHeadersTest.cpp
// Tests for header building utilities
// These tests verify correct header composition without Platform/Ashita dependencies

#include <catch2/catch_test_macros.hpp>
#include "Protocol/HttpHeaders.h"
#include <algorithm>

using namespace XIFriendList::Protocol;

TEST_CASE("HttpHeaders - Build header list with all fields", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.apiKey = "test-api-key-123";
    ctx.characterName = "TestCharacter";
    ctx.realmId = "horizon";
    ctx.sessionId = "session-456-789";
    ctx.contentType = "application/json";
    
    HeaderList headers = HttpHeaders::buildHeaderList(ctx);
    
    // Verify all required headers are present
    REQUIRE(headers.size() == 6);
    
    // Helper to find header by name
    auto findHeader = [&headers](const std::string& name) -> std::string {
        auto it = std::find_if(headers.begin(), headers.end(), 
            [&name](const Header& h) { return h.first == name; });
        return (it != headers.end()) ? it->second : "";
    };
    
    REQUIRE(findHeader("Content-Type") == "application/json");
    REQUIRE(findHeader("X-API-Key") == "test-api-key-123");
    REQUIRE(findHeader("characterName") == "TestCharacter");
    REQUIRE(findHeader("X-Realm-Id") == "horizon");
    REQUIRE(findHeader("X-Protocol-Version") == "2.0.0");
    REQUIRE(findHeader("X-Session-Id") == "session-456-789");
}

TEST_CASE("HttpHeaders - Empty optional fields are omitted", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.contentType = "application/json";
    // All other fields empty
    
    HeaderList headers = HttpHeaders::buildHeaderList(ctx);
    
    // Should only have Content-Type and Protocol-Version (required)
    REQUIRE(headers.size() == 2);
    
    auto findHeader = [&headers](const std::string& name) -> bool {
        return std::find_if(headers.begin(), headers.end(), 
            [&name](const Header& h) { return h.first == name; }) != headers.end();
    };
    
    REQUIRE(findHeader("Content-Type"));
    REQUIRE(findHeader("X-Protocol-Version"));
    REQUIRE_FALSE(findHeader("X-API-Key"));
    REQUIRE_FALSE(findHeader("characterName"));
    REQUIRE_FALSE(findHeader("X-Realm-Id"));
    REQUIRE_FALSE(findHeader("X-Session-Id"));
}

TEST_CASE("HttpHeaders - Serialize to WinHTTP format", "[Protocol][HttpHeaders]") {
    HeaderList headers;
    headers.emplace_back("Content-Type", "application/json");
    headers.emplace_back("X-API-Key", "test-key");
    
    std::string serialized = HttpHeaders::serializeForWinHttp(headers);
    
    // WinHTTP format: "Name: value\r\n"
    REQUIRE(serialized.find("Content-Type: application/json\r\n") != std::string::npos);
    REQUIRE(serialized.find("X-API-Key: test-key\r\n") != std::string::npos);
}

TEST_CASE("HttpHeaders - Build convenience method", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.apiKey = "key123";
    ctx.characterName = "Char";
    ctx.realmId = "eden";
    ctx.sessionId = "sess";
    ctx.contentType = "application/json";
    
    std::string headers = HttpHeaders::build(ctx);
    
    // Verify format
    REQUIRE(headers.find("Content-Type: application/json\r\n") != std::string::npos);
    REQUIRE(headers.find("X-API-Key: key123\r\n") != std::string::npos);
    REQUIRE(headers.find("characterName: Char\r\n") != std::string::npos);
    REQUIRE(headers.find("X-Realm-Id: eden\r\n") != std::string::npos);
    REQUIRE(headers.find("X-Protocol-Version: 2.0.0\r\n") != std::string::npos);
    REQUIRE(headers.find("X-Session-Id: sess\r\n") != std::string::npos);
}

TEST_CASE("HttpHeaders - Has required headers check", "[Protocol][HttpHeaders]") {
    SECTION("Valid headers pass") {
        HeaderList headers;
        headers.emplace_back("Content-Type", "application/json");
        headers.emplace_back("X-Protocol-Version", "2.0.0");
        
        REQUIRE(HttpHeaders::hasRequiredHeaders(headers));
    }
    
    SECTION("Missing Content-Type fails") {
        HeaderList headers;
        headers.emplace_back("X-Protocol-Version", "2.0.0");
        
        REQUIRE_FALSE(HttpHeaders::hasRequiredHeaders(headers));
    }
    
    SECTION("Missing Protocol-Version fails") {
        HeaderList headers;
        headers.emplace_back("Content-Type", "application/json");
        
        REQUIRE_FALSE(HttpHeaders::hasRequiredHeaders(headers));
    }
    
    SECTION("Empty list fails") {
        HeaderList headers;
        REQUIRE_FALSE(HttpHeaders::hasRequiredHeaders(headers));
    }
}

TEST_CASE("HttpHeaders - Protocol version is always included", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.contentType = "application/json";
    // Everything else empty
    
    std::string headers = HttpHeaders::build(ctx);
    
    // Protocol version should ALWAYS be present
    REQUIRE(headers.find("X-Protocol-Version: 2.0.0\r\n") != std::string::npos);
}

TEST_CASE("HttpHeaders - Header name constants match expected casing", "[Protocol][HttpHeaders]") {
    // These header names must match exactly what the server expects
    REQUIRE(std::string(HttpHeaders::HEADER_API_KEY) == "X-API-Key");
    REQUIRE(std::string(HttpHeaders::HEADER_CHARACTER_NAME) == "characterName");
    REQUIRE(std::string(HttpHeaders::HEADER_REALM_ID) == "X-Realm-Id");
    REQUIRE(std::string(HttpHeaders::HEADER_PROTOCOL_VERSION) == "X-Protocol-Version");
    REQUIRE(std::string(HttpHeaders::HEADER_SESSION_ID) == "X-Session-Id");
    REQUIRE(std::string(HttpHeaders::HEADER_CONTENT_TYPE) == "Content-Type");
}

TEST_CASE("HttpHeaders - Session ID included when set", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.contentType = "application/json";
    ctx.sessionId = "unique-session-id-12345";
    
    HeaderList headers = HttpHeaders::buildHeaderList(ctx);
    
    auto findHeader = [&headers](const std::string& name) -> std::string {
        auto it = std::find_if(headers.begin(), headers.end(), 
            [&name](const Header& h) { return h.first == name; });
        return (it != headers.end()) ? it->second : "";
    };
    
    REQUIRE(findHeader("X-Session-Id") == "unique-session-id-12345");
}

TEST_CASE("HttpHeaders - Session ID omitted when empty", "[Protocol][HttpHeaders]") {
    RequestContext ctx;
    ctx.contentType = "application/json";
    ctx.sessionId = "";
    
    HeaderList headers = HttpHeaders::buildHeaderList(ctx);
    
    auto findHeader = [&headers](const std::string& name) -> bool {
        return std::find_if(headers.begin(), headers.end(), 
            [&name](const Header& h) { return h.first == name; }) != headers.end();
    };
    
    REQUIRE_FALSE(findHeader("X-Session-Id"));
}

