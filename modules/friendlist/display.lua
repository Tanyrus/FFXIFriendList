local imgui = require('imgui')
local ThemeHelper = require('libs.themehelper')
local UIConstants = require('core.UIConstants')
local scaling = require('scaling')
local FontManager = require('app.ui.FontManager')

local Sidebar = require('modules.friendlist.components.Sidebar')
local TopBar = require('modules.friendlist.components.TopBar')
local AddFriendSection = require('modules.friendlist.components.AddFriendSection')
local RequestsTab = require('modules.friendlist.components.RequestsTab')
local FriendsTable = require('modules.friendlist.components.FriendsTable')
local FriendContextMenu = require('modules.friendlist.components.FriendContextMenu')
local FriendDetailsPopup = require('modules.friendlist.components.FriendDetailsPopup')
local PrivacyTab = require('modules.friendlist.components.PrivacyTab')
local NotificationsTab = require('modules.friendlist.components.NotificationsTab')
local ControlsTab = require('modules.friendlist.components.ControlsTab')
local ThemesTab = require('modules.friendlist.components.ThemesTab')
local HelpTab = require('modules.friendlist.components.HelpTab')
local CollapsibleTagSection = require('modules.friendlist.components.CollapsibleTagSection')
local TagManager = require('modules.friendlist.components.TagManager')
local utils = require('modules.friendlist.components.helpers.utils')
local tagcore = require('core.tagcore')
local taggrouper = require('core.taggrouper')

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
    themeFontSizePx = 14,
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
    },
    columnOrder = {"Name", "Job", "Zone", "Nation/Rank", "Last Seen", "Added As"},
    columnWidths = {
        Name = 120.0,
        Job = 100.0,
        Zone = 120.0,
        ["Nation/Rank"] = 80.0,
        ["Last Seen"] = 120.0,
        ["Added As"] = 100.0
    },
    draggingColumn = nil,
    hoveredColumn = nil,
    tagManagerExpanded = false,
    groupByOnlineStatus = false,
    collapsedOnlineSection = false,
    collapsedOfflineSection = false,
    groupByStatusExpanded = true,
    compactFriendListExpanded = false
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
        if settings.columnOrder and type(settings.columnOrder) == "table" and #settings.columnOrder > 0 then
            state.columnOrder = {}
            for i, col in ipairs(settings.columnOrder) do
                state.columnOrder[i] = col
            end
        end
        if settings.columnWidths and type(settings.columnWidths) == "table" then
            for colName, width in pairs(settings.columnWidths) do
                state.columnWidths[colName] = width
            end
        end
        if settings.groupByOnlineStatus ~= nil then
            state.groupByOnlineStatus = settings.groupByOnlineStatus
        end
        if settings.collapsedOnlineSection ~= nil then
            state.collapsedOnlineSection = settings.collapsedOnlineSection
        end
        if settings.collapsedOfflineSection ~= nil then
            state.collapsedOfflineSection = settings.collapsedOfflineSection
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
    gConfig.friendListSettings.columnOrder = {}
    for i, col in ipairs(state.columnOrder) do
        gConfig.friendListSettings.columnOrder[i] = col
    end
    gConfig.friendListSettings.columnWidths = {}
    for colName, width in pairs(state.columnWidths) do
        gConfig.friendListSettings.columnWidths[colName] = width
    end
    gConfig.friendListSettings.groupByOnlineStatus = state.groupByOnlineStatus
    gConfig.friendListSettings.collapsedOnlineSection = state.collapsedOnlineSection
    gConfig.friendListSettings.collapsedOfflineSection = state.collapsedOfflineSection
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
    local app = _G.FFXIFriendListApp
    local globalLocked = false
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        globalLocked = prefs and prefs.windowsLocked or false
    end
    if globalLocked or (gConfig and gConfig.windows and gConfig.windows.friendList and gConfig.windows.friendList.locked) then
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
        local app = _G.FFXIFriendListApp
        local fontSizePx = 14
        if app and app.features and app.features.themes then
            fontSizePx = app.features.themes:getFontSizePx() or 14
        end
        
        FontManager.withFont(fontSizePx, function()
            M.SaveWindowState()
            M.RenderContent(dataModule)
        end)
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
    
    local sidebarWidth = FontManager.scaled(UIConstants.SIDEBAR_WIDTH)
    imgui.BeginChild("Sidebar", {sidebarWidth, 0}, false)
    Sidebar.Render(state, dataModule, M.SaveWindowState)
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
        RequestsTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 2 then
        M.RenderGeneralTab(dataModule, callbacks)
    elseif state.selectedTab == 3 then
        M.RenderPrivacyTab(dataModule, callbacks)
    elseif state.selectedTab == 4 then
        M.RenderTagsTab()
    elseif state.selectedTab == 5 then
        NotificationsTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 6 then
        ControlsTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 7 then
        ThemesTab.Render(state, dataModule, callbacks)
    elseif state.selectedTab == 8 then
        HelpTab.Render(state, dataModule, callbacks)
    end
