-- TimingConstants.lua
-- Timing constants for the addon

local M = {}

-- Heartbeat interval: 60-120 seconds (safety-only, fire-and-forget)
-- This is NOT polling - heartbeat response body is IGNORED
M.HEARTBEAT_INTERVAL_MS = 90000  -- 90 seconds (middle of 60-120 range)

-- Zone change debounce
M.ZONE_CHANGE_DEBOUNCE_MS = 1000

-- Retry settings for failed requests
M.INITIAL_RETRY_DELAY_MS = 1000
M.MAX_RETRY_DELAY_MS = 30000
M.MAX_RETRIES = 3

-- WebSocket reconnection delays (longer than HTTP retries)
M.WS_BASE_RECONNECT_DELAY_MS = 1000
M.WS_MAX_RECONNECT_DELAY_MS = 60000

-- Half-open socket detection. The server pings every 30s, so a CONNECTED socket
-- that has seen NO inbound bytes for this long is presumed dead (NAT/route drop
-- with no RST) and force-disconnected so reconnect backoff can take over.
M.WS_INBOUND_STALE_TIMEOUT_MS = 90000  -- 3 missed server pings

-- After (re)connect the server re-sends friend_online for every online friend
-- (plus a friends_snapshot). Suppress "friend online" toasts for this long so
-- the re-sync burst is not mistaken for fresh online events.
M.WS_ONLINE_RESYNC_GRACE_MS = 5000

-- Notification display durations
M.NOTIFICATION_DEFAULT_DURATION_MS = 5000
M.NOTIFICATION_LONG_DURATION_MS = 8000

-- Request cleanup
M.REQUEST_CLEANUP_TIMEOUT_SECONDS = 60

return M
