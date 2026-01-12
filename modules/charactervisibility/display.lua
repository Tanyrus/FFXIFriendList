--[[
* Character Visibility Module - Display
* ImGui rendering for Per-Character Visibility settings
]]

local M = {}

local state = {
    initialized = false,
    hidden = false,
    windowOpen = false
}

local WINDOW_ID = "##character_visibility_main"

function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    
    if gConfig and gConfig.windows and gConfig.windows.characterVisibility then
        local windowState = gConfig.windows.characterVisibility
        if windowState.visible ~= nil then
            state.windowOpen = windowState.visible
        end
    end
    
    return nil
end

function M.UpdateVisuals(settings)
    state.settings = settings or {}
end

function M.SetHidden(hidden)
    state.hidden = hidden or false
end

function M.IsWindowOpen()
    return state.windowOpen
end

function M.SetWindowOpen(open)
    state.windowOpen = open or false
    M.SaveWindowState()
end

function M.ToggleWindow()
    state.windowOpen = not state.windowOpen
    M.SaveWindowState()
end

function M.DrawWindow(settings, dataModule)
    if not state.initialized or state.hidden or not state.windowOpen then
        return
    end
    
    if not dataModule then
        return
    end
    
    local guiManager = ashita and ashita.gui and ashita.gui.GetGuiManager and ashita.gui:GetGuiManager()
    if not guiManager then
        return
    end
    
    local windowFlags = 0
    local app = _G.FFXIFriendListApp
    local globalPositionLocked = false
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        globalPositionLocked = prefs and prefs.windowsPositionLocked or false
    end
    if globalPositionLocked or (gConfig and gConfig.windows and gConfig.windows.characterVisibility and gConfig.windows.characterVisibility.locked) then
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoMove)
        windowFlags = bit.bor(windowFlags, ImGuiWindowFlags_NoResize)
    end
    
    -- Restore window position/size from config
    if gConfig and gConfig.windows and gConfig.windows.characterVisibility then
        local windowState = gConfig.windows.characterVisibility
        if windowState.posX and windowState.posY then
            guiManager:SetNextWindowPos(windowState.posX, windowState.posY, ImGuiCond_FirstUseEver)
        end
        if windowState.sizeX and windowState.sizeY then
            guiManager:SetNextWindowSize(windowState.sizeX, windowState.sizeY, ImGuiCond_FirstUseEver)
        end
    else
        guiManager:SetNextWindowSize(400, 300, ImGuiCond_FirstUseEver)
    end
    
    local windowTitle = "Per-Character Visibility" .. WINDOW_ID
    local shouldDraw, isOpen = guiManager:Begin(windowTitle, state.windowOpen, windowFlags)
    
    if isOpen ~= state.windowOpen then
        state.windowOpen = isOpen
        M.SaveWindowState()
    end
    
    if shouldDraw then
        M.RenderContent(dataModule)
    end
    
    guiManager:End()
end

function M.RenderContent(dataModule)
    local guiManager = ashita.gui:GetGuiManager()
    if not guiManager then
        return
    end
    
    local featureState = dataModule.GetState()
    
    -- Header with explanation
    guiManager:TextWrapped("Control whether each of your characters appears online to friends.")
    guiManager:Spacing()
    guiManager:TextColored(0.7, 0.7, 0.7, 1.0, "If disabled, the character will appear Offline to others even while online.")
    guiManager:Spacing()
    guiManager:Separator()
    guiManager:Spacing()
    
    -- Loading indicator
    if featureState.isLoading then
        guiManager:Text("Loading...")
        return
    end
    
    -- Error display
    if featureState.lastError then
        guiManager:TextColored(1.0, 0.3, 0.3, 1.0, "Error: " .. tostring(featureState.lastError))
        guiManager:Spacing()
        if guiManager:Button("Retry##charvis") then
            dataModule.Refresh()
        end
        return
    end
    
    local characters = dataModule.GetCharacters()
    
    if not characters or #characters == 0 then
        guiManager:Text("No characters found.")
        guiManager:Spacing()
        if guiManager:Button("Refresh##charvis") then
            dataModule.Refresh()
        end
        return
    end
    
    -- Refresh button
    if guiManager:Button("Refresh##charvis_refresh") then
        dataModule.Refresh()
    end
    guiManager:SameLine()
    guiManager:TextColored(0.6, 0.6, 0.6, 1.0, string.format("(%d characters)", #characters))
    guiManager:Spacing()
    
    -- Character table
    local tableFlags = bit.bor(
        ImGuiTableFlags_Borders,
        ImGuiTableFlags_RowBg,
        ImGuiTableFlags_SizingStretchProp
    )
    
    if guiManager:BeginTable("CharacterVisibilityTable", 2, tableFlags) then
        -- Headers
        guiManager:TableSetupColumn("Character", ImGuiTableColumnFlags_None, 0.6)
        guiManager:TableSetupColumn("Visible", ImGuiTableColumnFlags_None, 0.4)
        guiManager:TableHeadersRow()
        
        -- Rows
        for i, char in ipairs(characters) do
            guiManager:TableNextRow()
            
            -- Character name column
            guiManager:TableNextColumn()
            guiManager:Text(char.characterName or "Unknown")
            
            -- Visibility toggle column
            guiManager:TableNextColumn()
            local checkboxId = "##charvis_" .. tostring(i)
            local changed, newValue = guiManager:Checkbox(checkboxId, char.shareVisibility)
            
            if changed then
                dataModule.SetVisibility(char.characterId, newValue)
            end
            
            -- Tooltip on hover
            if guiManager:IsItemHovered() then
                guiManager:BeginTooltip()
                if char.shareVisibility then
                    guiManager:Text("This character's real status is visible to friends.")
                else
                    guiManager:Text("This character appears OFFLINE to friends.")
                end
                guiManager:EndTooltip()
            end
        end
        
        guiManager:EndTable()
    end
    
    guiManager:Spacing()
    guiManager:Separator()
    guiManager:Spacing()
    
    -- Help text at bottom
    guiManager:TextWrapped("Note: These settings take effect immediately. When visibility is disabled, friends will see you as offline even if you're online or away.")
end

function M.SaveWindowState()
    if not gConfig then
        return
    end
    
    local guiManager = ashita and ashita.gui and ashita.gui.GetGuiManager and ashita.gui:GetGuiManager()
    if not guiManager then
        return
    end
    
    if not gConfig.windows then
        gConfig.windows = {}
    end
    
    if not gConfig.windows.characterVisibility then
        gConfig.windows.characterVisibility = {}
    end
    
    local windowState = gConfig.windows.characterVisibility
    windowState.visible = state.windowOpen
    
    if state.windowOpen then
        local pos = guiManager:GetWindowPos()
        if pos then
            windowState.posX = pos.x
            windowState.posY = pos.y
        end
        
        local size = guiManager:GetWindowSize()
        if size then
            windowState.sizeX = size.x
            windowState.sizeY = size.y
        end
    end
end

function M.Cleanup()
    state.initialized = false
end

return M
