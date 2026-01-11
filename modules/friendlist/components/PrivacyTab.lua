local imgui = require('imgui')
local PresenceStatusPicker = require('ui.widgets.PresenceStatusPicker')
local nations = require('core.nations')

local M = {}

function M.Render(state, dataModule, callbacks)
    M.RenderMenuDetectionSection(state, callbacks)
    imgui.Spacing()
    M.RenderFriendViewSettingsSection(state, callbacks)
    imgui.Spacing()
    M.RenderHoverTooltipSettings(state, callbacks)
    imgui.Spacing()
    M.RenderPrivacyControlsSection(state, callbacks)
    imgui.Spacing()
    M.RenderBlockedPlayersSection(state, dataModule, callbacks)
    imgui.Spacing()
    M.RenderAltVisibilitySection(state, dataModule, callbacks)
end

function M.RenderMenuDetectionSection(state, callbacks)
    local headerLabel = "Menu Detection"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.menuDetectionExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.menuDetectionExpanded then
        state.menuDetectionExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    local menuDetectionEnabled = gConfig and gConfig.menuDetectionEnabled ~= false
    local checked = {menuDetectionEnabled}
    
    if imgui.Checkbox("Enable Menu Detection", checked) then
        if gConfig then
            gConfig.menuDetectionEnabled = checked[1]
            local settings = require('libs.settings')
            if settings and settings.save then
                settings.save()
            end
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("When enabled, Compact Friend List window automatically opens when\nyou open the in-game Friend List menu (To List).\n\nDisable if you prefer to open windows manually.")
    end
end

function M.RenderFriendViewSettingsSection(state, callbacks)
    local headerLabel = "Friend View Settings"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.friendViewSettingsExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.friendViewSettingsExpanded then
        state.friendViewSettingsExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    imgui.Text("Main Window Columns:")
    
    if imgui.Checkbox("Show Job", {state.columnVisible.job}) then
        state.columnVisible.job = not state.columnVisible.job
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Job column (current job of online friends).")
    end
    
    if imgui.Checkbox("Show Zone", {state.columnVisible.zone}) then
        state.columnVisible.zone = not state.columnVisible.zone
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Zone column (current zone of online friends).")
    end
    
    if imgui.Checkbox("Show Nation/Rank", {state.columnVisible.nationRank}) then
        state.columnVisible.nationRank = not state.columnVisible.nationRank
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Nation and Rank column.")
    end
    
    if imgui.Checkbox("Show Last Seen", {state.columnVisible.lastSeen}) then
        state.columnVisible.lastSeen = not state.columnVisible.lastSeen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Last Seen column (when the friend was last online).\nDisplays 'Now' for online friends.")
    end
    
    if imgui.Checkbox("Show Added As", {state.columnVisible.addedAs}) then
        state.columnVisible.addedAs = not state.columnVisible.addedAs
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Added As column (the character name used when adding this friend).")
    end
    
    if imgui.Checkbox("Show Realm", {state.columnVisible.realm}) then
        state.columnVisible.realm = not state.columnVisible.realm
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Show the Realm column (the server/realm the friend is on).")
    end
end

