--[[
* Themes Module - Display
* ImGui rendering for theme editor window
]]

local imgui = require('imgui')
local ThemeHelper = require('libs.themehelper')

local M = {}

-- ImGui constants
local ImGuiWindowFlags_NoMove = 0x00000004
local ImGuiWindowFlags_NoResize = 0x00000002
local ImGuiWindowFlags_NoTitleBar = 0x00000001
local ImGuiWindowFlags_AlwaysAutoResize = 0x00000040
local ImGuiWindowFlags_NoSavedSettings = 0x00002000
local ImGuiWindowFlags_NoFocusOnAppearing = 0x00001000
local ImGuiWindowFlags_NoNav = 0x00000800
local ImGuiWindowFlags_NoInputs = 0x00000400
local ImGuiWindowFlags_NoDecoration = 0x00000020
local ImGuiCond_FirstUseEver = 0x00000001
local ImGuiCond_Always = 0x00000000
local ImGuiColorEditFlags_AlphaBar = 0x00000002
local ImGuiColorEditFlags_NoInputs = 0x00000008

-- Module state
local state = {
    initialized = false,
    hidden = false,
    windowOpen = true,
    locked = false,
    lastCloseKeyState = false,
    -- Color buffers (for editing)
    colorBuffers = {
        windowBg = {1.0, 1.0, 1.0, 1.0},
        childBg = {1.0, 1.0, 1.0, 1.0},
        frameBg = {1.0, 1.0, 1.0, 1.0},
        frameBgHovered = {1.0, 1.0, 1.0, 1.0},
        frameBgActive = {1.0, 1.0, 1.0, 1.0},
        titleBg = {1.0, 1.0, 1.0, 1.0},
        titleBgActive = {1.0, 1.0, 1.0, 1.0},
        titleBgCollapsed = {1.0, 1.0, 1.0, 1.0},
        button = {1.0, 1.0, 1.0, 1.0},
        buttonHover = {1.0, 1.0, 1.0, 1.0},
        buttonActive = {1.0, 1.0, 1.0, 1.0},
        separator = {1.0, 1.0, 1.0, 1.0},
        separatorHovered = {1.0, 1.0, 1.0, 1.0},
        separatorActive = {1.0, 1.0, 1.0, 1.0},
        scrollbarBg = {1.0, 1.0, 1.0, 1.0},
        scrollbarGrab = {1.0, 1.0, 1.0, 1.0},
        scrollbarGrabHovered = {1.0, 1.0, 1.0, 1.0},
        scrollbarGrabActive = {1.0, 1.0, 1.0, 1.0},
        checkMark = {1.0, 1.0, 1.0, 1.0},
        sliderGrab = {1.0, 1.0, 1.0, 1.0},
        sliderGrabActive = {1.0, 1.0, 1.0, 1.0},
        header = {1.0, 1.0, 1.0, 1.0},
        headerHovered = {1.0, 1.0, 1.0, 1.0},
        headerActive = {1.0, 1.0, 1.0, 1.0},
        text = {1.0, 1.0, 1.0, 1.0},
        textDisabled = {1.0, 1.0, 1.0, 1.0}
    },
    -- Theme selection state
    selectedPresetIndex = 0,
    selectedThemeIndex = 0,
    newThemeName = {""},
    -- Collapsible sections
    presetExpanded = true,
    themeSelectionExpanded = false,
    customColorsExpanded = false,
    themeManagementExpanded = false,
    -- Alpha values (will be loaded from data module)
    backgroundAlpha = 0.95,
    textAlpha = 1.0,
    -- Available presets (will be loaded from data module)
    availablePresets = {},
    lastThemeIndex = -999
}

-- Window ID
local WINDOW_ID = "##themes_main"

-- Initialize display module
function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    
    -- Load window state from config
    if gConfig and gConfig.windows and gConfig.windows.themes then
        local windowState = gConfig.windows.themes
        if windowState.visible ~= nil then
            state.windowOpen = windowState.visible
        end
        if windowState.locked ~= nil then
            state.locked = windowState.locked
        end
    end
