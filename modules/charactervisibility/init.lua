--[[
* Character Visibility Module - Init
* Public API and lifecycle management for Per-Character Visibility settings
]]

local charactervisibility = {}

-- Load sub-modules
charactervisibility.data = require('modules.charactervisibility.data')
charactervisibility.display = require('modules.charactervisibility.display')

-- Module state
local moduleState = {
    initialized = false,
    hidden = false
}

-- Initialize the module
function charactervisibility.Initialize(settings)
    if moduleState.initialized then
        return nil
    end
    
    local _ = charactervisibility.data.Initialize(settings)
    local _ = charactervisibility.display.Initialize(settings)
    
    moduleState.initialized = true
    return nil
end

-- Update visuals when settings change
function charactervisibility.UpdateVisuals(settings)
    if not moduleState.initialized then
        return
    end
    
    charactervisibility.display.UpdateVisuals(settings)
end

-- Main render function
function charactervisibility.DrawWindow(settings)
    if not moduleState.initialized or moduleState.hidden then
        return
    end
    
    charactervisibility.data.Update()
    charactervisibility.display.DrawWindow(settings, charactervisibility.data)
end

-- Hide/show module
function charactervisibility.SetHidden(hidden)
    moduleState.hidden = hidden or false
    if charactervisibility.display.SetHidden then
        charactervisibility.display.SetHidden(hidden)
    end
end

-- Cleanup
function charactervisibility.Cleanup()
    if charactervisibility.display.Cleanup then
        charactervisibility.display.Cleanup()
    end
    if charactervisibility.data.Cleanup then
        charactervisibility.data.Cleanup()
    end
    moduleState.initialized = false
end

-- Is window open
function charactervisibility.IsWindowOpen()
    if charactervisibility.display.IsWindowOpen then
        return charactervisibility.display.IsWindowOpen()
    end
    return false
end

-- Set window open state
function charactervisibility.SetWindowOpen(open)
    if charactervisibility.display.SetWindowOpen then
        charactervisibility.display.SetWindowOpen(open)
    end
end

-- Toggle window
function charactervisibility.ToggleWindow()
    if charactervisibility.display.ToggleWindow then
        charactervisibility.display.ToggleWindow()
    end
end

-- Refresh data from server
function charactervisibility.Refresh()
    if charactervisibility.data.Refresh then
        charactervisibility.data.Refresh()
    end
end

return charactervisibility
