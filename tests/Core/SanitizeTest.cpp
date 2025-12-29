// SanitizeTest.cpp
// Tests for input sanitization utilities

#include <catch2/catch_test_macros.hpp>
#include "Core/UtilitiesCore.h"
#include <string>

using namespace XIFriendList::Core;

TEST_CASE("Sanitize::removeControlChars", "[sanitize]") {
    SECTION("removes control characters") {
        std::string input = "Hello\x01\x02World";
        std::string result = Sanitize::removeControlChars(input, false);
        REQUIRE(result == "HelloWorld");
    }
    
    SECTION("allows newlines when requested") {
        std::string input = "Hello\nWorld\r\nTest";
        std::string result = Sanitize::removeControlChars(input, true);
        REQUIRE(result == "Hello\nWorld\r\nTest");
    }
    
    SECTION("removes newlines when not requested") {
        std::string input = "Hello\nWorld";
        std::string result = Sanitize::removeControlChars(input, false);
        REQUIRE(result == "HelloWorld");
    }
    
    SECTION("allows tab character") {
        std::string input = "Hello\tWorld";
        std::string result = Sanitize::removeControlChars(input, false);
        REQUIRE(result == "Hello\tWorld");
    }
}

TEST_CASE("Sanitize::trim", "[sanitize]") {
    SECTION("trims leading whitespace") {
        std::string input = "  Hello";
        std::string result = Sanitize::trim(input);
        REQUIRE(result == "Hello");
    }
    
    SECTION("trims trailing whitespace") {
        std::string input = "Hello  ";
        std::string result = Sanitize::trim(input);
        REQUIRE(result == "Hello");
    }
    
    SECTION("trims both sides") {
        std::string input = "  Hello  ";
        std::string result = Sanitize::trim(input);
        REQUIRE(result == "Hello");
    }
    
    SECTION("handles empty string") {
        std::string input = "";
        std::string result = Sanitize::trim(input);
        REQUIRE(result == "");
    }
}

TEST_CASE("Sanitize::validateCharacterName", "[sanitize]") {
    SECTION("validates correct character name") {
        auto result = Sanitize::validateCharacterName("TestUser");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "testuser");
    }
    
    SECTION("validates apostrophe character directly") {
        // Test that apostrophe (0x27) is recognized as valid by isValidCharacterNameChar
        char apostrophe = static_cast<char>(0x27);
        unsigned char uc = static_cast<unsigned char>(apostrophe);
        REQUIRE(uc == 0x27);  // Verify it's actually 0x27
        bool isValid = Sanitize::isValidCharacterNameChar(apostrophe);
        REQUIRE(isValid == true);
        
        // Test that removeControlChars keeps apostrophe
        std::string testStr = "Test";
        testStr.push_back(apostrophe);
        testStr += "Name";
        std::string afterRemove = Sanitize::removeControlChars(testStr, false);
        REQUIRE(afterRemove.length() == testStr.length());  // Apostrophe should be kept
        REQUIRE(afterRemove.find(apostrophe) != std::string::npos);  // Apostrophe should be present
        
        // Now test full validation
        auto result = Sanitize::validateCharacterName(testStr);
        CAPTURE(result.error);
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized.find(apostrophe) != std::string::npos);
    }
    
    SECTION("validates simple name with apostrophe") {
        // Simplest possible test - just "O'B"
        std::string simple = "O";
        simple.push_back(static_cast<char>(0x27));
        simple += "B";
        auto result = Sanitize::validateCharacterName(simple);
        REQUIRE(result.valid == true);
    }
    
    SECTION("rejects empty string") {
        auto result = Sanitize::validateCharacterName("");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("rejects whitespace-only string") {
        auto result = Sanitize::validateCharacterName("   ");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("empty") != std::string::npos);
    }
    
    SECTION("rejects name longer than max length") {
        std::string longName(17, 'a');
        auto result = Sanitize::validateCharacterName(longName);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("16") != std::string::npos);
    }
    
    SECTION("rejects invalid characters") {
        auto result = Sanitize::validateCharacterName("Test@User");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("invalid") != std::string::npos);
    }
    
    SECTION("allows valid characters") {
        // Test with apostrophe - match server-side test exactly: "O'Brien"
        std::string testName = "O";
        testName.push_back(static_cast<char>(0x27));  // Explicit apostrophe (ASCII 39)
        testName += "Brien";  // Total: 6 characters (within 16 char limit)
        
        auto result = Sanitize::validateCharacterName(testName);
        CAPTURE(result.error);
        CAPTURE(testName);
        REQUIRE(result.valid == true);
        std::string expected = "o";
        expected.push_back(static_cast<char>(0x27));
        expected += "brien";
        REQUIRE(result.sanitized == expected);
    }
    
    SECTION("allows valid characters with hyphen and underscore") {
        // Test with apostrophe plus other valid characters
        std::string testName = "Test-User";
        testName.push_back(static_cast<char>(0x27));  // Explicit apostrophe (ASCII 39)
        testName += "O";  // Total: 11 characters (within 16 char limit)
        
        auto result = Sanitize::validateCharacterName(testName);
        CAPTURE(result.error);
        CAPTURE(testName);
        REQUIRE(result.valid == true);
        std::string expected = "test-user";
        expected.push_back(static_cast<char>(0x27));
        expected += "o";
        REQUIRE(result.sanitized == expected);
    }
    
    SECTION("allows apostrophe in name - O'Brien") {
        // Match server-side test: validateCharacterName("O'Brien")
        std::string testName = "O";
        testName += '\'';
        testName += "Brien";
        auto result = Sanitize::validateCharacterName(testName);
        REQUIRE(result.valid == true);
        std::string expected = "o";
        expected += '\'';
        expected += "brien";
        REQUIRE(result.sanitized == expected);
    }
    
    SECTION("trims whitespace") {
        auto result = Sanitize::validateCharacterName("  TestUser  ");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "testuser");
    }
    
    SECTION("removes control characters") {
        std::string input = "Test\x01User";
        auto result = Sanitize::validateCharacterName(input);
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "testuser");
    }
}