end

-- Update visuals when settings change
function M.UpdateVisuals(settings)
    state.settings = settings or {}
end

-- Set hidden state
function M.SetHidden(hidden)
    state.hidden = hidden or false
end

-- Set window open state
function M.SetWindowOpen(open)
    state.windowOpen = open
    M.SaveWindowState()
end

-- Check if window is visible (for close policy)
function M.IsVisible()
    return state.windowOpen or false
end

-- Set window visibility (for close policy - alias for SetWindowOpen)
function M.SetVisible(visible)
    M.SetWindowOpen(visible)
end

-- Save window state to config
function M.SaveWindowState()
    if not gConfig then
        gConfig = {}
    end
    if not gConfig.windows then
        gConfig.windows = {}
    end
    if not gConfig.windows.themes then
        gConfig.windows.themes = {}
    end
    
    local windowState = gConfig.windows.themes
    windowState.visible = state.windowOpen
    windowState.locked = state.locked
    
    if state.windowOpen then
        local posX, posY = imgui.GetWindowPos()
        if posX and posY then
            windowState.posX = posX
            windowState.posY = posY
        end
        local sizeX, sizeY = imgui.GetWindowSize()
        if sizeX and sizeY then
            windowState.sizeX = sizeX
            windowState.sizeY = sizeY
        end
    end
end

