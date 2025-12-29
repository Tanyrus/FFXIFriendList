#include <catch2/catch_test_macros.hpp>
#include "Protocol/MessageTypes.h"
#include <string>

using namespace XIFriendList::Protocol;

TEST_CASE("RequestType - String conversion", "[protocol]") {
    SECTION("all RequestType enum values convert correctly") {
        REQUIRE(requestTypeToString(RequestType::GetFriendList) == "GetFriendList");
        REQUIRE(requestTypeToString(RequestType::SetFriendList) == "SetFriendList");
        REQUIRE(requestTypeToString(RequestType::GetStatus) == "GetStatus");
        REQUIRE(requestTypeToString(RequestType::UpdatePresence) == "UpdatePresence");
        REQUIRE(requestTypeToString(RequestType::SendFriendRequest) == "SendFriendRequest");
        REQUIRE(requestTypeToString(RequestType::GetNotes) == "GetNotes");
        REQUIRE(requestTypeToString(RequestType::PutNote) == "PutNote");
        REQUIRE(requestTypeToString(RequestType::SetActiveCharacter) == "SetActiveCharacter");
    }
    
    SECTION("invalid enum handling") {
        RequestType out;
        REQUIRE(stringToRequestType("InvalidType", out) == false);
    }
    
    SECTION("round-trip conversion") {
        RequestType original = RequestType::GetFriendList;
        std::string str = requestTypeToString(original);
        RequestType converted;
        REQUIRE(stringToRequestType(str, converted) == true);
        REQUIRE(converted == original);
    }
}

TEST_CASE("ResponseType - String conversion", "[protocol]") {
    SECTION("all ResponseType enum values convert correctly") {
        REQUIRE(responseTypeToString(ResponseType::FriendList) == "FriendList");
        REQUIRE(responseTypeToString(ResponseType::Status) == "Status");
        REQUIRE(responseTypeToString(ResponseType::Presence) == "Presence");
        REQUIRE(responseTypeToString(ResponseType::FriendRequest) == "FriendRequest");
        REQUIRE(responseTypeToString(ResponseType::Error) == "Error");
        REQUIRE(responseTypeToString(ResponseType::Success) == "Success");
        REQUIRE(responseTypeToString(ResponseType::NotesList) == "NotesList");
    }
    
    SECTION("invalid enum handling") {
        ResponseType out;
        REQUIRE(stringToResponseType("InvalidType", out) == false);
    }
    
    SECTION("multiple string mappings") {
        ResponseType out;
        REQUIRE(stringToResponseType("FriendsListResponse", out) == true);
        REQUIRE(out == ResponseType::FriendList);
        REQUIRE(stringToResponseType("Error", out) == true);
        REQUIRE(out == ResponseType::Error);
    }
}

