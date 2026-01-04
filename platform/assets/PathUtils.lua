-- PathUtils.lua

local M = {}

function M.getConfigPath(filename)
    local addonPath = string.match(debug.getinfo(1, "S").source:sub(2), "(.*[/\\])")
    if addonPath then
        local gameDir = string.match(addonPath, "(.*[/\\])")
        if gameDir then
            return gameDir .. "config\\addons\\FFXIFriendList\\" .. filename
        end
    end
    
    return "C:\\HorizonXI\\HorizonXI\\Game\\config\\addons\\FFXIFriendList\\" .. filename
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
    if dir then
        os.execute('mkdir "' .. dir .. '" 2>nul')
    end
end

return M

