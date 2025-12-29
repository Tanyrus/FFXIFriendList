// ResponseDecoderTest.cpp
// Tests for response decoding

#include <catch2/catch_test_macros.hpp>
#include "Protocol/ResponseDecoder.h"
#include "Protocol/RequestEncoder.h"
#include "Protocol/JsonUtils.h"

using namespace XIFriendList::Protocol;

TEST_CASE("Decode response message", "[protocol][decoder]") {
    SECTION("Valid response with friends (server canonical format)") {
        // Server returns friends directly, not in payload
        std::string json = R"({"protocolVersion":"2.0.0","type":"FriendsListResponse","success":true,"friends":[]})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.protocolVersion == "2.0.0");
        REQUIRE(msg.type == ResponseType::FriendList);
        REQUIRE(msg.success == true);
        // Payload should be synthesized from friends as statuses format
        REQUIRE(msg.payload == "{\"statuses\":[]}");
    }
    
    SECTION("Response with error") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"Error","success":false,"error":"Invalid request"})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.success == false);
        REQUIRE(msg.error == "Invalid request");
    }
    
    SECTION("Invalid JSON") {
        std::string json = "{invalid json";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::InvalidJson);
    }
    
    SECTION("Missing protocolVersion") {
        std::string json = R"({"type":"FriendsListResponse","success":true})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Invalid version") {
        std::string json = R"({"protocolVersion":"99.0.0","type":"FriendsListResponse","success":true})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::InvalidVersion);
    }
    
    SECTION("Invalid type") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"InvalidType","success":true})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::InvalidType);
    }
}

TEST_CASE("Decode FriendList payload", "[protocol][decoder]") {
    SECTION("Canonical format with statuses array") {
        // Server returns statuses with characterName and friendedAs
        std::string payloadJson = R"({"statuses":[{"name":"user1","friendedAs":"User1"},{"name":"user2","friendedAs":"User2"}]})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.size() == 2);
        REQUIRE(payload.friendsData[0].name == "user1");
        REQUIRE(payload.friendsData[0].friendedAs == "User1");
        REQUIRE(payload.friendsData[1].name == "user2");
        REQUIRE(payload.friendsData[1].friendedAs == "User2");
    }
    
    SECTION("Full friend status data") {
        // Server returns full friend data including status fields
        std::string payloadJson = R"({"statuses":[{"name":"user1","friendedAs":"User1","isOnline":true,"job":"WHM 75"}]})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.size() == 1);
        REQUIRE(payload.friendsData[0].name == "user1");
        REQUIRE(payload.friendsData[0].friendedAs == "User1");
    }
    
    SECTION("Friend without friendedAs") {
        // friendedAs is optional (defaults to empty string)
        std::string payloadJson = R"({"statuses":[{"name":"friend1"}]})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.size() == 1);
        REQUIRE(payload.friendsData[0].name == "friend1");
        REQUIRE(payload.friendsData[0].friendedAs == "");  // Default empty
        REQUIRE(payload.friendsData[0].linkedCharacters.empty());
    }
    
    SECTION("Many friends") {
        std::string payloadJson = R"({"statuses":[{"name":"friend1"},{"name":"friend2"},{"name":"friend3"},{"name":"friend4"},{"name":"friend5"}]})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.size() == 5);
    }
    
    SECTION("Missing statuses (must fail with MissingField)") {
        std::string payloadJson = R"({})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("statuses wrong type - string instead of array (must fail with InvalidPayload)") {
        std::string payloadJson = R"({"statuses":"not an array"})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::InvalidPayload);
    }
    
    SECTION("statuses wrong type - object instead of array (must fail with InvalidPayload)") {
        std::string payloadJson = R"({"statuses":{"name":"friend1"}})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::InvalidPayload);
    }
    
    SECTION("Stringified JSON body scenario - double-encoded payload") {
        // Simulate server returning payload as JSON-encoded string
        std::string innerPayload = R"({"statuses":[{"name":"user1"}]})";
        std::string doubleEncoded = JsonUtils::encodeString(innerPayload);
        
        // decodeFriendListPayload should handle this by detecting the leading quote
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(doubleEncoded, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.size() == 1);
        REQUIRE(payload.friendsData[0].name == "user1");
    }
    
    SECTION("Empty statuses array") {
        std::string payloadJson = R"({"statuses":[]})";
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.empty());
    }
    
    SECTION("Empty statuses array as JSON string") {
        std::string payloadJsonString = R"({"statuses":[]})";
        std::string encodedPayload = JsonUtils::encodeString(payloadJsonString);
        
        std::string decodedPayload;
        REQUIRE(JsonUtils::decodeString(encodedPayload, decodedPayload));
        
        FriendListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeFriendListPayload(decodedPayload, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.friendsData.empty());
    }
}

