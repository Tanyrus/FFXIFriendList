--[[
* libs/settings.lua
* Account-wide configuration management using Ashita-style Lua table persistence
* Stores in config/addons/FFXIFriendList/config.lua
* Contains: API keys, preferences, window states, UI settings
* NOT per-character - shared across all characters
]]--

require('common')

local M = {}

-- Settings file path (account-wide, not per-character)
local settingsPath = nil
local settingsFile = 'config.lua'

-- Current settings (loaded from disk)
local currentSettings = nil

-- Default settings structure (gConfig-compatible)
local defaultSettings = T{
    -- Schema version for migrations
    _version = 1,
    
    -- Window visibility flags
    showFriendList = false,
    showQuickOnline = false,
    showServerSelection = false,
    showNoteEditor = false,
    
    -- Window states
    windows = T{
        friendList = T{
            visible = false,
            locked = false,
            posX = 100,
            posY = 100,
            sizeX = 600,
            sizeY = 400
        },
        quickOnline = T{
            visible = false,
            locked = false,
            posX = 100,
            posY = 100,
            sizeX = 300,
            sizeY = 200
        },
        noteEditor = T{
            visible = false,
            posX = 100,
            posY = 100
        },
        serverSelection = T{
            visible = false
        }
    },
    
    -- Friend list settings
    friendListSettings = T{
        selectedTab = 0,
        sort = T{
            column = "name",
            direction = "asc"
        },
        sortMode = "status",
        sortDirection = "asc",
        filter = "",
        columns = T{
            job = T{ visible = true },
            zone = T{ visible = false },
            nationRank = T{ visible = false },
            lastSeen = T{ visible = false }
        },
        sections = T{
            pendingRequests = T{ expanded = true },
            friendViewSettings = T{ expanded = true },
            privacyControls = T{ expanded = false },
            altVisibility = T{ expanded = false },
            notificationsSettings = T{ expanded = true },
            controlsSettings = T{ expanded = true },
            themeSettings = T{ expanded = true },
            tagManager = T{ expanded = false }
        },
        columnOrder = T{ "Name", "Job", "Zone", "Nation/Rank", "Last Seen", "Added As" },
        columnWidths = T{
            Name = 120.0,
            Job = 100.0,
            Zone = 120.0,
            ["Nation/Rank"] = 80.0,
            ["Last Seen"] = 120.0,
            ["Added As"] = 100.0
        },
        groupByOnlineStatus = false,
        collapsedOnlineSection = false,
        collapsedOfflineSection = false
    },
    
    -- Quick online settings
    quickOnlineSettings = T{
        sort = T{
            column = "name",
            direction = "asc"
        },
        sortMode = "status",
        sortDirection = "asc",
        filter = "",
        columns = T{
            name = true,
            job = true,
            zone = true,
            status = true
        },
        groupByOnlineStatus = false,
        collapsedOnlineSection = false,
        collapsedOfflineSection = false,
        hideTopBar = false
    },
    
    -- Tag settings
    friendTags = T{},
    tagOrder = T{ "Favorite" },
    collapsedTags = T{},
    
    -- Data section (API keys, server selection, preferences)
    data = T{
        apiKeys = T{},
        serverSelection = T{
            savedServerId = "",
            savedServerBaseUrl = "https://api.horizonfriendlist.com",
            detectedServerShownOnce = false
        },
        preferences = T{
            useServerNotes = false,
            debugMode = false,
            overwriteNotesOnUpload = false,
            overwriteNotesOnDownload = false,
            shareJobWhenAnonymous = false,
            showOnlineStatus = true,
            shareLocation = true,
            shareFriendsAcrossAlts = true,
            notificationDuration = 8.0,
            notificationPositionX = 10.0,
            notificationPositionY = 15.0,
            customCloseKeyCode = 27,
            controllerCloseButton = 8192,
            windowsLocked = false,
            notificationSoundsEnabled = true,
            soundOnFriendOnline = true,
            soundOnFriendRequest = true,
            notificationSoundVolume = 0.6,
            mainShowJob = true,
            mainShowZone = false,
            mainShowNationRank = false,
            mainShowLastSeen = false,
            -- Hover tooltip settings (main window)
            mainHoverShowJob = true,
            mainHoverShowZone = true,
            mainHoverShowNationRank = true,
            mainHoverShowLastSeen = false,
            mainHoverShowFriendedAs = true,
            -- Hover tooltip settings (quick online)
            quickHoverShowJob = true,
            quickHoverShowZone = true,
            quickHoverShowNationRank = true,
            quickHoverShowLastSeen = false,
            quickHoverShowFriendedAs = true
        }
    }
}

-- Serialize a key for Lua table output
local function serializeKey(k)
    if type(k) == 'number' then
        return '[' .. tostring(k) .. ']'
    elseif type(k) == 'string' then
        -- Check if it's a valid identifier
        if k:match('^[%a_][%w_]*$') then
            return k
        else
            return '["' .. k:gsub('"', '\\"') .. '"]'
        end
    elseif type(k) == 'boolean' then
        return '[' .. tostring(k) .. ']'
    end
    return nil
