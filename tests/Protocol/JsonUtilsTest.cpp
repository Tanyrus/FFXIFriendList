#include <catch2/catch_test_macros.hpp>
#include "Protocol/JsonUtils.h"
#include <string>
#include <vector>
#include <cstdint>

using namespace XIFriendList::Protocol::JsonUtils;

TEST_CASE("JsonUtils::encodeString", "[json]") {
    SECTION("normal strings") {
        REQUIRE(encodeString("hello") == "\"hello\"");
        REQUIRE(encodeString("test") == "\"test\"");
    }
    
    SECTION("special characters") {
        REQUIRE(encodeString("hello\"world") == "\"hello\\\"world\"");
        REQUIRE(encodeString("hello\\world") == "\"hello\\\\world\"");
        REQUIRE(encodeString("hello\nworld") == "\"hello\\nworld\"");
        REQUIRE(encodeString("hello\rworld") == "\"hello\\rworld\"");
        REQUIRE(encodeString("hello\tworld") == "\"hello\\tworld\"");
    }
    
    SECTION("empty strings") {
        REQUIRE(encodeString("") == "\"\"");
    }
}

TEST_CASE("JsonUtils::encodeNumber", "[json]") {
    SECTION("int") {
        REQUIRE(encodeNumber(42) == "42");
        REQUIRE(encodeNumber(0) == "0");
        REQUIRE(encodeNumber(-42) == "-42");
    }
    
    SECTION("int64_t") {
        REQUIRE(encodeNumber(static_cast<int64_t>(1234567890)) == "1234567890");
        REQUIRE(encodeNumber(static_cast<int64_t>(-1234567890)) == "-1234567890");
    }
    
    SECTION("uint64_t") {
        REQUIRE(encodeNumber(static_cast<uint64_t>(1234567890)) == "1234567890");
        REQUIRE(encodeNumber(static_cast<uint64_t>(0)) == "0");
    }
}

TEST_CASE("JsonUtils::encodeBoolean", "[json]") {
    SECTION("true") {
        REQUIRE(encodeBoolean(true) == "true");
    }
    
    SECTION("false") {
        REQUIRE(encodeBoolean(false) == "false");
    }
}

TEST_CASE("JsonUtils::encodeStringArray", "[json]") {
    SECTION("empty array") {
        std::vector<std::string> arr;
        REQUIRE(encodeStringArray(arr) == "[]");
    }
    
    SECTION("single element") {
        std::vector<std::string> arr = {"hello"};
        REQUIRE(encodeStringArray(arr) == "[\"hello\"]");
    }
    
    SECTION("multiple elements") {
        std::vector<std::string> arr = {"hello", "world", "test"};
        std::string result = encodeStringArray(arr);
        REQUIRE(result.find("\"hello\"") != std::string::npos);
        REQUIRE(result.find("\"world\"") != std::string::npos);
        REQUIRE(result.find("\"test\"") != std::string::npos);
        REQUIRE(result[0] == '[');
        REQUIRE(result[result.length() - 1] == ']');
    }
}

TEST_CASE("JsonUtils::decodeString", "[json]") {
    SECTION("normal strings") {
        std::string out;
        REQUIRE(decodeString("\"hello\"", out) == true);
        REQUIRE(out == "hello");
    }
    
    SECTION("escaped characters") {
        std::string out;
        REQUIRE(decodeString("\"hello\\\"world\"", out) == true);
        REQUIRE(out == "hello\"world");
        REQUIRE(decodeString("\"hello\\nworld\"", out) == true);
        REQUIRE(out == "hello\nworld");
    }
    
    SECTION("empty strings") {
        std::string out;
        REQUIRE(decodeString("\"\"", out) == true);
        REQUIRE(out == "");
    }
    
    SECTION("invalid input") {
        std::string out;
        REQUIRE(decodeString("hello", out) == false);
        REQUIRE(decodeString("", out) == false);
    }
}

