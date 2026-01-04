-- DecoderTest.lua
-- Test individual payload decoders

local FriendsListDecoder = require("protocol.Decoders.FriendsList")
local HeartbeatDecoder = require("protocol.Decoders.Heartbeat")
local PreferencesDecoder = require("protocol.Decoders.Preferences")

local function assert(condition, message)
    if not condition then
        error("ASSERTION FAILED: " .. (message or "unknown"))
    end
end

local function testFriendsListDecoder()
    local payload = {
        friends = {{name = "Friend1"}, {name = "Friend2"}},
        serverTime = 1234567890
    }
    
    local ok, result = FriendsListDecoder.decode(payload)
    assert(ok, "Should decode valid payload")
    assert(#result.friends == 2, "Should decode friends array")
    assert(result.serverTime == 1234567890, "Should decode serverTime")
    print("✓ testFriendsListDecoder passed")
end

local function testFriendsListDecoderMissingFields()
    local payload = {}
    
    local ok, result = FriendsListDecoder.decode(payload)
    assert(ok, "Should decode even with missing fields")
    assert(type(result.friends) == "table", "Should default friends to empty table")
    assert(result.serverTime == 0, "Should default serverTime to 0")
    print("✓ testFriendsListDecoderMissingFields passed")
end

local function testHeartbeatDecoder()
    local payload = {
        serverTime = 1234567890,
        nextHeartbeatMs = 30000,
        onlineThresholdMs = 60000,
        is_outdated = false
    }
    
    local ok, result = HeartbeatDecoder.decode(payload)
    assert(ok, "Should decode valid payload")
    assert(result.serverTime == 1234567890, "Should decode serverTime")
    assert(result.nextHeartbeatMs == 30000, "Should decode nextHeartbeatMs")
    assert(result.is_outdated == false, "Should decode is_outdated")
    print("✓ testHeartbeatDecoder passed")
end

local function testPreferencesDecoder()
    local payload = {
        preferences = {useServerNotes = true},
        privacy = {shareLocation = false}
    }
    
    local ok, result = PreferencesDecoder.decode(payload)
    assert(ok, "Should decode valid payload")
    assert(result.preferences.useServerNotes == true, "Should decode preferences")
    assert(result.privacy.shareLocation == false, "Should decode privacy")
    print("✓ testPreferencesDecoder passed")
end

local function testDecoderInvalidPayload()
    local ok, errorMsg = FriendsListDecoder.decode("not a table")
    assert(not ok, "Should reject invalid payload type")
    assert(errorMsg:match("expected table"), "Should return appropriate error")
    print("✓ testDecoderInvalidPayload passed")
end

local function runAllTests()
    print("Running Decoder tests...")
    testFriendsListDecoder()
    testFriendsListDecoderMissingFields()
    testHeartbeatDecoder()
    testPreferencesDecoder()
    testDecoderInvalidPayload()
    print("All Decoder tests passed!")
end

if arg and arg[0] and arg[0]:match("DecoderTest") then
    runAllTests()
end

return {
    runAllTests = runAllTests
}