function M.RenderHoverTooltipSettings(state, callbacks)
    local headerLabel = "Hover Tooltip Settings"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.hoverTooltipSettingsExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.hoverTooltipSettingsExpanded then
        state.hoverTooltipSettingsExpanded = isOpen
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
    
    imgui.TextWrapped("Configure what appears when hovering over a friend's name.")
    imgui.TextDisabled("Hold SHIFT while hovering to show all fields.")
    imgui.Spacing()
    
    imgui.Text("Main Window Hover:")
    imgui.Indent()
    
    local mainHover = prefs.mainHoverTooltip
    if mainHover then
        local changed = false
        
        local showJob = {mainHover.showJob}
        if imgui.Checkbox("Job##main_hover", showJob) then
            mainHover.showJob = showJob[1]
            changed = true
        end
        imgui.SameLine()
        
        local showZone = {mainHover.showZone}
        if imgui.Checkbox("Zone##main_hover", showZone) then
            mainHover.showZone = showZone[1]
            changed = true
        end
        imgui.SameLine()
        
        local showNationRank = {mainHover.showNationRank}
        if imgui.Checkbox("Nation/Rank##main_hover", showNationRank) then
            mainHover.showNationRank = showNationRank[1]
            changed = true
        end
        
        local showLastSeen = {mainHover.showLastSeen}
        if imgui.Checkbox("Last Seen##main_hover", showLastSeen) then
            mainHover.showLastSeen = showLastSeen[1]
            changed = true
        end
        imgui.SameLine()
        
        local showFriendedAs = {mainHover.showFriendedAs}
        if imgui.Checkbox("Added As##main_hover", showFriendedAs) then
            mainHover.showFriendedAs = showFriendedAs[1]
            changed = true
        end
        
        if changed and app.features.preferences then
            app.features.preferences:save()
        end
    end
    
    imgui.Unindent()
    imgui.Spacing()
    
    imgui.Text("Compact Friend List Hover:")
    imgui.Indent()
    
    local quickHover = prefs.quickOnlineHoverTooltip
    if quickHover then
        local changed = false
        
        local showJob = {quickHover.showJob}
        if imgui.Checkbox("Job##quick_hover", showJob) then
            quickHover.showJob = showJob[1]
            changed = true
        end
        imgui.SameLine()
        
        local showZone = {quickHover.showZone}
        if imgui.Checkbox("Zone##quick_hover", showZone) then
            quickHover.showZone = showZone[1]
            changed = true
        end
        imgui.SameLine()
        
        local showNationRank = {quickHover.showNationRank}
        if imgui.Checkbox("Nation/Rank##quick_hover", showNationRank) then
            quickHover.showNationRank = showNationRank[1]
            changed = true
        end
        
        local showLastSeen = {quickHover.showLastSeen}
        if imgui.Checkbox("Last Seen##quick_hover", showLastSeen) then
            quickHover.showLastSeen = showLastSeen[1]
            changed = true
        end
        imgui.SameLine()
        
        local showFriendedAs = {quickHover.showFriendedAs}
        if imgui.Checkbox("Added As##quick_hover", showFriendedAs) then
            quickHover.showFriendedAs = showFriendedAs[1]
            changed = true
        end
        
        if changed and app.features.preferences then
            app.features.preferences:save()
        end
    end
    
    imgui.Unindent()
end

function M.RenderPrivacyControlsSection(state, callbacks)
    local headerLabel = "Privacy"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.privacyControlsExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.privacyControlsExpanded then
        state.privacyControlsExpanded = isOpen
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
    
    local shareJobWhenAnonymous = {prefs.shareJobWhenAnonymous or false}
    if imgui.Checkbox("Share Job, Nation, and Rank when Anonymous", shareJobWhenAnonymous) then
        if app and app.features and app.features.preferences then
            app.features.preferences:setPref("shareJobWhenAnonymous", shareJobWhenAnonymous[1])
            app.features.preferences:save()
            app.features.preferences:syncToServer()
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("When enabled, your job, nation, and rank are shared even when\nyou're set to anonymous mode in-game.\n\n[Account-wide: Synced to server for all characters]")
    end
    
    imgui.Spacing()
    
    local shareLocation = {prefs.shareLocation ~= false}
    if imgui.Checkbox("Share Location", shareLocation) then
        if app and app.features and app.features.preferences then
            app.features.preferences:setPref("shareLocation", shareLocation[1])
            app.features.preferences:save()
            app.features.preferences:syncToServer()
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("When enabled, your current zone is shared with friends.\nWhen disabled, friends will only see that you're online.\n\n[Account-wide: Synced to server for all characters]")
    end
    
    imgui.Spacing()
    
    imgui.Text("Presence Status:")
    if imgui.IsItemHovered() then
        imgui.SetTooltip("Controls how your online status appears to friends.\n\n[Account-wide: Synced to server for all characters]")
    end
    
    PresenceStatusPicker.RenderRadioButtons("_privacy_tab", true)
    
    imgui.Spacing()
    imgui.Spacing()
    
    -- Show preview of how friends see your information
    M.RenderPresencePreview(prefs)
end

