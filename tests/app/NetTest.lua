-- NetTest.lua
-- Unit tests for libs/net.lua — focused on the contract that a request's
-- callback ALWAYS fires exactly once, including on cancel/timeout. Feature-level
-- in-flight flags are only cleared from the callback, so a dropped request that
-- never calls back wedges reconnect/heartbeat permanently.

-- Install a controllable fake for the third-party http lib BEFORE requiring net,
-- so net's module-local `http` binds to our stub. The stub captures the wrapped
-- callback so a test can simulate a late completion AFTER a cancel, and its
-- cancel() deliberately does NOT invoke the callback (matching the real lib).
local fakeHttp
package.loaded["libs.nonBlockingRequests.nonBlockingRequests"] = nil
package.preload["libs.nonBlockingRequests.nonBlockingRequests"] = function()
    fakeHttp = {
        active = 0,
        lastWrapped = nil,
        nextId = 1,
        getActiveCount = function() return fakeHttp.active end,
        post = function(_url, _body, _headers, cb)
            fakeHttp.lastWrapped = cb
            fakeHttp.active = fakeHttp.active + 1
            local id = fakeHttp.nextId
            fakeHttp.nextId = fakeHttp.nextId + 1
            return id
        end,
        get = function(_url, _headers, cb)
            return fakeHttp.post(_url, "", _headers, cb)
        end,
        cancel = function(_id)
            -- Real lib: cancelling drops the request without firing the callback.
            fakeHttp.active = math.max(0, fakeHttp.active - 1)
        end,
        processAll = function() end,
    }
    return fakeHttp
end

-- Force a fresh load of net against the stub.
package.loaded["libs.net"] = nil
local net = require("libs.net")

local function newRecorder()
    local rec = { calls = {} }
    rec.cb = function(success, body, err, status)
        rec.calls[#rec.calls + 1] = { success = success, body = body, err = err, status = status }
    end
    return rec
end

local function testCancelFiresCallbackOnce()
    local rec = newRecorder()
    local id = net.request({ url = "http://x/test", method = "POST", body = "{}", callback = rec.cb })
    assert(id ~= nil, "request should return an id")
    assert(#rec.calls == 0, "callback must not fire before completion")

    net.cancel(id)
    assert(#rec.calls == 1, "cancel must fire the callback exactly once")
    assert(rec.calls[1].success == false, "cancelled request reports failure")
    return true
end

local function testLateCompletionDoesNotDoubleFireAfterCancel()
    local rec = newRecorder()
    local id = net.request({ url = "http://x/test", method = "POST", body = "{}", callback = rec.cb })
    local wrapped = fakeHttp.lastWrapped

    net.cancel(id)
    assert(#rec.calls == 1, "cancel fired once")

    -- Simulate the underlying lib delivering a (late) success after cancel.
    wrapped("response-body", nil, 200)
    assert(#rec.calls == 1, "settled guard must suppress the double-fire")
    return true
end

local function testNormalCompletionFiresSuccess()
    local rec = newRecorder()
    net.request({ url = "http://x/ok", method = "GET", callback = rec.cb })
    local wrapped = fakeHttp.lastWrapped
    wrapped('{"ok":true}', nil, 200)
    assert(#rec.calls == 1, "normal completion fires once")
    assert(rec.calls[1].success == true, "2xx with body is success")
    assert(rec.calls[1].body == '{"ok":true}', "body is passed through")
    return true
end

local function testCleanupTimeoutFiresCallback()
    local rec = newRecorder()
    local id = net.request({ url = "http://x/slow", method = "POST", body = "{}", callback = rec.cb })
    -- Drive the same drop path cleanup uses; cancel() is the public equivalent
    -- and exercises the shared dropRequest helper.
    net.cancel(id)
    assert(#rec.calls == 1 and rec.calls[1].success == false, "timed-out/cancelled request must notify caller")
    return true
end

local function run()
    local tests = {
        { name = "testCancelFiresCallbackOnce", fn = testCancelFiresCallbackOnce },
        { name = "testLateCompletionDoesNotDoubleFireAfterCancel", fn = testLateCompletionDoesNotDoubleFireAfterCancel },
        { name = "testNormalCompletionFiresSuccess", fn = testNormalCompletionFiresSuccess },
        { name = "testCleanupTimeoutFiresCallback", fn = testCleanupTimeoutFiresCallback },
    }
    local passed, failed = 0, 0
    for _, t in ipairs(tests) do
        local ok, err = pcall(t.fn)
        if ok then passed = passed + 1; print("  PASS: " .. t.name)
        else failed = failed + 1; print("  FAIL: " .. t.name .. " - " .. tostring(err)) end
    end
    print(string.format("NetTest: %d passed, %d failed", passed, failed))
    return failed == 0
end

return { run = run }
