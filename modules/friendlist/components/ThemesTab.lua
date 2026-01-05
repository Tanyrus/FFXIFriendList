local imgui = require('imgui')

local M = {}

function M.Render(state, dataModule, callbacks)
    local themesDataModule = require('modules.themes.data')
    if not themesDataModule then
        imgui.Text("Themes data module not available")
        return
    end
    
    local testNames = themesDataModule.GetBuiltInThemeNames()
    if not testNames or #testNames == 0 then
        themesDataModule.Initialize({})
    end
    
    themesDataModule.Update()
    
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.themes then
        state.themeBackgroundAlpha = app.features.themes:getBackgroundAlpha() or 0.95
        state.themeTextAlpha = app.features.themes:getTextAlpha() or 1.0
        state.themeFontSizePx = app.features.themes:getFontSizePx() or 14
    end
    
    local currentThemeIndex = themesDataModule.GetCurrentThemeIndex()
    if currentThemeIndex ~= state.themeLastThemeIndex then
        M.SyncThemeColorsToBuffers(state, themesDataModule)
        state.themeLastThemeIndex = currentThemeIndex
    end
    
    M.RenderThemeSelection(state, themesDataModule)
    imgui.Spacing()
    M.RenderThemeManagement(state, themesDataModule)
end

function M.SyncThemeColorsToBuffers(state, dataModule)
    if not dataModule or dataModule.IsDefaultTheme() then
        return
    end
    
    local colors = dataModule.GetCurrentThemeColors()
    if not colors then
        return
    end
    
    local function syncColor(bufferKey, colorObj)
        if colorObj then
            state.themeColorBuffers[bufferKey] = {colorObj.r, colorObj.g, colorObj.b, colorObj.a}
        end
    end
    
    syncColor("windowBg", colors.windowBgColor)
    syncColor("childBg", colors.childBgColor)
    syncColor("frameBg", colors.frameBgColor)
    syncColor("frameBgHovered", colors.frameBgHovered)
    syncColor("frameBgActive", colors.frameBgActive)
    syncColor("titleBg", colors.titleBg)
    syncColor("titleBgActive", colors.titleBgActive)
    syncColor("titleBgCollapsed", colors.titleBgCollapsed)
    syncColor("button", colors.buttonColor)
    syncColor("buttonHover", colors.buttonHoverColor)
    syncColor("buttonActive", colors.buttonActiveColor)
    syncColor("separator", colors.separatorColor)
    syncColor("separatorHovered", colors.separatorHovered)
    syncColor("separatorActive", colors.separatorActive)
    syncColor("scrollbarBg", colors.scrollbarBg)
    syncColor("scrollbarGrab", colors.scrollbarGrab)
    syncColor("scrollbarGrabHovered", colors.scrollbarGrabHovered)
    syncColor("scrollbarGrabActive", colors.scrollbarGrabActive)
    syncColor("checkMark", colors.checkMark)
    syncColor("sliderGrab", colors.sliderGrab)
    syncColor("sliderGrabActive", colors.sliderGrabActive)
    syncColor("header", colors.header)
    syncColor("headerHovered", colors.headerHovered)
    syncColor("headerActive", colors.headerActive)
    syncColor("text", colors.textColor)
    syncColor("textDisabled", colors.textDisabled)
    syncColor("border", colors.borderColor)
end

function M.SyncThemeBuffersToColors(state)
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.themes then
        return
    end
    
    local themesFeature = app.features.themes
    
    local function bufferToColor(bufferKey)
        local b = state.themeColorBuffers[bufferKey]
        return {r = b[1], g = b[2], b = b[3], a = b[4]}
    end
    
    local colors = {
        windowBgColor = bufferToColor("windowBg"),
        childBgColor = bufferToColor("childBg"),
        frameBgColor = bufferToColor("frameBg"),
        frameBgHovered = bufferToColor("frameBgHovered"),
        frameBgActive = bufferToColor("frameBgActive"),
        titleBg = bufferToColor("titleBg"),
        titleBgActive = bufferToColor("titleBgActive"),
        titleBgCollapsed = bufferToColor("titleBgCollapsed"),
        buttonColor = bufferToColor("button"),
        buttonHoverColor = bufferToColor("buttonHover"),
        buttonActiveColor = bufferToColor("buttonActive"),
        separatorColor = bufferToColor("separator"),
        separatorHovered = bufferToColor("separatorHovered"),
        separatorActive = bufferToColor("separatorActive"),
        scrollbarBg = bufferToColor("scrollbarBg"),
        scrollbarGrab = bufferToColor("scrollbarGrab"),
        scrollbarGrabHovered = bufferToColor("scrollbarGrabHovered"),
        scrollbarGrabActive = bufferToColor("scrollbarGrabActive"),
        checkMark = bufferToColor("checkMark"),
        sliderGrab = bufferToColor("sliderGrab"),
        sliderGrabActive = bufferToColor("sliderGrabActive"),
        header = bufferToColor("header"),
        headerHovered = bufferToColor("headerHovered"),
        headerActive = bufferToColor("headerActive"),
        textColor = bufferToColor("text"),
        textDisabled = bufferToColor("textDisabled"),
        borderColor = bufferToColor("border")
    }
    
    themesFeature:updateCurrentThemeColors(colors)