TEST_CASE("Decode Status payload", "[protocol][decoder]") {
    // Server canonical format: uses "name" (not "charname") and "friendedAsName"
    std::string payloadJson = R"({"statuses":[{"name":"user1","friendedAsName":"User1","isOnline":true,"job":"WAR75","rank":"10","zone":"Bastok Markets"}]})";
    
    StatusResponsePayload payload;
    DecodeResult result = ResponseDecoder::decodeStatusPayload(payloadJson, payload);
    
    REQUIRE(result == DecodeResult::Success);
    REQUIRE(payload.statuses.size() == 1);
    REQUIRE(payload.statuses[0].characterName == "user1");
    REQUIRE(payload.statuses[0].displayName == "user1");  // Same as name in new format
    REQUIRE(payload.statuses[0].isOnline == true);
    REQUIRE(payload.statuses[0].job == "WAR75");
    REQUIRE(payload.statuses[0].friendedAs == "User1");
}

TEST_CASE("Decode FriendRequest payload", "[protocol][decoder]") {
    std::string payloadJson = R"({"requestId":"req123","fromCharacterName":"user1","toCharacterName":"user2","fromAccountId":1,"toAccountId":2,"status":"pending","createdAt":1234567890})";
    
    FriendRequestPayload payload;
    DecodeResult result = ResponseDecoder::decodeFriendRequestPayload(payloadJson, payload);
    
    REQUIRE(result == DecodeResult::Success);
    REQUIRE(payload.requestId == "req123");
    REQUIRE(payload.fromCharacterName == "user1");
    REQUIRE(payload.toCharacterName == "user2");
    REQUIRE(payload.fromAccountId == 1);
    REQUIRE(payload.toAccountId == 2);
    REQUIRE(payload.status == "pending");
    REQUIRE(payload.createdAt == 1234567890);
}

TEST_CASE("Decode FriendRequests payload", "[protocol][decoder]") {
    std::string payloadJson = R"({"incoming":[{"requestId":"req1","fromCharacterName":"user1","toCharacterName":"me","fromAccountId":1,"toAccountId":2,"status":"pending","createdAt":1000}],"outgoing":[{"requestId":"req2","fromCharacterName":"me","toCharacterName":"user2","fromAccountId":2,"toAccountId":3,"status":"pending","createdAt":2000}]})";
    
    FriendRequestsResponsePayload payload;
    DecodeResult result = ResponseDecoder::decodeFriendRequestsPayload(payloadJson, payload);
    
    REQUIRE(result == DecodeResult::Success);
    REQUIRE(payload.incoming.size() == 1);
    REQUIRE(payload.incoming[0].requestId == "req1");
    REQUIRE(payload.incoming[0].fromCharacterName == "user1");
    REQUIRE(payload.outgoing.size() == 1);
    REQUIRE(payload.outgoing[0].requestId == "req2");
    REQUIRE(payload.outgoing[0].toCharacterName == "user2");
}

