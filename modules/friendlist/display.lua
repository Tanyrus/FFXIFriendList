local imgui = require('imgui')
local ThemeHelper = require('libs.themehelper')
local UIConstants = require('core.UIConstants')
local scaling = require('scaling')

local Sidebar = require('modules.friendlist.components.Sidebar')
local TopBar = require('modules.friendlist.components.TopBar')
local AddFriendSection = require('modules.friendlist.components.AddFriendSection')
local PendingRequests = require('modules.friendlist.components.PendingRequests')
local FriendsTable = require('modules.friendlist.components.FriendsTable')
local FriendContextMenu = require('modules.friendlist.components.FriendContextMenu')
local FriendDetailsPopup = require('modules.friendlist.components.FriendDetailsPopup')
local PrivacyTab = require('modules.friendlist.components.PrivacyTab')
local NotificationsTab = require('modules.friendlist.components.NotificationsTab')
local ControlsTab = require('modules.friendlist.components.ControlsTab')
local ThemesTab = require('modules.friendlist.components.ThemesTab')
local utils = require('modules.friendlist.components.helpers.utils')

local M = {}

local state = {
    initialized = false,
    hidden = false,
    selectedFriendForDetails = nil,
    newFriendName = {""},
    newFriendNote = {""},
    selectedTab = 0,
    locked = false,
    lastCloseKeyState = false,
    pendingRequestsExpanded = true,
    friendViewSettingsExpanded = true,
    hoverTooltipSettingsExpanded = false,
    privacyControlsExpanded = false,
    altVisibilityExpanded = false,
    notificationsSettingsExpanded = true,
    controlsSettingsExpanded = true,
    themeSettingsExpanded = true,
    themeSelectionExpanded = true,
    customColorsExpanded = false,
    themeManagementExpanded = false,
    showAboutPopup = false,
    aboutPopupJustOpened = false,
    menuDetectionExpanded = true,
    showServerSelectionPopup = false,
    serverSelectionPopupOpened = false,
    serverListRefreshTriggered = false,
    serverWindowNeedsCenter = false,
    themeColorBuffers = {
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
        textDisabled = {1.0, 1.0, 1.0, 1.0},
        border = {1.0, 1.0, 1.0, 1.0}
    },
    themeBackgroundAlpha = 0.95,
    themeTextAlpha = 1.0,
    themeNewThemeName = {""},
    themeLastThemeIndex = -999,
    sortColumn = "name",
    sortDirection = "asc",
    filterText = {""},
    columnVisible = {
        name = true,
        job = true,
        zone = false,
        nationRank = false,
        lastSeen = false,
        addedAs = false
    }
}

local WINDOW_ID = "##friendlist_main"

function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    
    if gConfig and gConfig.windows and gConfig.windows.friendList then
        local windowState = gConfig.windows.friendList
        if windowState.visible ~= nil and gConfig then
            gConfig.showFriendList = windowState.visible
        end
        if windowState.locked ~= nil then
            state.locked = windowState.locked
        end
    end
    
    if gConfig and gConfig.showFriendList == nil then
        gConfig.showFriendList = false
    end
    
    if gConfig and gConfig.friendListSettings then
        local settings = gConfig.friendListSettings
        if settings.selectedTab ~= nil then
            state.selectedTab = settings.selectedTab
        end
        if settings.sections then
            if settings.sections.pendingRequests and settings.sections.pendingRequests.expanded ~= nil then
                state.pendingRequestsExpanded = settings.sections.pendingRequests.expanded
            end
            if settings.sections.friendViewSettings and settings.sections.friendViewSettings.expanded ~= nil then
                state.friendViewSettingsExpanded = settings.sections.friendViewSettings.expanded
            end
            if settings.sections.privacyControls and settings.sections.privacyControls.expanded ~= nil then
                state.privacyControlsExpanded = settings.sections.privacyControls.expanded
            end
            if settings.sections.altVisibility and settings.sections.altVisibility.expanded ~= nil then
                state.altVisibilityExpanded = settings.sections.altVisibility.expanded
            end
            if settings.sections.notificationsSettings and settings.sections.notificationsSettings.expanded ~= nil then
                state.notificationsSettingsExpanded = settings.sections.notificationsSettings.expanded
            end
            if settings.sections.controlsSettings and settings.sections.controlsSettings.expanded ~= nil then
                state.controlsSettingsExpanded = settings.sections.controlsSettings.expanded
            end
            if settings.sections.themeSettings and settings.sections.themeSettings.expanded ~= nil then
                state.themeSettingsExpanded = settings.sections.themeSettings.expanded
            end
        end
        if settings.sort then
            if settings.sort.column then
                state.sortColumn = settings.sort.column
            end
            if settings.sort.direction then
                state.sortDirection = settings.sort.direction
            end
        end
        if settings.filter then
            state.filterText[1] = settings.filter
        end
        if settings.columns then
            for colName, colState in pairs(settings.columns) do
                if colState.visible ~= nil then
                    state.columnVisible[colName] = colState.visible
                end
            end
        end
    end