end

function M.RenderThemeSelection(state, dataModule)
    if imgui.CollapsingHeader("Theme Selection", state.themeSelectionExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0) then
        state.themeSelectionExpanded = true
        
        local builtInNames = dataModule.GetBuiltInThemeNames() or {}
        local customThemes = dataModule.GetCustomThemes() or {}
        
        local allThemeNames = {}
        local themeIndices = {}
        
        for i, name in ipairs(builtInNames) do
            table.insert(allThemeNames, name)
            local themeIndex = i - 1
            table.insert(themeIndices, themeIndex)
        end
        
        for _, customTheme in ipairs(customThemes) do
            table.insert(allThemeNames, customTheme.name or "Unknown")
            table.insert(themeIndices, -1)
        end
        
        if #allThemeNames == 0 then
            imgui.Text("No themes available")
            return
        end
        
        local currentThemeIndex = dataModule.GetCurrentThemeIndex()
        local currentThemeName = dataModule.GetCurrentThemeName()
        local currentComboIndex = 0
        
        for i, themeIndex in ipairs(themeIndices) do
            if themeIndex == currentThemeIndex then
                if themeIndex == -1 then
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
        
        local previewText = allThemeNames[currentComboIndex + 1] or "Classic"
        if imgui.BeginCombo("Theme", previewText) then
            for i = 1, #allThemeNames do
                local isSelected = (i - 1 == currentComboIndex)
                local itemText = allThemeNames[i]
                if imgui.Selectable(itemText, isSelected) then
                    if not isSelected then
                        local selectedIndex = themeIndices[i]
                        local app = _G.FFXIFriendListApp
                        if app and app.features and app.features.themes then
                            if selectedIndex == -1 then
                                local themeName = allThemeNames[i]
                                app.features.themes:setCustomTheme(themeName)
                            else
                                app.features.themes:applyTheme(selectedIndex)
                            end
                        end
                    end
                end
                if isSelected then
                    imgui.SetItemDefaultFocus()
                end
            end
            imgui.EndCombo()
        end
        
        imgui.Spacing()
        
        imgui.Text("Background Transparency:")
        local bgAlpha = {state.themeBackgroundAlpha}
        if imgui.SliderFloat("##bgAlpha", bgAlpha, 0.0, 1.0, "%.2f") then
            state.themeBackgroundAlpha = bgAlpha[1]
            local app = _G.FFXIFriendListApp
            if app and app.features and app.features.themes then
                app.features.themes:setBackgroundAlpha(bgAlpha[1])
            end
        end
        
        imgui.Text("Text Transparency:")
        local textAlpha = {state.themeTextAlpha}
        if imgui.SliderFloat("##textAlpha", textAlpha, 0.0, 1.0, "%.2f") then
            state.themeTextAlpha = textAlpha[1]
            local app = _G.FFXIFriendListApp
            if app and app.features and app.features.themes then
                app.features.themes:setTextAlpha(textAlpha[1])
            end
        end
        
        imgui.Spacing()
        
        imgui.Text("Font Size:")
        local fontSizes = {14, 18, 24, 32}
        local fontSizeLabels = {"14px", "18px", "24px", "32px"}
        local currentFontSize = state.themeFontSizePx or 14
        local currentFontIndex = 0
        for i, size in ipairs(fontSizes) do
            if size == currentFontSize then
                currentFontIndex = i - 1
                break
            end
        end
        
        local previewLabel = fontSizeLabels[currentFontIndex + 1] or "14px"
        if imgui.BeginCombo("##fontSizePx", previewLabel) then
            for i, label in ipairs(fontSizeLabels) do
                local isSelected = (i - 1 == currentFontIndex)
                if imgui.Selectable(label, isSelected) then
                    state.themeFontSizePx = fontSizes[i]
                    local app = _G.FFXIFriendListApp
                    if app and app.features and app.features.themes then
                        app.features.themes:setFontSizePx(fontSizes[i])
                    end
                end
                if isSelected then
                    imgui.SetItemDefaultFocus()
                end
            end
            imgui.EndCombo()
        end
        
        imgui.Spacing()
        
        M.RenderCustomColors(state)
    else
        state.themeSelectionExpanded = false
    end
