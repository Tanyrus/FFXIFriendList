--[[
* Font Capability Probe (DEV ONLY)
* Diagnoses Ashita v4 ImGui Lua binding capabilities for font management
* 
* This module probes the available ImGui functions to determine if:
* - PushFont/PopFont are accessible for font switching
* - Font atlas manipulation is possible from Lua
* - Font handles can be obtained and used
*
* Run once on addon load when _FFXIFL_DEV_PROBE_FONTS = true
]]--

local M = {}

-- Probe results storage
local results = {
    imgui_functions = {},
    io_properties = {},
    fonts_atlas = {},
    conclusion = ""
}

-- Helper to safely check if a function/property exists
local function probe(tbl, key)
    if tbl == nil then
        return nil, "table is nil"
    end
    
    local success, value = pcall(function()
        return tbl[key]
    end)
    
    if not success then
        return nil, "access error"
    end
    
    return value, type(value)
end

-- Helper to format result for logging
local function formatResult(name, value, valueType)
    if value == nil then
        return string.format("  %s: nil (%s)", name, valueType or "not found")
    elseif valueType == "function" then
        return string.format("  %s: EXISTS (function)", name)
    elseif valueType == "userdata" then
        return string.format("  %s: EXISTS (userdata)", name)
    elseif valueType == "table" then
        return string.format("  %s: EXISTS (table)", name)
    elseif valueType == "number" then
        return string.format("  %s: %s (number)", name, tostring(value))
    else
        return string.format("  %s: %s (%s)", name, tostring(value), valueType)
    end
end

