// RequestEncoderTest.cpp
// Tests for request encoding

#include <catch2/catch_test_macros.hpp>
#include "Protocol/RequestEncoder.h"
#include "Protocol/ResponseDecoder.h"
#include "Core/FriendsCore.h"
#include "Core/ModelsCore.h"
#include "Protocol/JsonUtils.h"

using namespace XIFriendList::Protocol;
using namespace XIFriendList::Core;

TEST_CASE("Encode GetFriendList", "[protocol][encoder]") {
    std::string json = RequestEncoder::encodeGetFriendList();
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    // Verify it contains protocolVersion
    std::string version;
    REQUIRE(JsonUtils::extractStringField(json, "protocolVersion", version));
    REQUIRE(version == PROTOCOL_VERSION);
    
    // Verify type
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "GetFriendList");
}

TEST_CASE("Encode SetFriendList", "[protocol][encoder]") {
    std::vector<Friend> friends;
    friends.push_back(Friend("testuser", "TestUser"));
    friends.push_back(Friend("anotheruser", "AnotherUser"));
    
    std::string json = RequestEncoder::encodeSetFriendList(friends);
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    // Verify payload contains statuses array (canonical format)
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    // Extract statuses array (array of objects)
    std::string statusesArray;
    REQUIRE(JsonUtils::extractField(payload, "statuses", statusesArray));
    REQUIRE(statusesArray[0] == '[');
    
    // Parse the array to verify it contains the expected friends
    REQUIRE(statusesArray.find("\"name\":\"testuser\"") != std::string::npos);
    REQUIRE(statusesArray.find("\"name\":\"anotheruser\"") != std::string::npos);
}

TEST_CASE("Encode GetStatus", "[protocol][encoder]") {
    std::string json = RequestEncoder::encodeGetStatus("testuser");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string characterName;
    REQUIRE(JsonUtils::extractStringField(payload, "characterName", characterName));
    REQUIRE(characterName == "testuser");
}

TEST_CASE("Encode UpdatePresence", "[protocol][encoder]") {
    Presence presence;
    presence.characterName = "testuser";
    presence.job = "WAR75";
    presence.rank = "10";
    presence.nation = 2;
    presence.zone = "Bastok Markets";
    presence.isAnonymous = false;
    presence.timestamp = 1234567890;
    
    std::string json = RequestEncoder::encodeUpdatePresence(presence);
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string characterName;
    REQUIRE(JsonUtils::extractStringField(payload, "characterName", characterName));
    REQUIRE(characterName == "testuser");
    
    std::string job;
    REQUIRE(JsonUtils::extractStringField(payload, "job", job));
    REQUIRE(job == "WAR75");
    
    int nation = 0;
    REQUIRE(JsonUtils::extractNumberField(payload, "nation", nation));
    REQUIRE(nation == 2);
    
    bool isAnonymous = false;
    REQUIRE(JsonUtils::extractBooleanField(payload, "isAnonymous", isAnonymous));
    REQUIRE_FALSE(isAnonymous);
}

TEST_CASE("Encode SendFriendRequest", "[protocol][encoder]") {
    std::string json = RequestEncoder::encodeSendFriendRequest("targetuser");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string toUserId;
    REQUIRE(JsonUtils::extractStringField(payload, "toUserId", toUserId));
    REQUIRE(toUserId == "targetuser");
}

TEST_CASE("Encode AcceptFriendRequest", "[protocol][encoder]") {
    std::string json = RequestEncoder::encodeAcceptFriendRequest("req123");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string requestId;
    REQUIRE(JsonUtils::extractStringField(payload, "requestId", requestId));
    REQUIRE(requestId == "req123");
}

TEST_CASE("Encode GetHeartbeat", "[protocol][encoder]") {
    std::string json = RequestEncoder::encodeGetHeartbeat("testuser", 1000, 2000);
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string characterName;
    REQUIRE(JsonUtils::extractStringField(payload, "characterName", characterName));
    REQUIRE(characterName == "testuser");
    
    uint64_t lastEventTimestamp = 0;
    REQUIRE(JsonUtils::extractNumberField(payload, "lastEventTimestamp", lastEventTimestamp));
    REQUIRE(lastEventTimestamp == 1000);
    
    uint64_t lastRequestEventTimestamp = 0;
    REQUIRE(JsonUtils::extractNumberField(payload, "lastRequestEventTimestamp", lastRequestEventTimestamp));
    REQUIRE(lastRequestEventTimestamp == 2000);

    // Heartbeat must be minimal (alive-only) and must NOT include presence fields.
    REQUIRE(payload.find("\"job\"") == std::string::npos);
    REQUIRE(payload.find("\"rank\"") == std::string::npos);
    REQUIRE(payload.find("\"nation\"") == std::string::npos);
    REQUIRE(payload.find("\"zone\"") == std::string::npos);
    REQUIRE(payload.find("\"isAnonymous\"") == std::string::npos);
}

