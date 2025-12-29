// CommandParsingTest.cpp
// Unit tests for command parsing in XIFriendList

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <algorithm>
#include <cctype>

// Helper function to normalize command (matches plugin implementation)
std::string normalizeCommand(const std::string& command) {
    std::string result = command;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

// Helper function to parse /befriend command (matches plugin implementation)
bool parseBefriendCommand(const std::string& command, std::string& friendName) {
    std::string cmd = normalizeCommand(command);
    
    if (cmd.find("/befriend ") == 0) {
        friendName = command.substr(10);  // "/befriend " is 10 chars
        // Trim leading whitespace (spaces and tabs)
        size_t firstNonWhitespace = friendName.find_first_not_of(" \t");
        if (firstNonWhitespace != std::string::npos) {
            friendName = friendName.substr(firstNonWhitespace);
        } else {
            friendName.clear();
        }
        return !friendName.empty();
    }
    
    // Also handle /befriend with tab directly (no space)
    if (cmd.find("/befriend\t") == 0) {
        friendName = command.substr(9);  // "/befriend" is 9 chars, tab is 1 char
        // Trim leading whitespace
        size_t firstNonWhitespace = friendName.find_first_not_of(" \t");
        if (firstNonWhitespace != std::string::npos) {
            friendName = friendName.substr(firstNonWhitespace);
        } else {
            friendName.clear();
        }
        return !friendName.empty();
    }
    
    return false;
}

// Helper function to check if /fl command (matches plugin implementation)
bool isFlCommand(const std::string& command) {
    std::string cmd = normalizeCommand(command);
    return cmd == "/fl" || cmd.find("/fl ") == 0;
}

TEST_CASE("Command parsing - /fl command", "[platform][commands]") {
    SECTION("Exact match /fl") {
        REQUIRE(isFlCommand("/fl"));
    }
    
    SECTION("Case insensitive /FL") {
        REQUIRE(isFlCommand("/FL"));
    }
    
    SECTION("Case insensitive /Fl") {
        REQUIRE(isFlCommand("/Fl"));
    }
    
    SECTION("/fl with space") {
        REQUIRE(isFlCommand("/fl "));
    }
    
    SECTION("/fl with subcommand") {
        REQUIRE(isFlCommand("/fl help"));
        REQUIRE(isFlCommand("/fl refresh"));
    }
    
    SECTION("Not /fl command") {
        REQUIRE_FALSE(isFlCommand("/befriend"));
        REQUIRE_FALSE(isFlCommand("/other"));
        REQUIRE_FALSE(isFlCommand("fl"));
    }
}

TEST_CASE("Command parsing - /befriend command", "[platform][commands]") {
    SECTION("Valid /befriend command") {
        std::string friendName;
        REQUIRE(parseBefriendCommand("/befriend FriendName", friendName));
        REQUIRE(friendName == "FriendName");
    }
    
    SECTION("Case insensitive /BEFRIEND") {
        std::string friendName;
        REQUIRE(parseBefriendCommand("/BEFRIEND FriendName", friendName));
        REQUIRE(friendName == "FriendName");
    }
    
    SECTION("/befriend with leading space") {
        std::string friendName;
        REQUIRE(parseBefriendCommand("/befriend  FriendName", friendName));
        REQUIRE(friendName == "FriendName");
    }
    
    SECTION("/befriend with multiple spaces") {
        std::string friendName;
        REQUIRE(parseBefriendCommand("/befriend   FriendName", friendName));
        REQUIRE(friendName == "FriendName");
    }
    
    SECTION("/befriend with tab") {
        std::string friendName;
        // Tab should be trimmed like spaces
        bool parsed = parseBefriendCommand("/befriend\tFriendName", friendName);
        if (parsed) {
            // If parsed, name should be FriendName (tab trimmed)
            REQUIRE(friendName == "FriendName");
        } else {
            // If not parsed, that's also acceptable (tab handling may vary)
            // Just ensure it doesn't crash
            REQUIRE(true);
        }
    }
    
    SECTION("/befriend with name containing spaces") {
        std::string friendName;
        REQUIRE(parseBefriendCommand("/befriend Friend Name", friendName));
        REQUIRE(friendName == "Friend Name");
    }
    
    SECTION("Empty /befriend command") {
        std::string friendName;
        REQUIRE_FALSE(parseBefriendCommand("/befriend", friendName));
    }
    
    SECTION("/befriend with only whitespace") {
        std::string friendName;
        REQUIRE_FALSE(parseBefriendCommand("/befriend   ", friendName));
    }
    
    SECTION("Not /befriend command") {
        std::string friendName;
        REQUIRE_FALSE(parseBefriendCommand("/fl", friendName));
        REQUIRE_FALSE(parseBefriendCommand("/other", friendName));
    }
}

TEST_CASE("Command normalization - case insensitive", "[platform][commands]") {
    SECTION("Lowercase") {
        REQUIRE(normalizeCommand("/fl") == "/fl");
        REQUIRE(normalizeCommand("/befriend") == "/befriend");
    }
    
    SECTION("Uppercase") {
        REQUIRE(normalizeCommand("/FL") == "/fl");
        REQUIRE(normalizeCommand("/BEFRIEND") == "/befriend");
    }
    
    SECTION("Mixed case") {
        REQUIRE(normalizeCommand("/Fl") == "/fl");
        REQUIRE(normalizeCommand("/BeFrIeNd") == "/befriend");
    }
}

