-- PathUtils.lua

local M = {}

-- Get the proper config path using Ashita's install path
-- This ensures all config files are saved to: <Ashita_Install>/config/addons/FFXIFriendList/
function M.getConfigPath(filename)
    -- Use AshitaCore:GetInstallPath() for correct path resolution (matches libs/settings.lua)
    if AshitaCore and AshitaCore.GetInstallPath then
        local installPath = AshitaCore:GetInstallPath():gsub('\\$', '')
        return installPath .. '\\config\\addons\\FFXIFriendList\\' .. filename
    end
    
    -- Fallback for non-Ashita environments (testing, etc.)
    return "config\\addons\\FFXIFriendList\\" .. filename
end

function M.getAssetPath(category, filename)
    local addonPath = string.match(debug.getinfo(1, "S").source:sub(2), "(.*[/\\])")
    if addonPath then
        return addonPath .. "..\\assets\\" .. category .. "\\" .. filename
    end
    
    return "assets\\" .. category .. "\\" .. filename
end

function M.ensureDirectory(filePath)
    local dir = string.match(filePath, "(.*[/\\])")
    if dir and ashita and ashita.fs and ashita.fs.create_directory then
        -- Use Ashita's fs rather than spawning a synchronous shell (os.execute
        -- mkdir) on the render thread; a theme save calls this several times.
        if not (ashita.fs.exists and ashita.fs.exists(dir)) then
            ashita.fs.create_directory(dir)
        end
    end
end

return M