-- Helper: Sync buffers to theme colors
local function syncBuffersToColors()
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.themes then
        return
    end
    
    local themesFeature = app.features.themes
    
    -- Build color structure from buffers
    local colors = {
        windowBgColor = {r = state.colorBuffers.windowBg[1], g = state.colorBuffers.windowBg[2], b = state.colorBuffers.windowBg[3], a = state.colorBuffers.windowBg[4]},
        childBgColor = {r = state.colorBuffers.childBg[1], g = state.colorBuffers.childBg[2], b = state.colorBuffers.childBg[3], a = state.colorBuffers.childBg[4]},
        frameBgColor = {r = state.colorBuffers.frameBg[1], g = state.colorBuffers.frameBg[2], b = state.colorBuffers.frameBg[3], a = state.colorBuffers.frameBg[4]},
        frameBgHovered = {r = state.colorBuffers.frameBgHovered[1], g = state.colorBuffers.frameBgHovered[2], b = state.colorBuffers.frameBgHovered[3], a = state.colorBuffers.frameBgHovered[4]},
        frameBgActive = {r = state.colorBuffers.frameBgActive[1], g = state.colorBuffers.frameBgActive[2], b = state.colorBuffers.frameBgActive[3], a = state.colorBuffers.frameBgActive[4]},
        titleBg = {r = state.colorBuffers.titleBg[1], g = state.colorBuffers.titleBg[2], b = state.colorBuffers.titleBg[3], a = state.colorBuffers.titleBg[4]},
        titleBgActive = {r = state.colorBuffers.titleBgActive[1], g = state.colorBuffers.titleBgActive[2], b = state.colorBuffers.titleBgActive[3], a = state.colorBuffers.titleBgActive[4]},
        titleBgCollapsed = {r = state.colorBuffers.titleBgCollapsed[1], g = state.colorBuffers.titleBgCollapsed[2], b = state.colorBuffers.titleBgCollapsed[3], a = state.colorBuffers.titleBgCollapsed[4]},
        buttonColor = {r = state.colorBuffers.button[1], g = state.colorBuffers.button[2], b = state.colorBuffers.button[3], a = state.colorBuffers.button[4]},
        buttonHoverColor = {r = state.colorBuffers.buttonHover[1], g = state.colorBuffers.buttonHover[2], b = state.colorBuffers.buttonHover[3], a = state.colorBuffers.buttonHover[4]},
        buttonActiveColor = {r = state.colorBuffers.buttonActive[1], g = state.colorBuffers.buttonActive[2], b = state.colorBuffers.buttonActive[3], a = state.colorBuffers.buttonActive[4]},
        separatorColor = {r = state.colorBuffers.separator[1], g = state.colorBuffers.separator[2], b = state.colorBuffers.separator[3], a = state.colorBuffers.separator[4]},
        separatorHovered = {r = state.colorBuffers.separatorHovered[1], g = state.colorBuffers.separatorHovered[2], b = state.colorBuffers.separatorHovered[3], a = state.colorBuffers.separatorHovered[4]},
        separatorActive = {r = state.colorBuffers.separatorActive[1], g = state.colorBuffers.separatorActive[2], b = state.colorBuffers.separatorActive[3], a = state.colorBuffers.separatorActive[4]},
        scrollbarBg = {r = state.colorBuffers.scrollbarBg[1], g = state.colorBuffers.scrollbarBg[2], b = state.colorBuffers.scrollbarBg[3], a = state.colorBuffers.scrollbarBg[4]},
        scrollbarGrab = {r = state.colorBuffers.scrollbarGrab[1], g = state.colorBuffers.scrollbarGrab[2], b = state.colorBuffers.scrollbarGrab[3], a = state.colorBuffers.scrollbarGrab[4]},
        scrollbarGrabHovered = {r = state.colorBuffers.scrollbarGrabHovered[1], g = state.colorBuffers.scrollbarGrabHovered[2], b = state.colorBuffers.scrollbarGrabHovered[3], a = state.colorBuffers.scrollbarGrabHovered[4]},
        scrollbarGrabActive = {r = state.colorBuffers.scrollbarGrabActive[1], g = state.colorBuffers.scrollbarGrabActive[2], b = state.colorBuffers.scrollbarGrabActive[3], a = state.colorBuffers.scrollbarGrabActive[4]},
        checkMark = {r = state.colorBuffers.checkMark[1], g = state.colorBuffers.checkMark[2], b = state.colorBuffers.checkMark[3], a = state.colorBuffers.checkMark[4]},
        sliderGrab = {r = state.colorBuffers.sliderGrab[1], g = state.colorBuffers.sliderGrab[2], b = state.colorBuffers.sliderGrab[3], a = state.colorBuffers.sliderGrab[4]},
        sliderGrabActive = {r = state.colorBuffers.sliderGrabActive[1], g = state.colorBuffers.sliderGrabActive[2], b = state.colorBuffers.sliderGrabActive[3], a = state.colorBuffers.sliderGrabActive[4]},
        header = {r = state.colorBuffers.header[1], g = state.colorBuffers.header[2], b = state.colorBuffers.header[3], a = state.colorBuffers.header[4]},
        headerHovered = {r = state.colorBuffers.headerHovered[1], g = state.colorBuffers.headerHovered[2], b = state.colorBuffers.headerHovered[3], a = state.colorBuffers.headerHovered[4]},
        headerActive = {r = state.colorBuffers.headerActive[1], g = state.colorBuffers.headerActive[2], b = state.colorBuffers.headerActive[3], a = state.colorBuffers.headerActive[4]},
        textColor = {r = state.colorBuffers.text[1], g = state.colorBuffers.text[2], b = state.colorBuffers.text[3], a = state.colorBuffers.text[4]},
        textDisabled = {r = state.colorBuffers.textDisabled[1], g = state.colorBuffers.textDisabled[2], b = state.colorBuffers.textDisabled[3], a = state.colorBuffers.textDisabled[4]},
        borderColor = {r = state.colorBuffers.border[1], g = state.colorBuffers.border[2], b = state.colorBuffers.border[3], a = state.colorBuffers.border[4]}
    }
    
    -- Update theme colors (stores in currentCustomTheme for immediate application)
    themesFeature:updateCurrentThemeColors(colors)
end

-- Helper: Apply color change (syncs buffers to theme system)
local function applyColorChange()
    syncBuffersToColors()