-- Main probe function
function M.run()
    local imgui = require('imgui')
    
    print("[FontProbe] ========================================")
    print("[FontProbe] Ashita v4 ImGui Font Capability Probe")
    print("[FontProbe] ========================================")
    
    -- Section 1: Probe imgui direct functions
    print("[FontProbe]")
    print("[FontProbe] 1. ImGui Direct Functions:")
    
    local functionsToProbe = {
        "PushFont",
        "PopFont",
        "GetFont",
        "GetFontSize",
        "SetWindowFontScale",
        "GetIO",
        "GetCurrentContext",
    }
    
    for _, funcName in ipairs(functionsToProbe) do
        local value, valueType = probe(imgui, funcName)
        results.imgui_functions[funcName] = { value = value, type = valueType }
        print("[FontProbe] " .. formatResult(funcName, value, valueType))
    end
    
    -- Section 2: Probe GetIO() contents
    print("[FontProbe]")
    print("[FontProbe] 2. GetIO() Contents:")
    
    local io = nil
    if type(imgui.GetIO) == "function" then
        local success, ioResult = pcall(function()
            return imgui.GetIO()
        end)
        
        if success and ioResult then
            io = ioResult
            print("[FontProbe]   GetIO() call: SUCCESS")
            
            local ioPropsToProbe = {
                "Fonts",
                "FontGlobalScale",
                "FontDefault",
                "FontAllowUserScaling",
                "DisplaySize",
            }
            
            for _, propName in ipairs(ioPropsToProbe) do
                local value, valueType = probe(io, propName)
                results.io_properties[propName] = { value = value, type = valueType }
                print("[FontProbe] " .. formatResult("io." .. propName, value, valueType))
            end
        else
            print("[FontProbe]   GetIO() call: FAILED or returned nil")
        end
    else
        print("[FontProbe]   GetIO not available")
    end
    
    -- Section 3: Probe Font Atlas (if io.Fonts exists)
    print("[FontProbe]")
    print("[FontProbe] 3. Font Atlas (io.Fonts):")
    
    if io then
        local fonts, fontsType = probe(io, "Fonts")
        
        if fonts and (fontsType == "userdata" or fontsType == "table") then
            print("[FontProbe]   io.Fonts exists, probing methods...")
            
            local atlasMethodsToProbe = {
                "AddFontFromFileTTF",
                "AddFontDefault",
                "Clear",
                "Build",
                "GetTexDataAsRGBA32",
                "GetTexDataAsAlpha8",
                "GetGlyphRangesDefault",
            }
            
            for _, methodName in ipairs(atlasMethodsToProbe) do
                local value, valueType = probe(fonts, methodName)
                results.fonts_atlas[methodName] = { value = value, type = valueType }
                print("[FontProbe] " .. formatResult("Fonts." .. methodName, value, valueType))
            end
            
            -- Try to get font count or similar
            local countMethods = {"GetFontCount", "Size", "size", "count"}
            for _, methodName in ipairs(countMethods) do
                local value, valueType = probe(fonts, methodName)
                if value then
                    print("[FontProbe] " .. formatResult("Fonts." .. methodName, value, valueType))
                end
            end
        else
            print("[FontProbe]   io.Fonts: NOT ACCESSIBLE (" .. tostring(fontsType) .. ")")
        end
    else
        print("[FontProbe]   Skipped (io not available)")
    end
    
    -- Section 4: Probe GetFont() result (if available)
    print("[FontProbe]")
    print("[FontProbe] 4. GetFont() Result:")
    
    local currentFont = nil
    if type(imgui.GetFont) == "function" then
        local success, fontResult = pcall(function()
            return imgui.GetFont()
        end)
        
        if success and fontResult then
            currentFont = fontResult
            print("[FontProbe]   GetFont() returned: " .. type(fontResult))
            
            -- Probe the font object
            local fontPropsToProbe = {
                "FontSize",
                "Scale",
                "GetCharAdvance",
                "GetDebugName",
            }
            
            for _, propName in ipairs(fontPropsToProbe) do
                local value, valueType = probe(fontResult, propName)
                if value then
                    print("[FontProbe] " .. formatResult("font." .. propName, value, valueType))
                end
            end
        else
            print("[FontProbe]   GetFont() returned nil or failed")
        end
    else
        print("[FontProbe]   GetFont not available")
    end
    
    -- Section 5: Try PushFont/PopFont with current font
    print("[FontProbe]")
    print("[FontProbe] 5. PushFont/PopFont Test:")
    
    if type(imgui.PushFont) == "function" and type(imgui.PopFont) == "function" then
        -- Try pushing the current font
        if currentFont then
            local pushSuccess, pushErr = pcall(function()
                imgui.PushFont(currentFont)
            end)
            
            if pushSuccess then
                print("[FontProbe]   PushFont(currentFont): SUCCESS")
                
                -- Try to pop
                local popSuccess, popErr = pcall(function()
                    imgui.PopFont()
                end)
                
                if popSuccess then
                    print("[FontProbe]   PopFont(): SUCCESS")
                else
                    print("[FontProbe]   PopFont(): FAILED - " .. tostring(popErr))
                end
            else
                print("[FontProbe]   PushFont(currentFont): FAILED - " .. tostring(pushErr))
            end
        end
        
        -- Try pushing nil (to see error message)
        local nilPushSuccess, nilPushErr = pcall(function()
            imgui.PushFont(nil)
        end)
        print("[FontProbe]   PushFont(nil): " .. (nilPushSuccess and "accepted" or "rejected: " .. tostring(nilPushErr)))
        if nilPushSuccess then
            pcall(imgui.PopFont)
        end
    else
        print("[FontProbe]   PushFont/PopFont not available")
    end
    
    -- Section 6: Probe io.Fonts for indexed font access
    print("[FontProbe]")
    print("[FontProbe] 6. Font Atlas Indexed Access:")
    
    local fontsVector = nil
    if io then
        local fonts, fontsType = probe(io, "Fonts")
        
        if fonts then
            -- Try various ways to access fonts by index
            local accessMethods = {
                {"Fonts[0]", function() return fonts[0] end},
                {"Fonts[1]", function() return fonts[1] end},
                {"Fonts.Fonts", function() return fonts.Fonts end},
                {"Fonts:GetFonts()", function() return fonts:GetFonts() end},
                {"Fonts.TexID", function() return fonts.TexID end},
                {"Fonts.TexWidth", function() return fonts.TexWidth end},
                {"Fonts.TexHeight", function() return fonts.TexHeight end},
            }
            
            for _, method in ipairs(accessMethods) do
                local name, accessor = method[1], method[2]
                local success, result = pcall(accessor)
                if success and result ~= nil then
                    print("[FontProbe]   " .. name .. ": " .. type(result) .. " = " .. tostring(result))
                    if name == "Fonts.Fonts" then
                        fontsVector = result
                    end
                end
            end
        end
    end
    
    -- Section 6b: Probe the Fonts vector for individual font access
    print("[FontProbe]")
    print("[FontProbe] 6b. Fonts Vector Access:")
    
    if fontsVector then
        -- Try to access fonts by index
        for i = 0, 5 do
            local success, font = pcall(function() return fontsVector[i] end)
            if success and font ~= nil then
                local fontSize = "?"
                local fontSizeSuccess, fs = pcall(function() return font.FontSize end)
                if fontSizeSuccess and fs then fontSize = tostring(fs) end
                print("[FontProbe]   fontsVector[" .. i .. "]: " .. type(font) .. " (FontSize=" .. fontSize .. ")")
                
                -- Try pushing this font
                local pushSuccess = pcall(function()
                    imgui.PushFont(font)
                    imgui.PopFont()
                end)
                print("[FontProbe]     PushFont test: " .. (pushSuccess and "SUCCESS" or "FAILED"))
            end
        end
        
        -- Try Size/size/Count methods
        local sizeMethods = {"Size", "size", "Count", "count", "Length", "length"}
        for _, method in ipairs(sizeMethods) do
            local success, result = pcall(function() return fontsVector[method] end)
            if success and result ~= nil then
                if type(result) == "function" then
                    local callSuccess, callResult = pcall(result, fontsVector)
                    if callSuccess then
                        print("[FontProbe]   fontsVector." .. method .. "(): " .. tostring(callResult))
                    end
                else
                    print("[FontProbe]   fontsVector." .. method .. ": " .. tostring(result))
                end
            end
        end
    else
        print("[FontProbe]   fontsVector not accessible")
    end
    
    -- Section 7: Conclusion
    print("[FontProbe]")
    print("[FontProbe] ========================================")
    print("[FontProbe] CONCLUSION:")
    
    local hasPushFont = results.imgui_functions["PushFont"] and results.imgui_functions["PushFont"].type == "function"
    local hasPopFont = results.imgui_functions["PopFont"] and results.imgui_functions["PopFont"].type == "function"
    local hasGetFont = results.imgui_functions["GetFont"] and results.imgui_functions["GetFont"].type == "function"
    local hasFontsAtlas = results.io_properties["Fonts"] and (results.io_properties["Fonts"].type == "userdata" or results.io_properties["Fonts"].type == "table")
    local hasAddFont = results.fonts_atlas["AddFontFromFileTTF"] and results.fonts_atlas["AddFontFromFileTTF"].type == "function"
    
    -- Check if we found preloaded fonts
    local preloadedFontCount = 0
    if fontsVector then
        local success, count = pcall(function() return fontsVector.size() end)
        if success and count then
            preloadedFontCount = count
        end
    end
    
    if preloadedFontCount > 1 then
        results.conclusion = "PRELOADED_FONTS_AVAILABLE"
        print("[FontProbe]   SUCCESS: " .. preloadedFontCount .. " preloaded fonts found!")
        print("[FontProbe]   Font sizes available via fontsVector[1-" .. preloadedFontCount .. "]")
        print("[FontProbe]   Recommendation: Use PushFont/PopFont with preloaded fonts")
        print("[FontProbe]   Implementation:")
        print("[FontProbe]     1. Store font refs: io.Fonts.Fonts[1], [2], [3], [4]")
        print("[FontProbe]     2. User selects font size (14/18/24/32)")
        print("[FontProbe]     3. Call imgui.PushFont(selectedFont) after Begin()")
        print("[FontProbe]     4. Call imgui.PopFont() before End()")
    elseif hasPushFont and hasPopFont and hasAddFont then
        results.conclusion = "FULL_SUPPORT"
        print("[FontProbe]   FULL SUPPORT: PushFont/PopFont + AddFont available")
        print("[FontProbe]   Recommendation: Preload fonts at different sizes, use PushFont switching")
    elseif hasPushFont and hasPopFont and hasGetFont then
        results.conclusion = "PARTIAL_SUPPORT"
        print("[FontProbe]   PARTIAL: PushFont/PopFont exist, but no AddFont access")
        print("[FontProbe]   Check if Ashita preloads fonts that can be accessed")
    elseif hasPushFont and hasPopFont then
        results.conclusion = "LIMITED_SUPPORT"
        print("[FontProbe]   LIMITED: PushFont/PopFont exist but no font handle access")
        print("[FontProbe]   Cannot use for multi-font switching from Lua")
    else
        results.conclusion = "NO_SUPPORT"
        print("[FontProbe]   NO SUPPORT: Font switching not available from Lua")
        print("[FontProbe]   Recommendation: Use SetWindowFontScale or abandon font-size feature")
    end
    
    print("[FontProbe] ========================================")
    
    return results
end

-- Get stored results (for UI display if needed)
function M.getResults()
    return results
end

return M

