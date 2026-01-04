--[[
* QuickOnline Module - Data
* State management: provides friends list for quick view
]]

local M = {}

-- Module state
local state = {
    initialized = false,
    friends = {},  -- All friends (online and offline)
    connectionState = "uninitialized",
    lastError = nil,
    lastUpdatedAt = nil
}

-- Initialize data module
function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
end

-- Update data from app state (called every frame before render)
function M.Update()
    if not state.initialized then
        return
    end
    
    -- Get app instance from global (set by entry file)
    local app = _G.FFXIFriendListApp
    if not app then
        return
    end
    
    -- Get app state
    local App = require("app.App")
    local appState = App.getState(app)
    if not appState then
        return
    end
    
    state.friends = {}
    if appState.friends and appState.friends.friends then
        for _, friend in ipairs(appState.friends.friends) do
            table.insert(state.friends, friend)
        end
    end
    
    -- Update connection state
    if appState.connection then
        if appState.connection.isConnected then
            state.connectionState = "connected"
        elseif appState.connection.state then
            state.connectionState = string.lower(appState.connection.state)
        else
            state.connectionState = "uninitialized"
        end
    else
        state.connectionState = "uninitialized"
    end
    
    -- Update error and timestamp
    if appState.friends then
        state.lastError = appState.friends.lastError
        state.lastUpdatedAt = appState.friends.lastUpdatedAt
    end
end

-- Get all friends list
function M.GetFriends()
    return state.friends
end

-- Backwards compatibility alias
function M.GetOnlineFriends()
    return state.friends
end

-- Get connection state
function M.GetConnectionState()
    return state.connectionState
end

-- Check if connected
function M.IsConnected()
    return state.connectionState == "connected"
end

-- Get last error
function M.GetLastError()
    return state.lastError
end

-- Get last updated timestamp
function M.GetLastUpdatedAt()
    return state.lastUpdatedAt
end

-- Cleanup
function M.Cleanup()
    state.initialized = false
    state.friends = {}
end

return M