end

-- Serialize a value for Lua table output
local function serializeValue(v, indent)
    indent = indent or ''
    local nextIndent = indent .. '    '
    
    if type(v) == 'nil' then
        return 'nil'
    elseif type(v) == 'boolean' then
        return tostring(v)
    elseif type(v) == 'number' then
        return tostring(v)
    elseif type(v) == 'string' then
        return string.format('%q', v)
    elseif type(v) == 'table' then
        local parts = {}
        local isArray = true
        local maxIndex = 0
        
        -- Check if it's an array
        for k, _ in pairs(v) do
            if type(k) ~= 'number' or k < 1 or math.floor(k) ~= k then
                isArray = false
                break
            end
            if k > maxIndex then maxIndex = k end
        end
        
        if isArray and maxIndex > 0 then
            -- Array format
            for i = 1, maxIndex do
                table.insert(parts, serializeValue(v[i], nextIndent))
            end
            if #parts == 0 then
                return 'T{ }'
            end
            return 'T{\n' .. nextIndent .. table.concat(parts, ',\n' .. nextIndent) .. '\n' .. indent .. '}'
        else
            -- Table format
            local keys = {}
            for k in pairs(v) do
                table.insert(keys, k)
            end
            table.sort(keys, function(a, b)
                if type(a) == type(b) then
                    return tostring(a) < tostring(b)
                end
                return type(a) < type(b)
            end)
            
            for _, k in ipairs(keys) do
                local keyStr = serializeKey(k)
                if keyStr then
                    local valStr = serializeValue(v[k], nextIndent)
                    table.insert(parts, keyStr .. ' = ' .. valStr)
                end
            end
            
            if #parts == 0 then
                return 'T{ }'
            end
            return 'T{\n' .. nextIndent .. table.concat(parts, ',\n' .. nextIndent) .. '\n' .. indent .. '}'
        end
    end
    
    return 'nil'
end

-- Get the settings folder path
function M.getSettingsPath()
    if not settingsPath then
        settingsPath = ('%s\\config\\addons\\%s\\'):format(AshitaCore:GetInstallPath(), 'FFXIFriendList')
    end
    return settingsPath
end

-- Load settings from disk
local function loadFromDisk()
    local path = M.getSettingsPath()
    local file = path .. settingsFile
    
    -- Check if file exists
    if not ashita.fs.exists(file) then
        return nil
    end
    
    -- Load the settings file
    local settings = nil
    local status, err = pcall(function()
        settings = loadfile(file)()
    end)
    
    if not status or type(settings) ~= 'table' then
        return nil
    end
    
    return T(settings)
end

-- Save settings to disk
local function saveToDisk(settings)
    local path = M.getSettingsPath()
    
    -- Create directory if it doesn't exist
    if not ashita.fs.exists(path) then
        ashita.fs.create_directory(path)
    end
    
    local file = path .. settingsFile
    
    -- Serialize settings
    local content = "require('common')\n\nlocal settings = " .. serializeValue(settings, '') .. "\n\nreturn settings;\n"
    
    -- Write to file
    local f = io.open(file, 'w+')
    if not f then
        -- Try creating parent directories manually
        os.execute('mkdir "' .. path:gsub('/', '\\') .. '" 2>nul')
        f = io.open(file, 'w+')
        if not f then
            return false
        end
    end
    
    f:write(content)
    f:close()
    
    return true
end

-- Deep merge tables (source values override target, but preserve target structure)
local function deepMerge(target, source)
    if type(target) ~= 'table' or type(source) ~= 'table' then
        return source ~= nil and source or target
    end
    
    for k, v in pairs(source) do
        if type(v) == 'table' and type(target[k]) == 'table' then
            deepMerge(target[k], v)
        else
            target[k] = v
        end
    end
    
    return target
end

-- Initialize settings system
-- Returns gConfig-compatible table
function M.initialize()
    -- Load from disk
    local loaded = loadFromDisk()
    
    -- Start with defaults
    currentSettings = defaultSettings:copy(true)
    
    -- Merge loaded settings
    if loaded then
        deepMerge(currentSettings, loaded)
    end
    
    -- Save merged settings (ensures new defaults are persisted)
    saveToDisk(currentSettings)
    
    return currentSettings
end

-- Get the full settings table
function M.getAll()
    if not currentSettings then
        currentSettings = M.initialize()
    end
    return currentSettings
end

-- Get a category of settings
function M.get(category)
    local settings = M.getAll()
    return settings[category]
end

-- Get a specific setting by path (e.g., "windows.friendList.visible")
function M.getValue(path)
    local settings = M.getAll()
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    
    local current = settings
    for _, part in ipairs(parts) do
        if type(current) ~= 'table' then
            return nil
        end
        current = current[part]
    end
    return current
end

