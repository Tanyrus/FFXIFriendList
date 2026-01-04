-- DecodeRouterTest.lua
-- Test table-driven routing by response type

local DecodeRouter = require("protocol.DecodeRouter")
local Envelope = require("protocol.Envelope")
local MessageTypes = require("protocol.MessageTypes")
local ProtocolVersion = require("protocol.ProtocolVersion")
local Json = require("protocol.Json")

local function assert(condition, message)
    if not condition then
        error("ASSERTION FAILED: " .. (message or "unknown"))
    end
end

local function testRouterFriendsList()
    local envelope = {
        protocolVersion = ProtocolVersion.PROTOCOL_VERSION,
        type = MessageTypes.ResponseType.FriendsListResponse,
        success = true,
        payload = {
            friends = {{name = "TestFriend"}},
            serverTime = 1234567890
        }
    }
    
    local ok, result = DecodeRouter.decode(envelope)
    assert(ok, "Should route FriendsListResponse")
    assert(result.friends ~= nil, "Should decode friends")
    assert(result.serverTime == 1234567890, "Should decode serverTime")
    print("✓ testRouterFriendsList passed")
end

local function testRouterHeartbeat()
    local envelope = {
        protocolVersion = ProtocolVersion.PROTOCOL_VERSION,
        type = MessageTypes.ResponseType.HeartbeatResponse,
        success = true,
        payload = {
            serverTime = 1234567890,
            nextHeartbeatMs = 30000,
            onlineThresholdMs = 60000
        }
    }
    
    local ok, result = DecodeRouter.decode(envelope)
    assert(ok, "Should route HeartbeatResponse")
    assert(result.serverTime == 1234567890, "Should decode serverTime")
    assert(result.nextHeartbeatMs == 30000, "Should decode nextHeartbeatMs")
    print("✓ testRouterHeartbeat passed")
end

local function testRouterUnknownType()
    local envelope = {
        protocolVersion = ProtocolVersion.PROTOCOL_VERSION,
        type = "UnknownResponseType",
        success = true,
        payload = {}
    }
    
    local ok, errorMsg = DecodeRouter.decode(envelope)
    assert(not ok, "Should reject unknown type")
    assert(errorMsg:match("Unknown response type"), "Should return appropriate error")
    print("✓ testRouterUnknownType passed")
end

local function testRouterMissingType()
    local envelope = {
        protocolVersion = ProtocolVersion.PROTOCOL_VERSION,
        success = true,
        payload = {}
    }
    
    local ok, errorMsg = DecodeRouter.decode(envelope)
    assert(not ok, "Should reject missing type")
    assert(errorMsg:match("missing type"), "Should return appropriate error")
    print("✓ testRouterMissingType passed")
end

local function testRouterPreferences()
    local envelope = {
        protocolVersion = ProtocolVersion.PROTOCOL_VERSION,
        type = MessageTypes.ResponseType.PreferencesResponse,
        success = true,
        payload = {
            preferences = {useServerNotes = true},
            privacy = {shareLocation = false}
        }
    }
    
    local ok, result = DecodeRouter.decode(envelope)
    assert(ok, "Should route PreferencesResponse")
    assert(result.preferences ~= nil, "Should decode preferences")
    assert(result.privacy ~= nil, "Should decode privacy")
    print("✓ testRouterPreferences passed")
end

local function runAllTests()
    print("Running DecodeRouter tests...")
    testRouterFriendsList()
    testRouterHeartbeat()
    testRouterUnknownType()
    testRouterMissingType()
    testRouterPreferences()
    print("All DecodeRouter tests passed!")
end

if arg and arg[0] and arg[0]:match("DecodeRouterTest") then
    runAllTests()
end

return {
    runAllTests = runAllTests
}