end

-- Helper: Sync colors from theme to buffers
local function syncColorsToBuffers(dataModule)
    if not dataModule or dataModule.IsDefaultTheme() then
        return
    end
    
    -- Get current theme colors
    local colors = dataModule.GetCurrentThemeColors()
    if not colors then
        return
    end
    
    -- Sync colors to buffers
    if colors.windowBgColor then
        state.colorBuffers.windowBg = {colors.windowBgColor.r, colors.windowBgColor.g, colors.windowBgColor.b, colors.windowBgColor.a}
    end
    if colors.childBgColor then
        state.colorBuffers.childBg = {colors.childBgColor.r, colors.childBgColor.g, colors.childBgColor.b, colors.childBgColor.a}
    end
    if colors.frameBgColor then
        state.colorBuffers.frameBg = {colors.frameBgColor.r, colors.frameBgColor.g, colors.frameBgColor.b, colors.frameBgColor.a}
    end
    if colors.frameBgHovered then
        state.colorBuffers.frameBgHovered = {colors.frameBgHovered.r, colors.frameBgHovered.g, colors.frameBgHovered.b, colors.frameBgHovered.a}
    end
    if colors.frameBgActive then
        state.colorBuffers.frameBgActive = {colors.frameBgActive.r, colors.frameBgActive.g, colors.frameBgActive.b, colors.frameBgActive.a}
    end
    if colors.titleBg then
        state.colorBuffers.titleBg = {colors.titleBg.r, colors.titleBg.g, colors.titleBg.b, colors.titleBg.a}
    end
    if colors.titleBgActive then
        state.colorBuffers.titleBgActive = {colors.titleBgActive.r, colors.titleBgActive.g, colors.titleBgActive.b, colors.titleBgActive.a}
    end
    if colors.titleBgCollapsed then
        state.colorBuffers.titleBgCollapsed = {colors.titleBgCollapsed.r, colors.titleBgCollapsed.g, colors.titleBgCollapsed.b, colors.titleBgCollapsed.a}
    end
    if colors.buttonColor then
        state.colorBuffers.button = {colors.buttonColor.r, colors.buttonColor.g, colors.buttonColor.b, colors.buttonColor.a}
    end
    if colors.buttonHoverColor then
        state.colorBuffers.buttonHover = {colors.buttonHoverColor.r, colors.buttonHoverColor.g, colors.buttonHoverColor.b, colors.buttonHoverColor.a}
    end
    if colors.buttonActiveColor then
        state.colorBuffers.buttonActive = {colors.buttonActiveColor.r, colors.buttonActiveColor.g, colors.buttonActiveColor.b, colors.buttonActiveColor.a}
    end
    if colors.separatorColor then
        state.colorBuffers.separator = {colors.separatorColor.r, colors.separatorColor.g, colors.separatorColor.b, colors.separatorColor.a}
    end
    if colors.separatorHovered then
        state.colorBuffers.separatorHovered = {colors.separatorHovered.r, colors.separatorHovered.g, colors.separatorHovered.b, colors.separatorHovered.a}
    end
    if colors.separatorActive then
        state.colorBuffers.separatorActive = {colors.separatorActive.r, colors.separatorActive.g, colors.separatorActive.b, colors.separatorActive.a}
    end
    if colors.scrollbarBg then
        state.colorBuffers.scrollbarBg = {colors.scrollbarBg.r, colors.scrollbarBg.g, colors.scrollbarBg.b, colors.scrollbarBg.a}
    end
    if colors.scrollbarGrab then
        state.colorBuffers.scrollbarGrab = {colors.scrollbarGrab.r, colors.scrollbarGrab.g, colors.scrollbarGrab.b, colors.scrollbarGrab.a}
    end
    if colors.scrollbarGrabHovered then
        state.colorBuffers.scrollbarGrabHovered = {colors.scrollbarGrabHovered.r, colors.scrollbarGrabHovered.g, colors.scrollbarGrabHovered.b, colors.scrollbarGrabHovered.a}
    end
    if colors.scrollbarGrabActive then
        state.colorBuffers.scrollbarGrabActive = {colors.scrollbarGrabActive.r, colors.scrollbarGrabActive.g, colors.scrollbarGrabActive.b, colors.scrollbarGrabActive.a}
    end
    if colors.checkMark then
        state.colorBuffers.checkMark = {colors.checkMark.r, colors.checkMark.g, colors.checkMark.b, colors.checkMark.a}
    end
    if colors.sliderGrab then
        state.colorBuffers.sliderGrab = {colors.sliderGrab.r, colors.sliderGrab.g, colors.sliderGrab.b, colors.sliderGrab.a}
    end
    if colors.sliderGrabActive then
        state.colorBuffers.sliderGrabActive = {colors.sliderGrabActive.r, colors.sliderGrabActive.g, colors.sliderGrabActive.b, colors.sliderGrabActive.a}
    end
    if colors.header then
        state.colorBuffers.header = {colors.header.r, colors.header.g, colors.header.b, colors.header.a}
    end
    if colors.headerHovered then
        state.colorBuffers.headerHovered = {colors.headerHovered.r, colors.headerHovered.g, colors.headerHovered.b, colors.headerHovered.a}
    end
    if colors.headerActive then
        state.colorBuffers.headerActive = {colors.headerActive.r, colors.headerActive.g, colors.headerActive.b, colors.headerActive.a}
    end
    if colors.textColor then
        state.colorBuffers.text = {colors.textColor.r, colors.textColor.g, colors.textColor.b, colors.textColor.a}
    end
    if colors.textDisabled then
        state.colorBuffers.textDisabled = {colors.textDisabled.r, colors.textDisabled.g, colors.textDisabled.b, colors.textDisabled.a}
    end
    if colors.borderColor then
        state.colorBuffers.border = {colors.borderColor.r, colors.borderColor.g, colors.borderColor.b, colors.borderColor.a}
    end