end

function M.UpdateVisuals(settings)
    state.settings = settings or {}
end

function M.SetHidden(hidden)
    state.hidden = hidden or false
end

function M.IsVisible()
    return gConfig and gConfig.showFriendList or false
end

function M.SetVisible(visible)
    if gConfig then
        local newValue = visible or false
        if gConfig.showFriendList ~= newValue then
            gConfig.showFriendList = newValue
            M.SaveWindowState()
        end
    end
end

function M.SaveWindowState()
    if not gConfig then return end
    
    if not gConfig.windows then
        gConfig.windows = {}
    end
    if not gConfig.windows.friendList then
        gConfig.windows.friendList = {}
    end
    
    gConfig.windows.friendList.visible = gConfig.showFriendList
    gConfig.windows.friendList.locked = state.locked
    
    if not gConfig.friendListSettings then
        gConfig.friendListSettings = {}
    end
    
    gConfig.friendListSettings.selectedTab = state.selectedTab
    gConfig.friendListSettings.sections = {
        pendingRequests = {expanded = state.pendingRequestsExpanded},
        friendViewSettings = {expanded = state.friendViewSettingsExpanded},
        privacyControls = {expanded = state.privacyControlsExpanded},
        altVisibility = {expanded = state.altVisibilityExpanded},
        notificationsSettings = {expanded = state.notificationsSettingsExpanded},
        controlsSettings = {expanded = state.controlsSettingsExpanded},
        themeSettings = {expanded = state.themeSettingsExpanded}
    }
    gConfig.friendListSettings.sort = {
        column = state.sortColumn,
        direction = state.sortDirection
    }
    gConfig.friendListSettings.filter = state.filterText[1]
    gConfig.friendListSettings.columns = {}
    for colName, visible in pairs(state.columnVisible) do
        gConfig.friendListSettings.columns[colName] = {visible = visible}
    end
end

function M.GetWindowTitle(dataModule)
    local title = "FFXIFriendList"
    
    if AshitaCore then
        local memoryMgr = AshitaCore:GetMemoryManager()
        if memoryMgr then
            local party = memoryMgr:GetParty()
            if party then
                local playerName = party:GetMemberName(0)
                if playerName and playerName ~= "" then
                    title = title .. " - " .. utils.capitalizeName(playerName)
                end
            end
        end
    end
    
    return title
end

function M.DrawWindow(settings, dataModule)
    if not state.initialized or state.hidden then
        return
    end
    
    if not gConfig or not gConfig.showFriendList then
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
        if not state.serverSelectionPopupOpened then
            state.serverSelectionPopupOpened = true
            state.serverWindowNeedsCenter = true
            state.serverListRefreshTriggered = false
        end
        M.RenderServerSelectionWindow()
        return
    end
    
    -- Server is configured - render main window normally
    local windowFlags = 0
    if gConfig and gConfig.windows and gConfig.windows.friendList and gConfig.windows.friendList.locked then
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoMove)
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoResize)
    end
    
    if gConfig and gConfig.windows and gConfig.windows.friendList then
        local windowState = gConfig.windows.friendList
        if windowState.forcePosition and windowState.posX and windowState.posY then
            imgui.SetNextWindowPos({windowState.posX, windowState.posY}, ImGuiCond_Always)
            windowState.forcePosition = false
        elseif windowState.posX and windowState.posY then
            imgui.SetNextWindowPos({windowState.posX, windowState.posY}, ImGuiCond_FirstUseEver)
        end
        if windowState.sizeX and windowState.sizeY then
            imgui.SetNextWindowSize({windowState.sizeX, windowState.sizeY}, ImGuiCond_FirstUseEver)
        end
    end
    
    local windowTitle = M.GetWindowTitle(dataModule) .. WINDOW_ID
    
    if gConfig and gConfig.windows and gConfig.windows.friendList then
        state.locked = gConfig.windows.friendList.locked or false
    end
    
    local themePushed = M.ApplyTheme()
    
    local isOpen = gConfig and gConfig.showFriendList or false
    local windowOpenTable = {isOpen}
    local windowOpen = imgui.Begin(windowTitle, windowOpenTable, windowFlags)
    
    local xButtonClicked = isOpen and not windowOpenTable[1]
    
    if xButtonClicked then
        if state.locked then
        else
            local closeGating = require('ui.close_gating')
            if not closeGating.shouldDeferClose() then
                if gConfig then
                    gConfig.showFriendList = false
                end
                M.SaveWindowState()
            end
        end
    end
    
    if windowOpen then
        M.SaveWindowState()
        M.RenderContent(dataModule)
    end
    
    imgui.End()
    
    M.PopTheme(themePushed)
    
    if state.selectedFriendForDetails then
        FriendDetailsPopup.Render(state.selectedFriendForDetails, state, M.GetCallbacks(dataModule))
    end
