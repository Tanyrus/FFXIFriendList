--[[
* Themes Module - Init
* Public API and lifecycle management
]]

local themes = {}

-- Load sub-modules
themes.data = require('modules.themes.data')
themes.display = require('modules.themes.display')

-- Module state
local moduleState = {
    initialized = false,
    hidden = false
}

-- Initialize the module (called once on addon load)
function themes.Initialize(settings)
    if moduleState.initialized then
        return nil
    end
    
    local _ = themes.data.Initialize(settings)  -- Capture return value
    local _ = themes.display.Initialize(settings)  -- Capture return value
    
    moduleState.initialized = true
    return nil  -- Explicitly return nil
end

-- Update visuals when settings change (fonts, themes, etc.)
function themes.UpdateVisuals(settings)
    if not moduleState.initialized then
        return
    end
    
    themes.display.UpdateVisuals(settings)
end

-- Main render function (called every frame)
function themes.DrawWindow(settings)
    if not moduleState.initialized or moduleState.hidden then
        return
    end
    
    -- Update data from app state
    themes.data.Update()
    
    -- Render display
    themes.display.DrawWindow(settings, themes.data)
end

-- Hide/show module elements
function themes.SetHidden(hidden)
    moduleState.hidden = hidden or false
    if themes.display.SetHidden then
        themes.display.SetHidden(hidden)
    end
end

-- Cleanup resources (called on addon unload)
function themes.Cleanup()
    if themes.display.Cleanup then
        themes.display.Cleanup()
    end
    if themes.data.Cleanup then
        themes.data.Cleanup()
    end
    
    moduleState.initialized = false
end

return themes