TEST_CASE("Round-trip: Encode then decode", "[protocol][roundtrip]") {
    SECTION("GetFriendList request") {
        std::string encoded = RequestEncoder::encodeGetFriendList();
        
        // Verify it's valid JSON
        REQUIRE(JsonUtils::isValidJson(encoded));
        
        // Extract and verify fields
        std::string version;
        REQUIRE(JsonUtils::extractStringField(encoded, "protocolVersion", version));
        REQUIRE(version == PROTOCOL_VERSION);
        
        std::string type;
        REQUIRE(JsonUtils::extractStringField(encoded, "type", type));
        REQUIRE(type == "GetFriendList");
    }
}

TEST_CASE("Decode MailMessageData - server canonical format", "[protocol][decoder][mail]") {
    SECTION("Message with body (full mode)") {
        // Server uses fromName/toName/sentAt (new canonical format)
        std::string json = R"({"messageId":"msg1","fromName":"sender","toName":"recipient","subject":"Test","body":"Full body content","sentAt":1000,"isRead":false})";
        
        MailMessageData msg;
        DecodeResult result = ResponseDecoder::decodeMailMessageData(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.messageId == "msg1");
        REQUIRE(msg.fromUserId == "sender");
        REQUIRE(msg.toUserId == "recipient");
        REQUIRE(msg.body == "Full body content");
        REQUIRE(msg.createdAt == 1000);
    }
    
    SECTION("Message without body (meta mode)") {
        std::string json = R"({"messageId":"msg2","fromName":"sender","toName":"recipient","subject":"Test","sentAt":1000,"isRead":false})";
        
        MailMessageData msg;
        DecodeResult result = ResponseDecoder::decodeMailMessageData(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.messageId == "msg2");
        REQUIRE(msg.body.empty()); // Body should be empty (not present in meta mode)
    }
    
    SECTION("Message with empty body string") {
        std::string json = R"({"messageId":"msg3","fromName":"sender","toName":"recipient","subject":"Test","body":"","sentAt":1000,"isRead":false})";
        
        MailMessageData msg;
        DecodeResult result = ResponseDecoder::decodeMailMessageData(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.messageId == "msg3");
        REQUIRE(msg.body.empty());
    }
    
    SECTION("MailList with mixed body presence") {
        std::string payloadJson = R"({"messages":[
            {"messageId":"msg1","fromName":"sender1","toName":"recipient","subject":"Test1","body":"Body1","sentAt":1000,"isRead":false},
            {"messageId":"msg2","fromName":"sender2","toName":"recipient","subject":"Test2","sentAt":2000,"isRead":false}
        ]})";
        
        MailListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeMailListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.messages.size() == 2);
        REQUIRE(payload.messages[0].body == "Body1");
        REQUIRE(payload.messages[1].body.empty()); // Missing body in meta mode
    }
}

TEST_CASE("Decode NotesList payload", "[protocol][decoder][notes]") {
    SECTION("Payload as JSON object (direct)") {
        std::string payloadJson = R"({"notes":[{"friendName":"friend1","note":"Note 1","updatedAt":1000},{"friendName":"friend2","note":"Note 2","updatedAt":2000}]})";
        
        NotesListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotesListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.notes.size() == 2);
        REQUIRE(payload.notes[0].friendName == "friend1");
        REQUIRE(payload.notes[0].note == "Note 1");
        REQUIRE(payload.notes[0].updatedAt == 1000);
        REQUIRE(payload.notes[1].friendName == "friend2");
        REQUIRE(payload.notes[1].note == "Note 2");
        REQUIRE(payload.notes[1].updatedAt == 2000);
    }
    
    SECTION("Empty notes list") {
        std::string payloadJson = R"({"notes":[]})";
        
        NotesListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotesListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.notes.empty());
    }
    
    SECTION("Missing notes field") {
        std::string payloadJson = R"({})";
        
        NotesListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotesListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Invalid notes array") {
        std::string payloadJson = R"({"notes":"not an array"})";
        
        NotesListResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotesListPayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::InvalidPayload);
    }
}

