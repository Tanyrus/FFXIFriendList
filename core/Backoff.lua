-- core/Backoff.lua
-- Shared exponential-backoff-with-jitter helper.
--
-- Previously the WebSocket connection manager and the HTTP auth retry path each
-- hand-rolled the same "base * 2^(attempt-1), clamp to max, apply +/- jitter,
-- floor to a minimum" math with subtly different jitter implementations. They
-- could fight each other and drift. This is the single source of truth.

local M = {}

-- Compute the delay (ms) before retry `attempt` (1-based).
-- opts:
--   baseMs        base delay (default 1000)
--   maxMs         clamp ceiling before jitter (default 60000)
--   jitterPercent +/- jitter as a fraction of the delay (default 0.2)
--   minMs         hard floor after jitter (default 100)
--   rand          () -> [0,1) source, injectable for tests (default math.random)
function M.compute(attempt, opts)
    opts = opts or {}
    local baseMs = opts.baseMs or 1000
    local maxMs = opts.maxMs or 60000
    local jitterPercent = opts.jitterPercent or 0.2
    local minMs = opts.minMs or 100
    local rand = opts.rand or math.random

    attempt = math.max(1, attempt or 1)

    local delay = baseMs * (2 ^ (attempt - 1))
    delay = math.min(delay, maxMs)

    -- Symmetric jitter: rand() in [0,1) maps to [-jitterRange, +jitterRange).
    local jitterRange = delay * jitterPercent
    local jitter = (rand() * 2 - 1) * jitterRange
    delay = math.floor(delay + jitter)

    return math.max(minMs, delay)
end

return M
