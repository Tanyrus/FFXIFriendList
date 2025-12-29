// MalformedInputTest.cpp
// Tests for handling malformed input gracefully

#include <catch2/catch_test_macros.hpp>
#include "Protocol/ResponseDecoder.h"
#include "Protocol/MessageValidator.h"
#include "Protocol/JsonUtils.h"

using namespace XIFriendList::Protocol;

TEST_CASE("Malformed JSON handling", "[protocol][malformed]") {
    SECTION("Empty string") {
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode("", msg);
        REQUIRE(result == DecodeResult::InvalidJson);
    }
    
    SECTION("Invalid JSON structure") {
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode("{invalid}", msg);
        REQUIRE(result == DecodeResult::InvalidJson);
    }
    
    SECTION("Unclosed string") {
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(R"({"protocolVersion":"unclosed)", msg);
        REQUIRE(result == DecodeResult::InvalidJson);
    }
    
    SECTION("Unclosed object") {
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(R"({"protocolVersion":"2.0.0")", msg);
        REQUIRE(result == DecodeResult::InvalidJson);
    }
    
    SECTION("Unclosed array") {
        std::string json = R"({"friends":["user1")";
        std::vector<std::string> arr;
        REQUIRE_FALSE(JsonUtils::decodeStringArray(json, arr));
    }
}

TEST_CASE("Missing required fields", "[protocol][malformed]") {
    SECTION("Missing protocolVersion") {
        std::string json = R"({"type":"FriendsListResponse","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Missing type") {
        std::string json = R"({"protocolVersion":"2.0.0","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Missing success") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse"})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::MissingField);
    }
}

TEST_CASE("Invalid field types", "[protocol][malformed]") {
    SECTION("protocolVersion is not a string") {
        std::string json = R"({"protocolVersion":1.0.0,"type":"FriendsListResponse","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("type is not a string") {
        std::string json = R"({"protocolVersion":"2.0.0","type":123,"success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::InvalidType);
    }
    
    SECTION("success is not a boolean") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":"true"})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::MissingField);
    }
}

TEST_CASE("Invalid version formats", "[protocol][malformed]") {
    SECTION("Non-numeric version") {
        std::string json = R"({"protocolVersion":"a.b.c","type":"FriendsListResponse","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::InvalidVersion);
    }
    
    SECTION("Incompatible major version") {
        std::string json = R"({"protocolVersion":"99.0.0","type":"FriendsListResponse","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::InvalidVersion);
    }
    
    SECTION("Empty version") {
        std::string json = R"({"protocolVersion":"","type":"FriendsListResponse","success":true})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::InvalidVersion);
    }
}

TEST_CASE("Invalid payload structures", "[protocol][malformed]") {
    SECTION("Invalid FriendList payload - non-canonical friends field (rejected)") {
        std::string payload = R"({"friends":["user1","user2"]})";
        FriendListResponsePayload out;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payload, out);
        // Non-canonical "friends" field format is rejected (canonical format requires statuses)
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Invalid FriendList payload - missing statuses field") {
        std::string payload = R"({})";
        FriendListResponsePayload out;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payload, out);
        // Missing statuses field (canonical format)
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Invalid Status payload - missing name field") {
        // Server uses "name" (not "charname") in canonical format
        std::string payload = R"({"statuses":[{"isOnline":true}]})";
        StatusResponsePayload out;
        DecodeResult result = ResponseDecoder::decodeStatusPayload(payload, out);
        // Should handle gracefully - name is required but decoder should return MissingField
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Invalid FriendRequest payload - missing requestId") {
        std::string payload = R"({"fromCharacterName":"user1"})";
        FriendRequestPayload out;
        DecodeResult result = ResponseDecoder::decodeFriendRequestPayload(payload, out);
        REQUIRE(result == DecodeResult::MissingField);
    }
}

TEST_CASE("Extremely large payloads", "[protocol][malformed]") {
    SECTION("Payload too large") {
        RequestMessage msg;
        msg.protocolVersion = PROTOCOL_VERSION;
        msg.type = RequestType::GetFriendList;
        msg.payload = std::string(2 * 1024 * 1024, 'a');  // 2MB
        
        ValidationResult result = MessageValidator::validateRequest(msg);
        REQUIRE(result == ValidationResult::PayloadTooLarge);
    }
}

TEST_CASE("Nested malformed structures", "[protocol][malformed]") {
    SECTION("Malformed nested object in array") {
        // Server uses "name" (not "charname") in canonical format
        std::string payload = R"({"statuses":[{"name":"user1","invalid"}]})";
        StatusResponsePayload out;
        DecodeResult result = ResponseDecoder::decodeStatusPayload(payload, out);
        // Should handle gracefully
        REQUIRE((result == DecodeResult::Success || result == DecodeResult::InvalidPayload));
    }
    
    SECTION("Valid nested object") {
        // Server uses "name" in canonical format
        std::string payload = R"({"statuses":[{"name":"user1"}]})";
        StatusResponsePayload out;
        DecodeResult result = ResponseDecoder::decodeStatusPayload(payload, out);
        // Should succeed with valid structure
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(out.statuses.size() == 1);
        REQUIRE(out.statuses[0].characterName == "user1");
    }
}

TEST_CASE("Special characters in strings", "[protocol][malformed]") {
    SECTION("Escaped characters") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Error with \"quotes\""})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.error == "Error with \"quotes\"");
    }
    
    SECTION("Newlines in strings") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Error\nwith\nnewlines"})";
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        REQUIRE(result == DecodeResult::Success);
    }
}

