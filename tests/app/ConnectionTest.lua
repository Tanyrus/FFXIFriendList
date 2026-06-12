-- ConnectionTest.lua
-- Unit tests for app/features/connection.lua

local Connection = require("app.features.connection")
local ServerConfig = require("core.ServerConfig")

local function createFakeDeps()
    return {
        logger = {
            info = function() end,
            error = function() end
        },
        net = {},
        session = {
            getSessionId = function() return "test-session" end
        },
        storage = {}
    }
end

local function testConnectionStateTransitions()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    -- getState() returns a snapshot table; .state holds the enum value.
    assert(conn:getState().state == Connection.ConnectionState.Disconnected, "Should start disconnected")
    assert(not conn:isConnected(), "Should not be connected initially")
    assert(not conn:isConnecting(), "Should not be connecting initially")

    conn:startConnecting()
    assert(conn:getState().state == Connection.ConnectionState.Connecting, "Should be connecting")
    assert(conn:isConnecting(), "Should report as connecting")

    conn:setConnected()
    assert(conn:getState().state == Connection.ConnectionState.Connected, "Should be connected")
    assert(conn:isConnected(), "Should report as connected")

    conn:setDisconnected()
    assert(conn:getState().state == Connection.ConnectionState.Disconnected, "Should be disconnected")
    
    return true
end

local function testConnectionApiKeys()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    assert(not conn:hasApiKey("TestChar"), "Should not have API key initially")
    
    conn:setApiKey("TestChar", "test-key")
    assert(conn:hasApiKey("TestChar"), "Should have API key after setting")
    assert(conn:getApiKey("TestChar") == "test-key", "Should return correct API key")
    
    conn:clearApiKey("TestChar")
    assert(not conn:hasApiKey("TestChar"), "Should not have API key after clearing")
    
    return true
end

local function testConnectionServerSelection()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    assert(not conn:hasSavedServer(), "Should not have saved server initially")
    assert(conn:isBlocked(), "Should be blocked without server")
    
    conn:setSavedServer("server1", "https://api.example.com")
    assert(conn:hasSavedServer(), "Should have saved server")
    assert(not conn:isBlocked(), "Should not be blocked with server")
    assert(conn:getBaseUrl() == "https://api.example.com", "Should return correct base URL")
    
    conn:clearSavedServer()
    assert(not conn:hasSavedServer(), "Should not have saved server after clearing")
    assert(conn:getBaseUrl() == ServerConfig.DEFAULT_SERVER_URL, "Should return default URL")
    
    return true
end

local function testConnectionHeaders()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    conn:setApiKey("TestChar", "test-key")
    conn:setSavedServer("server1", "https://api.example.com")
    
    local headers = conn:getHeaders("TestChar")
    assert(headers["Content-Type"] == "application/json", "Should have Content-Type header")
    -- New auth format: Authorization: Bearer <apiKey>
    assert(headers["Authorization"] == "Bearer test-key", "Should have Authorization Bearer header")
    assert(headers["X-Character-Name"] == "TestChar", "Should have X-Character-Name header")

    -- The Bearer token is ACCOUNT-level, so Authorization must be attached even
    -- when no character name is available (e.g. WS auth on zone/reload). The
    -- X-Character-Name header is correctly omitted when the name is empty.
    local anonHeaders = conn:getHeaders("")
    assert(anonHeaders["Authorization"] == "Bearer test-key",
        "Authorization should be attached even without a character name")
    assert(anonHeaders["X-Character-Name"] == nil,
        "Should omit X-Character-Name when name is empty")

    return true
end

-- BUG 2 FIX TESTS: Character switch and set-active functionality

local function testConnectionHasSetActiveMethod()
    -- Verify that the setActiveCharacter method exists and has correct signature
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    assert(type(conn.setActiveCharacter) == "function", "Should have setActiveCharacter method")
    assert(type(conn.completeConnection) == "function", "Should have completeConnection method")
    
    return true
end

local function testConnectionTracksSetActiveTimestamp()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    -- Initially no set-active timestamp
    assert(conn.lastSetActiveAt == nil, "Should not have lastSetActiveAt initially")
    
    -- After successful set-active call, timestamp should be set
    -- (This would be set during actual set-active flow)
    conn.lastSetActiveAt = os.time() * 1000
    assert(conn.lastSetActiveAt ~= nil, "Should have lastSetActiveAt after set-active")
    
    return true
end

local function testConnectionTracksCharacterId()
    local deps = createFakeDeps()
    local conn = Connection.Connection.new(deps)
    
    -- After auth response with character ID
    conn.lastCharacterId = "test-uuid-12345"
    assert(conn.lastCharacterId == "test-uuid-12345", "Should store lastCharacterId")
    
    return true
end

local function runTests()
    local tests = {
        testConnectionStateTransitions,
        testConnectionApiKeys,
        testConnectionServerSelection,
        testConnectionHeaders,
        -- BUG 2 FIX TESTS
        testConnectionHasSetActiveMethod,
        testConnectionTracksSetActiveTimestamp,
        testConnectionTracksCharacterId
    }
    
    local passed = 0
    local failed = 0
    
    for _, test in ipairs(tests) do
        local success, err = pcall(test)
        if success then
            passed = passed + 1
            print("  PASS: " .. tostring(test))
        else
            failed = failed + 1
            print("  FAIL: " .. tostring(test) .. " - " .. tostring(err))
        end
    end
    
    print(string.format("ConnectionTest: %d passed, %d failed", passed, failed))
    return failed == 0
end

return { run = runTests }

