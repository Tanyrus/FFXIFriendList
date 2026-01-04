--[[
* QuickOnline Module - Display
* ImGui rendering for condensed friend list window (online friends sorted first)
]]

local imgui = require('imgui')
local ThemeHelper = require('libs.themehelper')
local icons = require('libs.icons')
local scaling = require('scaling')
local InputHelper = require('ui.helpers.InputHelper')
local HoverTooltip = require('ui.widgets.HoverTooltip')

-- Use same components as main friend list window
local FriendContextMenu = require('modules.friendlist.components.FriendContextMenu')
local FriendDetailsPopup = require('modules.friendlist.components.FriendDetailsPopup')

local M = {}

local function capitalizeName(name)
    if not name or name == "" then
        return "Unknown"
    end
    return string.upper(string.sub(name, 1, 1)) .. string.sub(name, 2)
end

-- Module state
local state = {
    initialized = false,
    hidden = false,
    -- NOTE: Window visibility is controlled by gConfig.showQuickOnline (single source of truth)
    selectedFriendForDetails = nil,
    locked = false,
    -- Friend table state (simplified - minimal columns)
    sortColumn = "name",
    sortDirection = "asc",
    filterText = {""},
    columnVisible = {
        name = true,
        job = true,
        zone = true,
        status = true
    },
    lastCloseKeyState = false,  -- For edge detection of close key
    -- Server selection state
    serverSelectionOpened = false,
    serverWindowNeedsCenter = false,
    serverListRefreshTriggered = false
}