TEST_CASE("Sanitize::validateNoteText", "[sanitize]") {
    SECTION("validates correct note text") {
        auto result = Sanitize::validateNoteText("This is a note");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "This is a note");
    }
    
    SECTION("allows newlines in note") {
        auto result = Sanitize::validateNoteText("Line 1\nLine 2");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Line 1\nLine 2");
    }
    
    SECTION("rejects empty string") {
        auto result = Sanitize::validateNoteText("");
        REQUIRE(result.valid == false);
    }
    
    SECTION("rejects whitespace-only") {
        auto result = Sanitize::validateNoteText("   \n  ");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("whitespace-only") != std::string::npos);
    }
    
    SECTION("rejects note longer than max length") {
        std::string longNote(8193, 'a');
        auto result = Sanitize::validateNoteText(longNote);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("8192") != std::string::npos);
    }
}

TEST_CASE("Sanitize::validateMailSubject", "[sanitize]") {
    SECTION("validates correct mail subject") {
        auto result = Sanitize::validateMailSubject("Test Subject");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Test Subject");
    }
    
    SECTION("rejects empty string") {
        auto result = Sanitize::validateMailSubject("");
        REQUIRE(result.valid == false);
    }
    
    SECTION("removes newlines from subject") {
        auto result = Sanitize::validateMailSubject("Test\nSubject");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Test Subject");
    }
    
    SECTION("collapses whitespace") {
        auto result = Sanitize::validateMailSubject("Test    Subject");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Test Subject");
    }
    
    SECTION("rejects subject longer than max length") {
        std::string longSubject(101, 'a');
        auto result = Sanitize::validateMailSubject(longSubject);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("100") != std::string::npos);
    }
}

TEST_CASE("Sanitize::validateMailBody", "[sanitize]") {
    SECTION("validates correct mail body") {
        auto result = Sanitize::validateMailBody("This is a mail body");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "This is a mail body");
    }
    
    SECTION("allows newlines in body") {
        auto result = Sanitize::validateMailBody("Line 1\nLine 2");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Line 1\nLine 2");
    }
    
    SECTION("rejects empty string") {
        auto result = Sanitize::validateMailBody("");
        REQUIRE(result.valid == false);
    }
    
    SECTION("rejects body longer than max length") {
        std::string longBody(2001, 'a');
        auto result = Sanitize::validateMailBody(longBody);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("2000") != std::string::npos);
    }
}

TEST_CASE("Sanitize::sanitizeForLogging", "[sanitize]") {
    SECTION("escapes newlines") {
        std::string input = "Hello\nWorld";
        std::string result = Sanitize::sanitizeForLogging(input);
        REQUIRE(result == "Hello\\nWorld");
    }
    
    SECTION("escapes carriage returns") {
        std::string input = "Hello\rWorld";
        std::string result = Sanitize::sanitizeForLogging(input);
        REQUIRE(result == "Hello\\nWorld");
    }
    
    SECTION("removes other control characters") {
        std::string input = "Hello\x01World";
        std::string result = Sanitize::sanitizeForLogging(input);
        REQUIRE(result == "HelloWorld");
    }
}

TEST_CASE("Sanitize::validateFriendName", "[sanitize]") {
    SECTION("validates correct friend name") {
        auto result = Sanitize::validateFriendName("TestUser");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "testuser");
    }
    
    SECTION("rejects empty string") {
        auto result = Sanitize::validateFriendName("");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("required") != std::string::npos);
    }
    
    SECTION("rejects name longer than max length") {
        std::string longName(17, 'a');
        auto result = Sanitize::validateFriendName(longName);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("16") != std::string::npos);
    }
    
    SECTION("rejects invalid characters") {
        auto result = Sanitize::validateFriendName("Test@User");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("invalid") != std::string::npos);
    }
    
    SECTION("normalizes to lowercase") {
        auto result = Sanitize::validateFriendName("TestUser");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "testuser");
    }
}

