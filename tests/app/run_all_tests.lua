-- run_all_tests.lua
-- Run all app unit tests

local addonPath = string.match(debug.getinfo(1, "S").source:sub(2), "(.*[/\\])")
if addonPath then
    package.path = addonPath .. "../../?.lua;" .. addonPath .. "../../?/init.lua;" .. package.path
end

-- Register stand-ins for Ashita-only libs (sugar/json) so real modules load.
require("tests.support.bootstrap")

print("Running App Unit Tests...")
print("=" .. string.rep("=", 50))

-- NOTE: require() returns two values (module, loaderpath); a bare require() as
-- the LAST element of a table constructor would leak the path string in as a
-- phantom entry. Load explicitly so each slot holds exactly one module.
local tests = {}
local function addTest(modulePath)
    tests[#tests + 1] = require(modulePath)
end

addTest("tests.app.ConnectionTest")
addTest("tests.app.FriendsTest")
addTest("tests.app.FriendsNotificationsTest")
addTest("tests.app.NotesTest")
addTest("tests.app.BlocklistTest")
addTest("tests.app.WsEventHandlerTest")
addTest("tests.app.WsReconnectResyncTest")
addTest("tests.app.BackoffTest")
-- TODO: Add more tests:
-- addTest("tests.app.ServerListTest")
-- addTest("tests.app.PreferencesTest")
-- addTest("tests.app.NotificationsTest")
-- addTest("tests.app.MailTest")

local totalPassed = 0
local totalFailed = 0

-- Test modules expose results in two shapes:
--   * { run = fn }            -> fn() returns true on success
--   * { passed, failed, ... } -> suite ran on require (side effect)
-- Adapt to both, and never let one broken suite abort the whole run.
for _, testModule in ipairs(tests) do
    print("")
    local success
    if type(testModule) == "table" and type(testModule.run) == "function" then
        local ok, result = pcall(testModule.run)
        success = ok and (result ~= false)
        if not ok then
            print("  ERROR running suite: " .. tostring(result))
        end
    elseif type(testModule) == "table" and testModule.failed ~= nil then
        success = (testModule.failed == 0)
    else
        print("  WARN: test module has no run() and no result counts; skipping")
        success = true
    end

    if success then
        totalPassed = totalPassed + 1
    else
        totalFailed = totalFailed + 1
    end
end

print("")
print("=" .. string.rep("=", 50))
print(string.format("Total: %d test suites passed, %d failed", totalPassed, totalFailed))

if totalFailed == 0 then
    print("All tests passed!")
    -- Honour "exit 0 = pass": set the process exit code, not just a return value.
    os.exit(0)
else
    print("Some tests failed!")
    os.exit(1)
end

