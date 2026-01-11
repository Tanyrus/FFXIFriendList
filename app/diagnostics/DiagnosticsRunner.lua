--[[
* Diagnostics Runner
* Provides command-driven diagnostics for validating HTTP + WS endpoints
*
* Usage:
*   /fl diag help          - Show available commands
*   /fl diag status        - Show connection status
*   /fl diag http all      - Test all HTTP endpoints
*   /fl diag http auth     - Test auth endpoints
*   /fl diag http friends  - Test friends endpoints
*   /fl diag http presence - Test presence endpoints
*   /fl diag ws            - Test WebSocket connection
*   /fl diag all           - Run full diagnostics
*   /fl diag probe         - Probe all endpoints (no mutations)
*
* All diagnostic output is gated behind DebugMode = true in config.
* Sensitive data (API keys) are never logged.
]]--

local Endpoints = require('protocol.Endpoints')
local Json = require('protocol.Json')

local M = {}
M.__index = M

-- Test result status
local STATUS = {
    PASS = "PASS",
    FAIL = "FAIL",
    SKIP = "SKIP",
    PENDING = "PENDING"
}

-- ANSI-ish colors for chat output (FFXI uses color codes)
local COLORS = {
    GREEN = "\30\2",   -- Green text
    RED = "\30\68",    -- Red text
    YELLOW = "\30\8",  -- Yellow text
    CYAN = "\30\207",  -- Cyan text
    WHITE = "\30\1",   -- White/default
    RESET = "\30\1"    -- Reset
}

