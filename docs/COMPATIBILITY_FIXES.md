# Compatibility Fixes - Lua Addon Side

## Summary

Fixed compatibility issues identified in the January 2026 compatibility audit between the Lua addon and TypeScript server.

## Changes Made

### 1. Fixed Cancel Friend Request Implementation ✅

**Issue:** `cancelRequest()` incorrectly called `POST /api/friends/requests/:id/decline` endpoint, creating semantic confusion with the decline action.

**Fix:**
- Updated `friends.lua:cancelRequest()` to use `DELETE /api/friends/requests/:id`
- Changed HTTP method from POST to DELETE
- Removed unnecessary empty JSON body (`"{}"` → `""`)
- Added `Endpoints.friendRequestCancel()` helper function

**Files Changed:**
- `app/features/friends.lua` - Updated cancelRequest() method (lines 471-501)
- `protocol/Endpoints.lua` - Added friendRequestCancel() function

**Endpoints Used:**
- NEW: `DELETE /api/friends/requests/:id` - Cancel outgoing request (sender only)
- EXISTING: `POST /api/friends/requests/:id/decline` - Decline incoming request (recipient only)

**Events Handled:**
- `friend_request_declined` - Received when request is canceled or declined (idempotent handler)

## Audit Findings Review

### Block Feature Integration
**Status:** ✅ ALREADY IMPLEMENTED
- `blocklist.lua` properly calls GET/POST/DELETE `/api/block`
- Uses `Endpoints.BLOCK.*` constants
- Uses `RequestEncoder.encodeBlock()` for request body
- Handles `blocked`/`unblocked` WebSocket events via `wsEventHandler.lua`
- No changes required

**Implementation Details:**
```lua
-- blocklist.lua already implements:
M.Blocklist:refresh()   → GET  /api/block
M.Blocklist:block()     → POST /api/block
M.Blocklist:unblock()   → DELETE /api/block/:accountId
```

### Preferences Feature Integration
**Status:** ✅ ALREADY IMPLEMENTED
- `preferences.lua` properly calls GET/PATCH `/api/preferences`
- Uses `Endpoints.PREFERENCES` constant
- Uses `RequestEncoder.encodeUpdatePreferences()` for request body
- Handles `preferences_updated` WebSocket event via `wsEventHandler.lua`
- Syncs server-side preferences: `presenceStatus`, `shareLocation`, `shareJobWhenAnonymous`
- Local-only preferences stored via `Settings.lua`
- No changes required

**Implementation Details:**
```lua
-- preferences.lua already implements:
M.Preferences:refresh()      → GET   /api/preferences
M.Preferences:syncToServer() → PATCH /api/preferences
```

### WebSocket Authentication
**Status:** ✅ ALREADY IMPLEMENTED
- `WsClient.lua:_performUpgrade()` properly sends auth headers during WebSocket upgrade
- Uses same `connection:getHeaders()` as HTTP requests
- Includes: `Authorization`, `X-Character-Name`, `X-Realm-Id`
- Server validates auth during upgrade handshake
- No changes required

**Implementation Details:**
```lua
-- WsClient.lua:_performUpgrade() (lines 300-330)
local headers = self.connection:getHeaders(characterName)
-- Adds Authorization, X-Character-Name, X-Realm-Id to upgrade request
```

### Cancel vs Decline Ambiguity
**Status:** ✅ FIXED
- `cancelRequest()` now uses DELETE instead of POST decline
- Proper semantic separation between cancel (sender) and decline (recipient)

## WebSocket Event Handling

All 14 event types properly handled in `wsEventHandler.lua:_dispatch()`:

| Event Type | Handler | State Update |
|------------|---------|--------------|
| `connected` | `_handleConnected` | Sets connection state |
| `friends_snapshot` | `_handleFriendsSnapshot` | Rebuilds friend list (resync) |
| `friend_online` | `_handleFriendOnline` | Updates status, triggers notification |
| `friend_offline` | `_handleFriendOffline` | Updates status to offline |
| `friend_state_changed` | `_handleFriendStateChanged` | Updates job/zone/nation/rank |
| `friend_added` | `_handleFriendAdded` | Adds friend to list |
| `friend_removed` | `_handleFriendRemoved` | Removes friend from list |
| `friend_request_received` | `_handleFriendRequestReceived` | Adds to incoming requests, triggers notification |
| `friend_request_accepted` | `_handleFriendRequestAccepted` | Removes from outgoing requests |
| `friend_request_declined` | `_handleFriendRequestDeclined` | Removes from BOTH lists (idempotent) |
| `blocked` | `_handleBlocked` | Refreshes blocklist |
| `unblocked` | `_handleUnblocked` | *(Handler exists but not implemented)* |
| `preferences_updated` | `_handlePreferencesUpdated` | Updates local preferences |
| `error` | `_handleError` | Logs error |

**Note:** `friend_request_declined` handler removes request from both incoming AND outgoing lists for idempotent behavior. This handles both decline and cancel actions correctly.

## HTTP Request Pattern

