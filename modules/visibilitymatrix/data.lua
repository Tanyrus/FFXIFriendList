--[[
* Visibility Matrix Module - Data Layer
* Handles HTTP fetching, caching, and update queue management
]]

local Endpoints = require('protocol.Endpoints')
local Envelope = require('protocol.Envelope')
local Json = require('protocol.Json')

local M = {}

-- Module state
local state = {
    initialized = false,
    snapshot = nil,          -- Last fetched matrix snapshot
    version = nil,           -- Snapshot version timestamp
    lastFetchTime = 0,       -- os.time() of last fetch
    pendingUpdates = {},     -- Queue of pending toggle changes
    updateTimer = 0,         -- Debounce timer
    isFetching = false,
    isUpdating = false,
    lastError = nil,
    friendFilter = '',       -- Search filter text
}

-- Dependencies (injected)
local deps = {
    log = nil,
}

-- Constants
local DEBOUNCE_DELAY = 0.2  -- 200ms debounce for batch updates
local REFETCH_INTERVAL = 300 -- 5 minutes before auto-refetch

function M.Initialize(settings)
    if state.initialized then return end
    
    -- Get dependencies from settings
    deps.log = settings and settings.log
    
    state.initialized = true
    if deps.log then deps.log:info('VisibilityMatrix data initialized') end
end

function M.GetSnapshot()
    return state.snapshot
end

function M.GetVersion()
    return state.version
end

function M.IsFetching()
    return state.isFetching
end

function M.IsUpdating()
    return state.isUpdating
end

function M.GetLastError()
    return state.lastError
end

function M.GetFriendFilter()
    return state.friendFilter
end

function M.SetFriendFilter(filter)
    state.friendFilter = filter or ''
end

function M.GetPendingUpdateCount()
    local count = 0
    for _ in pairs(state.pendingUpdates) do
        count = count + 1
    end
    return count
end