end

function M.RenderGeneralTab(dataModule, callbacks)
    -- General settings (Menu Detection, Friend View, Hover Tooltip, Group by Status, Compact Friend List)
    PrivacyTab.RenderMenuDetectionSection(state, callbacks)
    imgui.Spacing()
    PrivacyTab.RenderFriendViewSettingsSection(state, callbacks)
    imgui.Spacing()
    PrivacyTab.RenderHoverTooltipSettings(state, callbacks)
    imgui.Spacing()
    M.RenderGroupByStatusSection(callbacks)
    imgui.Spacing()
    M.RenderCompactFriendListSettings(callbacks)
    imgui.Spacing()
    M.RenderWindowLockSection(callbacks)
end

function M.RenderWindowLockSection(callbacks)
    local headerLabel = "Window Lock"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.windowLockExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.windowLockExpanded then
        state.windowLockExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    local app = _G.FFXIFriendListApp
    local prefs = nil
    if app and app.features and app.features.preferences then
        prefs = app.features.preferences:getPrefs()
    end
    
    if not prefs then
        imgui.Text("Preferences not available")
        return
    end
    
    local windowsLocked = {prefs.windowsLocked or false}
    if imgui.Checkbox("Lock All Windows", windowsLocked) then
        if app and app.features and app.features.preferences then
            app.features.preferences:setPref("windowsLocked", windowsLocked[1])
            app.features.preferences:save()
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("When enabled, all addon windows cannot be moved or resized.\nUseful for locking your UI layout.")
    end
end

function M.RenderGroupByStatusSection(callbacks)
    local headerLabel = "Group by Status"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.groupByStatusExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.groupByStatusExpanded then
        state.groupByStatusExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    imgui.TextWrapped("Group friends by Online/Offline status first, then by tags within each group.")
    imgui.Spacing()
    
    -- Main Window toggle
    local mainGroupByStatus = {state.groupByOnlineStatus}
    if imgui.Checkbox("Main Window##main_group_status", mainGroupByStatus) then
        state.groupByOnlineStatus = mainGroupByStatus[1]
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Enable Online/Offline sections in the Main Friend List window")
    end
    
    -- Compact Friend List toggle
    local qoGroupByStatus = {gConfig and gConfig.quickOnlineSettings and gConfig.quickOnlineSettings.groupByOnlineStatus or false}
    if imgui.Checkbox("Compact Friend List##qo_group_status", qoGroupByStatus) then
        if gConfig then
            if not gConfig.quickOnlineSettings then
                gConfig.quickOnlineSettings = {}
            end
            gConfig.quickOnlineSettings.groupByOnlineStatus = qoGroupByStatus[1]
            local settings = require('libs.settings')
            if settings and settings.save then
                settings.save()
            end
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Enable Online/Offline sections in the Compact Friend List window")
    end
end