-- Window ID (stable internal ID with ## pattern)
local WINDOW_ID = "##quickonline_main"

-- Initialize display module
function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    
    if gConfig and gConfig.windows and gConfig.windows.quickOnline then
        local windowState = gConfig.windows.quickOnline
        if windowState.locked ~= nil then
            state.locked = windowState.locked
        end
    end
    
    if gConfig then
        gConfig.showQuickOnline = false
    end
    
    -- Load quick online settings from config
    if gConfig and gConfig.quickOnlineSettings then
        local settings = gConfig.quickOnlineSettings
        if settings.sort then
            if settings.sort.column then
                state.sortColumn = settings.sort.column
            end
            if settings.sort.direction then
                state.sortDirection = settings.sort.direction
            end
        end
        if settings.filter then
            state.filterText[1] = settings.filter or ""
        end
        if settings.columns then
            if settings.columns.name ~= nil then
                state.columnVisible.name = settings.columns.name
            end
            if settings.columns.job ~= nil then
                state.columnVisible.job = settings.columns.job
            end
            if settings.columns.zone ~= nil then
                state.columnVisible.zone = settings.columns.zone
            end
            if settings.columns.status ~= nil then
                state.columnVisible.status = settings.columns.status
            end
        end
    end
end

-- Update visuals (called when settings change)
function M.UpdateVisuals(settings)
    -- No special visual updates needed
end

-- Set hidden state
function M.SetHidden(hidden)
    state.hidden = hidden or false
end

-- Check if window is visible (for close policy)
function M.IsVisible()
    return gConfig and gConfig.showQuickOnline or false
end

-- Set window visibility (for close policy)
function M.SetVisible(visible)
    if gConfig then
        local newValue = visible or false
        if gConfig.showQuickOnline ~= newValue then
            gConfig.showQuickOnline = newValue
            M.SaveWindowState()
        end
    end
end

-- Main render function
function M.DrawWindow(settings, dataModule)
    -- Don't draw if window is closed (gConfig.showQuickOnline is the single source of truth)
    if not state.initialized or state.hidden or not gConfig or not gConfig.showQuickOnline then
        return
    end
    
    if not dataModule then
        return
    end
    
    -- Check if server selection is needed BEFORE rendering main window
    local app = _G.FFXIFriendListApp
    local needsServerSelection = false
    if app and app.features and app.features.serverlist then
        needsServerSelection = not app.features.serverlist:isServerSelected()
    end
    
    -- If server selection needed, only show server selection window (not main window)
    if needsServerSelection then
        if not state.serverSelectionOpened then
            state.serverSelectionOpened = true
            state.serverWindowNeedsCenter = true
            state.serverListRefreshTriggered = false
        end
        M.RenderServerSelectionWindow()
        return
    end
    
    -- Server is configured - render main window normally
    local windowFlags = 0
    if state.locked then
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoMove)
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoResize)
    end
    
    if gConfig and gConfig.windows and gConfig.windows.quickOnline then
        local windowState = gConfig.windows.quickOnline
        if windowState.forcePosition and windowState.posX and windowState.posY then
            imgui.SetNextWindowPos({windowState.posX, windowState.posY}, ImGuiCond_Always)
            windowState.forcePosition = false
        elseif windowState.posX and windowState.posY then
            imgui.SetNextWindowPos({windowState.posX, windowState.posY}, ImGuiCond_FirstUseEver)
        end
        if windowState.sizeX and windowState.sizeY then
            imgui.SetNextWindowSize({windowState.sizeX, windowState.sizeY}, ImGuiCond_FirstUseEver)
        else
            imgui.SetNextWindowSize({420, 320}, ImGuiCond_FirstUseEver)
        end
    else
        imgui.SetNextWindowSize({420, 320}, ImGuiCond_FirstUseEver)
    end
    
    -- Window title
    local windowTitle = "Compact Friend List" .. WINDOW_ID
    
    -- Update locked state from config
    if gConfig and gConfig.windows and gConfig.windows.quickOnline then
        state.locked = gConfig.windows.quickOnline.locked or false
    end
    
    -- NOTE: ESC/close key handling is now centralized in ui/input/close_input.lua
    -- This matches C++ handleEscapeKey() being in AshitaAdapter, not per-window
    
    -- Apply theme styles (only if not default theme)
    local themePushed = false
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
                    app.deps.logger.error("[QuickOnline Display] Theme application failed: " .. tostring(err))
                else
                    print("[FFXIFriendList ERROR] QuickOnline Display: Theme application failed: " .. tostring(err))
                end
            end
        end
    end
    
    -- Pass windowOpen as a table so ImGui can modify it when X button is clicked
    -- gConfig.showQuickOnline is the source of truth
    local isOpen = gConfig and gConfig.showQuickOnline or false
    local windowOpenTable = {isOpen}
    local windowOpen = imgui.Begin(windowTitle, windowOpenTable, windowFlags)
    
    -- Detect if X button was clicked (ImGui sets windowOpenTable[1] to false)
    local xButtonClicked = isOpen and not windowOpenTable[1]
    
    -- Handle X button close attempt
    if xButtonClicked then
        -- Block close if locked (C++ parity: locked windows don't close via X)
        if state.locked then
            -- Keep window open - locked windows can't be closed via X
            -- (do nothing, gConfig.showQuickOnline stays true)
        else
            local closeGating = require('ui.close_gating')
            if not closeGating.shouldDeferClose() then
                -- Close the window
                if gConfig then
                    gConfig.showQuickOnline = false
                end
                M.SaveWindowState()
            end
            -- If popup gating defers close, do nothing (window stays open)
        end
    end
    
    -- Update window state based on Begin return value
    if windowOpen then
        -- Window is open - render content
        -- Save window state periodically (position, size)
        M.SaveWindowState()
        
        -- Render top bar
        M.RenderTopBar(dataModule)
        
        imgui.Spacing()
        
        -- Friend table (online friends only)
        imgui.BeginChild("##quick_online_body", {0, 0}, false)
        M.RenderFriendsTable(dataModule)
        imgui.EndChild()
    end
    
    imgui.End()
    
    -- Pop theme styles if we pushed them
    if themePushed then
        local success, err = pcall(ThemeHelper.popThemeStyles)
        if not success then
            local app = _G.FFXIFriendListApp
            if app and app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[QuickOnline Display] Theme pop failed: " .. tostring(err))
            else
                print("[FFXIFriendList ERROR] QuickOnline Display: Theme pop failed: " .. tostring(err))
            end
        end
    end
    
    -- Render friend details popup if needed (use shared component from main window)
    if state.selectedFriendForDetails then
        FriendDetailsPopup.Render(state.selectedFriendForDetails, state, M.GetCallbacks(dataModule))
    end
end

-- Render top bar (lock button, refresh button)
function M.RenderTopBar(dataModule)
    local isConnected = dataModule.IsConnected()
    
    -- Increase button frame padding and add rounding for all buttons
    imgui.PushStyleVar(ImGuiStyleVar_FramePadding, {6, 6})
    imgui.PushStyleVar(ImGuiStyleVar_FrameRounding, 4)
    
    -- Lock button (first)
    local lockIconName = state.locked and "lock" or "unlock"
    local lockTooltip = state.locked and "Window locked (click to unlock)" or "Lock window position"
    
    local clicked = icons.RenderIconButton(lockIconName, 21, 21, lockTooltip)
    if clicked == nil then
        local lockLabel = state.locked and "Locked" or "Unlocked"
        clicked = imgui.Button(lockLabel .. "##lock_btn")
        if imgui.IsItemHovered() then
            imgui.SetTooltip(lockTooltip)
        end
    end
    
    if clicked then
        state.locked = not state.locked
        if gConfig then
            if not gConfig.windows then
                gConfig.windows = {}
            end
            if not gConfig.windows.quickOnline then
                gConfig.windows.quickOnline = {}
            end
            gConfig.windows.quickOnline.locked = state.locked
        end
    end
    
    imgui.SameLine(0, 8)
    
    -- Refresh button
    if not isConnected then
        imgui.PushStyleColor(ImGuiCol_Button, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.3, 0.3, 0.3, 1.0})
    end
    
    if imgui.Button("Refresh") then
        if isConnected then
            local app = _G.FFXIFriendListApp
            if app and app.features and app.features.friends then
                app.features.friends:refresh()
            end
        end
    end
    
    if not isConnected then
        imgui.PopStyleColor(3)
    end
    
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Refresh friend list")
    end
    
    imgui.SameLine(0, 8)
    
    if imgui.Button("Full") then
        if gConfig then
            local posX, posY = imgui.GetWindowPos()
            
            if not gConfig.windows then gConfig.windows = {} end
            if not gConfig.windows.friendList then gConfig.windows.friendList = {} end
            gConfig.windows.friendList.posX = posX
            gConfig.windows.friendList.posY = posY
            gConfig.windows.friendList.forcePosition = true
            
            gConfig.showQuickOnline = false
            gConfig.showFriendList = true
            M.SaveWindowState()
            local settings = require('libs.settings')
            if settings and settings.save then
                settings.save()
            end
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Switch to full view (Main Window)")
    end
    
    -- Pop button frame padding and rounding
    imgui.PopStyleVar(2)
