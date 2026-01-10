# Addon Diagnostics

The FFXIFriendList addon includes built-in diagnostics to validate server connectivity and endpoint functionality.

## Quick Start

1. Enable debug mode:
   ```
   /fl debug
   ```

2. Run diagnostics:
   ```
   /fl diag all
   ```

## Commands

| Command | Description |
|---------|-------------|
| `/fl debug` | Toggle debug mode (required for diagnostics) |
| `/fl diag help` | Show available diagnostic commands |
| `/fl diag status` | Show current connection status |
| `/fl diag http all` | Test all HTTP endpoints |
| `/fl diag http auth` | Test authentication endpoints |
| `/fl diag http friends` | Test friends endpoints |
| `/fl diag http presence` | Test presence endpoints |
| `/fl diag http prefs` | Test preferences endpoints |
| `/fl diag http block` | Test block endpoints |
| `/fl diag ws` | Test WebSocket connection |
| `/fl diag probe` | Probe all endpoints (read-only) |
| `/fl diag all` | Run complete diagnostics |

## Test Coverage

### HTTP Endpoints (Read-Only Probe)

The probe command tests these endpoints without making changes:

- `GET /health` - Server health
- `GET /api/version` - Protocol version
- `GET /api/servers` - Available realms
- `POST /api/auth/ensure` - Account verification
- `GET /api/auth/me` - Account info
- `GET /api/friends` - Friend list
- `GET /api/friends/requests/pending` - Pending requests
- `GET /api/friends/requests/outgoing` - Outgoing requests
- `GET /api/preferences` - User preferences
- `GET /api/block` - Block list

### HTTP Endpoints (Full Test)

Full tests include validation of:

1. **401 responses** - Requests without authentication
2. **400 responses** - Requests missing required fields
3. **2xx responses** - Successful authenticated requests

### WebSocket Tests

- Connection state verification
- `connected` event receipt
- `friends_snapshot` event receipt

## Sample Output

```
═══════════════════════════════════════
ALL HTTP ENDPOINT TESTS
═══════════════════════════════════════
─── PUBLIC ENDPOINTS ───
✓ GET /health [45ms]
✓ GET /api/version [32ms]
✓ GET /api/servers [28ms]
─── AUTH ENDPOINTS ───
✓ POST /auth/ensure - no auth (401) [15ms]
✓ POST /auth/ensure - success [67ms]
✓ GET /auth/me - success [43ms]
─── FRIENDS ENDPOINTS ───
✓ GET /friends - no auth (401) [12ms]
✓ GET /friends - success [89ms]
✓ GET /friends/requests/pending [34ms]
✓ GET /friends/requests/outgoing [31ms]
✓ POST /friends/request - no char (400) [18ms]
...
─── WEBSOCKET ───
✓ WS connection - Connected
✓ WS friends_snapshot - Received
═══════════════════════════════════════
DIAGNOSTICS SUMMARY
═══════════════════════════════════════
Total: 18 | Passed: 18 | Failed: 0 | Skipped: 0
ALL TESTS PASSED
```

## Connection Status

Use `/fl diag status` to see current connection state:

```
═══════════════════════════════════════
CONNECTION STATUS
═══════════════════════════════════════
Server: https://api.horizonfriendlist.com
Auth: Authenticated (API key set)
Character: MyCharacter@horizon
WebSocket: connected
Snapshot: Received
═══════════════════════════════════════
```

## Requirements

### Debug Mode

Diagnostics require debug mode to be enabled:

```
/fl debug
```

This prevents accidental diagnostic output during normal gameplay.

### Authentication

Most tests require a valid API key. Ensure you have:
1. Selected a server
2. Authenticated with the server
3. Set an active character

## Interpreting Results

### PASS (✓)

The endpoint responded as expected.

### FAIL (✗)

The endpoint did not respond as expected. Common causes:
- Server not reachable
- Invalid or expired API key
- Missing character context
- Server error

### SKIP (○)

The test was skipped, usually because a prerequisite wasn't met (e.g., WS client not available).

## Troubleshooting

### "Diagnostics disabled"

Enable debug mode first:
```
/fl debug
```

### All tests fail

1. Check server connection: `/fl diag status`
2. Verify you're authenticated (API key set)
3. Check if server is reachable

### WebSocket tests fail

1. Check WS state in status output
2. Ensure character is logged in
3. Try reconnecting by zoning

### "No network client"

The addon's network layer isn't initialized. Try:
1. Reload the addon: `/addon unload FFXIFriendList` then `/addon load FFXIFriendList`
2. Check for Lua errors in the console

## Request Correlation

Each diagnostic request includes an `X-Request-Id` header with format `diag-xxxxxxxxxxxx`. This ID can be used to correlate addon tests with server-side logs for debugging.

## Security

- API keys are never shown in diagnostic output
- Diagnostic commands require explicit debug mode
- Read-only probe mode available to avoid state changes
- No sensitive user data is logged

