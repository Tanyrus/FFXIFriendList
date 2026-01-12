--[[
* Visibility Matrix Module - Init
* Public API and lifecycle management for per-friend per-character visibility control
]]

local visibilitymatrix = {}

-- Load sub-modules
visibilitymatrix.data = require('modules.visibilitymatrix.data')
visibilitymatrix.display = require('modules.visibilitymatrix.display')

-- Module state
local moduleState = {
    initialized = false,
    hidden = false
}

-- Initialize the module
function visibilitymatrix.Initialize(settings)
    if moduleState.initialized then
        return nil
    end
    
    local _ = visibilitymatrix.data.Initialize(settings)
    local _ = visibilitymatrix.display.Initialize(settings)
    
    moduleState.initialized = true
    return nil
end

-- Update (called each frame)
function visibilitymatrix.Update(deltaTime)
    if not moduleState.initialized then
        return
    end
    
    visibilitymatrix.data.Update(deltaTime)
end

-- Update visuals when settings change
function visibilitymatrix.UpdateVisuals(settings)
    if not moduleState.initialized then
        return
    end
    
    visibilitymatrix.display.UpdateVisuals(settings)
end

-- Main render function
function visibilitymatrix.DrawWindow(settings)
    if not moduleState.initialized or moduleState.hidden then
        return
    end
    
    visibilitymatrix.display.DrawWindow(settings, visibilitymatrix.data)
end

-- Hide/show module
function visibilitymatrix.SetHidden(hidden)
    moduleState.hidden = hidden or false
    if visibilitymatrix.display.SetHidden then
        visibilitymatrix.display.SetHidden(hidden)
    end
end

-- Cleanup
function visibilitymatrix.Cleanup()
    if visibilitymatrix.data.Cleanup then
        visibilitymatrix.data.Cleanup()
    end
    if visibilitymatrix.display.Cleanup then
        visibilitymatrix.display.Cleanup()
    end
    moduleState.initialized = false
end

-- Is window open
function visibilitymatrix.IsWindowOpen()
    if visibilitymatrix.display.IsWindowOpen then
        return visibilitymatrix.display.IsWindowOpen()
    end
    return false
end

-- Set window open state
function visibilitymatrix.SetWindowOpen(open)
    if visibilitymatrix.display.SetWindowOpen then
        visibilitymatrix.display.SetWindowOpen(open)
    end
    
    -- Auto-fetch when opening
    if open and not visibilitymatrix.data.GetSnapshot() then
        visibilitymatrix.Refresh()
    end
end

-- Open window (convenience function)
function visibilitymatrix.openWindow()
    visibilitymatrix.SetWindowOpen(true)
end

-- Toggle window
function visibilitymatrix.ToggleWindow()
    if visibilitymatrix.display.ToggleWindow then
        visibilitymatrix.display.ToggleWindow()
    end
    
    -- Auto-fetch when opening
    if visibilitymatrix.IsWindowOpen() and not visibilitymatrix.data.GetSnapshot() then
        visibilitymatrix.Refresh()
    end
end

-- Refresh data from server
function visibilitymatrix.Refresh()
    if visibilitymatrix.data.Fetch then
        visibilitymatrix.data.Fetch()
    end
end

-- Force save pending changes
function visibilitymatrix.SaveNow()
    if visibilitymatrix.data.SaveNow then
        visibilitymatrix.data.SaveNow()
    end
end

return visibilitymatrix
