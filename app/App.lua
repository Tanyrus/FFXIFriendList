-- App.lua
-- App orchestrator: composes features and provides unified API
-- Entry file calls initialize() on load and release() on unload

local Connection = require("app.features.connection")
local Friends = require("app.features.friends")
local ServerList = require("app.features.serverlist")
local Themes = require("app.features.themes")
local Notifications = require("app.features.notifications")
local Preferences = require("app.features.preferences")
local Notes = require("app.features.notes")
local AltVisibility = require("app.features.altvisibility")
local CharacterVisibility = require("app.features.characterVisibility")
local Tags = require("app.features.Tags")
local Blocklist = require("app.features.blocklist")
local WsClientAsync = require("platform.services.WsClientAsync")
local WsConnectionManager = require("platform.services.WsConnectionManager")
local WsEventHandler = require("app.features.wsEventHandler")

local App = {}

-- Create app instance with injected dependencies
-- deps: { net, storage, logger, session, time? }
-- Returns app object with features and lifecycle methods
function App.create(deps)
    deps = deps or {}
    
    local app = {
        deps = deps,
        features = {},
        initialized = false,
        _missingFeatureLogged = {},  -- Track missing features for logging
        _startupRefreshCompleted = false  -- Track if startup refresh has run
    }
    
    -- Initialize features with deps (all use new(deps) signature)
    app.features.connection = Connection.Connection.new(deps)
    
    -- Add connection to deps for features that need it
    deps.connection = app.features.connection
    
    app.features.friends = Friends.Friends.new(deps)
    app.features.serverlist = ServerList.ServerList.new(deps)
    app.features.themes = Themes.Themes.new(deps)
    app.features.notifications = Notifications.Notifications.new(deps)
    app.features.preferences = Preferences.Preferences.new(deps)
    app.features.notes = Notes.Notes.new(deps)
    app.features.altVisibility = AltVisibility.AltVisibility.new(deps)
    app.features.characterVisibility = CharacterVisibility.CharacterVisibility.new(deps)
    app.features.tags = Tags.Tags.new(deps)
    app.features.blocklist = Blocklist.Blocklist.new(deps)
    
    -- Initialize non-blocking WebSocket client
    app.features.wsClient = WsClientAsync.WsClientAsync.new(deps)
    
    -- Initialize connection manager with backoff (wraps wsClient)
    app.features.wsConnectionManager = WsConnectionManager.WsConnectionManager.new({
        logger = deps.logger,
        wsClient = app.features.wsClient,
        connection = app.features.connection,
        time = deps.time
    })
    
    -- Initialize event handler
    app.features.wsEventHandler = WsEventHandler.WsEventHandler.new({
        logger = deps.logger,
        friends = app.features.friends,
        blocklist = app.features.blocklist,
        preferences = app.features.preferences,
        notifications = app.features.notifications,
        connection = app.features.connection,
        tags = app.features.tags,
        notes = app.features.notes,
        time = deps.time
    })
    
    -- Register WS event handler with the event router
    if app.features.wsClient and app.features.wsClient.eventRouter then
        -- Create a single handler that routes to wsEventHandler
        local handler = function(payload, eventType, seq, timestamp)
            if app.features.wsEventHandler then
                app.features.wsEventHandler:handleEvent(payload, eventType, seq, timestamp)
            end
        end
        
        -- Register handlers for all WS event types (from friendlist-server/src/types/events.ts)
        local eventTypes = {
            "connected", "friends_snapshot", "friend_online", "friend_offline",
            "friend_state_changed", "friend_added", "friend_removed",
            "friend_request_received", "friend_request_accepted", "friend_request_declined",
            "blocked", "unblocked", "preferences_updated", "error"
        }
        for _, eventType in ipairs(eventTypes) do
            app.features.wsClient.eventRouter:registerHandler(eventType, handler)
        end
        
        if deps.logger and deps.logger.debug then
            deps.logger.debug("[App] Registered " .. #eventTypes .. " WS event handlers")
        end
    end
    
    return app
end

-- Initialize app (load persisted state, start connections)
-- Entry file calls this on load event
function App.initialize(app)
    if app.initialized then
        return true
    end
    
    if app.deps.logger and app.deps.logger.info then
        app.deps.logger.info("[App] Initializing")
    end
    
    -- Load persisted preferences via deps.storage
    if app.features.preferences and app.features.preferences.load and app.deps.storage then
        local _ = app.features.preferences:load()
    end
    
    -- Load persisted notes via deps.storage
    if app.features.notes and app.features.notes.load and app.deps.storage then
        local _ = app.features.notes:load()
    end
    
    -- Load persisted tags via deps.storage
    if app.features.tags and app.features.tags.load and app.deps.storage then
        local _ = app.features.tags:load()
    end
    
    -- Load persisted themes from file
    if app.features.themes and app.features.themes.loadThemes then
        app.features.themes:loadThemes()
    end
    
    -- Refresh server list on startup (needed for server selection to work)
    if app.features.serverlist and app.features.serverlist.refresh then
        app.features.serverlist:refresh()
    end
    
    -- Sync connection module with serverlist if server is already selected
    if app.features.connection and app.features.serverlist then
        local selected = app.features.serverlist:getSelected()
        if selected then
            app.features.connection:setServer(selected.id, selected.baseUrl)
        end
    end
    
    app.initialized = true
    return nil
end

-- Check if game is ready and character is loaded
-- Exported as App.isGameReady() for use by entry file and other modules
function App.isGameReady()
    if not AshitaCore then
        return false
    end
    
    local success, result = pcall(function()
        local memoryMgr = AshitaCore:GetMemoryManager()
        if not memoryMgr then
            return false
        end
        
        local party = memoryMgr:GetParty()
        if not party then
            return false
        end
        
        -- Check if character name is available
        local playerName = party:GetMemberName(0)
        if not playerName or playerName == "" then
            return false
        end
        
        return true
    end)
    
    return success and result == true
end

-- Tick app state machines (called with deltaTime in seconds)
-- dtSeconds: delta time in seconds
function App.tick(app, dtSeconds)
    if not app.initialized then
        return
    end
    
    -- Wait for game to be ready and character to be loaded before doing anything
    if not App.isGameReady() then
        return
    end
    
    -- Note: Startup refresh is now triggered immediately in connection.lua:handleAuthResponse()
    -- when connection is established, not waiting for tick. This ensures fastest possible startup.
    
    -- Tick all features that have tick methods
    if app.features.connection and app.features.connection.tick then
        app.features.connection:tick(dtSeconds)
    end
    if app.features.friends and app.features.friends.tick then
        app.features.friends:tick(dtSeconds)
    end
    if app.features.serverlist and app.features.serverlist.tick then
        app.features.serverlist:tick(dtSeconds)
    end
    
    -- Tick WebSocket connection manager (non-blocking backoff/retry)
    if app.features.wsConnectionManager then
        app.features.wsConnectionManager:tick()
    end
    
    -- Tick WebSocket client connect steps (non-blocking step machine)
    if app.features.wsClient and app.features.wsClient.tickConnect then
        app.features.wsClient:tickConnect()
    end
    
    -- Tick WebSocket client messages (non-blocking message processing)
    if app.features.wsClient and app.features.wsClient.tickMessages then
        app.features.wsClient:tickMessages()
    end
end

-- Trigger startup refresh after auto-connect completes
-- Fires all requests in parallel for maximum speed
-- Also establishes WebSocket connection for real-time updates
function App._triggerStartupRefresh(app)
    if not app.features.connection or not app.features.connection:isConnected() then
        return
    end
    
    -- Get timing for logs
    local timeMs = 0
    if app.deps and app.deps.time then
        timeMs = app.deps.time()
    else
        timeMs = os.time() * 1000
    end
    
    if app.deps.logger and app.deps.logger.info then
        app.deps.logger.info(string.format("[App] [%d] Triggering startup refresh (parallel)", timeMs))
    end
    
    -- Request WebSocket connection via connection manager (non-blocking)
    if app.features.wsConnectionManager then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Requesting WebSocket connection", timeMs))
        end
        app.features.wsConnectionManager:requestConnect()
    end
    
    -- Fire all requests in parallel (they're independent)
    -- 1. Send heartbeat (safety signal only - NOT for friend status)
    if app.features.friends and app.features.friends.sendHeartbeat then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Firing heartbeat", timeMs))
        end
        app.features.friends:sendHeartbeat()
    end
    
    -- 2. Refresh friend list (bootstrap - WS will provide updates after)
    if app.features.friends and app.features.friends.refresh then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Firing friend list refresh", timeMs))
        end
        app.features.friends:refresh()
    end
    
    -- 3. Sync preferences from server - parallel with other requests
    if app.features.preferences and app.features.preferences.refresh then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Firing preferences refresh", timeMs))
        end
        app.features.preferences:refresh()
    end
    
    -- 4. Refresh friend requests (bootstrap - WS will provide updates after)
    if app.features.friends and app.features.friends.refreshFriendRequests then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Firing friend requests refresh", timeMs))
        end
        app.features.friends:refreshFriendRequests()
    end
    
    -- 5. Refresh blocklist - parallel with other requests
    if app.features.blocklist and app.features.blocklist.refresh then
        if app.deps.logger and app.deps.logger.debug then
            app.deps.logger.debug(string.format("[App] [%d] Startup: Firing blocklist refresh", timeMs))
        end
        app.features.blocklist:refresh()
    end
end

-- Get read-only state snapshot for UI modules
-- Returns state table even if app not initialized (with uninitialized state)
function App.getState(app)
    local state = {
        connection = { state = "uninitialized" },
        friends = { friends = {}, incomingRequests = {}, outgoingRequests = {} },
        serverlist = { servers = {}, selected = nil },
        themes = { themeIndex = 0, presetName = "" },
        notifications = {},
        preferences = { prefs = {} },
        notes = {},
        tags = { tagOrder = {}, collapsedTags = {}, tagCount = 0 }
    }
    
    if not app.initialized then
        return state
    end
    
    -- Get state from each feature via getState() method only
    -- Never call feature-specific methods like isConnected(), getFriends(), etc.
    if app.features.connection and app.features.connection.getState then
        state.connection = app.features.connection:getState()
    elseif not app._missingFeatureLogged.connection then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing connection feature or getState() method")
        end
        app._missingFeatureLogged.connection = true
    end
    
    if app.features.friends and app.features.friends.getState then
        state.friends = app.features.friends:getState()
    elseif not app._missingFeatureLogged.friends then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing friends feature or getState() method")
        end
        app._missingFeatureLogged.friends = true
    end
    
    if app.features.serverlist and app.features.serverlist.getState then
        state.serverlist = app.features.serverlist:getState()
    elseif not app._missingFeatureLogged.serverlist then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing serverlist feature or getState() method")
        end
        app._missingFeatureLogged.serverlist = true
    end
    
    if app.features.themes and app.features.themes.getState then
        state.themes = app.features.themes:getState()
    elseif not app._missingFeatureLogged.themes then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing themes feature or getState() method")
        end
        app._missingFeatureLogged.themes = true
    end
    
    if app.features.notifications and app.features.notifications.getState then
        state.notifications = app.features.notifications:getState()
    elseif not app._missingFeatureLogged.notifications then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing notifications feature or getState() method")
        end
        app._missingFeatureLogged.notifications = true
    end
    
    if app.features.preferences and app.features.preferences.getState then
        state.preferences = app.features.preferences:getState()
    elseif not app._missingFeatureLogged.preferences then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing preferences feature or getState() method")
        end
        app._missingFeatureLogged.preferences = true
    end
    
    if app.features.notes and app.features.notes.getState then
        state.notes = app.features.notes:getState()
    elseif not app._missingFeatureLogged.notes then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing notes feature or getState() method")
        end
        app._missingFeatureLogged.notes = true
    end
    
    if app.features.tags and app.features.tags.getState then
        state.tags = app.features.tags:getState()
    elseif not app._missingFeatureLogged.tags then
        if app.deps.logger and app.deps.logger.error then
            app.deps.logger.error("[App] Missing tags feature or getState() method")
        end
        app._missingFeatureLogged.tags = true
    end
    
    -- Add WebSocket connection manager status for UI
    if app.features.wsConnectionManager then
        local statusText, statusDetail = app.features.wsConnectionManager:getStatus()
        local retryInfo = app.features.wsConnectionManager:getRetryInfo()
        state.wsConnection = {
            state = app.features.wsConnectionManager:getState(),
            isConnected = app.features.wsConnectionManager:isConnected(),
            statusText = statusText,
            statusDetail = statusDetail,
            retryInfo = retryInfo
        }
    else
        state.wsConnection = {
            state = "UNAVAILABLE",
            isConnected = false,
            statusText = "Unavailable",
            statusDetail = ""
        }
    end
    
    return state
end

-- Release app (save state, cleanup)
-- Entry file calls this on unload event
function App.release(app)
    if not app.initialized then
        return
    end
    
    if app.deps.logger and app.deps.logger.info then
        app.deps.logger.info("[App] Releasing")
    end
    
    -- Disconnect WebSocket cleanly
    if app.features.wsClient and app.features.wsClient.disconnect then
        app.features.wsClient:disconnect()
    end
    
    -- Save persisted state via deps.storage
    if app.features.preferences and app.features.preferences.save and app.deps.storage then
        local _ = app.features.preferences:save()
    end
    
    if app.features.notes and app.features.notes.save and app.deps.storage then
        local _ = app.features.notes:save()
    end
    
    if app.features.tags and app.features.tags.save and app.deps.storage then
        local _ = app.features.tags:save()
    end
    
    app.initialized = false
    return nil
end

return App