function M.RenderCompactFriendListSettings(callbacks)
    local headerLabel = "Compact Friend List"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.compactFriendListExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.compactFriendListExpanded then
        state.compactFriendListExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    -- Hide Top Bar toggle
    local hideTopBar = {gConfig and gConfig.quickOnlineSettings and gConfig.quickOnlineSettings.hideTopBar or false}
    if imgui.Checkbox("Hide Top Bar##qo_hide_topbar", hideTopBar) then
        if gConfig then
            if not gConfig.quickOnlineSettings then
                gConfig.quickOnlineSettings = {}
            end
            gConfig.quickOnlineSettings.hideTopBar = hideTopBar[1]
            local settings = require('libs.settings')
            if settings and settings.save then
                settings.save()
            end
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Hide the top bar (lock, refresh, switch view buttons) in the Compact Friend List window")
    end
end

function M.RenderPrivacyTab(dataModule, callbacks)
    -- Privacy settings (Privacy Controls, Alt Visibility)
    PrivacyTab.RenderPrivacyControlsSection(state, callbacks)
    imgui.Spacing()
    PrivacyTab.RenderAltVisibilitySection(state, dataModule, callbacks)
end

function M.RenderTagsTab()
    imgui.Text("Tag Management")
    imgui.Separator()
    imgui.Spacing()
    TagManager.Render(true)
end

function M.RenderFriendsTab(dataModule, callbacks)
    AddFriendSection.Render(state, dataModule, callbacks.onAddFriend)
    
    imgui.Dummy({0, 6})
    
    imgui.BeginChild("##friends_table_child", {0, 0}, false)
    M.RenderTaggedFriendSections(dataModule, callbacks)
    imgui.EndChild()
    
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.tags then
        app.features.tags:flushRetagQueue()
    end
end

