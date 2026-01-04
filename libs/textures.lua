--[[
* libs/textures.lua
* Texture/icon caching and management
* Provides texture loading and caching for UI modules
]]--

local M = {}

-- Texture cache
local textureCache = {}

-- Get texture path (relative to addon directory)
local function getTexturePath(addonPath, filename)
    if not addonPath or addonPath == "" then
        return nil
    end
    
    addonPath = addonPath:gsub('\\$', '')
    return addonPath .. "\\assets\\icons\\" .. filename
end

-- Load texture (cached)
function M.loadTexture(addonPath, filename, guiManager)
    if not addonPath or not filename or not guiManager then
        return nil
    end
    
    -- Check cache
    local cacheKey = filename
    if textureCache[cacheKey] then
        return textureCache[cacheKey]
    end
    
    -- Load texture
    local path = getTexturePath(addonPath, filename)
    if not path then
        return nil
    end
    
    local texture = nil
    if guiManager and guiManager.CreateTextureFromFile then
        texture = guiManager:CreateTextureFromFile(path)
    end
    
    if texture then
        textureCache[cacheKey] = texture
    end
    
    return texture
end

-- Get cached texture (returns nil if not loaded)
function M.getTexture(filename)
    return textureCache[filename]
end

-- Clear texture cache
function M.clearCache()
    -- Note: Actual texture cleanup would require guiManager, so we just clear the cache table
    textureCache = {}
end

-- Preload common textures
function M.preloadTextures(addonPath, filenames, guiManager)
    if not addonPath or not filenames or not guiManager then
        return
    end
    
    for _, filename in ipairs(filenames) do
        M.loadTexture(addonPath, filename, guiManager)
    end
end

return M