-- Set a specific setting by path
function M.setValue(path, value, autoSave)
    local settings = M.getAll()
    local parts = {}
    for part in string.gmatch(path, "[^.]+") do
        table.insert(parts, part)
    end
    
    local current = settings
    for i = 1, #parts - 1 do
        local part = parts[i]
        if type(current[part]) ~= 'table' then
            current[part] = T{}
        end
        current = current[part]
    end
    
    current[parts[#parts]] = value
    
    -- Auto-save if requested (default: true)
    if autoSave ~= false then
        M.save()
    end
end

-- Update a category of settings
function M.set(category, values, autoSave)
    local settings = M.getAll()
    if type(values) == 'table' then
        if type(settings[category]) == 'table' then
            for k, v in pairs(values) do
                settings[category][k] = v
            end
        else
            settings[category] = values
        end
    else
        settings[category] = values
    end
    
    -- Auto-save if requested (default: true)
    if autoSave ~= false then
        M.save()
    end
end

-- Save settings to disk
function M.save()
    -- Sync from gConfig if they're different references
    -- This ensures all settings modified via gConfig are saved
    if _G.gConfig and currentSettings and _G.gConfig ~= currentSettings then
        -- Sync all top-level settings
        for k, v in pairs(_G.gConfig) do
            if k ~= "_version" then
                currentSettings[k] = v
            end
        end
    end
    
    if currentSettings then
        saveToDisk(currentSettings)
    end
end

-- Reload settings from disk
function M.reload()
    currentSettings = M.initialize()
    return currentSettings
end

-- Reset a category to defaults
function M.reset(category)
    local settings = M.getAll()
    if defaultSettings[category] then
        if type(defaultSettings[category]) == 'table' then
            settings[category] = defaultSettings[category]:copy(true)
        else
            settings[category] = defaultSettings[category]
        end
        M.save()
    end
end

-- Reset all settings to defaults (except API keys)
function M.resetAll()
    -- Preserve API keys
    local apiKeys = nil
    if currentSettings and currentSettings.data and currentSettings.data.apiKeys then
        apiKeys = currentSettings.data.apiKeys
    end
    
    currentSettings = defaultSettings:copy(true)
    
    -- Restore API keys
    if apiKeys then
        currentSettings.data.apiKeys = apiKeys
    end
    
    M.save()
    return currentSettings
end

-- ========================================
-- Custom Asset Override Folders
-- ========================================

-- README content for custom assets folder
local CUSTOM_ASSETS_README = [[
# FFXIFriendList Custom Assets

This folder allows you to override the default sounds and images used by FFXIFriendList.

## Sounds Folder

Place custom .wav files in the `sounds/` folder to override notification sounds:

- `online.wav` - Played when a friend comes online
- `friend-request.wav` - Played when you receive a friend request

Example:
  sounds/online.wav
  sounds/friend-request.wav

## Images Folder

Place custom .png files in the `images/` folder to override icons:

- `online.png` - Online status indicator
- `offline.png` - Offline status indicator
- `friend_request.png` - Friend request/pending icon
- `lock_icon.png` - Window locked icon
- `unlock_icon.png` - Window unlocked icon
- `discord.png` - Discord social button
- `github.png` - GitHub social button
- `heart.png` - About/Special Thanks button
- `sandy_icon.png` - San d'Oria nation icon
- `bastok_icon.png` - Bastok nation icon
- `windy_icon.png` - Windurst nation icon

Example:
  images/online.png
  images/discord.png

## Notes

- Files must match the exact names listed above
- Sounds must be in WAV format
- Images must be in PNG format
- If a custom file is not found, the default bundled asset will be used
- Changes take effect after reloading the addon

]]

-- Get custom assets base path
function M.getCustomAssetsPath()
    return M.getSettingsPath()
end

-- Get custom sounds folder path
function M.getCustomSoundsPath()
    return M.getSettingsPath() .. 'sounds\\'
end

-- Get custom images folder path
function M.getCustomImagesPath()
    return M.getSettingsPath() .. 'images\\'
end

-- Create custom asset folders and README on load
function M.ensureCustomAssetFolders()
    local basePath = M.getSettingsPath()
    local soundsPath = basePath .. 'sounds'
    local imagesPath = basePath .. 'images'
    local readmePath = basePath .. 'README.txt'
    
    -- Create base folder if needed
    if not ashita.fs.exists(basePath) then
        ashita.fs.create_directory(basePath)
    end
    
    -- Create sounds folder if needed
    if not ashita.fs.exists(soundsPath) then
        ashita.fs.create_directory(soundsPath)
    end
    
    -- Create images folder if needed
    if not ashita.fs.exists(imagesPath) then
        ashita.fs.create_directory(imagesPath)
    end
    
    -- Create README if it doesn't exist
    if not ashita.fs.exists(readmePath) then
        local f = io.open(readmePath, 'w')
        if f then
            f:write(CUSTOM_ASSETS_README)
            f:close()
        end
    end
    
    return true
end

-- Check if a custom sound file exists
function M.getCustomSoundPath(filename)
    local customPath = M.getCustomSoundsPath() .. filename
    if ashita.fs.exists(customPath) then
        return customPath
    end
    return nil
end

-- Check if a custom image file exists
function M.getCustomImagePath(filename)
    local customPath = M.getCustomImagesPath() .. filename
    if ashita.fs.exists(customPath) then
        return customPath
    end
    return nil
end

return M
