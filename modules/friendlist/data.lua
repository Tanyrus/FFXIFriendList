--[[
* FriendList Module - Data
* State management and business logic (read-only from app state)
]]

local M = {}

-- Module state
local state = {
    initialized = false,
    friends = {},
    incomingRequests = {},
    outgoingRequests = {},
    connectionState = "uninitialized",
    friendsState = "idle",  -- Track friends sync state: idle, syncing, error
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
    
    -- Update friends state
    if appState.friends then
        state.friends = appState.friends.friends or {}
        state.incomingRequests = appState.friends.incomingRequests or {}
        state.outgoingRequests = appState.friends.outgoingRequests or {}
        state.lastError = appState.friends.lastError
        state.lastUpdatedAt = appState.friends.lastUpdatedAt
        state.friendsState = appState.friends.state or "idle"  -- Track sync state
    else
        state.friendsState = "idle"
    end
    
    -- Update connection state from connection feature (not friends feature)
    if appState.connection then
        if appState.connection.isConnected then
            state.connectionState = "connected"
        elseif appState.connection.state then
            -- Normalize state to lowercase for comparison
            state.connectionState = string.lower(appState.connection.state)
        else
            state.connectionState = "uninitialized"
        end
    else
        state.connectionState = "uninitialized"
    end
end

-- Get friends list
function M.GetFriends()
    return state.friends
end

-- Get incoming requests
function M.GetIncomingRequests()
    return state.incomingRequests
end

-- Get outgoing requests
function M.GetOutgoingRequests()
    return state.outgoingRequests
end

-- Get incoming requests count
function M.GetIncomingRequestsCount()
    return #state.incomingRequests
end

-- Get connection state
function M.GetConnectionState()
    return state.connectionState
end

-- Get last error
function M.GetLastError()
    return state.lastError
end

-- Get last updated timestamp
function M.GetLastUpdatedAt()
    return state.lastUpdatedAt
end

-- Get friends sync state
function M.GetFriendsState()
    return state.friendsState or "idle"
end

-- Check if connected
function M.IsConnected()
    return state.connectionState == "connected"
end

-- Cleanup
function M.Cleanup()
    state.initialized = false
    state.friends = {}
    state.incomingRequests = {}
    state.outgoingRequests = {}
end

return M