TEST_CASE("Encode UpdatePresence includes full presence fields", "[protocol][encoder]") {
    Presence presence;
    presence.characterName = "testuser";
    presence.job = "WAR75";
    presence.rank = "10";
    presence.nation = 2;
    presence.zone = "Bastok Markets";
    presence.isAnonymous = true;
    presence.timestamp = 1234567890;

    std::string json = RequestEncoder::encodeUpdatePresence(presence);
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));

    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));

    // Required full-sync fields
    std::string characterName;
    REQUIRE(JsonUtils::extractStringField(payload, "characterName", characterName));
    REQUIRE(characterName == "testuser");

    std::string job;
    REQUIRE(JsonUtils::extractStringField(payload, "job", job));
    REQUIRE(job == "WAR75");

    std::string rank;
    REQUIRE(JsonUtils::extractStringField(payload, "rank", rank));
    REQUIRE(rank == "10");

    int nation = 0;
    REQUIRE(JsonUtils::extractNumberField(payload, "nation", nation));
    REQUIRE(nation == 2);

    std::string zone;
    REQUIRE(JsonUtils::extractStringField(payload, "zone", zone));
    REQUIRE(zone == "Bastok Markets");

    bool isAnonymous = false;
    REQUIRE(JsonUtils::extractBooleanField(payload, "isAnonymous", isAnonymous));
    REQUIRE(isAnonymous == true);

    uint64_t timestamp = 0;
    REQUIRE(JsonUtils::extractNumberField(payload, "timestamp", timestamp));
    REQUIRE(timestamp == 1234567890);
}

TEST_CASE("Encode GetNotes", "[protocol][encoder][notes]") {
    std::string json = RequestEncoder::encodeGetNotes();
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "GetNotes");
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    REQUIRE(payload == "{}");
}

TEST_CASE("Encode GetNote", "[protocol][encoder][notes]") {
    std::string json = RequestEncoder::encodeGetNote("testfriend");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "GetNote");
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string friendName;
    REQUIRE(JsonUtils::extractStringField(payload, "friendName", friendName));
    REQUIRE(friendName == "testfriend");
}

TEST_CASE("Encode PutNote", "[protocol][encoder][notes]") {
    std::string json = RequestEncoder::encodePutNote("testfriend", "This is a test note");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "PutNote");
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string friendName;
    REQUIRE(JsonUtils::extractStringField(payload, "friendName", friendName));
    REQUIRE(friendName == "testfriend");
    
    std::string note;
    REQUIRE(JsonUtils::extractStringField(payload, "note", note));
    REQUIRE(note == "This is a test note");
}

TEST_CASE("Encode DeleteNote", "[protocol][encoder][notes]") {
    std::string json = RequestEncoder::encodeDeleteNote("testfriend");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "DeleteNote");
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string friendName;
    REQUIRE(JsonUtils::extractStringField(payload, "friendName", friendName));
    REQUIRE(friendName == "testfriend");
}

TEST_CASE("Encode SubmitFeedback", "[protocol][encoder][support]") {
    std::string json = RequestEncoder::encodeSubmitFeedback("Test Subject", "Test feedback message");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string subject, message;
    REQUIRE(JsonUtils::extractStringField(payload, "subject", subject));
    REQUIRE(JsonUtils::extractStringField(payload, "message", message));
    REQUIRE(subject == "Test Subject");
    REQUIRE(message == "Test feedback message");
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "SubmitFeedback");
}

TEST_CASE("Encode SubmitIssue", "[protocol][encoder][support]") {
    std::string json = RequestEncoder::encodeSubmitIssue("Bug Report", "This is a bug description");
    
    REQUIRE_FALSE(json.empty());
    REQUIRE(JsonUtils::isValidJson(json));
    
    std::string payload;
    REQUIRE(JsonUtils::extractField(json, "payload", payload));
    
    std::string subject, message;
    REQUIRE(JsonUtils::extractStringField(payload, "subject", subject));
    REQUIRE(JsonUtils::extractStringField(payload, "message", message));
    REQUIRE(subject == "Bug Report");
    REQUIRE(message == "This is a bug description");
    
    std::string type;
    REQUIRE(JsonUtils::extractStringField(json, "type", type));
    REQUIRE(type == "SubmitIssue");
}

