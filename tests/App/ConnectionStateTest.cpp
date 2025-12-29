// ConnectionStateTest.cpp
// Unit tests for ConnectionStateMachine

#include <catch2/catch_test_macros.hpp>
#include "App/StateMachines/ConnectionState.h"

using namespace XIFriendList::App;

TEST_CASE("ConnectionStateMachine - Initial state", "[App][StateMachine]") {
    ConnectionStateMachine sm;
    
    REQUIRE(sm.getState() == ConnectionState::Disconnected);
    REQUIRE_FALSE(sm.isConnected());
    REQUIRE_FALSE(sm.isConnecting());
    REQUIRE(sm.canConnect());
}

TEST_CASE("ConnectionStateMachine - Connect flow", "[App][StateMachine]") {
    ConnectionStateMachine sm;
    
    sm.startConnecting();
    REQUIRE(sm.getState() == ConnectionState::Connecting);
    REQUIRE_FALSE(sm.isConnected());
    REQUIRE(sm.isConnecting());
    
    sm.setConnected();
    REQUIRE(sm.getState() == ConnectionState::Connected);
    REQUIRE(sm.isConnected());
    REQUIRE_FALSE(sm.isConnecting());
}

TEST_CASE("ConnectionStateMachine - Reconnect flow", "[App][StateMachine]") {
    ConnectionStateMachine sm;
    
    sm.startConnecting();
    sm.setConnected();
    
    sm.startReconnecting();
    REQUIRE(sm.getState() == ConnectionState::Reconnecting);
    REQUIRE_FALSE(sm.isConnected());
    REQUIRE(sm.isConnecting());
    
    sm.setConnected();
    REQUIRE(sm.getState() == ConnectionState::Connected);
}

TEST_CASE("ConnectionStateMachine - Failed state", "[App][StateMachine]") {
    ConnectionStateMachine sm;
    
    sm.startConnecting();
    sm.setFailed();
    REQUIRE(sm.getState() == ConnectionState::Failed);
    REQUIRE_FALSE(sm.isConnected());
    REQUIRE(sm.canConnect());  // Can retry from failed
}

TEST_CASE("ConnectionStateMachine - Disconnect", "[App][StateMachine]") {
    ConnectionStateMachine sm;
    
    sm.startConnecting();
    sm.setConnected();
    sm.setDisconnected();
    
    REQUIRE(sm.getState() == ConnectionState::Disconnected);
    REQUIRE_FALSE(sm.isConnected());
    REQUIRE(sm.canConnect());
}