function M.RenderTaggedFriendSections(dataModule, callbacks)
    local app = _G.FFXIFriendListApp
    local tagsFeature = app and app.features and app.features.tags
    
    local friends = dataModule.GetFriends()
    local filterText = state.filterText[1] or ""
    
    if filterText ~= "" then
        local lowerFilter = string.lower(filterText)
        local filtered = {}
        for _, friend in ipairs(friends) do
            local friendName = type(friend.name) == "string" and string.lower(friend.name) or ""
            local presence = friend.presence or {}
            local job = type(presence.job) == "string" and string.lower(presence.job) or ""
            local zone = type(presence.zone) == "string" and string.lower(presence.zone) or ""
            if string.find(friendName, lowerFilter, 1, true) or
               string.find(job, lowerFilter, 1, true) or
               string.find(zone, lowerFilter, 1, true) then
                table.insert(filtered, friend)
            end
        end
        friends = filtered
    end
    
    imgui.AlignTextToFramePadding()
    imgui.Text("Filter:")
    imgui.SameLine()
    imgui.PushItemWidth(200)
    if imgui.InputText("##filter_input", state.filterText, 64) then
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    imgui.PopItemWidth()
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Filter friends by name, job, or zone")
    end
    
    imgui.Spacing()
    
    if #friends == 0 then
        imgui.Text("No friends" .. (filterText ~= "" and " matching filter" or ""))
        return
    end
    
    local tagOrder = {}
    if tagsFeature then
        tagOrder = tagsFeature:getAllTags() or {}
    end
    
    local getFriendTag = function(friend)
        if not tagsFeature then
            return nil
        end
        local friendKey = tagcore.getFriendKey(friend)
        return tagsFeature:getTagForFriend(friendKey)
    end
    
    local groups = taggrouper.groupFriendsByTag(friends, tagOrder, getFriendTag)
    
    local sortMode = gConfig and gConfig.friendListSettings and gConfig.friendListSettings.sortMode or "status"
    local sortDirection = gConfig and gConfig.friendListSettings and gConfig.friendListSettings.sortDirection or "asc"
    taggrouper.sortAllGroups(groups, sortMode, sortDirection)
    
    local sectionCallbacks = {
        onSaveState = callbacks.onSaveState,
        onRenderContextMenu = callbacks.onRenderContextMenu,
        onQueueRetag = function(friendKey, newTag)
            if tagsFeature then
                tagsFeature:queueRetag(friendKey, newTag)
            end
        end
    }
    
    local renderFriendsTable = function(groupFriends, renderState, renderCallbacks, sectionTag)
        FriendsTable.RenderGroupTable(groupFriends, state, callbacks, sectionTag)
    end
    
    local columnVisibilityMap = {
        Name = true,
        Job = state.columnVisible and state.columnVisible.job,
        Zone = state.columnVisible and state.columnVisible.zone,
        ["Nation/Rank"] = state.columnVisible and state.columnVisible.nationRank,
        ["Last Seen"] = state.columnVisible and state.columnVisible.lastSeen,
        ["Added As"] = state.columnVisible and state.columnVisible.addedAs
    }
    
    local visibleColumns = {}
    for _, colName in ipairs(state.columnOrder) do
        if columnVisibilityMap[colName] then
            table.insert(visibleColumns, colName)
        end
    end
    
    local headerTableFlags = bit.bor(
        ImGuiTableFlags_Borders,
        ImGuiTableFlags_NoBordersInBody
    )
    if imgui.BeginTable("##friends_header_table", #visibleColumns, headerTableFlags) then
        for _, colName in ipairs(visibleColumns) do
            local width = state.columnWidths[colName] or 100.0
            imgui.TableSetupColumn(colName, ImGuiTableColumnFlags_WidthFixed, width)
        end
        
        imgui.TableNextRow(ImGuiTableRowFlags_Headers)
        for colIdx, colName in ipairs(visibleColumns) do
            imgui.TableSetColumnIndex(colIdx - 1)
            
            local isSelected = (state.draggingColumn == colName)
            if imgui.Selectable(colName .. "##col_header_" .. colIdx, isSelected, ImGuiSelectableFlags_None) then
            end
            
            if imgui.BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID) then
                state.draggingColumn = colName
                imgui.Text("Move: " .. colName)
                imgui.EndDragDropSource()
            elseif state.draggingColumn == colName and not imgui.IsMouseDragging(0) then
                if state.hoveredColumn and state.hoveredColumn ~= colName then
                    local fromIdx, toIdx
                    for i, col in ipairs(state.columnOrder) do
                        if col == state.draggingColumn then fromIdx = i end
                        if col == state.hoveredColumn then toIdx = i end
                    end
                    if fromIdx and toIdx and fromIdx ~= toIdx then
                        table.remove(state.columnOrder, fromIdx)
                        table.insert(state.columnOrder, toIdx, state.draggingColumn)
                        if callbacks.onSaveState then callbacks.onSaveState() end
                    end
                end
                state.draggingColumn = nil
                state.hoveredColumn = nil
            end
            
            if state.draggingColumn and imgui.IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) then
                state.hoveredColumn = colName
            end
        end
        
        
        if imgui.BeginPopupContextWindow("##header_column_context", 1) then
            imgui.Text("Show Columns:")
            imgui.Separator()
            
            local jobVisible = {state.columnVisible.job}
            if imgui.Checkbox("Job##ctx_col_job", jobVisible) then
                state.columnVisible.job = jobVisible[1]
                if callbacks.onSaveState then callbacks.onSaveState() end
            end
            
            local zoneVisible = {state.columnVisible.zone}
            if imgui.Checkbox("Zone##ctx_col_zone", zoneVisible) then
                state.columnVisible.zone = zoneVisible[1]
                if callbacks.onSaveState then callbacks.onSaveState() end
            end
            
            local nationRankVisible = {state.columnVisible.nationRank}
            if imgui.Checkbox("Nation/Rank##ctx_col_nation", nationRankVisible) then
                state.columnVisible.nationRank = nationRankVisible[1]
                if callbacks.onSaveState then callbacks.onSaveState() end
            end
            
            local lastSeenVisible = {state.columnVisible.lastSeen}
            if imgui.Checkbox("Last Seen##ctx_col_lastseen", lastSeenVisible) then
                state.columnVisible.lastSeen = lastSeenVisible[1]
                if callbacks.onSaveState then callbacks.onSaveState() end
            end
            
            local addedAsVisible = {state.columnVisible.addedAs}
            if imgui.Checkbox("Added As##ctx_col_addedas", addedAsVisible) then
                state.columnVisible.addedAs = addedAsVisible[1]
                if callbacks.onSaveState then callbacks.onSaveState() end
            end
            
            imgui.Spacing()
            imgui.Separator()
            imgui.Text("Column Widths:")
            imgui.Separator()
            
            for _, colName in ipairs(state.columnOrder) do
                imgui.PushID("col_width_" .. colName)
                local currentWidth = {state.columnWidths[colName] or 100.0}
                imgui.PushItemWidth(100)
                if imgui.SliderFloat(colName, currentWidth, 50.0, 250.0, "%.0f") then
                    state.columnWidths[colName] = currentWidth[1]
                    if callbacks.onSaveState then callbacks.onSaveState() end
                end
                imgui.PopItemWidth()
                imgui.PopID()
            end
            
            imgui.EndPopup()
        end
        
        imgui.EndTable()
    end
    
    if state.groupByOnlineStatus then
        -- Split friends by online/offline status first
        local onlineFriends = {}
        local offlineFriends = {}
        for _, friend in ipairs(friends) do
            if friend.isOnline then
                table.insert(onlineFriends, friend)
            else
                table.insert(offlineFriends, friend)
            end
        end
        
        -- Group online friends by tag
        local onlineGroups = taggrouper.groupFriendsByTag(onlineFriends, tagOrder, getFriendTag)
        taggrouper.sortAllGroups(onlineGroups, sortMode, sortDirection)
        
        -- Group offline friends by tag
        local offlineGroups = taggrouper.groupFriendsByTag(offlineFriends, tagOrder, getFriendTag)
        taggrouper.sortAllGroups(offlineGroups, sortMode, sortDirection)
        
        -- Render Online section
        local onlineCount = #onlineFriends
        local onlineCollapsed = {state.collapsedOnlineSection}
        -- Auto-collapse if empty
        local onlineShouldBeOpen = onlineCount > 0 and not onlineCollapsed[1]
        imgui.SetNextItemOpen(onlineShouldBeOpen)
        local onlineExpanded = imgui.CollapsingHeader("Online (" .. onlineCount .. ")##online_section")
        if imgui.IsItemClicked() then
            state.collapsedOnlineSection = not state.collapsedOnlineSection
            if callbacks.onSaveState then callbacks.onSaveState() end
        end
        
        -- Online section drop target for drag-and-drop (retag to first tag or untagged)
        if FriendsTable.isDraggingFriend() then
            if imgui.BeginDragDropTarget() then
                imgui.EndDragDropTarget()
            end
        end
        
        if onlineExpanded then
            imgui.Indent(10)
            CollapsibleTagSection.RenderAllSections(onlineGroups, state, sectionCallbacks, renderFriendsTable, "_online")
            imgui.Unindent(10)
        end
        
        -- Render Offline section
        local offlineCount = #offlineFriends
        local offlineCollapsed = {state.collapsedOfflineSection}
        -- Auto-collapse if empty
        local offlineShouldBeOpen = offlineCount > 0 and not offlineCollapsed[1]
        imgui.SetNextItemOpen(offlineShouldBeOpen)
        local offlineExpanded = imgui.CollapsingHeader("Offline (" .. offlineCount .. ")##offline_section")
        if imgui.IsItemClicked() then
            state.collapsedOfflineSection = not state.collapsedOfflineSection
            if callbacks.onSaveState then callbacks.onSaveState() end
        end
        
        -- Offline section drop target for drag-and-drop
        if FriendsTable.isDraggingFriend() then
            if imgui.BeginDragDropTarget() then
                imgui.EndDragDropTarget()
            end
        end
        
        if offlineExpanded then
            imgui.Indent(10)
            CollapsibleTagSection.RenderAllSections(offlineGroups, state, sectionCallbacks, renderFriendsTable, "_offline")
            imgui.Unindent(10)
        end
    else
        -- Default behavior: just tag sections without online/offline grouping
        CollapsibleTagSection.RenderAllSections(groups, state, sectionCallbacks, renderFriendsTable)
    end
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
            
            -- Check if help tab should auto-open
            if serverSelectionData.ShouldAutoOpenHelpTab() then
                state.selectedTab = 8
                local settings = require('libs.settings')
                if settings and settings.save then
                    settings.save()
                end
            end
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
