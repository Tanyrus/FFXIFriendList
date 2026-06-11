-- WsReconnectResyncTest.lua
-- Tests for item 1a: reconnect re-sync must NOT spam "friend online" toasts.
--
-- On (re)connect the server re-sends friend_online for every online friend (and
-- a friends_snapshot). These must be treated as a RE-SYNC, not as fresh events:
-- only a genuine offline->online transition should toast. These tests drive the
-- REAL WsEventHandler against the REAL Friends feature with a captured
-- notifications mock.

local Friends = require("app.features.friends")
local WsEventHandler = require("app.features.wsEventHandler")

-- ---------------------------------------------------------------- harness ----
local function makeTime()
    local t = { now = 100000 }
    function t.fn() return t.now end
    function t.advance(ms) t.now = t.now + ms end
    return t
end

local function makeNotifications()
    local pushed = {}
    return {
        pushed = pushed,
        push = function(self, toastType, payload)
            table.insert(pushed, { type = toastType, payload = payload })
        end,
    }
end

local function makePreferences()
    return {
        prefs = {},
        getPrefs = function() return {} end,
        getPreferences = function() return {} end,
        isFriendMuted = function() return false end,
    }
end

local function countOnlineToasts(notifications)
    local Notifications = require("app.features.notifications")
    local n = 0
    for _, t in ipairs(notifications.pushed) do
        if t.type == Notifications.ToastType.FriendOnline then n = n + 1 end
    end
    return n
end

-- Build a handler wired to a real Friends instance and captured notifications.
local function setup()
    local time = makeTime()
    local notifications = makeNotifications()
    local logger = { info = function() end, warn = function() end,
                     error = function() end, debug = function() end }
    local friends = Friends.Friends.new({ time = time.fn, logger = logger })
    local handler = WsEventHandler.WsEventHandler.new({
        logger = logger,
        friends = friends,
        notifications = notifications,
        preferences = makePreferences(),
        connection = { setConnected = function() end },
        time = time.fn,
    })
    -- checkForStatusChanges reaches for the global app's notifications; point it
    -- at the same captured mock so snapshot-diff toasts are observed too.
    _G.FFXIFriendListApp = { features = { notifications = notifications } }
    return handler, friends, notifications, time
end

local function snapshot(friendSpecs)
    local friendsData = {}
    for _, spec in ipairs(friendSpecs) do
        table.insert(friendsData, {
            accountId = spec.accountId,
            characterName = spec.name,
            isOnline = spec.online,
        })
    end
    return { friends = friendsData }
end

-- ------------------------------------------------------------------ tests ----

-- Reconnect re-send: a friend who was already online in the snapshot must NOT
-- produce another toast when the server re-sends friend_online for them.
local function testReconnectReSendDoesNotToastOnlineFriend()
    local handler, friends, notifications = setup()

    handler:_dispatch("connected", {})
    handler:_dispatch("friends_snapshot", snapshot({
        { accountId = "a1", name = "alice", online = true },
        { accountId = "a2", name = "bob", online = true },
    }))
    -- Snapshot establishes baseline silently.
    assert(countOnlineToasts(notifications) == 0,
        "initial snapshot must not toast online friends, got " .. countOnlineToasts(notifications))

    -- Simulate reconnect re-send burst: friend_online for everyone online.
    handler:_dispatch("friend_online", { accountId = "a1", characterName = "alice" })
    handler:_dispatch("friend_online", { accountId = "a2", characterName = "bob" })

    assert(countOnlineToasts(notifications) == 0,
        "reconnect re-send must NOT toast already-online friends, got " .. countOnlineToasts(notifications))
    return true
end

-- Genuine transition: a friend who was offline in the baseline coming online
-- AFTER the re-sync grace window must produce exactly one toast.
local function testGenuineOfflineToOnlineToasts()
    local handler, friends, notifications, time = setup()

    handler:_dispatch("connected", {})
    handler:_dispatch("friends_snapshot", snapshot({
        { accountId = "a1", name = "alice", online = false },
    }))
    assert(countOnlineToasts(notifications) == 0, "baseline silent")

    -- Move past any re-sync grace window.
    time.advance(30000)

    handler:_dispatch("friend_online", { accountId = "a1", characterName = "alice" })
    assert(countOnlineToasts(notifications) == 1,
        "genuine offline->online should toast once, got " .. countOnlineToasts(notifications))

    -- Re-send of the same online friend must not toast again.
    handler:_dispatch("friend_online", { accountId = "a1", characterName = "alice" })
    assert(countOnlineToasts(notifications) == 1,
        "repeat online must not toast, got " .. countOnlineToasts(notifications))
    return true
end

-- Offline then online round-trip in steady state should toast on the return.
local function testOfflineThenOnlineRoundTrip()
    local handler, friends, notifications, time = setup()

    handler:_dispatch("connected", {})
    handler:_dispatch("friends_snapshot", snapshot({
        { accountId = "a1", name = "alice", online = true },
    }))
    time.advance(30000)

    -- alice goes offline, then comes back.
    handler:_dispatch("friend_offline", { accountId = "a1" })
    handler:_dispatch("friend_online", { accountId = "a1", characterName = "alice" })

    assert(countOnlineToasts(notifications) == 1,
        "return from offline should toast once, got " .. countOnlineToasts(notifications))
    return true