end

function M.RenderCustomColors(state)
    if imgui.CollapsingHeader("Custom Colors", state.customColorsExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0) then
        state.customColorsExpanded = true
        
        -- ImGui color edit flags (correct values from Dear ImGui)
        local ImGuiColorEditFlags_AlphaBar = 0x00010000         -- 1 << 16: Show alpha bar
        local ImGuiColorEditFlags_AlphaPreviewHalf = 0x00040000 -- 1 << 18: Preview with checkerboard
        local colorEditFlags = ImGuiColorEditFlags_AlphaBar + ImGuiColorEditFlags_AlphaPreviewHalf
        
        local function renderColorPicker(label, bufferKey)
            local color = state.themeColorBuffers[bufferKey]
            if imgui.ColorEdit4(label, color, colorEditFlags) then
                M.SyncThemeBuffersToColors(state)
            end
        end
        
        imgui.Text("Window Colors:")
        renderColorPicker("Window Background", "windowBg")
        renderColorPicker("Child Background", "childBg")
        renderColorPicker("Frame Background", "frameBg")
        renderColorPicker("Frame Hovered", "frameBgHovered")
        renderColorPicker("Frame Active", "frameBgActive")
        
        imgui.Spacing()
        
        imgui.Text("Title Colors:")
        renderColorPicker("Title Background", "titleBg")
        renderColorPicker("Title Active", "titleBgActive")
        renderColorPicker("Title Collapsed", "titleBgCollapsed")
        
        imgui.Spacing()
        
        imgui.Text("Button Colors:")
        renderColorPicker("Button", "button")
        renderColorPicker("Button Hovered", "buttonHover")
        renderColorPicker("Button Active", "buttonActive")
        
        imgui.Spacing()
        
        imgui.Text("Separator Colors:")
        renderColorPicker("Separator", "separator")
        renderColorPicker("Separator Hovered", "separatorHovered")
        renderColorPicker("Separator Active", "separatorActive")
        
        imgui.Spacing()
        
        imgui.Text("Scrollbar Colors:")
        renderColorPicker("Scrollbar Bg", "scrollbarBg")
        renderColorPicker("Scrollbar Grab", "scrollbarGrab")
        renderColorPicker("Scrollbar Grab Hovered", "scrollbarGrabHovered")
        renderColorPicker("Scrollbar Grab Active", "scrollbarGrabActive")
        
        imgui.Spacing()
        
        imgui.Text("Check/Slider Colors:")
        renderColorPicker("Check Mark", "checkMark")
        renderColorPicker("Slider Grab", "sliderGrab")
        renderColorPicker("Slider Grab Active", "sliderGrabActive")
        
        imgui.Spacing()
        
        imgui.Text("Header Colors:")
        renderColorPicker("Header", "header")
        renderColorPicker("Header Hovered", "headerHovered")
        renderColorPicker("Header Active", "headerActive")
        
        imgui.Spacing()
        
        imgui.Text("Text Colors:")
        renderColorPicker("Text", "text")
        renderColorPicker("Text Disabled", "textDisabled")
        
        imgui.Spacing()
        
        imgui.Text("Border Color:")
        renderColorPicker("Border", "border")
    else
        state.customColorsExpanded = false
    end
end

function M.RenderThemeManagement(state, dataModule)
    if imgui.CollapsingHeader("Theme Management", state.themeManagementExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0) then
        state.themeManagementExpanded = true
        
        imgui.Text("Save Current Colors as Theme")
        imgui.Text("Theme Name:")
        if imgui.InputText("##saveThemeName", state.themeNewThemeName, 256) then
        end
        
        local canSave = state.themeNewThemeName[1] and state.themeNewThemeName[1] ~= "" and #state.themeNewThemeName[1] > 0
        if not canSave then
            imgui.PushStyleColor(ImGuiCol_Button, {0.5, 0.5, 0.5, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.5, 0.5, 0.5, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.5, 0.5, 0.5, 1.0})
        end
        
        if imgui.Button("Save Custom Theme") then
            if canSave then
                M.SyncThemeBuffersToColors(state)
                local app = _G.FFXIFriendListApp
                if app and app.features and app.features.themes then
                    local colors = dataModule.GetCurrentThemeColors()
                    if colors then
                        colors.name = state.themeNewThemeName[1]
                        app.features.themes:saveCustomTheme(state.themeNewThemeName[1], colors)
                        state.themeNewThemeName[1] = ""
                    end
                end
            end
        end
        
        if not canSave then
            imgui.PopStyleColor(3)
        end
        
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

return M