end

-- Render friends table (Name-only, hover for details)
function M.RenderFriendsTable(dataModule)
    local friends = dataModule.GetOnlineFriends()
    
    -- Apply filter
    local filteredFriends = {}
    local filterText = string.lower(state.filterText[1] or "")
    if filterText == "" then
        filteredFriends = friends
    else
        for _, friend in ipairs(friends) do
            local friendName = type(friend.name) == "string" and string.lower(friend.name) or ""
            local presence = friend.presence or {}
            local job = type(presence.job) == "string" and string.lower(presence.job) or ""
            local zone = type(presence.zone) == "string" and string.lower(presence.zone) or ""
            if string.find(friendName, filterText, 1, true) or
               string.find(job, filterText, 1, true) or
               string.find(zone, filterText, 1, true) then
                table.insert(filteredFriends, friend)
            end
        end
    end
    
    -- Apply sorting (online first, then by name)
    table.sort(filteredFriends, function(a, b)
        local aOnline = a.isOnline == true
        local bOnline = b.isOnline == true
        
        if aOnline ~= bOnline then
            return aOnline
        end
        
        local aVal = type(a.name) == "string" and string.lower(a.name) or ""
        local bVal = type(b.name) == "string" and string.lower(b.name) or ""
        
        if state.sortDirection == "asc" then
            return aVal < bVal
        else
            return aVal > bVal
        end
    end)
    
    if #filteredFriends == 0 then
        imgui.Text("No friends" .. (filterText ~= "" and " (filtered)" or ""))
        return
    end
    
    -- Get hover tooltip settings
    local app = _G.FFXIFriendListApp
    local hoverSettings = nil
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        hoverSettings = prefs and prefs.quickOnlineHoverTooltip
    end
    
    -- Name-only table (no column options, hover for details)
    if imgui.BeginTable("##quick_online_table", 1, 0) then
        imgui.TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch)
        
        for i, friend in ipairs(filteredFriends) do
            imgui.TableNextRow()
            imgui.TableNextColumn()
            
            local friendName = capitalizeName(friend.name or "Unknown")
            local isOnline = friend.isOnline == true
            
            if not icons.RenderStatusIcon(isOnline, false, 12) then
                if isOnline then
                    imgui.TextColored({0.0, 1.0, 0.0, 1.0}, "O")
                else
                    imgui.TextColored({0.5, 0.5, 0.5, 1.0}, "O")
                end
            end
            imgui.SameLine()
            
            if not isOnline then
                imgui.PushStyleColor(ImGuiCol_Text, {0.6, 0.6, 0.6, 1.0})
            end
            
            if imgui.Selectable(friendName .. "##friend_" .. i, false, ImGuiSelectableFlags_SpanAllColumns) then
                if state.selectedFriendForDetails and state.selectedFriendForDetails.name == friend.name then
                    state.selectedFriendForDetails = nil
                else
                    state.selectedFriendForDetails = friend
                end
            end
            
            local itemHovered = imgui.IsItemHovered()
            
            if not isOnline then
                imgui.PopStyleColor()
            end
            
            if itemHovered then
                local forceAll = InputHelper.isShiftDown()
                HoverTooltip.Render(friend, hoverSettings, forceAll)
            end
            
            if imgui.BeginPopupContextItem("##friend_context_" .. i) then
                FriendContextMenu.Render(friend, state, M.GetCallbacks(dataModule))
                imgui.EndPopup()
            end
        end
        
        imgui.EndTable()
    end