-- Generate a simple request ID
local function generateRequestId()
    local chars = "abcdefghijklmnopqrstuvwxyz0123456789"
    local id = "diag-"
    for i = 1, 16 do
        local idx = math.random(1, #chars)
        id = id .. chars:sub(idx, idx)
    end
    return id
end

--[[
* Create a new DiagnosticsRunner
* @param deps - Dependencies table with:
*   - logger: Logging functions
*   - net: Network client (nonBlockingRequests wrapper)
*   - connection: Connection feature for auth headers
*   - wsClient: WebSocket client
]]--
function M.new(deps)
    local self = setmetatable({}, M)
    self.deps = deps
    self.logger = deps.logger or {
        debug = function() end,
        info = function() end,
        error = function(msg) print("[Diag ERROR] " .. tostring(msg)) end,
        warn = function() end
    }
    self.net = deps.net
    self.connection = deps.connection
    self.wsClient = deps.wsClient
    self.enabled = false  -- Must be enabled via config
    self.results = {}
    self.pendingTests = {}
    self.currentTest = nil
    self.onComplete = nil
    return self
end

--[[
* Enable/disable diagnostics
]]--
function M:setEnabled(enabled)
    self.enabled = enabled
end

--[[
* Check if diagnostics are enabled
]]--
function M:isEnabled()
    return self.enabled
end

--[[
* Print a message to chat
]]--
function M:printChat(message, color)
    color = color or COLORS.WHITE
    print(color .. "[FFXIFriendList Diag] " .. message .. COLORS.RESET)
end

--[[
* Print a test result
]]--
function M:printResult(name, status, details)
    local color = COLORS.WHITE
    local icon = "?"
    
    if status == STATUS.PASS then
        color = COLORS.GREEN
        icon = "✓"
    elseif status == STATUS.FAIL then
        color = COLORS.RED
        icon = "✗"
    elseif status == STATUS.SKIP then
        color = COLORS.YELLOW
        icon = "○"
    end
    
    local msg = icon .. " " .. name
    if details then
        msg = msg .. " - " .. details
    end
    
    self:printChat(msg, color)
end

--[[
* Get auth headers from connection
]]--
function M:getAuthHeaders(includeCharacter)
    if not self.connection then
        return nil, "No connection feature"
    end
    
    -- Get character name for auth
    local charName = ""
    if self.connection.getCharacterName then
        charName = self.connection:getCharacterName() or ""
    end
    
    local headers = self.connection:getHeaders(charName)
    if not headers then
        return nil, "Not authenticated"
    end
    
    -- Add request ID
    local requestId = generateRequestId()
    headers["X-Request-Id"] = requestId
    
    -- If character context not needed, remove those headers
    if not includeCharacter then
        headers["X-Character-Name"] = nil
        headers["X-Realm-Id"] = nil
    end
    
    return headers, requestId
end

--[[
* Make an HTTP request and wait for result
* Returns via callback(success, response, httpStatus, requestId)
]]--
function M:httpRequest(method, endpoint, body, headers, callback)
    if not self.net then
        callback(false, nil, nil, "No network client")
        return
    end
    
    local baseUrl = ""
    if self.connection and self.connection.getBaseUrl then
        baseUrl = self.connection:getBaseUrl()
    end
    
    local url = baseUrl .. endpoint
    local requestId = headers and headers["X-Request-Id"] or generateRequestId()
    
    -- Build request options
    local options = {
        url = url,
        method = method,
        headers = headers or {},
        body = body,
        callback = function(success, response, errorStr, httpStatus)
            -- net.lua callback signature: (success, response, errorStr, httpStatus)
            -- Use httpStatus if provided, otherwise try to parse from error string
            local status = httpStatus
            if not status and errorStr and type(errorStr) == "string" then
                local statusMatch = errorStr:match("HTTP%s+(%d+)")
                if statusMatch then
                    status = tonumber(statusMatch)
                end
            end
            -- If success and no status from error, assume 200
            if success and not status then
                status = 200
            end
            callback(success, response, status, requestId, not success and errorStr or nil)
        end
    }
    
    -- Use the net wrapper's request method
    if self.net and self.net.request then
        self.net.request(options)
    else
        callback(false, nil, nil, requestId, "Network request method not available")
    end
end

--[[
* Test a single HTTP endpoint
* @param name - Test name
* @param method - HTTP method
* @param endpoint - API endpoint path
* @param options - Table with:
*   - auth: 'none', 'basic', 'character'
*   - body: Request body (optional)
*   - expectedStatus: Expected HTTP status code
*   - expectedError: Expected error code (optional)
*   - mutate: Whether this test mutates state (default false)
]]--
function M:testEndpoint(name, method, endpoint, options, callback)
    options = options or {}
    local auth = options.auth or 'basic'
    local expectedStatus = options.expectedStatus or 200
    
    -- Get headers based on auth type
    local headers = nil
    local requestId = generateRequestId()
    
    if auth == 'basic' then
        local h, rid = self:getAuthHeaders(false)
        if h then
            headers = h
            requestId = rid
        end
    elseif auth == 'character' then
        local h, rid = self:getAuthHeaders(true)
        if h then
            headers = h
            requestId = rid
        end
    end
    
    -- Add content type for POST/PATCH
    if method == "POST" or method == "PATCH" then
        headers = headers or {}
        headers["Content-Type"] = "application/json"
    end
    
    local startTime = os.clock()
    
    self:httpRequest(method, endpoint, options.body, headers, function(success, response, status, rid, err)
        local durationMs = math.floor((os.clock() - startTime) * 1000)
        
        local result = {
            name = name,
            endpoint = endpoint,
            method = method,
            requestId = rid or requestId,
            durationMs = durationMs,
            httpStatus = status,
            status = STATUS.PENDING
        }
        
        -- First check if we got the expected status code (even on "failure" responses like 401, 400)
        if status and type(expectedStatus) == "number" and status == expectedStatus then
            result.status = STATUS.PASS
        elseif status and expectedStatus == "any2xx" and status >= 200 and status < 300 then
            result.status = STATUS.PASS
        elseif status and expectedStatus == "any4xx" and status >= 400 and status < 500 then
            result.status = STATUS.PASS
        elseif err then
            result.status = STATUS.FAIL
            result.reason = "Error: " .. tostring(err)
        elseif not status then
            result.status = STATUS.FAIL
            result.reason = "No response"
        elseif type(expectedStatus) == "number" and status ~= expectedStatus then
            result.status = STATUS.FAIL
            result.reason = string.format("Expected %d, got %d", expectedStatus, status)
        elseif expectedStatus == "any2xx" and (status < 200 or status >= 300) then
            result.status = STATUS.FAIL
            result.reason = string.format("Expected 2xx, got %d", status)
        elseif expectedStatus == "any4xx" and (status < 400 or status >= 500) then
            result.status = STATUS.FAIL
            result.reason = string.format("Expected 4xx, got %d", status)
        else
            result.status = STATUS.PASS
        end
        
        table.insert(self.results, result)
        self:printResult(name, result.status, result.reason and result.reason or string.format("[%dms]", durationMs))
        
        if callback then
            callback(result)
        end
    end)
end

--[[
* Run a sequence of tests
]]--
function M:runTestSequence(tests, onComplete)
    local index = 1
    self.results = {}
    
    local function runNext()
        if index > #tests then
            -- All tests complete
            self:printSummary()
            if onComplete then
                onComplete(self.results)
            end
            return
        end
        
        local test = tests[index]
        index = index + 1
        
        self:testEndpoint(test.name, test.method, test.endpoint, test, function()
            -- Small delay between tests to avoid rate limiting
            runNext()
        end)
    end
    
    runNext()
end

--[[
* Print test summary
]]--
function M:printSummary()
    local passed = 0
    local failed = 0
    local skipped = 0
    
    for _, result in ipairs(self.results) do
        if result.status == STATUS.PASS then
            passed = passed + 1
        elseif result.status == STATUS.FAIL then
            failed = failed + 1
        elseif result.status == STATUS.SKIP then
            skipped = skipped + 1
        end
    end
    
    local total = passed + failed + skipped
    
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("DIAGNOSTICS SUMMARY", COLORS.CYAN)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat(string.format("Total: %d | Passed: %d | Failed: %d | Skipped: %d", total, passed, failed, skipped))
    
    if failed > 0 then
        self:printChat(string.format("%d TEST(S) FAILED", failed), COLORS.RED)
    else
        self:printChat("ALL TESTS PASSED", COLORS.GREEN)
    end
end

--[[
* Test public endpoints (no auth required)
]]--
function M:testPublicEndpoints(callback)
    local tests = {
        {name = "GET /health", method = "GET", endpoint = Endpoints.HEALTH, auth = "none", expectedStatus = 200},
        {name = "GET /api/version", method = "GET", endpoint = Endpoints.VERSION, auth = "none", expectedStatus = 200},
        {name = "GET /api/servers", method = "GET", endpoint = Endpoints.SERVERS, auth = "none", expectedStatus = 200},
    }
    
    self:printChat("─── PUBLIC ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test auth endpoints
]]--
function M:testAuthEndpoints(callback)
    local tests = {
        {name = "POST /auth/ensure - no auth (401)", method = "POST", endpoint = Endpoints.AUTH.ENSURE, auth = "none", expectedStatus = 401},
        {name = "POST /auth/ensure - success", method = "POST", endpoint = Endpoints.AUTH.ENSURE, auth = "basic", expectedStatus = 200},
        {name = "GET /auth/me - success", method = "GET", endpoint = Endpoints.AUTH.ME, auth = "basic", expectedStatus = 200},
    }
    
    self:printChat("─── AUTH ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test friends endpoints (read-only)
]]--
function M:testFriendsEndpoints(callback)
    local tests = {
        {name = "GET /friends - no auth (401)", method = "GET", endpoint = Endpoints.FRIENDS.LIST, auth = "none", expectedStatus = 401},
        {name = "GET /friends - success", method = "GET", endpoint = Endpoints.FRIENDS.LIST, auth = "basic", expectedStatus = 200},
        {name = "GET /friends/requests/pending", method = "GET", endpoint = Endpoints.FRIENDS.REQUESTS_PENDING, auth = "basic", expectedStatus = 200},
        {name = "GET /friends/requests/outgoing", method = "GET", endpoint = Endpoints.FRIENDS.REQUESTS_OUTGOING, auth = "basic", expectedStatus = 200},
        {name = "POST /friends/request - no char (400)", method = "POST", endpoint = Endpoints.FRIENDS.SEND_REQUEST, auth = "basic", body = '{"characterName":"Test","realmId":"horizon"}', expectedStatus = 400},
    }
    
    self:printChat("─── FRIENDS ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test presence endpoints
]]--
function M:testPresenceEndpoints(callback)
    local tests = {
        {name = "POST /presence/heartbeat - no auth (401)", method = "POST", endpoint = Endpoints.PRESENCE.HEARTBEAT, auth = "none", expectedStatus = 401},
        {name = "POST /presence/heartbeat - no char (400)", method = "POST", endpoint = Endpoints.PRESENCE.HEARTBEAT, auth = "basic", body = '{}', expectedStatus = 400},
        {name = "POST /presence/heartbeat - success (204)", method = "POST", endpoint = Endpoints.PRESENCE.HEARTBEAT, auth = "character", body = '{}', expectedStatus = 204},
        {name = "POST /presence/update - no char (400)", method = "POST", endpoint = Endpoints.PRESENCE.UPDATE, auth = "basic", body = '{}', expectedStatus = 400},
    }
    
    self:printChat("─── PRESENCE ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test preferences endpoints
]]--
function M:testPreferencesEndpoints(callback)
    local tests = {
        {name = "GET /preferences - no auth (401)", method = "GET", endpoint = Endpoints.PREFERENCES, auth = "none", expectedStatus = 401},
        {name = "GET /preferences - success", method = "GET", endpoint = Endpoints.PREFERENCES, auth = "basic", expectedStatus = 200},
    }
    
    self:printChat("─── PREFERENCES ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test block endpoints
]]--
function M:testBlockEndpoints(callback)
    local tests = {
        {name = "GET /block - no auth (401)", method = "GET", endpoint = Endpoints.BLOCK.LIST, auth = "none", expectedStatus = 401},
        {name = "GET /block - success", method = "GET", endpoint = Endpoints.BLOCK.LIST, auth = "basic", expectedStatus = 200},
    }
    
    self:printChat("─── BLOCK ENDPOINTS ───", COLORS.CYAN)
    self:runTestSequence(tests, callback)