end

-- Grace window: a friend_online arriving within the re-sync window right after
-- 'connected' (before any snapshot) must be suppressed (treated as re-sync).
local function testGraceWindowSuppressesBurstAfterConnected()
    local handler, friends, notifications, time = setup()

    -- Establish a baseline first (so suppression isn't only due to cold start).
    handler:_dispatch("friends_snapshot", snapshot({
        { accountId = "a1", name = "alice", online = false },
    }))
    time.advance(30000)

    -- Reconnect: connected event opens the grace window, burst arrives immediately.
    handler:_dispatch("connected", {})
    handler:_dispatch("friend_online", { accountId = "a1", characterName = "alice" })

    assert(countOnlineToasts(notifications) == 0,
        "burst within grace window after connected must be suppressed, got " .. countOnlineToasts(notifications))
    return true
end

-- A forward gap in the per-event seq means we missed events; trigger a one-shot
-- HTTP friends:refresh() to reconcile stale presence.
local function testSeqGapTriggersReconcileRefresh()
    local handler, friends = setup()
    local refreshCount = 0
    friends.refresh = function() refreshCount = refreshCount + 1; return true end

    handler:handleEvent({}, "connected", 1)
    handler:handleEvent({ accountId = "a1", characterName = "alice" }, "friend_online", 2)
    assert(refreshCount == 0, "contiguous seq must not reconcile, got " .. refreshCount)

    -- seq jumps 2 -> 5 (missed 3 and 4).
    handler:handleEvent({ accountId = "a1" }, "friend_offline", 5)
    assert(refreshCount == 1, "a seq gap should trigger exactly one reconcile refresh, got " .. refreshCount)
    return true
end

-- A reconnect resets the seq baseline (new session) and the snapshot reconciles
-- on its own; the lower seq must NOT be mistaken for a gap.
local function testReconnectSeqResetDoesNotReconcile()
    local handler, friends = setup()
    local refreshCount = 0
    friends.refresh = function() refreshCount = refreshCount + 1; return true end

    handler:handleEvent({}, "connected", 100)
    handler:handleEvent({ accountId = "a1", characterName = "alice" }, "friend_online", 101)
    -- Reconnect: new session, seq restarts low; snapshot is the reconcile.
    handler:handleEvent({}, "connected", 1)
    handler:handleEvent(snapshot({ { accountId = "a1", name = "alice", online = true } }), "friends_snapshot", 2)
    handler:handleEvent({ accountId = "a1", characterName = "alice" }, "friend_online", 3)
    assert(refreshCount == 0, "reconnect/snapshot must not trigger gap reconcile, got " .. refreshCount)
    return true
end

-- getState() caches the rebuilt friends array and only rebuilds when the
-- friend list actually mutates (FriendList.revision), not every frame.
local function testGetStateCachesUntilMutation()
    local handler, friends = setup()
    handler:_dispatch("friends_snapshot", snapshot({
        { accountId = "a1", name = "alice", online = true },
    }))

    local s1 = friends:getState()
    local s2 = friends:getState()
    assert(s1.friends == s2.friends, "friends array should be reused while unchanged (same table)")

    -- A mutation must invalidate the cache (new array).
    handler:_dispatch("friend_offline", { accountId = "a1" })
    local s3 = friends:getState()
    assert(s3.friends ~= s1.friends, "friends array should be rebuilt after a mutation")
    -- And it must reflect the change.
    assert(s3.friends[1].isOnline == false, "rebuilt snapshot should reflect offline state")
    return true
end

-- ------------------------------------------------------------------ runner ---
local function run()
    local tests = {
        { name = "testGetStateCachesUntilMutation", fn = testGetStateCachesUntilMutation },
        { name = "testReconnectReSendDoesNotToastOnlineFriend", fn = testReconnectReSendDoesNotToastOnlineFriend },
        { name = "testGenuineOfflineToOnlineToasts", fn = testGenuineOfflineToOnlineToasts },
        { name = "testOfflineThenOnlineRoundTrip", fn = testOfflineThenOnlineRoundTrip },
        { name = "testGraceWindowSuppressesBurstAfterConnected", fn = testGraceWindowSuppressesBurstAfterConnected },
        { name = "testSeqGapTriggersReconcileRefresh", fn = testSeqGapTriggersReconcileRefresh },
        { name = "testReconnectSeqResetDoesNotReconcile", fn = testReconnectSeqResetDoesNotReconcile },
    }
    local passed, failed = 0, 0
    for _, t in ipairs(tests) do
        local ok, err = pcall(t.fn)
        _G.FFXIFriendListApp = nil
        if ok then
            passed = passed + 1
            print("  PASS: " .. t.name)
        else
            failed = failed + 1
            print("  FAIL: " .. t.name .. " - " .. tostring(err))
        end
    end
    print(string.format("WsReconnectResyncTest: %d passed, %d failed", passed, failed))
    return failed == 0
end

return { run = run }