end

-- Get callbacks for shared components (matches main window pattern)
function M.GetCallbacks(dataModule)
    local app = _G.FFXIFriendListApp
    
    return {
        onRefresh = function()
            if app and app.features and app.features.friends then
                app.features.friends:refresh()
            end
        end,
        
        onRemoveFriend = function(friendName)
            if app and app.features and app.features.friends then
                app.features.friends:removeFriend(friendName)
            end
        end,
        
        onOpenNoteEditor = function(friendName)
            local noteEditorModule = require('modules.noteeditor')
            if noteEditorModule and noteEditorModule.OpenEditor then
                noteEditorModule.OpenEditor(friendName)
            end
        end,
        
        onDeleteNote = function(friendName)
            -- Note deletion handled by note editor or friend details
        end,
        
        onSaveState = M.SaveWindowState,
    }
end

-- Save window state
function M.SaveWindowState()
    if not gConfig then
        return
    end
    
    if not gConfig.windows then
        gConfig.windows = {}
    end
    if not gConfig.windows.quickOnline then
        gConfig.windows.quickOnline = {}
    end
    
    local windowState = gConfig.windows.quickOnline
    
    -- Save visibility (from the single source of truth)
    windowState.visible = gConfig.showQuickOnline or false
    
    -- Save position and size if window is open
    if gConfig.showQuickOnline then
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
    
    windowState.locked = state.locked
    
    -- Save quick online settings
    if not gConfig.quickOnlineSettings then
        gConfig.quickOnlineSettings = {}
    end
    
    local settings = gConfig.quickOnlineSettings
    settings.sort = {
        column = state.sortColumn,
        direction = state.sortDirection
    }
    settings.filter = state.filterText[1] or ""
    settings.columns = {
        name = state.columnVisible.name,
        job = state.columnVisible.job,
        zone = state.columnVisible.zone,
        status = state.columnVisible.status
    }
    
    -- Persist to disk
    local settings = require('libs.settings')
    if settings and settings.save then
        settings.save()
    end
end