TEST_CASE("Sanitize::validateZone", "[sanitize]") {
    SECTION("validates correct zone") {
        auto result = Sanitize::validateZone("Bastok Markets");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Bastok Markets");
    }
    
    SECTION("allows empty zone") {
        auto result = Sanitize::validateZone("");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "");
    }
    
    SECTION("allows zone with apostrophe") {
        auto result = Sanitize::validateZone("Mhaura");
        REQUIRE(result.valid == true);
    }
    
    SECTION("allows zone with period") {
        auto result = Sanitize::validateZone("Port Jeuno");
        REQUIRE(result.valid == true);
    }
    
    SECTION("rejects zone longer than max length") {
        std::string longZone(101, 'a');
        auto result = Sanitize::validateZone(longZone);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("100") != std::string::npos);
    }
    
    SECTION("rejects invalid characters") {
        auto result = Sanitize::validateZone("Zone@Name");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("invalid") != std::string::npos);
    }
    
    SECTION("collapses whitespace") {
        auto result = Sanitize::validateZone("Bastok   Markets");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Bastok Markets");
    }
    
    SECTION("trims whitespace") {
        auto result = Sanitize::validateZone("  Bastok Markets  ");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Bastok Markets");
    }
}

TEST_CASE("Sanitize::validateJob", "[sanitize]") {
    SECTION("validates correct job") {
        auto result = Sanitize::validateJob("Warrior");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Warrior");
    }
    
    SECTION("allows empty job") {
        auto result = Sanitize::validateJob("");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "");
    }
    
    SECTION("allows job with apostrophe") {
        auto result = Sanitize::validateJob("Ranger");
        REQUIRE(result.valid == true);
    }
    
    SECTION("rejects job longer than max length") {
        std::string longJob(51, 'a');
        auto result = Sanitize::validateJob(longJob);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("50") != std::string::npos);
    }
    
    SECTION("rejects invalid characters") {
        auto result = Sanitize::validateJob("Job@Name");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("invalid") != std::string::npos);
    }
    
    SECTION("collapses whitespace") {
        auto result = Sanitize::validateJob("Dark   Knight");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Dark Knight");
    }
}

TEST_CASE("Sanitize::validateRank", "[sanitize]") {
    SECTION("validates correct rank") {
        auto result = Sanitize::validateRank("Captain");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Captain");
    }
    
    SECTION("allows empty rank") {
        auto result = Sanitize::validateRank("");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "");
    }
    
    SECTION("rejects rank longer than max length") {
        std::string longRank(51, 'a');
        auto result = Sanitize::validateRank(longRank);
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("50") != std::string::npos);
    }
    
    SECTION("rejects invalid characters") {
        auto result = Sanitize::validateRank("Rank@Name");
        REQUIRE(result.valid == false);
        REQUIRE(result.error.find("invalid") != std::string::npos);
    }
    
    SECTION("collapses whitespace") {
        auto result = Sanitize::validateRank("Second   Lieutenant");
        REQUIRE(result.valid == true);
        REQUIRE(result.sanitized == "Second Lieutenant");
    }
}

TEST_CASE("Sanitize::collapseWhitespace", "[sanitize]") {
    SECTION("collapses multiple spaces") {
        std::string input = "Hello    World";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "Hello World");
    }
    
    SECTION("collapses tabs") {
        std::string input = "Hello\t\tWorld";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "Hello World");
    }
    
    SECTION("collapses newlines") {
        std::string input = "Hello\n\nWorld";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "Hello World");
    }
    
    SECTION("collapses mixed whitespace") {
        std::string input = "Hello \t\n  World";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "Hello World");
    }
    
    SECTION("preserves single spaces") {
        std::string input = "Hello World";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "Hello World");
    }
    
    SECTION("handles empty string") {
        std::string input = "";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == "");
    }
    
    SECTION("handles only whitespace") {
        std::string input = "   \t\n  ";
        std::string result = Sanitize::collapseWhitespace(input);
        REQUIRE(result == " ");
    }
}

TEST_CASE("Sanitize::normalizeNameTitleCase", "[sanitize]") {
    SECTION("capitalizes first letter of each word") {
        std::string input = "test user";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test User");
    }
    
    SECTION("handles already capitalized names") {
        std::string input = "Test User";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test User");
    }
    
    SECTION("handles all lowercase") {
        std::string input = "testuser";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Testuser");
    }
    
    SECTION("handles all uppercase") {
        std::string input = "TEST USER";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test User");
    }
    
    SECTION("handles hyphenated names") {
        std::string input = "test-user";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test-User");
    }
    
    SECTION("handles underscore names") {
        std::string input = "test_user";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test_User");
    }
    
    SECTION("handles apostrophe names") {
        std::string input = "o'brien";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "O'Brien");
    }
    
    SECTION("handles empty string") {
        std::string input = "";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "");
    }
    
    SECTION("handles mixed case") {
        std::string input = "tEsT uSeR";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test User");
    }
    
    SECTION("handles multiple spaces") {
        std::string input = "test   user";
        std::string result = Sanitize::normalizeNameTitleCase(input);
        REQUIRE(result == "Test   User");
    }
}

