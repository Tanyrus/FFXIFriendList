#include <catch2/catch_test_macros.hpp>
#include "Core/ModelsCore.h"
#include <string>

using namespace XIFriendList::Core;

TEST_CASE("MailMessage - Construction", "[mail]") {
    SECTION("default construction") {
        MailMessage msg;
        REQUIRE(msg.messageId.empty());
        REQUIRE(msg.fromUserId.empty());
        REQUIRE(msg.toUserId.empty());
        REQUIRE(msg.subject.empty());
        REQUIRE(msg.body.empty());
        REQUIRE(msg.createdAt == 0);
        REQUIRE(msg.readAt == 0);
        REQUIRE(msg.isRead == false);
        REQUIRE(msg.isUnread() == true);
    }
    
    SECTION("parameterized construction") {
        MailMessage msg;
        msg.messageId = "msg123";
        msg.fromUserId = "sender";
        msg.toUserId = "recipient";
        msg.subject = "Test Subject";
        msg.body = "Test Body";
        msg.createdAt = 1000;
        msg.readAt = 2000;
        msg.isRead = true;
        
        REQUIRE(msg.messageId == "msg123");
        REQUIRE(msg.fromUserId == "sender");
        REQUIRE(msg.toUserId == "recipient");
        REQUIRE(msg.subject == "Test Subject");
        REQUIRE(msg.body == "Test Body");
        REQUIRE(msg.createdAt == 1000);
        REQUIRE(msg.readAt == 2000);
        REQUIRE(msg.isRead == true);
        REQUIRE(msg.isUnread() == false);
    }
}

TEST_CASE("MailMessage - Equality", "[mail]") {
    SECTION("equality by messageId") {
        MailMessage msg1;
        msg1.messageId = "msg123";
        msg1.fromUserId = "sender1";
        
        MailMessage msg2;
        msg2.messageId = "msg123";
        msg2.fromUserId = "sender2";
        
        REQUIRE(msg1 == msg2);
        REQUIRE(!(msg1 != msg2));
    }
    
    SECTION("inequality by messageId") {
        MailMessage msg1;
        msg1.messageId = "msg123";
        
        MailMessage msg2;
        msg2.messageId = "msg456";
        
        REQUIRE(msg1 != msg2);
        REQUIRE(!(msg1 == msg2));
    }
    
    SECTION("inequality with empty messageId") {
        MailMessage msg1;
        msg1.messageId = "msg123";
        
        MailMessage msg2;
        msg2.messageId = "";
        
        REQUIRE(msg1 != msg2);
    }
}

TEST_CASE("MailMessage - IsUnread", "[mail]") {
    SECTION("isUnread returns true when isRead is false") {
        MailMessage msg;
        msg.isRead = false;
        REQUIRE(msg.isUnread() == true);
    }
    
    SECTION("isUnread returns false when isRead is true") {
        MailMessage msg;
        msg.isRead = true;
        REQUIRE(msg.isUnread() == false);
    }
    
    SECTION("isUnread when readAt is set") {
        MailMessage msg;
        msg.isRead = true;
        msg.readAt = 1000;
        REQUIRE(msg.isUnread() == false);
    }
    
    SECTION("isUnread when readAt is zero but isRead is true") {
        MailMessage msg;
        msg.isRead = true;
        msg.readAt = 0;
        REQUIRE(msg.isUnread() == false);
    }
}

