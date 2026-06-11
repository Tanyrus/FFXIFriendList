-- BackoffTest.lua
-- Unit tests for the shared exponential-backoff helper (core/Backoff).

local Backoff = require("core.Backoff")

-- Deterministic rand: returns the configured value so jitter is predictable.
local function fixedRand(v) return function() return v end end

local function approx(a, b) return math.abs(a - b) <= 1 end

local function testNoJitterAtMidpoint()
    -- rand()=0.5 -> (0.5*2-1)=0 jitter, so delay == base*2^(attempt-1).
    local opts = { baseMs = 1000, maxMs = 60000, jitterPercent = 0.2, minMs = 100, rand = fixedRand(0.5) }
    assert(approx(Backoff.compute(1, opts), 1000), "attempt 1 should be base")
    assert(approx(Backoff.compute(2, opts), 2000), "attempt 2 should double")
    assert(approx(Backoff.compute(3, opts), 4000), "attempt 3 should double again")
    return true
end

local function testCappedAtMax()
    local opts = { baseMs = 1000, maxMs = 5000, jitterPercent = 0.0, minMs = 100, rand = fixedRand(0.5) }
    assert(approx(Backoff.compute(10, opts), 5000), "should cap at maxMs")
    return true
end

local function testMinFloor()
    -- Large negative jitter (rand()=0) must not push below minMs.
    local opts = { baseMs = 1000, maxMs = 60000, jitterPercent = 1.0, minMs = 250, rand = fixedRand(0) }
    -- jitter = (0*2-1)*1000 = -1000 -> 0 -> floored to minMs
    assert(Backoff.compute(1, opts) == 250, "should not drop below minMs")
    return true
end

local function testJitterWithinBounds()
    local opts = { baseMs = 1000, maxMs = 60000, jitterPercent = 0.2, minMs = 100 }
    -- rand()=0 -> -20%, rand()~1 -> +20%
    local low = Backoff.compute(2, { baseMs = 1000, maxMs = 60000, jitterPercent = 0.2, minMs = 100, rand = fixedRand(0) })
    local high = Backoff.compute(2, { baseMs = 1000, maxMs = 60000, jitterPercent = 0.2, minMs = 100, rand = fixedRand(0.999999) })
    assert(approx(low, 1600), "low jitter ~ -20% of 2000")
    assert(approx(high, 2400), "high jitter ~ +20% of 2000")
    return true
end

local function testAttemptFloored()
    -- attempt < 1 is clamped to 1.
    local opts = { baseMs = 1000, maxMs = 60000, jitterPercent = 0.0, minMs = 100, rand = fixedRand(0.5) }
    assert(approx(Backoff.compute(0, opts), 1000), "attempt 0 treated as 1")
    return true
end

local function run()
    local tests = {
        { name = "testNoJitterAtMidpoint", fn = testNoJitterAtMidpoint },
        { name = "testCappedAtMax", fn = testCappedAtMax },
        { name = "testMinFloor", fn = testMinFloor },
        { name = "testJitterWithinBounds", fn = testJitterWithinBounds },
        { name = "testAttemptFloored", fn = testAttemptFloored },
    }
    local passed, failed = 0, 0
    for _, t in ipairs(tests) do
        local ok, err = pcall(t.fn)
        if ok then passed = passed + 1; print("  PASS: " .. t.name)
        else failed = failed + 1; print("  FAIL: " .. t.name .. " - " .. tostring(err)) end
    end
    print(string.format("BackoffTest: %d passed, %d failed", passed, failed))
    return failed == 0
end

return { run = run }
