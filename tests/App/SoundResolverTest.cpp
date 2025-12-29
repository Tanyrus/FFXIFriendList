#include <catch2/catch_test_macros.hpp>
#include "App/SoundResolver.h"
#include <filesystem>
#include <fstream>
#include <cstring>

namespace XIFriendList {
namespace App {

TEST_CASE("SoundResolver - Embedded sound resolution", "[SoundResolver]") {
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    SoundResolver resolver(configDir);
    
    SECTION("Online sound") {
        auto resolution = resolver.resolve("online");
        if (resolution) {
            REQUIRE(resolution->source == SoundResolution::Source::Embedded);
            REQUIRE(resolution->embeddedData.size() > 0);
        }
    }
    
    SECTION("Friend request sound") {
        auto resolution = resolver.resolve("friend-request");
        if (resolution) {
            REQUIRE(resolution->source == SoundResolution::Source::Embedded);
            REQUIRE(resolution->embeddedData.size() > 0);
        }
    }
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("SoundResolver - File override resolution", "[SoundResolver]") {
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    std::filesystem::path soundDir = configDir / "sounds";
    std::filesystem::create_directories(soundDir);
    
    std::filesystem::path soundFile = soundDir / "online.wav";
    std::ofstream file(soundFile);
    file << "fake wav data";
    file.close();
    
    SoundResolver resolver(configDir);
    
    auto resolution = resolver.resolve("online");
    REQUIRE(resolution.has_value());
    REQUIRE(resolution->source == SoundResolution::Source::File);
    REQUIRE(resolution->filePath == soundFile);
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("SoundResolver - Missing sound handling", "[SoundResolver]") {
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    SoundResolver resolver(configDir);
    
    auto resolution = resolver.resolve("nonexistent-sound");
    REQUIRE(resolution.has_value() == false);
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("SoundResolver - Priority (file over embedded)", "[SoundResolver]") {
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    std::filesystem::path soundDir = configDir / "sounds";
    std::filesystem::create_directories(soundDir);
    
    std::filesystem::path soundFile = soundDir / "friend-request.wav";
    std::ofstream file(soundFile);
    file << "override wav data";
    file.close();
    
    SoundResolver resolver(configDir);
    
    auto resolution = resolver.resolve("friend-request");
    REQUIRE(resolution.has_value());
    REQUIRE(resolution->source == SoundResolution::Source::File);
    REQUIRE(resolution->filePath == soundFile);
    
    std::filesystem::remove_all(configDir);
}

} // namespace App
} // namespace XIFriendList

