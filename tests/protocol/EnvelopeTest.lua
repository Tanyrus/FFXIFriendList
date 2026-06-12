-- EnvelopeTest.lua
-- Tests the LIVE HTTP envelope contract decoded by connection/feature handlers:
--   success: { success = true, data = T, timestamp = string }
--   error:   { success = false, error = { code, message }, timestamp }
-- (The old protocolVersion/type/payload shape is retired — see protocol/Envelope.lua.)

local Envelope = require("protocol.Envelope")
local Json = require("protocol.Json")

local function assert(condition, message)
    if not condition then
        error("ASSERTION FAILED: " .. (message or "unknown"))
    end
end

local function testDecodeSuccess()
    local jsonStr = Json.encode({
        success = true,
        data = { friends = {}, serverTime = 1234567890 },
        timestamp = "2026-06-11T00:00:00Z",
    })

    local ok, envelope = Envelope.decode(jsonStr)
    assert(ok, "Should decode a valid success envelope")
    assert(envelope.success == true, "success should be true")
    assert(type(envelope.data) == "table", "data should be a table")
    assert(envelope.data.friends ~= nil, "data should carry the payload (friends)")
    assert(envelope.timestamp == "2026-06-11T00:00:00Z", "timestamp should be preserved")
    print("✓ testDecodeSuccess passed")
end

local function testDecodeSuccessDefaultsData()
    -- A success envelope with no data field yields an empty table, not nil.
    local jsonStr = Json.encode({ success = true })
    local ok, envelope = Envelope.decode(jsonStr)
    assert(ok, "Should decode success even without data")
    assert(type(envelope.data) == "table", "missing data defaults to empty table")
    assert(envelope.timestamp == "", "missing timestamp defaults to empty string")
    print("✓ testDecodeSuccessDefaultsData passed")
end

local function testDecodeErrorResponse()
    local jsonStr = Json.encode({
        success = false,
        error = { code = "TEST_ERROR", message = "Test error" },
        timestamp = "2026-06-11T00:00:00Z",
    })

    local ok, errorType, message, code = Envelope.decode(jsonStr)
    assert(not ok, "Error envelope should decode as failure")
    assert(errorType == Envelope.DecodeError.ServerError, "Should classify as ServerError")
    assert(message == "Test error", "Error message should be surfaced")
    assert(code == "TEST_ERROR", "Error code should be surfaced")
    print("✓ testDecodeErrorResponse passed")
end

local function testDecodeErrorDefaults()
    -- success=false with no error object still yields safe defaults.
    local jsonStr = Json.encode({ success = false })
    local ok, errorType, message, code = Envelope.decode(jsonStr)
    assert(not ok, "Should be a failure")
    assert(errorType == Envelope.DecodeError.ServerError, "Should classify as ServerError")
    assert(message == "Unknown server error", "Should default the message")
    assert(code == "UNKNOWN_ERROR", "Should default the code")
    print("✓ testDecodeErrorDefaults passed")
end

local function testDecodeMissingSuccess()
    local jsonStr = Json.encode({ data = {} })
    local ok, errorType = Envelope.decode(jsonStr)
    assert(not ok, "Should reject an envelope with no success field")
    assert(errorType == Envelope.DecodeError.MissingSuccess, "Should return MissingSuccess")
    print("✓ testDecodeMissingSuccess passed")
end

local function testDecodeInvalidJson()
    local ok, errorType = Envelope.decode("{ invalid json }")
    assert(not ok, "Should reject invalid JSON")
    assert(errorType == Envelope.DecodeError.InvalidJson, "Should return InvalidJson")
    print("✓ testDecodeInvalidJson passed")
end

local function testDecodeEmpty()
    local ok, errorType = Envelope.decode("")
    assert(not ok, "Should reject an empty response")
    assert(errorType == Envelope.DecodeError.InvalidJson, "Empty response is InvalidJson")
    print("✓ testDecodeEmpty passed")
end

local function testDecodeData()
    -- decodeData unwraps to just the data payload on success.
    local jsonStr = Json.encode({ success = true, data = { apiKey = "abc" } })
    local ok, data = Envelope.decodeData(jsonStr)
    assert(ok, "decodeData should succeed")
    assert(data.apiKey == "abc", "decodeData should return the inner data")

    -- And forwards the error message/code on failure.
    local errStr = Json.encode({ success = false, error = { code = "NOPE", message = "no" } })
    local ok2, msg, code = Envelope.decodeData(errStr)
    assert(not ok2, "decodeData should fail on an error envelope")
    assert(msg == "no" and code == "NOPE", "decodeData should forward message and code")
    print("✓ testDecodeData passed")
end

local function runAllTests()
    print("Running Envelope tests...")
    testDecodeSuccess()
    testDecodeSuccessDefaultsData()
    testDecodeErrorResponse()
    testDecodeErrorDefaults()
    testDecodeMissingSuccess()
    testDecodeInvalidJson()
    testDecodeEmpty()
    testDecodeData()
    print("All Envelope tests passed!")
end

if arg and arg[0] and arg[0]:match("EnvelopeTest") then
    runAllTests()
end

return {
    runAllTests = runAllTests
}
