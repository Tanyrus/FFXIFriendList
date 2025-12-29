// MessageValidatorTest.cpp
// Tests for message validation

#include <catch2/catch_test_macros.hpp>
#include "Protocol/MessageValidator.h"
#include "Protocol/MessageTypes.h"

using namespace XIFriendList::Protocol;

TEST_CASE("Validate request message", "[protocol][validator]") {
    SECTION("Valid request") {
        RequestMessage msg;
        msg.protocolVersion = PROTOCOL_VERSION;
        msg.type = RequestType::GetFriendList;
        msg.payload = "{}";
        
        ValidationResult result = MessageValidator::validateRequest(msg);
        REQUIRE(result == ValidationResult::Valid);
    }
    
    SECTION("Invalid version") {
        RequestMessage msg;
        msg.protocolVersion = "99.0.0";  // Clearly invalid version
        msg.type = RequestType::GetFriendList;
        msg.payload = "{}";
        
        ValidationResult result = MessageValidator::validateRequest(msg);
        REQUIRE(result == ValidationResult::InvalidVersion);
    }
    
    SECTION("Invalid type") {
        RequestMessage msg;
        msg.protocolVersion = PROTOCOL_VERSION;
        msg.type = static_cast<RequestType>(999);
        msg.payload = "{}";
        
        ValidationResult result = MessageValidator::validateRequest(msg);
        REQUIRE(result == ValidationResult::InvalidType);
    }
}

TEST_CASE("Validate response message", "[protocol][validator]") {
    SECTION("Valid response") {
        ResponseMessage msg;
        msg.protocolVersion = PROTOCOL_VERSION;
        msg.type = ResponseType::FriendList;
        msg.success = true;
        msg.payload = "{}";
        
        ValidationResult result = MessageValidator::validateResponse(msg);
        REQUIRE(result == ValidationResult::Valid);
    }
    
    SECTION("Invalid version") {
        ResponseMessage msg;
        msg.protocolVersion = "99.0.0";  // Clearly invalid version
        msg.type = ResponseType::FriendList;
        msg.success = true;
        
        ValidationResult result = MessageValidator::validateResponse(msg);
        REQUIRE(result == ValidationResult::InvalidVersion);
    }
}

TEST_CASE("Validate character name", "[protocol][validator]") {
    SECTION("Valid names") {
        REQUIRE(MessageValidator::validateCharacterName("TestUser") == ValidationResult::Valid);
        REQUIRE(MessageValidator::validateCharacterName("Test User") == ValidationResult::Valid);
        REQUIRE(MessageValidator::validateCharacterName("Test-User") == ValidationResult::Valid);
        REQUIRE(MessageValidator::validateCharacterName("Test_User") == ValidationResult::Valid);
    }
    
    SECTION("Invalid names") {
        REQUIRE(MessageValidator::validateCharacterName("") == ValidationResult::MissingRequiredField);
        REQUIRE(MessageValidator::validateCharacterName("Test@User") == ValidationResult::InvalidFieldValue);
        REQUIRE(MessageValidator::validateCharacterName("Test.User") == ValidationResult::InvalidFieldValue);
    }
    
    SECTION("Name too long") {
        std::string longName(17, 'a');
        REQUIRE(MessageValidator::validateCharacterName(longName) == ValidationResult::InvalidFieldValue);
    }
}

TEST_CASE("Validate friend list size", "[protocol][validator]") {
    SECTION("Valid sizes") {
        REQUIRE(MessageValidator::validateFriendListSize(0) == ValidationResult::Valid);
        REQUIRE(MessageValidator::validateFriendListSize(100) == ValidationResult::Valid);
        REQUIRE(MessageValidator::validateFriendListSize(1000) == ValidationResult::Valid);
    }
    
    SECTION("Invalid size") {
        REQUIRE(MessageValidator::validateFriendListSize(1001) == ValidationResult::InvalidFieldValue);
    }
}

TEST_CASE("Get error message", "[protocol][validator]") {
    REQUIRE_FALSE(MessageValidator::getErrorMessage(ValidationResult::InvalidVersion).empty());
    REQUIRE_FALSE(MessageValidator::getErrorMessage(ValidationResult::InvalidType).empty());
    REQUIRE_FALSE(MessageValidator::getErrorMessage(ValidationResult::MissingRequiredField).empty());
}

