-- run_all_tests.lua
-- Run all protocol tests

-- Register stand-ins for Ashita-only libs (sugar/json) so protocol modules load
-- outside the client — same bootstrap the app suite uses.
require("tests.support.bootstrap")

print("=" .. string.rep("=", 60))
print("Running Protocol Layer Tests")
print("=" .. string.rep("=", 60))
print()

-- Load explicitly: require() returns (module, loaderpath); a bare require() as
-- the last element of a table constructor would leak the path string in as a
-- phantom entry (and break test.runAllTests()).
local tests = {}
local function addTest(modulePath)
    tests[#tests + 1] = require(modulePath)
end

addTest("tests.protocol.JsonTest")
addTest("tests.protocol.ProtocolVersionTest")
addTest("tests.protocol.EnvelopeTest")
addTest("tests.protocol.DecoderTest")
addTest("tests.protocol.RequestEncoderTest")
addTest("tests.protocol.WsEnvelopeTest")

local passed = 0
local failed = 0

for i, test in ipairs(tests) do
    local success, err = pcall(function()
        test.runAllTests()
    end)
    
    if success then
        passed = passed + 1
    else
        failed = failed + 1
        print("ERROR: " .. tostring(err))
    end
    print()
end

print("=" .. string.rep("=", 60))
print(string.format("Tests: %d passed, %d failed", passed, failed))
print("=" .. string.rep("=", 60))

if failed > 0 then
    os.exit(1)
end