--- Render a preview showing how friends see your information
function M.RenderPresencePreview(prefs)
    local app = _G.FFXIFriendListApp
    
    -- Get current player presence
    local presence = nil
    if app and app.features and app.features.friends then
        presence = app.features.friends:queryPlayerPresence()
    end
    
    local presenceStatus = prefs.presenceStatus or "online"
    local shareLocation = prefs.shareLocation ~= false
    local shareJobWhenAnonymous = prefs.shareJobWhenAnonymous or false
    
    imgui.TextDisabled("Preview: How friends see you")
    imgui.Separator()
    
    if presenceStatus == "invisible" then
        -- Invisible: friends see you as offline
        imgui.BeginGroup()
        imgui.TextDisabled("Status:")
        imgui.SameLine(80)
        imgui.TextColored({0.5, 0.5, 0.5, 1.0}, "Offline")
        imgui.EndGroup()
        
        imgui.TextDisabled("(You appear completely offline to friends)")
    else
        -- Online or Away
        local statusColor = presenceStatus == "away" and {1.0, 0.7, 0.2, 1.0} or {0.4, 0.9, 0.4, 1.0}
        local statusText = presenceStatus == "away" and "Away" or "Online"
        
        imgui.BeginGroup()
        imgui.TextDisabled("Status:")
        imgui.SameLine(80)
        imgui.TextColored(statusColor, statusText)
        imgui.EndGroup()
        
        -- Character name
        local charName = "Your Character"
        if presence and presence.characterName and presence.characterName ~= "" then
            charName = presence.characterName:sub(1,1):upper() .. presence.characterName:sub(2)
        end
        
        imgui.TextDisabled("Name:")
        imgui.SameLine(80)
        imgui.Text(charName)
        
        -- Job (depends on anonymous status and shareJobWhenAnonymous)
        local jobVisible = true
        local jobText = "Unknown"
        if presence then
            if presence.isAnonymous and not shareJobWhenAnonymous then
                jobVisible = false
                jobText = "Anonymous"
            elseif presence.job and presence.job ~= "" then
                jobText = presence.job
            end
        end
        
        imgui.TextDisabled("Job:")
        imgui.SameLine(80)
        if jobVisible then
            imgui.Text(jobText)
        else
            imgui.TextColored({0.4, 0.65, 0.85, 1.0}, jobText)
        end
        
        -- Nation/Rank (depends on anonymous status and shareJobWhenAnonymous)
        local nationVisible = true
        local nationText = "Unknown"
        if presence then
            if presence.isAnonymous and not shareJobWhenAnonymous then
                nationVisible = false
                nationText = "Anonymous"
            else
                local nationName = nations.getDisplayName(presence.nation, "Unknown")
                local rankText = presence.rank or ""
                if rankText ~= "" then
                    nationText = nationName .. " " .. rankText
                else
                    nationText = nationName
                end
            end
        end
        
        imgui.TextDisabled("Nation:")
        imgui.SameLine(80)
        if nationVisible then
            imgui.Text(nationText)
        else
            imgui.TextColored({0.4, 0.65, 0.85, 1.0}, nationText)
        end
        
        -- Zone (depends on shareLocation)
        imgui.TextDisabled("Zone:")
        imgui.SameLine(80)
        if shareLocation then
            local zoneText = "Unknown"
            if presence and presence.zone and presence.zone ~= "" then
                zoneText = presence.zone
            end
            imgui.Text(zoneText)
        else
            imgui.TextColored({0.4, 0.65, 0.85, 1.0}, "Anonymous")
        end
    end
end

