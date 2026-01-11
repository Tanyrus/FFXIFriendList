--[[
* Character Visibility Module - Data
* Data access and state management
]]

local M = {}

local state = {
    initialized = false,
    characters = {},
    isLoading = false,
    lastError = nil,
    lastFetchTime = 0,
    needsRefresh = true
}

function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    return nil
end

function M.Update()
    if not state.initialized then
        return
    end
    
    -- Auto-refresh if needed and connected
    if state.needsRefresh and not state.isLoading then
        M.Refresh()
    end
end

function M.Refresh()
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.characterVisibility then
        return
    end
    
    local feature = app.features.characterVisibility
    feature:fetch()
    state.needsRefresh = false
end

function M.GetCharacters()
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.characterVisibility then
        return {}
    end
    
    return app.features.characterVisibility:getCharacters()
end

function M.GetState()
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.characterVisibility then
        return {
            characters = {},
            isLoading = false,
            lastError = nil
        }
    end
    
    return app.features.characterVisibility:getState()
end

function M.SetVisibility(characterId, shareVisibility)
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.characterVisibility then
        return
    end
    
    app.features.characterVisibility:setVisibility(characterId, shareVisibility)
end

function M.ToggleVisibility(characterId)
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.characterVisibility then
        return
    end
    
    app.features.characterVisibility:toggleVisibility(characterId)
end

function M.SetNeedsRefresh()
    state.needsRefresh = true
end

function M.Cleanup()
    state.initialized = false
    state.characters = {}
    state.needsRefresh = true
end

return M