-- Render server selection window (separate from main window)
function M.RenderServerSelectionWindow()
    local serverSelectionData = require('modules.serverselection.data')
    
    -- Ensure data module is initialized
    if not serverSelectionData.GetServers or #(serverSelectionData.GetServers() or {}) == 0 then
        if serverSelectionData.Initialize then
            serverSelectionData.Initialize({})
        end
    end
    
    if serverSelectionData.Update then
        serverSelectionData.Update()
    end
    
    local screenWidth = scaling.window.w
    local screenHeight = scaling.window.h
    
    local windowFlags = bit.bor(
        ImGuiWindowFlags_NoCollapse,
        ImGuiWindowFlags_NoScrollbar,
        ImGuiWindowFlags_AlwaysAutoResize
    )
    
    -- Center on screen when first opened
    if state.serverWindowNeedsCenter then
        local windowWidth = 350
        local windowHeight = 300
        local posX = (screenWidth - windowWidth) / 2
        local posY = (screenHeight - windowHeight) / 2
        imgui.SetNextWindowPos({posX, posY}, ImGuiCond_Always)
        state.serverWindowNeedsCenter = false
    end
    
    imgui.SetNextWindowSize({350, 0}, ImGuiCond_FirstUseEver)
    
    -- Apply theme
    local themePushed = false
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.themes then
        local success, err = pcall(function()
            local themesFeature = app.features.themes
            local themeIndex = themesFeature:getThemeIndex()
            if themeIndex ~= -2 then
                local themeColors = themesFeature:getCurrentThemeColors()
                local backgroundAlpha = themesFeature:getBackgroundAlpha()
                local textAlpha = themesFeature:getTextAlpha()
                if themeColors then
                    themePushed = ThemeHelper.pushThemeStyles(themeColors, backgroundAlpha, textAlpha)
                end
            end
        end)
    end
    
    local windowOpenTable = {true}
    if not imgui.Begin("Select Server##serverselection_qo", windowOpenTable, windowFlags) then
        imgui.End()
        if themePushed then ThemeHelper.popThemeStyles() end
        return
    end
    
    if not windowOpenTable[1] then
        state.serverSelectionOpened = false
        imgui.End()
        if themePushed then ThemeHelper.popThemeStyles() end
        return
    end
    
    -- Apply button rounding for this popup
    imgui.PushStyleVar(ImGuiStyleVar_FrameRounding, 4)
    
    imgui.TextWrapped("Please select your server to connect to the FFXIFriendList service.")
    imgui.Separator()
    imgui.TextColored({1.0, 0.8, 0.0, 1.0}, "Warning:")
    imgui.TextWrapped("If you select the wrong server, you may not be able to find your friends.")
    imgui.Separator()
    
    local servers = serverSelectionData.GetServers() or {}
    local draftServerId = serverSelectionData.GetDraftSelectedServerId() or ""
    local detectedServerId = serverSelectionData.GetDetectedServerId()
    local detectedServerName = serverSelectionData.GetDetectedServerName()
    
    if #servers == 0 and not state.serverListRefreshTriggered then
        serverSelectionData.RefreshServerList()
        state.serverListRefreshTriggered = true
    end
    
    if detectedServerId and detectedServerName then
        imgui.Text("Detected server: " .. detectedServerName)
        imgui.Spacing()
    end
    
    if #servers == 0 then
        imgui.Text("Loading servers...")
        if imgui.Button("Refresh") then
            serverSelectionData.RefreshServerList()
        end
    else
        local previewText = "Select a server..."
        for _, server in ipairs(servers) do
            if server.id == draftServerId then
                previewText = server.name or server.id
                break
            end
        end
        
        if imgui.BeginCombo("Server", previewText) then
            for _, server in ipairs(servers) do
                local isSelected = (server.id == draftServerId)
                if detectedServerId and server.id == detectedServerId then
                    imgui.PushStyleColor(ImGuiCol_Text, {0.0, 1.0, 0.0, 1.0})
                end
                if imgui.Selectable(server.name or server.id, isSelected) then
                    serverSelectionData.SetDraftSelectedServerId(server.id)
                end
                if detectedServerId and server.id == detectedServerId then
                    imgui.PopStyleColor()
                end
                if isSelected then
                    imgui.SetItemDefaultFocus()
                end
            end
            imgui.EndCombo()
        end
    end
    
    imgui.Separator()
    
    local canSave = draftServerId ~= "" and draftServerId ~= nil
    if canSave then
        if imgui.Button("Save", {120, 0}) then
            serverSelectionData.SaveServerSelection(draftServerId)
            state.serverSelectionOpened = false
        end
    else
        imgui.PushStyleColor(ImGuiCol_Button, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_Text, {0.5, 0.5, 0.5, 1.0})
        imgui.Button("Save", {120, 0})
        imgui.PopStyleColor(4)
    end
    
    imgui.SameLine()
    
    if imgui.Button("Close", {120, 0}) then
        state.serverSelectionOpened = false
    end
    
    -- Pop button rounding
    imgui.PopStyleVar()
    
    imgui.End()
    if themePushed then ThemeHelper.popThemeStyles() end
end

-- Cleanup
function M.Cleanup()
    state.initialized = false
end

return M

