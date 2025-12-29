#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "Platform/Ashita/CommandHandlerHook.h"
#include <string>
#include <vector>

using namespace XIFriendList::Platform::Ashita;

TEST_CASE("CommandHandlerHook - Original handler executes", "[hook]") {
    bool handlerCalled = false;
    std::string receivedCommand;
    
    CommandHandlerHook hook([&](int32_t mode, const char* command, bool injected) {
        handlerCalled = true;
        receivedCommand = command ? command : "";
        return true;
    });
    
    bool result = hook.execute(0, "/fl", false);
    
    REQUIRE(handlerCalled);
    REQUIRE(result == true);
    REQUIRE(receivedCommand == "/fl");
}

TEST_CASE("CommandHandlerHook - Pre-hook executes before handler", "[hook]") {
    std::vector<std::string> executionOrder;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        executionOrder.push_back("handler");
        return true;
    });
    
    hook.addPreHook([&](int32_t, const char*, bool, bool) {
        executionOrder.push_back("pre-hook");
        return true;
    });
    
    hook.execute(0, "/fl", false);
    
    REQUIRE(executionOrder.size() == 2);
    REQUIRE(executionOrder[0] == "pre-hook");
    REQUIRE(executionOrder[1] == "handler");
}

TEST_CASE("CommandHandlerHook - Post-hook executes after handler", "[hook]") {
    std::vector<std::string> executionOrder;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        executionOrder.push_back("handler");
        return true;
    });
    
    hook.addPostHook([&](int32_t, const char*, bool, bool wasHandled) {
        executionOrder.push_back("post-hook");
        REQUIRE(wasHandled == true);
        return true;
    });
    
    hook.execute(0, "/fl", false);
    
    REQUIRE(executionOrder.size() == 2);
    REQUIRE(executionOrder[0] == "handler");
    REQUIRE(executionOrder[1] == "post-hook");
}

TEST_CASE("CommandHandlerHook - Multiple hooks execute in order", "[hook]") {
    std::vector<std::string> executionOrder;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        executionOrder.push_back("handler");
        return true;
    });
    
    hook.addPreHook([&](int32_t, const char*, bool, bool) {
        executionOrder.push_back("pre-hook-1");
        return true;
    });
    
    hook.addPreHook([&](int32_t, const char*, bool, bool) {
        executionOrder.push_back("pre-hook-2");
        return true;
    });
    
    hook.addPostHook([&](int32_t, const char*, bool, bool) {
        executionOrder.push_back("post-hook-1");
        return true;
    });
    
    hook.addPostHook([&](int32_t, const char*, bool, bool) {
        executionOrder.push_back("post-hook-2");
        return true;
    });
    
    hook.execute(0, "/fl", false);
    
    REQUIRE(executionOrder.size() == 5);
    REQUIRE(executionOrder[0] == "pre-hook-1");
    REQUIRE(executionOrder[1] == "pre-hook-2");
    REQUIRE(executionOrder[2] == "handler");
    REQUIRE(executionOrder[3] == "post-hook-1");
    REQUIRE(executionOrder[4] == "post-hook-2");
}

TEST_CASE("CommandHandlerHook - Pre-hook can stop execution", "[hook]") {
    bool handlerCalled = false;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        handlerCalled = true;
        return true;
    });
    
    hook.addPreHook([&](int32_t, const char*, bool, bool) {
        return false;  // Stop processing
    });
    
    bool result = hook.execute(0, "/fl", false);
    
    REQUIRE(handlerCalled == false);
    REQUIRE(result == false);
}

TEST_CASE("CommandHandlerHook - Post-hook receives handler result", "[hook]") {
    bool receivedWasHandled = false;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        return false;  // Handler returns false
    });
    
    hook.addPostHook([&](int32_t, const char*, bool, bool wasHandled) {
        receivedWasHandled = wasHandled;
        return true;
    });
    
    hook.execute(0, "/fl", false);
    
    REQUIRE(receivedWasHandled == false);
}

TEST_CASE("CommandHandlerHook - Hooks can be removed", "[hook]") {
    int preHookCallCount = 0;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        return true;
    });
    
    size_t hookId = hook.addPreHook([&](int32_t, const char*, bool, bool) {
        preHookCallCount++;
        return true;
    });
    
    hook.execute(0, "/fl", false);
    REQUIRE(preHookCallCount == 1);
    
    hook.removeHook(hookId);
    hook.execute(0, "/fl", false);
    REQUIRE(preHookCallCount == 1);  // Should not increment
}

TEST_CASE("CommandHandlerHook - All hooks can be cleared", "[hook]") {
    int hookCallCount = 0;
    
    CommandHandlerHook hook([&](int32_t, const char*, bool) {
        return true;
    });
    
    hook.addPreHook([&](int32_t, const char*, bool, bool) {
        hookCallCount++;
        return true;
    });
    
    hook.addPostHook([&](int32_t, const char*, bool, bool) {
        hookCallCount++;
        return true;
    });
    
    hook.execute(0, "/fl", false);
    REQUIRE(hookCallCount == 2);
    
    hook.clearHooks();
    hook.execute(0, "/fl", false);
    REQUIRE(hookCallCount == 2);  // Should not increment
}

TEST_CASE("CommandHandlerHook - Original handler still callable directly", "[hook]") {
    bool handlerCalled = false;
    
    CommandHandlerFunc originalHandler = [&](int32_t, const char*, bool) {
        handlerCalled = true;
        return true;
    };
    
    CommandHandlerHook hook(originalHandler);
    
    // Call original handler directly
    bool result = originalHandler(0, "/fl", false);
    
    REQUIRE(handlerCalled);
    REQUIRE(result == true);
}

TEST_CASE("CommandHandlerHook - Handler parameters passed correctly", "[hook]") {
    int32_t receivedMode = -1;
    std::string receivedCommand;
    bool receivedInjected = false;
    
    CommandHandlerHook hook([&](int32_t mode, const char* command, bool injected) {
        receivedMode = mode;
        receivedCommand = command ? command : "";
        receivedInjected = injected;
        return true;
    });
    
    hook.execute(42, "/befriend TestUser", true);
    
    REQUIRE(receivedMode == 42);
    REQUIRE(receivedCommand == "/befriend TestUser");
    REQUIRE(receivedInjected == true);
}