-- Fetch matrix snapshot from server
function M.Fetch(callback)
    if not state.initialized then
        state.lastError = 'Not initialized'
        if callback then callback(false, 'Not initialized') end
        return
    end
    
    -- Get app and services from global
    local app = _G.FFXIFriendListApp
    if not app or not app.features then
        state.lastError = 'App not available'
        if callback then callback(false, 'App not available') end
        return
    end
    
    local connection = app.features.connection
    if not connection then
        state.lastError = 'Connection service not available'
        if callback then callback(false, 'Connection not available') end
        return
    end
    
    -- Get network client from friends feature (they all share the same deps.net)
    local netClient = app.features.friends and app.features.friends.deps and app.features.friends.deps.net
    if not netClient then
        state.lastError = 'Network client not available'
        if callback then callback(false, 'Network not available') end
        return
    end
    
    if not connection:isConnected() then
        state.lastError = 'Not connected to server'
        if callback then callback(false, 'Not connected') end
        return
    end
    
    if state.isFetching then
        if callback then callback(false, 'Already fetching') end
        return
    end
    
    state.isFetching = true
    state.lastError = nil
    
    -- Get character name from connection service
    local characterName = ""
    if connection.getCharacterName then
        characterName = connection:getCharacterName()
    elseif connection.lastCharacterName then
        characterName = connection.lastCharacterName
    end
    
    if not characterName or characterName == "" then
        state.lastError = 'No character name available'
        state.isFetching = false
        if callback then callback(false, 'No character name') end
        return
    end
    
    local baseUrl = connection:getBaseUrl()
    local endpoint = Endpoints.VISIBILITY_MATRIX.GET
    local url = baseUrl .. endpoint
    local headers = connection:getHeaders(characterName)
    
    if deps.log then 
        deps.log:debug('Fetching visibility matrix from: ' .. url) 
    end
    
    local requestId = netClient.request({
        url = url,
        method = 'GET',
        headers = headers,
        callback = function(success, response)
            state.isFetching = false
            
            if not success then
                state.lastError = 'Network error - failed to connect'
                if deps.log then deps.log:error('Failed to fetch visibility matrix: network error') end
                if callback then callback(false, 'Network error') end
                return
            end
            
            if not response then
                state.lastError = 'No response from server'
                if deps.log then deps.log:error('Failed to fetch visibility matrix: no response') end
                if callback then callback(false, 'No response') end
                return
            end
            
            local parseOk, result, errorMsg, errorCode = Envelope.decode(response)
            
            if not parseOk then
                state.lastError = 'Parse error: ' .. tostring(errorMsg)
                if callback then callback(false, state.lastError) end
                return
            end
            
            if not result or not result.data then
                state.lastError = 'No data in response'
                if callback then callback(false, state.lastError) end
                return
            end
            
            -- Store snapshot
            state.snapshot = result.data
            state.version = result.data.version
            state.lastFetchTime = os.time()
            
            if deps.log then 
                deps.log:info(string.format('Visibility matrix fetched: %d characters, %d friends, %d explicit entries',
                    #(result.data.characters or {}),
                    #(result.data.friends or {}),
                    #(result.data.matrix or {})))
            end
            
            if callback then
                callback(true, result.data)
            end
        end
    })
end

-- Get visibility for a specific (character, friend) cell
-- Returns true if visible, false if hidden, nil if not found (defaults to true)
function M.GetVisibility(characterId, friendAccountId)
    if not state.snapshot or not state.snapshot.matrix then
        return true -- Default visible
    end
    
    -- Check pending updates first (optimistic UI)
    local key = characterId .. '|' .. friendAccountId
    if state.pendingUpdates[key] ~= nil then
        return state.pendingUpdates[key]
    end
    
    -- Check snapshot
    for _, entry in ipairs(state.snapshot.matrix) do
        if entry.ownerCharacterId == characterId and entry.viewerAccountId == friendAccountId then
            return entry.shareVisibility
        end
    end
    
    return true -- Default visible if no explicit entry
end

-- Toggle visibility for a cell (sends update immediately)
function M.ToggleVisibility(characterId, friendAccountId)
    local current = M.GetVisibility(characterId, friendAccountId)
    local newValue = not current
    
    -- Update pending state optimistically for UI
    local key = characterId .. '|' .. friendAccountId
    state.pendingUpdates[key] = newValue
    
    if deps.log then 
        deps.log:debug(string.format('Toggling visibility: char=%s, friend=%s, value=%s', 
            characterId, friendAccountId, tostring(newValue)))
    end
    
    -- Send immediately (no debounce)
    M.FlushPendingUpdates()
end

-- Flush pending updates to server (batch update)
function M.FlushPendingUpdates()
    if not state.initialized then return end
    
    local updateCount = 0
    for _ in pairs(state.pendingUpdates) do
        updateCount = updateCount + 1
    end
    
    if updateCount == 0 then return end
    
    if state.isUpdating then
        if deps.log then deps.log:warn('Already updating, skipping flush') end
        return
    end
    
    -- Get app and services from global
    local app = _G.FFXIFriendListApp
    if not app or not app.features then
        state.lastError = 'App not available'
        return
    end
    
    local connection = app.features.connection
    if not connection or not connection:isConnected() then
        state.lastError = 'Not connected to server'
        return
    end
    
    local netClient = app.features.friends and app.features.friends.deps and app.features.friends.deps.net
    if not netClient then
        state.lastError = 'Network client not available'
        return
    end
    
    -- Build updates array
    local updates = {}
    for key, shareVisibility in pairs(state.pendingUpdates) do
        local parts = {}
        for part in string.gmatch(key, '[^|]+') do
            table.insert(parts, part)
        end
        if #parts == 2 then
            table.insert(updates, {
                ownerCharacterId = parts[1],
                viewerAccountId = parts[2],
                shareVisibility = shareVisibility
            })
        end
    end
    
    if #updates == 0 then
        state.pendingUpdates = {}
        return
    end
    
    state.isUpdating = true
    
    -- Get character name from connection service
    local characterName = ""
    if connection.getCharacterName then
        characterName = connection:getCharacterName()
    elseif connection.lastCharacterName then
        characterName = connection.lastCharacterName
    end
    
    if not characterName or characterName == "" then
        state.lastError = 'No character name available'
        state.isUpdating = false
        return
    end
    
    if deps.log then 
        deps.log:info(string.format('Flushing %d visibility matrix updates', #updates)) 
    end
    
    local requestBody = {
        updates = updates
    }
    
    -- Encode body as JSON (Json.encode returns the string directly or throws error)
    local bodyJson
    local ok, encodeErr = pcall(function()
        bodyJson = Json.encode(requestBody)
    end)
    
    if not ok then
        state.lastError = 'Failed to encode request body: ' .. tostring(encodeErr)
        state.isUpdating = false
        if deps.log then deps.log:error('Failed to encode visibility matrix update request') end
        return
    end
    
    local baseUrl = connection:getBaseUrl()
    local endpoint = Endpoints.VISIBILITY_MATRIX.UPDATE_BATCH
    local url = baseUrl .. endpoint
    local headers = connection:getHeaders(characterName)
    
    netClient.request({
        method = 'PATCH',
        url = url,
        headers = headers,
        body = bodyJson,
        callback = function(success, response)
            state.isUpdating = false
            
            if not success then
                state.lastError = 'Network error during update'
                if deps.log then deps.log:error('Failed to update visibility matrix: network error') end
                -- Keep pending updates for retry
                return
            end
            
            local parseOk, result, errorMsg, errorCode = Envelope.decode(response)
            if not parseOk then
                state.lastError = 'Parse error: ' .. tostring(errorMsg)
                if deps.log then 
                    deps.log:error('Failed to update visibility matrix: ' .. tostring(state.lastError)) 
                end
                -- Keep pending updates for retry
                return
            end
            
            -- Success - clear pending updates and update snapshot
            state.pendingUpdates = {}
            state.lastError = nil
            
            if result and result.data then
                state.snapshot = result.data
                state.version = result.data.version
                state.lastFetchTime = os.time()
            end
            
            if deps.log then 
                deps.log:info(string.format('Visibility matrix updated: %d cells', #updates)) 
            end
            
            -- Trigger friend list refresh to update visibility instantly
            local app = _G.FFXIFriendListApp
            if app and app.features and app.features.friends then
                app.features.friends:refresh()
            end
        end
    })
end

-- Force immediate flush (for save button)
function M.SaveNow()
    state.updateTimer = 0
    M.FlushPendingUpdates()
end

-- Check if snapshot is stale and needs refresh
function M.NeedsRefresh()
    if not state.snapshot then return true end
    local age = os.time() - state.lastFetchTime
    return age > REFETCH_INTERVAL
end

function M.GetLastFetch()
    return state.lastFetchTime
end

function M.Cleanup()
    -- Flush any pending updates before cleanup
    if M.GetPendingUpdateCount() > 0 then
        M.SaveNow()
    end
    
    state.initialized = false
    state.snapshot = nil
    state.version = nil
    state.pendingUpdates = {}
end

return M