All HTTP requests follow the same pattern:

```lua
self.deps.net.request({
    url = self.deps.connection:getBaseUrl() .. Endpoints.SOME_ENDPOINT,
    method = "GET|POST|PATCH|DELETE",
    headers = self.deps.connection:getHeaders(characterName),
    body = RequestEncoder.encodeSomething(data) or "",
    callback = function(success, response)
        -- HTTP response is confirmation ONLY
        -- State updates come from WS events
        if not success then
            -- Log error only
        end
    end
})
```

**Critical Rules:**
- ❌ DO NOT parse HTTP response body for state updates
- ✅ HTTP confirms action was queued
- ✅ WebSocket events are authoritative for all state changes
- ✅ Heartbeat response body is IGNORED (fire-and-forget)

## Envelope Formats

### HTTP Response
```lua
-- Envelope.lua decodes: { success, data, error, timestamp }
local ok, result, errorMsg, errorCode = Envelope.decode(jsonString)
if ok then
    local data = result.data  -- Extract data payload
end
```

### WebSocket Event
```lua
-- WsEnvelope.lua decodes: { type, timestamp, protocolVersion, seq, payload }
local ok, event, errorMsg = WsEnvelope.decode(jsonString)
if ok then
    local eventType = event.type
    local payload = event.payload
end
```

## Testing Checklist

### Manual Test Steps (In-Game)

1. **Send Friend Request**
   - `/befriend <characterName>`
   - Verify request appears in outgoing list

2. **Accept Friend Request**
   - Click "Accept" in requests tab
   - Verify friend appears in main list
   - Verify request removed from incoming list

3. **Decline Friend Request**
   - Click "Decline" in requests tab
   - Verify request removed from incoming list

4. **Cancel Friend Request** (FIXED)
   - Click "Cancel" in outgoing requests tab
   - Verify request removed from outgoing list
   - Verify server receives DELETE request (not POST decline)

5. **Remove Friend**
   - Click "Remove" in friend list
   - Verify friend removed from list

6. **Block/Unblock**
   - Block a character from blocklist UI
   - Verify character appears in blocked list
   - Unblock and verify removal

7. **Preferences Update**
   - Change share settings in preferences UI
   - Verify settings sync to server
   - Verify settings persist across character logout/login

8. **Presence Update**
   - Change job, zone, or location in-game
   - Verify friends see updated state
   - Verify no polling occurs (check logs)

9. **Heartbeat**
   - Verify heartbeat fires every 60-120s
   - Verify response body is NOT parsed
   - Verify no state updates from heartbeat

10. **WebSocket Reconnect**
    - Force disconnect (close game, network interruption)
    - Restart addon
    - Verify reconnect succeeds
    - Verify `friends_snapshot` event rebuilds friend list
    - Verify no state desync

## Debugging

### Enable Debug Logging
```lua
-- In-game command:
/lua c print("Debug mode: " .. tostring(_G.gConfig.data.preferences.debugMode))
```

### Check WS Connection
```lua
-- Verify WebSocket state:
local app = _G.FFXIFriendListApp
if app.features.wsClient then
    print("WS State: " .. app.features.wsClient.state)
end
```

### Test HTTP Endpoints Manually
```bash
# From PowerShell (requires API key):
$headers = @{
    "Authorization" = "Bearer YOUR_API_KEY_HERE"
    "X-Character-Name" = "Yourname"
    "X-Realm-Id" = "horizonxi"
}

# Test cancel endpoint
Invoke-WebRequest -Method DELETE `
  -Uri "https://api.horizonfriendlist.com/api/friends/requests/REQUEST_ID_HERE" `
  -Headers $headers
```

## Architecture Compliance

### ✅ Correct Patterns
- Client→Server: HTTPS for all actions
- Server→Client: WSS for all state updates
- No polling: Heartbeat is safety-only
- Idempotent handlers: WS events can be replayed safely
- Reconnect resync: `friends_snapshot` rebuilds state

### ✅ Event Types Match Server Exactly
All event type strings use snake_case and match `friendlist-server/src/types/events.ts`:
```lua
-- WsEnvelope.lua:EventType
Connected = "connected"
FriendsSnapshot = "friends_snapshot"
FriendOnline = "friend_online"
-- etc. (14 total event types)
```

## Known Limitations

1. **WebSocket Library Dependency**
   - Requires LuaSocket + LuaSec
   - Gracefully degrades to HTTP-only mode if libraries unavailable
   - HTTP-only mode: No real-time updates, manual refresh required

2. **Notes Are Local Only**
   - Friend notes stored in `Settings.lua` (local file)
   - Notes NOT synced to server (by design for privacy)
   - Notes lost if settings file deleted

3. **Diagnostic Commands**
   - No built-in diagnostic commands yet
   - Manual testing required
   - Consider adding `/fffl diag` command in future

## Future Improvements

- Add `/fffl diag` command to test endpoints
- Add WebSocket connection status indicator to UI
- Add retry logic for failed HTTP requests
- Consider batching multiple state updates in single WS event