end

function M.ApplyTheme()
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
        if not success then
            if app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[FriendList Display] Theme application failed: " .. tostring(err))
            end
        end
    end
    return themePushed
end

function M.PopTheme(themePushed)
    if themePushed then
        local success, err = pcall(ThemeHelper.popThemeStyles)
        if not success then
            local app = _G.FFXIFriendListApp
            if app and app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[FriendList Display] Theme pop failed: " .. tostring(err))
            end
        end
    end
end

function M.RenderContent(dataModule)
    local callbacks = M.GetCallbacks(dataModule)
    
    local onViewToggle = function()
        if gConfig then
            local posX, posY = imgui.GetWindowPos()
            
            if not gConfig.windows then gConfig.windows = {} end
            if not gConfig.windows.quickOnline then gConfig.windows.quickOnline = {} end
            gConfig.windows.quickOnline.posX = posX
            gConfig.windows.quickOnline.posY = posY
            gConfig.windows.quickOnline.forcePosition = true
            
            gConfig.showFriendList = false
            gConfig.showQuickOnline = true
            M.SaveWindowState()
            local settings = require('libs.settings')
            if settings and settings.save then
                settings.save()
            end
        end
    end
    
    local showAboutPopup = function()
        state.showAboutPopup = true
        state.aboutPopupJustOpened = true
    end
    
    TopBar.Render(state, dataModule, callbacks.onRefresh, callbacks.onLockChanged, onViewToggle, showAboutPopup)
    
    M.RenderAboutPopup()
    
    imgui.Spacing()
    
    imgui.BeginChild("Sidebar", {UIConstants.SIDEBAR_WIDTH, 0}, false)
    Sidebar.Render(state, nil, M.SaveWindowState)
    imgui.EndChild()
    
    imgui.SameLine()
    
    imgui.BeginChild("ContentArea", {0, 0}, false)
    M.RenderContentArea(dataModule, callbacks)
    imgui.EndChild()
end

function M.RenderContentArea(dataModule, callbacks)
    if state.selectedTab == 0 then
        M.RenderFriendsTab(dataModule, callbacks)
    elseif state.selectedTab == 1 then
        PrivacyTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 2 then
        NotificationsTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 3 then
        ControlsTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 4 then
        ThemesTab.Render(state, dataModule, callbacks)
    end
end

function M.RenderFriendsTab(dataModule, callbacks)
    AddFriendSection.Render(state, dataModule, callbacks.onAddFriend)
    
    -- Padding above Pending Requests
    imgui.Dummy({0, 6})
    
    PendingRequests.Render(state, dataModule, callbacks)
    
    -- Padding below Pending Requests
    imgui.Dummy({0, 6})
    
    imgui.BeginChild("##friends_table_child", {0, 0}, false)
    FriendsTable.Render(state, dataModule, callbacks)
    imgui.EndChild()
end

function M.RenderAboutPopup()
    if state.aboutPopupJustOpened then
        imgui.OpenPopup("##about_popup")
        state.aboutPopupJustOpened = false
    end
    
    if imgui.BeginPopup("##about_popup") then
        imgui.Text("FFXIFriendList")
        
        local version = "v1.0.0"
        local app = _G.FFXIFriendListApp
        if app and app.version then
            version = app.version
        end
        imgui.Text("Version: " .. version)
        
        imgui.Separator()
        
        imgui.Text("Special Thanks")
        imgui.BulletText("Ashita Team - Plugin framework")
        
        imgui.Separator()
        
        if imgui.Button("Close##about") then
            state.showAboutPopup = false
            imgui.CloseCurrentPopup()
        end
        
        imgui.EndPopup()
    elseif state.showAboutPopup then
        state.showAboutPopup = false
    end
end