TEST_CASE("JsonUtils::decodeNumber", "[json]") {
    SECTION("int64_t") {
        int64_t out = 0;
        REQUIRE(decodeNumber("123", out) == true);
        REQUIRE(out == 123);
        REQUIRE(decodeNumber("-123", out) == true);
        REQUIRE(out == -123);
    }
    
    SECTION("uint64_t") {
        uint64_t out = 0;
        REQUIRE(decodeNumber("123", out) == true);
        REQUIRE(out == 123);
    }
    
    SECTION("int") {
        int out = 0;
        REQUIRE(decodeNumber("42", out) == true);
        REQUIRE(out == 42);
    }
    
    SECTION("invalid numbers") {
        int64_t out = 0;
        REQUIRE(decodeNumber("abc", out) == false);
        REQUIRE(decodeNumber("", out) == false);
    }
}

TEST_CASE("JsonUtils::decodeBoolean", "[json]") {
    SECTION("true") {
        bool out = false;
        REQUIRE(decodeBoolean("true", out) == true);
        REQUIRE(out == true);
    }
    
    SECTION("false") {
        bool out = true;
        REQUIRE(decodeBoolean("false", out) == true);
        REQUIRE(out == false);
    }
    
    SECTION("invalid booleans") {
        bool out;
        REQUIRE(decodeBoolean("yes", out) == false);
        REQUIRE(decodeBoolean("1", out) == false);
    }
}

TEST_CASE("JsonUtils::extractField", "[json]") {
    SECTION("simple fields") {
        std::string json = "{\"name\":\"test\",\"value\":123}";
        std::string out;
        REQUIRE(extractField(json, "name", out) == true);
        REQUIRE(out == "\"test\"");
    }
    
    SECTION("nested objects") {
        std::string json = "{\"nested\":{\"key\":\"value\"}}";
        std::string out;
        REQUIRE(extractField(json, "nested", out) == true);
        REQUIRE(out.find("{") != std::string::npos);
    }
    
    SECTION("arrays") {
        std::string json = "{\"items\":[\"a\",\"b\"]}";
        std::string out;
        REQUIRE(extractField(json, "items", out) == true);
        REQUIRE(out.find("[") != std::string::npos);
    }
    
    SECTION("missing fields") {
        std::string json = "{\"name\":\"test\"}";
        std::string out;
        REQUIRE(extractField(json, "missing", out) == false);
    }
}

TEST_CASE("JsonUtils::extractStringField", "[json]") {
    SECTION("string extraction") {
        std::string json = "{\"name\":\"test\"}";
        std::string out;
        REQUIRE(extractStringField(json, "name", out) == true);
        REQUIRE(out == "test");
    }
    
    SECTION("missing fields") {
        std::string json = "{\"name\":\"test\"}";
        std::string out;
        REQUIRE(extractStringField(json, "missing", out) == false);
    }
}

TEST_CASE("JsonUtils::extractNumberField", "[json]") {
    SECTION("number extraction") {
        std::string json = "{\"value\":42}";
        int64_t out = 0;
        REQUIRE(extractNumberField(json, "value", out) == true);
        REQUIRE(out == 42);
    }
    
    SECTION("type mismatches") {
        std::string json = "{\"value\":\"not a number\"}";
        int64_t out = 0;
        REQUIRE(extractNumberField(json, "value", out) == false);
    }
}

TEST_CASE("JsonUtils::isValidJson", "[json]") {
    SECTION("valid JSON") {
        bool result1 = isValidJson("{}");
        bool result2 = isValidJson("[]");
        bool result3 = isValidJson("{\"key\":\"value\"}");
        REQUIRE(result1 == true);
        REQUIRE(result2 == true);
        REQUIRE(result3 == true);
    }
    
    SECTION("invalid JSON") {
        bool result1 = isValidJson("{");
        bool result2 = isValidJson("[");
        bool result3 = isValidJson("{\"key\":}");
        REQUIRE(result1 == false);
        REQUIRE(result2 == false);
        REQUIRE(result3 == false);
    }
    
    SECTION("edge cases") {
        bool result = isValidJson("");
        REQUIRE(result == false);
    }
}