function M.RenderBlockedPlayersSection(state, dataModule, callbacks)
    local headerLabel = "Blocked Players"
    local blockedCount = dataModule.GetBlockedPlayersCount()
    if blockedCount > 0 then
        headerLabel = headerLabel .. " (" .. blockedCount .. ")"
    end
    
    local isOpen = imgui.CollapsingHeader(headerLabel, state.blockedPlayersExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.blockedPlayersExpanded then
        state.blockedPlayersExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
    end
    
    if not isOpen then return end
    
    local blocked = dataModule.GetBlockedPlayers()
    
    if #blocked == 0 then
        imgui.TextDisabled("No blocked players.")
        imgui.TextWrapped("You can block players from the Requests tab when they send you a friend request.")
    else
        imgui.TextWrapped("Blocked players cannot send you friend requests.")
        imgui.Spacing()
        
        imgui.BeginChild("##blocked_players_list", {0, 150}, true)
        
        for i, entry in ipairs(blocked) do
            imgui.PushID("blocked_" .. i)
            
            local displayName = entry.displayName or "Unknown"
            if #displayName > 0 then
                displayName = string.upper(string.sub(displayName, 1, 1)) .. string.sub(displayName, 2)
            end
            
            imgui.AlignTextToFramePadding()
            imgui.Text(displayName)
            
            imgui.SameLine()
            if imgui.Button("Unblock") then
                if callbacks.onUnblockPlayer then
                    callbacks.onUnblockPlayer(entry.accountId)
                end
            end
            
            imgui.PopID()
        end
        
        imgui.EndChild()
    end
end

function M.RenderAltVisibilitySection(state, dataModule, callbacks)
    local headerLabel = "Alt Online Visibility"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.altVisibilityExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.altVisibilityExpanded then
        state.altVisibilityExpanded = isOpen
        if callbacks.onSaveState then callbacks.onSaveState() end
        
        if isOpen and callbacks.onRefreshAltVisibility then
            callbacks.onRefreshAltVisibility()
        end
    end
    
    if isOpen then
        local app = _G.FFXIFriendListApp
        if app and app.features and app.features.altVisibility then
            local altVis = app.features.altVisibility
            local visState = altVis:getState()
            
            if visState.pendingRefresh then
                altVis:checkPendingRefresh()
            elseif #altVis:getRows() == 0 and not visState.isLoading and not visState.lastError and not visState.hasAttemptedRefresh then
                if callbacks.onRefreshAltVisibility then
                    callbacks.onRefreshAltVisibility()
                end
            end
        end
        
        M.RenderAltVisibilityContent(state, dataModule, callbacks)
    end
end

function M.RenderAltVisibilityContent(state, dataModule, callbacks)
    local app = _G.FFXIFriendListApp
    local prefs = nil
    if app and app.features and app.features.preferences then
        prefs = app.features.preferences:getPrefs()
    end
    
    if not prefs then
        imgui.Text("Preferences not available")
        return
    end
    
    local shareFriendsAcrossAlts = {prefs.shareFriendsAcrossAlts ~= false}
    if imgui.Checkbox("Share Visibility of Alts to all Friends", shareFriendsAcrossAlts) then
        if app and app.features and app.features.preferences then
            app.features.preferences:setPref("shareFriendsAcrossAlts", shareFriendsAcrossAlts[1])
            app.features.preferences:save()
            app.features.preferences:syncToServer()
            if callbacks.onRefreshAltVisibility then
                callbacks.onRefreshAltVisibility()
            end
        end
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip("When enabled, all your alt characters share visibility with all friends.\nWhen disabled, you can set visibility per friend per character\nusing the table below.\n\n[Account-wide: Synced to server for all characters]")
    end
    
    imgui.Spacing()
    
    if shareFriendsAcrossAlts[1] then
        imgui.Text("A table will appear below if you disable sharing to manage visibility per friend per character.")
    else
        M.RenderAltVisibilityFilter(state)
        imgui.Spacing()
        M.RenderAltVisibilityTable(state, callbacks)
    end
end

function M.RenderAltVisibilityFilter(state)
    if not state.altVisibilityFilterText then
        state.altVisibilityFilterText = {""}
    end
    
    imgui.Text("Search:")
    imgui.SameLine()
    imgui.SetNextItemWidth(200)
    imgui.InputText("##alt_visibility_filter", state.altVisibilityFilterText, 256)
end

function M.RenderAltVisibilityTable(state, callbacks)
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.altVisibility then
        imgui.Text("Alt visibility feature not available")
        return
    end
    
    local altVis = app.features.altVisibility
    local visState = altVis:getState()
    
    if visState.isLoading then
        if visState.pendingRefresh then
            imgui.Text("Waiting for connection...")
        else
            imgui.Text("Loading...")
        end
        return
    end
    
    if visState.lastError then
        imgui.TextColored({1.0, 0.0, 0.0, 1.0}, "Error: " .. tostring(visState.lastError))
        if imgui.Button("Retry") then
            if callbacks.onRefreshAltVisibility then
                callbacks.onRefreshAltVisibility()
            end
        end
        return
    end
    
    local characters = altVis:getCharacters()
    local filterText = state.altVisibilityFilterText and state.altVisibilityFilterText[1] or ""
    local rows = altVis:getFilteredRows(filterText)
    local allRows = altVis:getRows()
    
    if #allRows == 0 then
        if not visState.hasAttemptedRefresh and not visState.isLoading then
            imgui.Text("Loading visibility data...")
            if callbacks.onRefreshAltVisibility then
                callbacks.onRefreshAltVisibility()
            end
        elseif visState.isLoading then
            imgui.Text("Loading visibility data...")
        else
            imgui.TextColored({0.6, 0.8, 1.0, 1.0}, "No friends to configure visibility for.")
            imgui.TextDisabled("Add friends first, then manage which of your characters can see them here.")
            imgui.Spacing()
            if imgui.Button("Refresh") then
                if callbacks.onRefreshAltVisibility then
                    callbacks.onRefreshAltVisibility()
                end
            end
        end
        return
    end
    
    if #characters == 0 then
        imgui.Text("No characters found")
        return
    end
    
    if #rows == 0 then
        imgui.Text("No friends match filter")
        return
    end
    
    imgui.TextDisabled("Note: A '-' means visibility is inherited from the character that added the friend.")
    imgui.Spacing()
    
    local numColumns = 1 + #characters
    local tableFlags = bit.bor(
        ImGuiTableFlags_Borders,
        ImGuiTableFlags_RowBg,
        ImGuiTableFlags_ScrollY
    )
    
    if imgui.BeginTable("##alt_visibility_table", numColumns, tableFlags, {0, 200}) then
        imgui.TableSetupColumn("Friend", ImGuiTableColumnFlags_WidthFixed, 150)
        for _, charInfo in ipairs(characters) do
            local charName = charInfo.characterName or "Unknown"
            if #charName > 0 then
                charName = string.upper(string.sub(charName, 1, 1)) .. string.sub(charName, 2)
            end
            imgui.TableSetupColumn(charName, ImGuiTableColumnFlags_WidthFixed, 100)
        end
        
        imgui.TableHeadersRow()
        
        for _, row in ipairs(rows) do
            imgui.TableNextRow()
            
            imgui.TableNextColumn()
            local displayName = row.friendedAsName or ""
            if #displayName > 0 then
                displayName = string.upper(string.sub(displayName, 1, 1)) .. string.sub(displayName, 2)
            end
            imgui.Text(displayName)
            
            for i, charVis in ipairs(row.characterVisibility) do
                imgui.TableNextColumn()
                
                local shouldShow = altVis:shouldShowCheckbox(row, charVis)
                
                if shouldShow then
                    local isChecked = altVis:isCheckboxChecked(charVis)
                    local isEnabled = altVis:isCheckboxEnabled(charVis)
                    local checkboxId = "##vis_" .. tostring(row.friendAccountId) .. "_" .. tostring(charVis.characterId)
                    
                    if not isEnabled then
                        imgui.PushStyleColor(ImGuiCol_CheckMark, {0.5, 0.5, 0.5, 1.0})
                        imgui.PushStyleColor(ImGuiCol_FrameBg, {0.2, 0.2, 0.2, 1.0})
                    end
                    
                    local checked = {isChecked}
                    if imgui.Checkbox(checkboxId, checked) then
                        if isEnabled then
                            altVis:toggleVisibility(
                                row.friendAccountId,
                                row.friendedAsName,
                                charVis.characterId,
                                checked[1]
                            )
                        end
                    end
                    
                    if not isEnabled then
                        imgui.PopStyleColor(2)
                        
                        if charVis.visibilityState == "PendingRequest" then
                            if imgui.IsItemHovered() then
                                imgui.SetTooltip("Visibility request pending")
                            end
                        elseif charVis.isBusy then
                            if imgui.IsItemHovered() then
                                imgui.SetTooltip("Updating...")
                            end
                        end
                    end
                else
                    imgui.TextDisabled("-")
                    if imgui.IsItemHovered() then
                        imgui.SetTooltip("Added from this character")
                    end
                end
            end
        end
        
        imgui.EndTable()
    end
    
    imgui.Spacing()
    if imgui.Button("Refresh") then
        if callbacks.onRefreshAltVisibility then
            callbacks.onRefreshAltVisibility()
        end
    end
end

return M