end

-- Render theme preset selector (removed - presets no longer used)
local function renderThemePresetSelector(dataModule)
    -- Presets removed - this section is hidden
    return
end

-- Render theme selection
local function renderThemeSelection(dataModule)
    -- Presets removed - always show theme selection
    
    if imgui.CollapsingHeader("Theme Selection") then
        state.themeSelectionExpanded = true
        
        local builtInNames = dataModule.GetBuiltInThemeNames() or {}
        local customThemes = dataModule.GetCustomThemes() or {}
        
        -- Build theme list
        local allThemeNames = {}
        local themeIndices = {}
        
        -- Add built-in themes (only 4 themes: Classic, Modern Dark, Green Nature, Purple Mystic)
        -- Mapping: builtInThemeNames[1] = "Classic" (was Warm Brown) -> themeIndex 0
        --          builtInThemeNames[2] = "Modern Dark" -> themeIndex 1
        --          builtInThemeNames[3] = "Green Nature" -> themeIndex 2
        --          builtInThemeNames[4] = "Purple Mystic" -> themeIndex 3
        -- Note: "Default (No Theme)" (themeIndex -2) is excluded from the list
        for i, name in ipairs(builtInNames) do
            table.insert(allThemeNames, name)
            local themeIndex = i - 1  -- Index 0 maps to builtInThemeNames[1], etc.
            table.insert(themeIndices, themeIndex)
        end
        
        -- Add custom themes
        for _, customTheme in ipairs(customThemes) do
            table.insert(allThemeNames, customTheme.name or "Unknown")
            table.insert(themeIndices, -1)  -- -1 indicates custom theme
        end
        
        if #allThemeNames == 0 then
            imgui.Text("No themes available")
            return
        end
        
        -- Find current theme index
        local currentThemeIndex = dataModule.GetCurrentThemeIndex()
        local currentThemeName = dataModule.GetCurrentThemeName()
        local currentComboIndex = 0
        
        for i, themeIndex in ipairs(themeIndices) do
            if themeIndex == currentThemeIndex then
                if themeIndex == -1 then
                    -- Custom theme - check name
                    if allThemeNames[i] == currentThemeName then
                        currentComboIndex = i - 1
                        break
                    end
                else
                    currentComboIndex = i - 1
                    break
                end
            end
        end
        
        local comboIndex = {currentComboIndex}
        if imgui.Combo("Theme", comboIndex, allThemeNames, #allThemeNames) then
            if comboIndex[1] >= 0 and comboIndex[1] < #allThemeNames then
                local selectedIndex = themeIndices[comboIndex[1] + 1]
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    if selectedIndex == -1 then
                        -- Custom theme
                        local themeName = allThemeNames[comboIndex[1] + 1]
                        app.features.themes:setCustomTheme(themeName)
                    else
                        -- Built-in theme
                        app.features.themes:applyTheme(selectedIndex)
                    end
                end
            end
        end
        
        -- Transparency sliders (only if not default theme)
        if not dataModule.IsDefaultTheme() then
            imgui.Spacing()
            imgui.Text("Background Transparency:")
            local bgAlpha = {state.backgroundAlpha}
            if imgui.SliderFloat("##bgAlpha", bgAlpha, 0.0, 1.0, "%.2f") then
                state.backgroundAlpha = bgAlpha[1]
                -- Set background alpha
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    app.features.themes:setBackgroundAlpha(bgAlpha[1])
                end
            end
            if imgui.IsItemDeactivatedAfterEdit() then
                -- Alpha changed - theme system should auto-save
            end
            
            imgui.Text("Text Transparency:")
            local textAlpha = {state.textAlpha}
            if imgui.SliderFloat("##textAlpha", textAlpha, 0.0, 1.0, "%.2f") then
                state.textAlpha = textAlpha[1]
                -- Set text alpha
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    app.features.themes:setTextAlpha(textAlpha[1])
                end
            end
            if imgui.IsItemDeactivatedAfterEdit() then
                -- Alpha changed - theme system should auto-save
            end
            
            -- Custom Colors section
            renderCustomColors()
        end
    else
        state.themeSelectionExpanded = false
    end
end

-- Render custom colors
local function renderCustomColors()
    if imgui.CollapsingHeader("Custom Colors") then
        state.customColorsExpanded = true
        
        -- Helper to render a color picker
        local function renderColorPicker(label, bufferKey)
            local color = state.colorBuffers[bufferKey]
            if imgui.ColorEdit4(label, color, bit.bor(ImGuiColorEditFlags_AlphaBar, ImGuiColorEditFlags_NoInputs)) then
                applyColorChange()
            end
        end
        
        -- Window colors
        imgui.Text("Window Colors:")
        renderColorPicker("Window Background", "windowBg")
        renderColorPicker("Child Background", "childBg")
        renderColorPicker("Frame Background", "frameBg")
        renderColorPicker("Frame Hovered", "frameBgHovered")
        renderColorPicker("Frame Active", "frameBgActive")
        
        imgui.Spacing()
        
        -- Title colors
        imgui.Text("Title Colors:")
        renderColorPicker("Title Background", "titleBg")
        renderColorPicker("Title Active", "titleBgActive")
        renderColorPicker("Title Collapsed", "titleBgCollapsed")
        
        imgui.Spacing()
        
        -- Button colors
        imgui.Text("Button Colors:")
        renderColorPicker("Button", "button")
        renderColorPicker("Button Hovered", "buttonHover")
        renderColorPicker("Button Active", "buttonActive")
        
        imgui.Spacing()
        
        -- Separator colors
        imgui.Text("Separator Colors:")
        renderColorPicker("Separator", "separator")
        renderColorPicker("Separator Hovered", "separatorHovered")
        renderColorPicker("Separator Active", "separatorActive")
        
        imgui.Spacing()
        
        -- Scrollbar colors
        imgui.Text("Scrollbar Colors:")
        renderColorPicker("Scrollbar Bg", "scrollbarBg")
        renderColorPicker("Scrollbar Grab", "scrollbarGrab")
        renderColorPicker("Scrollbar Grab Hovered", "scrollbarGrabHovered")
        renderColorPicker("Scrollbar Grab Active", "scrollbarGrabActive")
        
        imgui.Spacing()
        
        -- Check/Slider colors
        imgui.Text("Check/Slider Colors:")
        renderColorPicker("Check Mark", "checkMark")
        renderColorPicker("Slider Grab", "sliderGrab")
        renderColorPicker("Slider Grab Active", "sliderGrabActive")
        
        imgui.Spacing()
        
        -- Header colors
        imgui.Text("Header Colors:")
        renderColorPicker("Header", "header")
        renderColorPicker("Header Hovered", "headerHovered")
        renderColorPicker("Header Active", "headerActive")
        
        imgui.Spacing()
        
        -- Text colors
        imgui.Text("Text Colors:")
        renderColorPicker("Text", "text")
        renderColorPicker("Text Disabled", "textDisabled")
    else
        state.customColorsExpanded = false
    end
end

-- Render theme management
local function renderThemeManagement(dataModule)
    if imgui.CollapsingHeader("Theme Management") then
        state.themeManagementExpanded = true
        
        imgui.Text("Save Current Colors as Theme")
        imgui.Text("Theme Name:")
        if imgui.InputText("##saveThemeName", state.newThemeName, 256) then
            -- Name updated
        end
        
        local canSave = state.newThemeName[1] and state.newThemeName[1] ~= "" and #state.newThemeName[1] > 0
        if not canSave then
            imgui.PushStyleColor(ImGuiCol_Button, {0.5, 0.5, 0.5, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.5, 0.5, 0.5, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.5, 0.5, 0.5, 1.0})
        end
        
        if imgui.Button("Save Custom Theme") then
            if canSave then
                -- Sync buffers to colors, then save
                syncBuffersToColors()
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    local colors = dataModule.GetCurrentThemeColors()
                    if colors then
                        colors.name = state.newThemeName[1]
                        app.features.themes:saveCustomTheme(state.newThemeName[1], colors)
                        state.newThemeName[1] = ""  -- Clear after save
                    end
                end
            end
        end
        
        if not canSave then
            imgui.PopStyleColor(3)
        end
        
        -- Delete current custom theme (if applicable)
        local currentCustomThemeName = dataModule.GetCurrentCustomThemeName() or ""
        if currentCustomThemeName ~= "" then
            imgui.Spacing()
            if imgui.Button("Delete Custom Theme") then
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    app.features.themes:deleteCustomTheme(currentCustomThemeName)
                end
            end
        end
    else
        state.themeManagementExpanded = false
    end
end

-- Main render function
function M.DrawWindow(settings, dataModule)
    if not state.initialized or state.hidden then
        return
    end
    
    if not dataModule then
        return
    end
    
    -- Window flags
    local windowFlags = 0
    if state.locked then
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoMove)
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoResize)
    end
    
    -- Restore window position/size from config
    if gConfig and gConfig.windows and gConfig.windows.themes then
        local windowState = gConfig.windows.themes
        if windowState.posX and windowState.posY then
            imgui.SetNextWindowPos({windowState.posX, windowState.posY}, ImGuiCond_FirstUseEver)
        end
        if windowState.sizeX and windowState.sizeY then
            imgui.SetNextWindowSize({windowState.sizeX, windowState.sizeY}, ImGuiCond_FirstUseEver)
        end
    else
        -- Default size
        imgui.SetNextWindowSize({600, 700}, ImGuiCond_FirstUseEver)
    end
    
    local windowTitle = "Themes" .. WINDOW_ID
    
    -- Update locked state from config
    if gConfig and gConfig.windows and gConfig.windows.themes then
        state.locked = gConfig.windows.themes.locked or false
    end
    
    -- NOTE: ESC/close key handling is now centralized in ui/input/close_input.lua
    
    -- Apply theme styles (only if not default theme)
    local themePushed = false
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.themes then
        local themesFeature = app.features.themes
        local themeIndex = themesFeature:getThemeIndex()
        
        -- Only apply theme if not default (themeIndex != -2)
        if themeIndex ~= -2 then
            local success, err = pcall(function()
                local themeColors = themesFeature:getCurrentThemeColors()
                local backgroundAlpha = themesFeature:getBackgroundAlpha()
                local textAlpha = themesFeature:getTextAlpha()
                
                if themeColors then
                    themePushed = ThemeHelper.pushThemeStyles(themeColors, backgroundAlpha, textAlpha)
                end
            end)
            if not success then
                if app.deps and app.deps.logger and app.deps.logger.error then
                    app.deps.logger.error("[Themes Display] Theme application failed: " .. tostring(err))
                else
                    print("[FFXIFriendList ERROR] Themes Display: Theme application failed: " .. tostring(err))
                end
            end
        end
    end
    
    -- Pass windowOpen as a table so ImGui can modify it when X button is clicked
    local windowOpenTable = {state.windowOpen}
    local wasOpen = state.windowOpen
    local windowOpen = imgui.Begin(windowTitle, windowOpenTable, windowFlags)
    
    -- Handle X button close with lock/gating
    if wasOpen and not windowOpenTable[1] then
        -- X button clicked - check if close should be blocked
        if state.locked then
            state.windowOpen = true  -- Keep open
        else
            local closeGating = require('ui.close_gating')
            if closeGating.shouldDeferClose() then
                state.windowOpen = true  -- Keep open
            else
                state.windowOpen = false
            end
        end
    else
        state.windowOpen = windowOpenTable[1]
    end
    
    if windowOpen then
        -- Update alpha values from data module
        if app and app.features and app.features.themes then
            state.backgroundAlpha = app.features.themes:getBackgroundAlpha() or 0.95
            state.textAlpha = app.features.themes:getTextAlpha() or 1.0
        end
        
        -- Sync colors from theme to buffers when theme changes
        local currentThemeIndex = dataModule.GetCurrentThemeIndex()
        if currentThemeIndex ~= state.lastThemeIndex then
            syncColorsToBuffers(dataModule)
            state.lastThemeIndex = currentThemeIndex
        end
        
        -- Render content in scrollable child
        imgui.BeginChild("##themes_body", {0, 0}, false)
        
        -- Render sections
        renderThemePresetSelector(dataModule)
        renderThemeSelection(dataModule)
        renderThemeManagement(dataModule)
        
        imgui.EndChild()
        
        -- Lock button
        imgui.Separator()
        local lockLabel = state.locked and "[Locked]" or "[Unlocked]"
        if imgui.Button(lockLabel) then
            state.locked = not state.locked
            if gConfig then
                if not gConfig.windows then
                    gConfig.windows = {}
                end
                if not gConfig.windows.themes then
                    gConfig.windows.themes = {}
                end
                gConfig.windows.themes.locked = state.locked
            end
        end
        if imgui.IsItemHovered() then
            imgui.SetTooltip(state.locked and "Window locked" or "Lock window")
        end
    end
    
    imgui.End()
    
    -- Pop theme styles if we pushed them
    if themePushed then
        local success, err = pcall(ThemeHelper.popThemeStyles)
        if not success then
            local app = _G.FFXIFriendListApp
            if app and app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[Themes Display] Theme pop failed: " .. tostring(err))
            else
                print("[FFXIFriendList ERROR] Themes Display: Theme pop failed: " .. tostring(err))
            end
        end
    end
    
    -- Save window state if changed
    if not windowOpen then
        M.SaveWindowState()
    end
end

-- Cleanup
function M.Cleanup()
    state.initialized = false
end

return M

