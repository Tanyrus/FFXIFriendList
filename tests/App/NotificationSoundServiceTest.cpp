#include <catch2/catch_test_macros.hpp>
#include "App/NotificationSoundService.h"
#include "UI/Notifications/Notification.h"
#include "Core/ModelsCore.h"
#include "FakeSoundPlayer.h"
#include "FakeClock.h"
#include "FakeLogger.h"
#include <filesystem>
#include <memory>
#include <cstring>
#include <fstream>

namespace XIFriendList {
namespace App {

TEST_CASE("NotificationSoundService - Sound type mapping", "[NotificationSoundService]") {
    auto soundPlayer = std::make_unique<FakeSoundPlayer>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    NotificationSoundService service(*soundPlayer, *clock, *logger, configDir);
    
    Core::Preferences prefs;
    prefs.notificationSoundsEnabled = true;
    prefs.soundOnFriendOnline = true;
    prefs.soundOnFriendRequest = true;
    prefs.notificationSoundVolume = 0.8f;
    
    SECTION("FriendOnline sound type") {
        UI::Notification notification;
        notification.message = "TestFriend has come online";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        bool played1 = soundPlayer->playWavBytesCalled_;
        bool played2 = soundPlayer->playWavFileCalled_;
        bool played = (played1 || played2);
        REQUIRE(played == true);
        REQUIRE(soundPlayer->lastVolume_ == 0.8f);
    }
    
    SECTION("FriendRequest sound type") {
        UI::Notification notification;
        notification.message = "You have a friend request";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        bool played1 = soundPlayer->playWavBytesCalled_;
        bool played2 = soundPlayer->playWavFileCalled_;
        bool played = (played1 || played2);
        REQUIRE(played == true);
    }
    
    SECTION("Unknown sound type") {
        UI::Notification notification;
        notification.message = "Some other message";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        REQUIRE(soundPlayer->playWavBytesCalled_ == false);
        REQUIRE(soundPlayer->playWavFileCalled_ == false);
    }
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("NotificationSoundService - Preference checking", "[NotificationSoundService]") {
    auto soundPlayer = std::make_unique<FakeSoundPlayer>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    NotificationSoundService service(*soundPlayer, *clock, *logger, configDir);
    
    SECTION("Master toggle disabled") {
        Core::Preferences prefs;
        prefs.notificationSoundsEnabled = false;
        prefs.soundOnFriendOnline = true;
        prefs.soundOnFriendRequest = true;
        
        UI::Notification notification;
        notification.message = "TestFriend has come online";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        REQUIRE(soundPlayer->playWavBytesCalled_ == false);
        REQUIRE(soundPlayer->playWavFileCalled_ == false);
    }
    
    SECTION("FriendOnline toggle disabled") {
        Core::Preferences prefs;
        prefs.notificationSoundsEnabled = true;
        prefs.soundOnFriendOnline = false;
        prefs.soundOnFriendRequest = true;
        
        UI::Notification notification;
        notification.message = "TestFriend has come online";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        REQUIRE(soundPlayer->playWavBytesCalled_ == false);
        REQUIRE(soundPlayer->playWavFileCalled_ == false);
    }
    
    SECTION("FriendRequest toggle disabled") {
        Core::Preferences prefs;
        prefs.notificationSoundsEnabled = true;
        prefs.soundOnFriendOnline = true;
        prefs.soundOnFriendRequest = false;
        
        UI::Notification notification;
        notification.message = "You have a friend request";
        notification.createdAt = 1000;
        
        service.maybePlaySound(notification, prefs);
        
        REQUIRE(soundPlayer->playWavBytesCalled_ == false);
        REQUIRE(soundPlayer->playWavFileCalled_ == false);
    }
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("NotificationSoundService - Throttling", "[NotificationSoundService]") {
    auto soundPlayer = std::make_unique<FakeSoundPlayer>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    NotificationSoundService service(*soundPlayer, *clock, *logger, configDir);
    
    Core::Preferences prefs;
    prefs.notificationSoundsEnabled = true;
    prefs.soundOnFriendOnline = true;
    prefs.notificationSoundVolume = 0.8f;
    
    UI::Notification notification;
    notification.message = "TestFriend has come online";
    
    clock->setTime(1000);
    notification.createdAt = 1000;
    service.maybePlaySound(notification, prefs);
    
    bool played1 = soundPlayer->playWavBytesCalled_;
    bool played2 = soundPlayer->playWavFileCalled_;
    bool played = (played1 || played2);
    REQUIRE(played == true);
    
    soundPlayer->playWavBytesCalled_ = false;
    soundPlayer->playWavFileCalled_ = false;
    
    clock->setTime(2000);
    notification.createdAt = 2000;
    service.maybePlaySound(notification, prefs);
    
    REQUIRE(soundPlayer->playWavBytesCalled_ == false);
    REQUIRE(soundPlayer->playWavFileCalled_ == false);
    
    std::filesystem::remove_all(configDir);
}

TEST_CASE("NotificationSoundService - Sound resolution", "[NotificationSoundService]") {
    auto soundPlayer = std::make_unique<FakeSoundPlayer>();
    auto clock = std::make_unique<FakeClock>();
    auto logger = std::make_unique<FakeLogger>();
    std::filesystem::path configDir = std::filesystem::temp_directory_path() / "FFXIFriendListTest";
    std::filesystem::create_directories(configDir);
    
    NotificationSoundService service(*soundPlayer, *clock, *logger, configDir);
    
    Core::Preferences prefs;
    prefs.notificationSoundsEnabled = true;
    prefs.soundOnFriendOnline = true;
    prefs.notificationSoundVolume = 0.8f;
    
    UI::Notification notification;
    notification.message = "TestFriend has come online";
    notification.createdAt = 1000;
    
    SECTION("Embedded sound resolution") {
        service.maybePlaySound(notification, prefs);
        
        bool played1 = soundPlayer->playWavBytesCalled_;
        bool played2 = soundPlayer->playWavFileCalled_;
        bool played = (played1 || played2);
        REQUIRE(played == true);
    }
    
    SECTION("File override resolution") {
        std::filesystem::path soundDir = configDir / "sounds";
        std::filesystem::create_directories(soundDir);
        std::filesystem::path soundFile = soundDir / "online.wav";
        std::ofstream file(soundFile);
        file << "fake wav data";
        file.close();
        
        service.maybePlaySound(notification, prefs);
        
        REQUIRE(soundPlayer->playWavFileCalled_ == true);
        REQUIRE(soundPlayer->lastWavFilePath_ == soundFile);
    }
    
    std::filesystem::remove_all(configDir);
}

} // namespace App
} // namespace XIFriendList