TEST_CASE("Decode Note payload", "[protocol][decoder][notes]") {
    SECTION("Valid note payload") {
        std::string payloadJson = R"({"note":{"friendName":"testfriend","note":"Test note text","updatedAt":1234567890}})";
        
        NoteResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotePayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.note.friendName == "testfriend");
        REQUIRE(payload.note.note == "Test note text");
        REQUIRE(payload.note.updatedAt == 1234567890);
    }
    
    SECTION("Empty note") {
        std::string payloadJson = R"({"note":{"friendName":"testfriend","note":"","updatedAt":1000}})";
        
        NoteResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotePayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(payload.note.friendName == "testfriend");
        REQUIRE(payload.note.note.empty());
        REQUIRE(payload.note.updatedAt == 1000);
    }
    
    SECTION("Missing note field") {
        std::string payloadJson = R"({})";
        
        NoteResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotePayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Missing required fields in note object") {
        std::string payloadJson = R"({"note":{"friendName":"testfriend"}})";
        
        NoteResponsePayload payload;
        DecodeResult result = ResponseDecoder::decodeNotePayload(payloadJson, payload);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
}

TEST_CASE("Decode NoteData", "[protocol][decoder][notes]") {
    SECTION("Valid note data") {
        std::string json = R"({"friendName":"testfriend","note":"Test note","updatedAt":1234567890})";
        
        NoteData note;
        DecodeResult result = ResponseDecoder::decodeNoteData(json, note);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(note.friendName == "testfriend");
        REQUIRE(note.note == "Test note");
        REQUIRE(note.updatedAt == 1234567890);
    }
    
    SECTION("Missing friendName") {
        std::string json = R"({"note":"Test note","updatedAt":1234567890})";
        
        NoteData note;
        DecodeResult result = ResponseDecoder::decodeNoteData(json, note);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Missing note field") {
        std::string json = R"({"friendName":"testfriend","updatedAt":1234567890})";
        
        NoteData note;
        DecodeResult result = ResponseDecoder::decodeNoteData(json, note);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Missing updatedAt") {
        std::string json = R"({"friendName":"testfriend","note":"Test note"})";
        
        NoteData note;
        DecodeResult result = ResponseDecoder::decodeNoteData(json, note);
        
        REQUIRE(result == DecodeResult::MissingField);
    }
    
    SECTION("Special characters in note") {
        std::string json = R"({"friendName":"testfriend","note":"Note with \"quotes\" and\nnewlines","updatedAt":1000})";
        
        NoteData note;
        DecodeResult result = ResponseDecoder::decodeNoteData(json, note);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(note.note == "Note with \"quotes\" and\nnewlines");
    }
}

TEST_CASE("Decode FeedbackResponse", "[protocol][decoder][support]") {
    SECTION("Valid feedback response") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"FeedbackResponse","success":true,"feedbackId":123})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.type == ResponseType::FeedbackResponse);
        REQUIRE(msg.success == true);
        
        FeedbackResponsePayload payload;
        DecodeResult payloadResult = ResponseDecoder::decodeFeedbackResponsePayload(json, payload);
        REQUIRE(payloadResult == DecodeResult::Success);
        REQUIRE(payload.feedbackId == 123);
    }
}

TEST_CASE("Decode IssueResponse", "[protocol][decoder][support]") {
    SECTION("Valid issue response") {
        std::string json = R"({"protocolVersion":"2.0.0","type":"IssueResponse","success":true,"issueId":456})";
        
        ResponseMessage msg;
        DecodeResult result = ResponseDecoder::decode(json, msg);
        
        REQUIRE(result == DecodeResult::Success);
        REQUIRE(msg.type == ResponseType::IssueResponse);
        REQUIRE(msg.success == true);
        
        IssueResponsePayload payload;
        DecodeResult payloadResult = ResponseDecoder::decodeIssueResponsePayload(json, payload);
        REQUIRE(payloadResult == DecodeResult::Success);
        REQUIRE(payload.issueId == 456);
    }
}