function M.RenderServerSelectionWindow()
    -- Use the serverselection data module for draft selection
    local serverSelectionData = require('modules.serverselection.data')
    
    -- Ensure data module is initialized and synced with app state
    if not serverSelectionData.GetServers or #(serverSelectionData.GetServers() or {}) == 0 then
        -- Initialize if needed
        if serverSelectionData.Initialize then
            serverSelectionData.Initialize({})
        end
    end
    
    -- Update data from app state
    if serverSelectionData.Update then
        serverSelectionData.Update()
    end
    
    local screenWidth = scaling.window.w
    local screenHeight = scaling.window.h
    
    -- Window flags - regular window with no collapse
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
    
    -- Set size for first use
    imgui.SetNextWindowSize({350, 0}, ImGuiCond_FirstUseEver)
    
    -- Apply theme
    local themePushed = M.ApplyTheme()
    
    -- Begin regular window (like original auto-popup)
    local windowOpenTable = {true}
    if not imgui.Begin("Select Server##serverselection", windowOpenTable, windowFlags) then
        imgui.End()
        M.PopTheme(themePushed)
        return
    end
    
    -- Handle X button close
    if not windowOpenTable[1] then
        state.serverSelectionPopupOpened = false
        imgui.End()
        M.PopTheme(themePushed)
        return
    end
    
    imgui.TextWrapped("Please select your server to connect to the FFXIFriendList service.")
    
    imgui.Separator()
    
    imgui.TextColored({1.0, 0.8, 0.0, 1.0}, "Warning:")
    imgui.TextWrapped("If you select the wrong server, you may not be able to find your friends.")
    
    imgui.Separator()
    
    -- Get server list data from serverselection data module
    local servers = serverSelectionData.GetServers() or {}
    local draftServerId = serverSelectionData.GetDraftSelectedServerId() or ""
    local detectedServerId = serverSelectionData.GetDetectedServerId()
    local detectedServerName = serverSelectionData.GetDetectedServerName()
    
    -- Auto-refresh if no servers loaded
    if #servers == 0 and not state.serverListRefreshTriggered then
        serverSelectionData.RefreshServerList()
        state.serverListRefreshTriggered = true
    end
    
    -- Show detected server if available
    if detectedServerId and detectedServerName then
        imgui.Text("Detected server: " .. detectedServerName)
        imgui.Spacing()
    end
    
    -- Server combo
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
                
                -- Highlight detected server in green
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
    
    -- Buttons
    local canSave = draftServerId ~= "" and draftServerId ~= nil
    
    if canSave then
        if imgui.Button("Save", {120, 0}) then
            serverSelectionData.SaveServerSelection(draftServerId)
            state.serverSelectionPopupOpened = false
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
        state.serverSelectionPopupOpened = false
    end
    
    imgui.End()
    M.PopTheme(themePushed)
end

function M.GetCallbacks(dataModule)
    local app = _G.FFXIFriendListApp
    
    return {
        onRefresh = function()
            if app and app.features and app.features.friends then
                app.features.friends:refresh()
                app.features.friends:refreshFriendRequests()
            end
        end,
        
        onLockChanged = function(locked)
            if gConfig then
                if not gConfig.windows then
                    gConfig.windows = {}
                end
                if not gConfig.windows.friendList then
                    gConfig.windows.friendList = {}
                end
                gConfig.windows.friendList.locked = locked
            end
        end,
        
        onAddFriend = function(name, note)
            if app and app.features and app.features.friends then
                app.features.friends:addFriend(name, note)
            end
        end,
        
        onAcceptRequest = function(requestId)
            if app and app.features and app.features.friends then
                app.features.friends:acceptRequest(requestId)
            end
        end,
        
        onRejectRequest = function(requestId)
            if app and app.features and app.features.friends then
                app.features.friends:rejectRequest(requestId)
            end
        end,
        
        onCancelRequest = function(requestId)
            if app and app.features and app.features.friends then
                app.features.friends:cancelRequest(requestId)
            end
        end,
        
        onRemoveFriend = function(friendName)
            if app and app.features and app.features.friends then
                app.features.friends:removeFriend(friendName)
            end
        end,
        
        onSendTell = function(friendName)
            local chatManager = AshitaCore:GetChatManager()
            if chatManager then
                -- Use sendkey to open chat with "/"
                chatManager:QueueCommand(-1, '/sendkey / down')
                -- Wait for chat input to open, then set the tell command
                ashita.tasks.once(0, function()
                    while chatManager:IsInputOpen() ~= 0x11 do
                        coroutine.sleepf(1)
                    end
                    chatManager:SetInputText('/tell ' .. utils.capitalizeName(friendName) .. ' ')
                end)
            end
        end,
        
        onOpenNoteEditor = function(friendName)
            local noteEditorModule = require('modules.noteeditor')
            if noteEditorModule and noteEditorModule.OpenEditor then
                noteEditorModule.OpenEditor(friendName)
            end
        end,
        
        onDeleteNote = function(friendName)
        end,
        
        onSaveState = M.SaveWindowState,
        
        onRenderContextMenu = function(friend)
            FriendContextMenu.Render(friend, state, M.GetCallbacks(dataModule))
        end,
        
        onRefreshAltVisibility = function()
            if app and app.features and app.features.altVisibility then
                app.features.altVisibility:refresh()
            end
        end
    }
end

return M
