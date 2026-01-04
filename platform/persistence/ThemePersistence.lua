-- ThemePersistence.lua

local Json = require("protocol.Json")
local PathUtils = require("platform.assets.PathUtils")

local M = {}

function M.loadFromFile()
    local path = PathUtils.getConfigPath("ffxifriendlist.ini")
    local file = io.open(path, "r")
    if not file then
        return nil
    end
    
    local themes = {}
    local currentTheme = nil
    
    for line in file:lines() do
        line = line:match("^%s*(.-)%s*$")
        if line == "" or line:match("^#") then
        elseif line:match("^%[") then
            currentTheme = line:match("%[([^%]]+)%]")
        elseif currentTheme and line:match("=") then
            local key, value = line:match("([^=]+)=(.+)")
            if key and value then
                if not themes[currentTheme] then
                    themes[currentTheme] = {}
                end
                themes[currentTheme][key:match("^%s*(.-)%s*$")] = value:match("^%s*(.-)%s*$")
            end
        end
    end
    
    file:close()
    return themes
end

function M.saveToFile(themes)
    local path = PathUtils.getConfigPath("ffxifriendlist.ini")
    PathUtils.ensureDirectory(path)
    
    local file = io.open(path, "w")
    if not file then
        return false
    end
    
    for themeName, themeData in pairs(themes) do
        file:write("[" .. themeName .. "]\n")
        for key, value in pairs(themeData) do
            file:write(key .. "=" .. tostring(value) .. "\n")
        end
        file:write("\n")
    end
    
    file:close()
    return true
end

return M

