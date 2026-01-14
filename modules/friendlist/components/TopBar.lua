local imgui = require('imgui')
local icons = require('libs.icons')
local FontManager = require('app.ui.FontManager')

local M = {}

local URL_DISCORD = "https://discord.gg/JXsx7zf3Dx"
local URL_GITHUB = "https://github.com/Tanyrus/FFXIFriendList"

local function openUrl(url)
    if os.execute then
        local cmd
        if package.config:sub(1, 1) == "\\" then
            cmd = 'start "" "' .. url .. '"'
        else
            cmd = 'xdg-open "' .. url .. '" 2>/dev/null || open "' .. url .. '"'
        end
        os.execute(cmd)
    end
end

function M.Render(state, dataModule, onRefresh, onLockChanged, onViewToggle, showAboutPopup)
    local isConnected = dataModule.IsConnected()
    local s = FontManager.scaled
    
    imgui.Dummy({0, s(3)})
    
    imgui.PushStyleVar(ImGuiStyleVar_FramePadding, {s(6), s(6)})
    imgui.PushStyleVar(ImGuiStyleVar_FrameRounding, s(4))
    
    local lockIconName = state.locked and "lock" or "unlock"
    local lockTooltip = state.locked and "Window locked (click to unlock)" or "Lock window position"
    
    local lockIconSize = s(21)
    local clicked = icons.RenderIconButton(lockIconName, lockIconSize, lockIconSize, lockTooltip)
    if clicked == nil then
        local lockLabel = state.locked and "Locked" or "Unlocked"
        clicked = imgui.Button(lockLabel .. "##lock_btn")
        if imgui.IsItemHovered() then
            imgui.SetTooltip(lockTooltip)
        end
    end
    
    if clicked then
        state.locked = not state.locked
        if onLockChanged then
            onLockChanged(state.locked)
        end
    end
    
    imgui.SameLine(0, s(8))
    
    if not isConnected then
        imgui.PushStyleColor(ImGuiCol_Button, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.3, 0.3, 0.3, 1.0})
    end
    
    -- Refresh button - always use icon
    local refreshIconSize = s(21)
    local refreshClicked = icons.RenderIconButton("refresh", refreshIconSize, refreshIconSize, "Refresh")
    if refreshClicked == nil then
        -- Fallback to text button
        refreshClicked = imgui.Button("Refresh")
    end
    
    if refreshClicked then
        if isConnected and onRefresh then
            onRefresh()
        end
    end
    
    if not isConnected then
        imgui.PopStyleColor(3)
    end
    
    imgui.SameLine(0, s(8))
    
    -- Condensed button - always use collapse icon
    local collapseIconSize = s(21)
    local collapseClicked = icons.RenderIconButton("collapse", collapseIconSize, collapseIconSize, "Switch to condensed view (Compact Friend List)")
    if collapseClicked == nil then
        -- Fallback to text button
        collapseClicked = imgui.Button("Condensed")
        if imgui.IsItemHovered() then
            imgui.SetTooltip("Switch to condensed view (Compact Friend List)")
        end
    end
    
    if collapseClicked then
        if onViewToggle then
            onViewToggle()
        end
    end
    
    local iconSize = s(24)
    local iconSpacing = s(4)
    local buttonPadding = s(4)
    local iconButtonSize = iconSize + buttonPadding
    
    local socialIconsWidth = (iconButtonSize * 3) + (iconSpacing * 2)
    
    imgui.SameLine(0, 0)
    local availWidth, _ = imgui.GetContentRegionAvail()
    if type(availWidth) == "number" and availWidth > socialIconsWidth then
        imgui.Dummy({availWidth - socialIconsWidth, 1})
        imgui.SameLine(0, 0)
    else
        imgui.SameLine(0, s(16))
    end
    
    local discordClicked = icons.RenderIconButton("discord", iconSize, iconSize, "Discord")
    if discordClicked == nil then
        -- Fallback to text button if icon not loaded
        if imgui.Button("Discord##social") then
            openUrl(URL_DISCORD)
        end
        if imgui.IsItemHovered() then imgui.SetTooltip("Discord") end
    elseif discordClicked then
        openUrl(URL_DISCORD)
    end
    
    imgui.SameLine(0, iconSpacing)
    
    -- GitHub button
    local githubClicked = icons.RenderIconButton("github", iconSize, iconSize, "GitHub")
    if githubClicked == nil then
        if imgui.Button("GitHub##social") then
            openUrl(URL_GITHUB)
        end
        if imgui.IsItemHovered() then imgui.SetTooltip("GitHub") end
    elseif githubClicked then
        openUrl(URL_GITHUB)
    end
    
    imgui.SameLine(0, iconSpacing)
    
    -- About/Heart button
    local heartClicked = icons.RenderIconButton("heart", iconSize, iconSize, "About / Special Thanks")
    if heartClicked == nil then
        if imgui.Button("About##social") then
            if showAboutPopup then showAboutPopup() end
        end
        if imgui.IsItemHovered() then imgui.SetTooltip("About / Special Thanks") end
    elseif heartClicked then
        if showAboutPopup then showAboutPopup() end
    end
    
    imgui.PopStyleVar(2)
    
    imgui.Dummy({0, s(3)})
end

return M
