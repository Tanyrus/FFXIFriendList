#include <catch2/catch_test_macros.hpp>
#include "Core/UtilitiesCore.h"
#include <cstdint>

using namespace XIFriendList::Core;

TEST_CASE("NotificationSoundPolicy - Construction", "[notification]") {
    SECTION("default state") {
        NotificationSoundPolicy policy;
        
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 0);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendRequest) == 0);
        
        uint64_t time = 1000;
        bool result1 = policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        bool result2 = policy.shouldPlay(NotificationSoundType::FriendRequest, time);
        REQUIRE(result1 == true);
        REQUIRE(result2 == true);
    }
}

TEST_CASE("NotificationSoundPolicy - Cooldown enforcement", "[notification]") {
    SECTION("FriendOnline cooldown is 10 seconds") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time) == true);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time + 5000) == false);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time + 9999) == false);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time + 10000) == true);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time + 20000) == true);
    }
    
    SECTION("FriendRequest cooldown is 5 seconds") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time) == true);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time + 4000) == false);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time + 4999) == false);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time + 5000) == true);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time + 10000) == true);
    }
    
    SECTION("cooldown resets after time passes") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time + 10000) == true);
        
        policy.shouldPlay(NotificationSoundType::FriendRequest, time);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time + 5000) == true);
    }
}

TEST_CASE("NotificationSoundPolicy - Suppressed count tracking", "[notification]") {
    SECTION("count increments when suppressed") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 0);
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time + 1000);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 1);
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time + 2000);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 2);
    }
    
    SECTION("count does not increment when sound plays") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 0);
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time + 10000);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 0);
    }
    
    SECTION("count tracked per sound type") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        policy.shouldPlay(NotificationSoundType::FriendRequest, time);
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time + 1000);
        policy.shouldPlay(NotificationSoundType::FriendRequest, time + 1000);
        
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 1);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendRequest) == 1);
    }
    
    SECTION("count returns 0 for unknown type") {
        NotificationSoundPolicy policy;
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::Unknown) == 0);
    }
}

TEST_CASE("NotificationSoundPolicy - Reset", "[notification]") {
    SECTION("reset clears state") {
        NotificationSoundPolicy policy;
        uint64_t time = 1000;
        
        policy.shouldPlay(NotificationSoundType::FriendOnline, time);
        policy.shouldPlay(NotificationSoundType::FriendOnline, time + 1000);
        policy.shouldPlay(NotificationSoundType::FriendRequest, time);
        policy.shouldPlay(NotificationSoundType::FriendRequest, time + 1000);
        
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 1);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendRequest) == 1);
        
        policy.reset();
        
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendOnline) == 0);
        REQUIRE(policy.getSuppressedCount(NotificationSoundType::FriendRequest) == 0);
        
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendOnline, time) == true);
        REQUIRE(policy.shouldPlay(NotificationSoundType::FriendRequest, time) == true);
    }
}