end

--[[
* Test WebSocket connection
]]--
function M:testWebSocket(callback)
    self:printChat("─── WEBSOCKET ───", COLORS.CYAN)
    
    if not self.wsClient then
        self:printResult("WS connection", STATUS.SKIP, "WS client not available")
        if callback then callback() end
        return
    end
    
    -- Check current WS state
    local state = self.wsClient:getState()
    
    -- WsClient uses capitalized states: "Connected", "Connecting", "Disconnected", etc.
    if state == "Connected" then
        self:printResult("WS connection", STATUS.PASS, "Connected")
        
        -- Check if we received snapshot
        if self.wsClient.receivedSnapshot then
            self:printResult("WS friends_snapshot", STATUS.PASS, "Received")
        else
            self:printResult("WS friends_snapshot", STATUS.FAIL, "Not received yet")
        end
    elseif state == "Connecting" then
        self:printResult("WS connection", STATUS.PENDING, "Still connecting...")
    else
        self:printResult("WS connection", STATUS.FAIL, "State: " .. tostring(state))
    end
    
    if callback then callback() end
end

--[[
* Show current connection status
]]--
function M:showStatus()
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("CONNECTION STATUS", COLORS.CYAN)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    
    -- Server URL
    if self.connection and self.connection.getServerUrl then
        self:printChat("Server: " .. tostring(self.connection:getServerUrl()))
    else
        self:printChat("Server: (unknown)")
    end
    
    -- Auth status
    if self.connection then
        local charName = self.connection:getCharacterName()
        local apiKey = self.connection:getApiKey(charName)
        if apiKey and apiKey ~= "" then
            self:printChat("Auth: Authenticated (API key set)", COLORS.GREEN)
        else
            self:printChat("Auth: Not authenticated", COLORS.RED)
        end
        
        local realmId = self.connection.savedRealmId or "(unknown)"
        if charName and charName ~= "" then
            self:printChat("Character: " .. tostring(charName) .. "@" .. tostring(realmId))
        else
            self:printChat("Character: Not set", COLORS.YELLOW)
        end
        
        -- BUG 2 FIX: Show active character info
        if self.connection.lastCharacterId then
            self:printChat("Character ID: " .. tostring(self.connection.lastCharacterId))
        end
        if self.connection.lastSetActiveAt then
            local ago = math.floor((os.time() * 1000 - self.connection.lastSetActiveAt) / 1000)
            self:printChat("Last set-active: " .. tostring(ago) .. "s ago", COLORS.GREEN)
        else
            self:printChat("Last set-active: (none)", COLORS.YELLOW)
        end
    end
    
    -- WebSocket status
    if self.wsClient then
        local state = self.wsClient:getState()
        local color = COLORS.WHITE
        if state == "connected" then
            color = COLORS.GREEN
        elseif state == "connecting" then
            color = COLORS.YELLOW
        else
            color = COLORS.RED
        end
        self:printChat("WebSocket: " .. tostring(state), color)
        
        if self.wsClient.receivedSnapshot then
            self:printChat("Snapshot: Received", COLORS.GREEN)
        else
            self:printChat("Snapshot: Not received", COLORS.YELLOW)
        end
    else
        self:printChat("WebSocket: Not available", COLORS.RED)
    end
    
    -- BUG 1 FIX: Show notes/tags pending counts
    self:printChat("─── LOCAL METADATA ───", COLORS.CYAN)
    local app = _G.FFXIFriendListApp
    if app and app.features then
        -- Notes state
        if app.features.notes then
            local notesState = app.features.notes:getState()
            self:printChat(string.format("Notes: %d stored, %d pending drafts", 
                notesState.noteCount or 0, notesState.pendingCount or 0))
        end
        
        -- Tags state
        if app.features.tags then
            local tagsState = app.features.tags:getState()
            self:printChat(string.format("Tags: %d tags, %d pending drafts", 
                tagsState.tagCount or 0, tagsState.pendingCount or 0))
        end
        
        -- BUG 3 FIX: Show friend presence snapshot info
        if app.features.friends and app.features.friends.friendList then
            local allFriends = app.features.friends.friendList:getFriends()
            local onlineCount = 0
            local awayCount = 0
            local hasAwayField = true
            
            for _, friend in ipairs(allFriends) do
                local status = app.features.friends.friendList:getFriendStatus(friend.name)
                if status then
                    if status.isOnline then
                        onlineCount = onlineCount + 1
                    end
                    if status.isAway then
                        awayCount = awayCount + 1
                    end
                    -- Check if isAway field exists (for diagnostics)
                    if status.isAway == nil then
                        hasAwayField = false
                    end
                end
            end
            
            self:printChat(string.format("Friends: %d total, %d online, %d away", 
                #allFriends, onlineCount, awayCount))
            
            if hasAwayField then
                self:printChat("Away status: Available in snapshot", COLORS.GREEN)
            else
                self:printChat("Away status: Missing from snapshot (BUG 3)", COLORS.RED)
            end
        end
    end
    
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
end

--[[
* Show help
]]--
function M:showHelp()
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("DIAGNOSTICS COMMANDS", COLORS.CYAN)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("/fl diag help          - Show this help")
    self:printChat("/fl diag status        - Show connection status")
    self:printChat("/fl diag http all      - Test all HTTP endpoints")
    self:printChat("/fl diag http auth     - Test auth endpoints")
    self:printChat("/fl diag http friends  - Test friends endpoints")
    self:printChat("/fl diag http presence - Test presence endpoints")
    self:printChat("/fl diag http prefs    - Test preferences endpoints")
    self:printChat("/fl diag http block    - Test block endpoints")
    self:printChat("/fl diag ws            - Test WebSocket connection")
    self:printChat("/fl diag probe         - Probe all endpoints (read-only)")
    self:printChat("/fl diag all           - Run full diagnostics")
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
end

--[[
* Run probe (all read-only endpoints)
]]--
function M:runProbe(callback)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("ENDPOINT PROBE (read-only)", COLORS.CYAN)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    
    local allTests = {
        -- Public
        {name = "GET /health", method = "GET", endpoint = Endpoints.HEALTH, auth = "none", expectedStatus = 200},
        {name = "GET /api/version", method = "GET", endpoint = Endpoints.VERSION, auth = "none", expectedStatus = 200},
        {name = "GET /api/servers", method = "GET", endpoint = Endpoints.SERVERS, auth = "none", expectedStatus = 200},
        -- Auth
        {name = "POST /auth/ensure", method = "POST", endpoint = Endpoints.AUTH.ENSURE, auth = "basic", expectedStatus = 200},
        {name = "GET /auth/me", method = "GET", endpoint = Endpoints.AUTH.ME, auth = "basic", expectedStatus = 200},
        -- Friends (read-only)
        {name = "GET /friends", method = "GET", endpoint = Endpoints.FRIENDS.LIST, auth = "basic", expectedStatus = 200},
        {name = "GET /friends/requests/pending", method = "GET", endpoint = Endpoints.FRIENDS.REQUESTS_PENDING, auth = "basic", expectedStatus = 200},
        {name = "GET /friends/requests/outgoing", method = "GET", endpoint = Endpoints.FRIENDS.REQUESTS_OUTGOING, auth = "basic", expectedStatus = 200},
        -- Preferences
        {name = "GET /preferences", method = "GET", endpoint = Endpoints.PREFERENCES, auth = "basic", expectedStatus = 200},
        -- Block
        {name = "GET /block", method = "GET", endpoint = Endpoints.BLOCK.LIST, auth = "basic", expectedStatus = 200},
    }
    
    self:runTestSequence(allTests, callback)
end

--[[
* Run all HTTP tests
]]--
function M:runAllHttpTests(callback)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    self:printChat("ALL HTTP ENDPOINT TESTS", COLORS.CYAN)
    self:printChat("═══════════════════════════════════════", COLORS.CYAN)
    
    local self_ = self
    
    self:testPublicEndpoints(function()
        self_:testAuthEndpoints(function()
            self_:testFriendsEndpoints(function()
                self_:testPresenceEndpoints(function()
                    self_:testPreferencesEndpoints(function()
                        self_:testBlockEndpoints(callback)
                    end)
                end)
            end)
        end)
    end)
end

--[[
* Run full diagnostics
]]--
function M:runAll(callback)
    self:printChat("╔═══════════════════════════════════════╗", COLORS.CYAN)
    self:printChat("║    FFXIFRIENDLIST FULL DIAGNOSTICS    ║", COLORS.CYAN)
    self:printChat("╚═══════════════════════════════════════╝", COLORS.CYAN)
    
    local self_ = self
    
    self:runAllHttpTests(function()
        self_:testWebSocket(callback)
    end)
end

--[[
* Handle diag command
* @param args - Array of command arguments (after 'diag')
]]--
function M:handleCommand(args)
    if not self.enabled then
        print("[FFXIFriendList] Diagnostics disabled. Enable DebugMode in config first.")
        return
    end
    
    if not args or #args == 0 then
        self:showHelp()
        return
    end
    
    local cmd = args[1]:lower()
    
    if cmd == "help" then
        self:showHelp()
    elseif cmd == "status" then
        self:showStatus()
    elseif cmd == "probe" then
        self:runProbe()
    elseif cmd == "ws" then
        self:testWebSocket()
    elseif cmd == "all" then
        self:runAll()
    elseif cmd == "http" then
        local subCmd = args[2] and args[2]:lower() or "all"
        
        if subCmd == "all" then
            self:runAllHttpTests()
        elseif subCmd == "auth" then
            self:testAuthEndpoints()
        elseif subCmd == "friends" then
            self:testFriendsEndpoints()
        elseif subCmd == "presence" then
            self:testPresenceEndpoints()
        elseif subCmd == "prefs" or subCmd == "preferences" then
            self:testPreferencesEndpoints()
        elseif subCmd == "block" then
            self:testBlockEndpoints()
        else
            self:printChat("Unknown HTTP test category: " .. subCmd, COLORS.RED)
            self:showHelp()
        end
    else
        self:printChat("Unknown command: " .. cmd, COLORS.RED)
        self:showHelp()
    end
end

return M

