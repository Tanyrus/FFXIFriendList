local imgui = require('imgui')
local icons = require('libs.icons')
local utils = require('modules.friendlist.components.helpers.utils')
local InputHelper = require('ui.helpers.InputHelper')
local HoverTooltip = require('ui.widgets.HoverTooltip')

local M = {}

local function sortFriends(friends, sortColumn, sortDirection)
    table.sort(friends, function(a, b)
        local aOnline = a.isOnline and 1 or 0
        local bOnline = b.isOnline and 1 or 0
        if aOnline ~= bOnline then
            return aOnline > bOnline
        end
        
        local aVal, bVal
        if sortColumn == "name" then
            aVal = type(a.name) == "string" and string.lower(a.name) or ""
            bVal = type(b.name) == "string" and string.lower(b.name) or ""
        elseif sortColumn == "job" then
            local aJob = (a.presence or {}).job
            local bJob = (b.presence or {}).job
            aVal = type(aJob) == "string" and string.lower(aJob) or ""
            bVal = type(bJob) == "string" and string.lower(bJob) or ""
        elseif sortColumn == "zone" then
            local aZone = (a.presence or {}).zone
            local bZone = (b.presence or {}).zone
            aVal = type(aZone) == "string" and string.lower(aZone) or ""
            bVal = type(bZone) == "string" and string.lower(bZone) or ""
        elseif sortColumn == "nationRank" then
            aVal = (a.presence or {}).nation or -1
            bVal = (b.presence or {}).nation or -1
        elseif sortColumn == "lastSeen" then
            local aLast = (a.presence or {}).lastSeenAt
            local bLast = (b.presence or {}).lastSeenAt
            aVal = type(aLast) == "number" and aLast or 0
            bVal = type(bLast) == "number" and bLast or 0
        else
            aVal = type(a.name) == "string" and string.lower(a.name) or ""
            bVal = type(b.name) == "string" and string.lower(b.name) or ""
        end
        
        if sortDirection == "asc" then
            return aVal < bVal
        else
            return aVal > bVal
        end
    end)
    return friends
end

local function filterFriends(friends, filterText)
    if filterText == "" then
        return friends
    end
    
    local filtered = {}
    local lowerFilter = string.lower(filterText)
    
    for _, friend in ipairs(friends) do
        local friendName = type(friend.name) == "string" and string.lower(friend.name) or ""
        local presence = friend.presence or {}
        -- Type check to prevent 'string expected, got function' errors
        local job = type(presence.job) == "string" and string.lower(presence.job) or ""
        local zone = type(presence.zone) == "string" and string.lower(presence.zone) or ""
        if string.find(friendName, lowerFilter, 1, true) or
           string.find(job, lowerFilter, 1, true) or
           string.find(zone, lowerFilter, 1, true) then
            table.insert(filtered, friend)
        end
    end
    
    return filtered
end

function M.Render(state, dataModule, callbacks)
    local friends = dataModule.GetFriends()
    local filterText = state.filterText[1] or ""
    
    local filteredFriends = filterFriends(friends, filterText)
    filteredFriends = sortFriends(filteredFriends, state.sortColumn, state.sortDirection)
    
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
    
    if #filteredFriends == 0 then
        imgui.Text("No friends" .. (filterText ~= "" and " matching filter" or ""))
        return
    end
    
    local visibleColumns = {}
    table.insert(visibleColumns, "Name")
    if state.columnVisible.job then table.insert(visibleColumns, "Job") end
    if state.columnVisible.zone then table.insert(visibleColumns, "Zone") end
    if state.columnVisible.nationRank then table.insert(visibleColumns, "Nation/Rank") end
    if state.columnVisible.lastSeen then table.insert(visibleColumns, "Last Seen") end
    if state.columnVisible.addedAs then table.insert(visibleColumns, "Added As") end
    
    if imgui.BeginTable("##friends_table", #visibleColumns, ImGuiTableFlags_Borders) then
        for _, colName in ipairs(visibleColumns) do
            local flags = ImGuiTableColumnFlags_WidthFixed
            if colName == "Name" then
                imgui.TableSetupColumn("Name", flags, 120.0)
            elseif colName == "Job" then
                imgui.TableSetupColumn("Job", flags, 100.0)
            elseif colName == "Zone" then
                imgui.TableSetupColumn("Zone", flags, 120.0)
            elseif colName == "Nation/Rank" then
                imgui.TableSetupColumn("Nation/Rank", flags, 80.0)
            elseif colName == "Last Seen" then
                imgui.TableSetupColumn("Last Seen", flags, 120.0)
            elseif colName == "Added As" then
                imgui.TableSetupColumn("Added As", flags, 100.0)
            end
        end
        imgui.TableHeadersRow()
        
        -- Open context menu on right-click anywhere in the table (for column visibility)
        if imgui.BeginPopupContextWindow("##table_column_context", 1) then  -- 1 = right mouse button only on items
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
            
            imgui.EndPopup()
        end
        
        for i, friend in ipairs(filteredFriends) do
            imgui.TableNextRow()
            
            for _, colName in ipairs(visibleColumns) do
                imgui.TableNextColumn()
                
                if colName == "Name" then
                    M.RenderNameCell(friend, i, state, callbacks)
                elseif colName == "Job" then
                    M.RenderJobCell(friend)
                elseif colName == "Zone" then
                    M.RenderZoneCell(friend)
                elseif colName == "Nation/Rank" then
                    M.RenderNationRankCell(friend)
                elseif colName == "Last Seen" then
                    M.RenderLastSeenCell(friend)
                elseif colName == "Added As" then
                    M.RenderAddedAsCell(friend)
                end
            end
        end
        
        imgui.EndTable()
    end
end

function M.RenderNameCell(friend, index, state, callbacks)
    local friendName = utils.capitalizeName(utils.getDisplayName(friend))
    local isOnline = friend.isOnline or false
    local isPending = friend.isPending or false
    
    if not icons.RenderStatusIcon(isOnline, isPending, 12) then
        local color = isPending and {1.0, 0.8, 0.0, 1.0} or 
                     isOnline and {0.0, 1.0, 0.0, 1.0} or 
                     {0.5, 0.5, 0.5, 1.0}
        imgui.TextColored(color, isPending and "?" or isOnline and "O" or "-")
    end
    imgui.SameLine()
    
    if not isOnline and not isPending then
        imgui.PushStyleColor(ImGuiCol_Text, {0.6, 0.6, 0.6, 1.0})
    end
    
    local flags = ImGuiSelectableFlags_SpanAllColumns
    if imgui.Selectable(friendName .. "##friend_" .. index, false, flags) then
        -- Toggle - clicking same friend again closes details
        if state.selectedFriendForDetails and state.selectedFriendForDetails.name == friend.name then
            state.selectedFriendForDetails = nil
        else
            state.selectedFriendForDetails = friend
        end
    end
    
    local itemHovered = imgui.IsItemHovered()
    
    if not isOnline and not isPending then
        imgui.PopStyleColor()
    end
    
    if itemHovered then
        local app = _G.FFXIFriendListApp
        local settings = nil
        if app and app.features and app.features.preferences then
            local prefs = app.features.preferences:getPrefs()
            settings = prefs and prefs.mainHoverTooltip
        end
        
        local forceAll = InputHelper.isShiftDown()
        HoverTooltip.Render(friend, settings, forceAll)
    end
    
    if imgui.BeginPopupContextItem("##friend_context_" .. index) then
        if callbacks.onRenderContextMenu then
            callbacks.onRenderContextMenu(friend)
        end
        imgui.EndPopup()
    end
end

function M.RenderJobCell(friend)
    local presence = friend.presence or {}
    local jobText = presence.job
    local isOnline = friend.isOnline or false
    if type(jobText) ~= "string" then
        jobText = ""
    end
    
    local anonColor = {0.4, 0.65, 0.85, 1.0}  -- Dark sky blue
    local anonColorDim = {0.3, 0.45, 0.6, 1.0}  -- Dimmed for offline
    
    if jobText == "" then
        if isOnline then
            imgui.TextColored(anonColor, "Anon")
        else
            imgui.TextColored(anonColorDim, "Anon")
        end
    else
        if isOnline then
            imgui.Text(jobText)
        else
            imgui.TextDisabled(jobText)
        end
    end
end

function M.RenderZoneCell(friend)
    local presence = friend.presence or {}
    local zoneText = presence.zone
    local isOnline = friend.isOnline or false
    if type(zoneText) ~= "string" then
        zoneText = ""
    end
    
    local anonColor = {0.4, 0.65, 0.85, 1.0}  -- Dark sky blue
    local anonColorDim = {0.3, 0.45, 0.6, 1.0}  -- Dimmed for offline
    
    if zoneText == "" then
        if isOnline then
            imgui.TextColored(anonColor, "Anon")
        else
            imgui.TextColored(anonColorDim, "Anon")
        end
    else
        if isOnline then
            imgui.Text(zoneText)
        else
            imgui.TextDisabled(zoneText)
        end
    end
end

function M.RenderNationRankCell(friend)
    local presence = friend.presence or {}
    local nation = presence.nation
    local rank = presence.rank
    local isOnline = friend.isOnline or false
    if type(rank) ~= "string" then rank = "" end
    
    local rankNum = rank:match("%d+") or ""
    local anonColor = {0.4, 0.65, 0.85, 1.0}  -- Dark sky blue
    local anonColorDim = {0.3, 0.45, 0.6, 1.0}  -- Dimmed for offline
    
    if nation == nil or (rankNum == "") then
        if isOnline then
            imgui.TextColored(anonColor, "Anon")
        else
            imgui.TextColored(anonColorDim, "Anon")
        end
    else
        local nationIcons = {
            [0] = "nation_sandoria",
            [1] = "nation_bastok",
            [2] = "nation_windurst"
        }
        local nationIcon = nationIcons[nation]
        
        if nationIcon and rankNum ~= "" then
            if icons.RenderIcon(nationIcon, 12, 12) then
                imgui.SameLine()
            end
            if isOnline then
                imgui.Text("Rank " .. rankNum)
            else
                imgui.TextDisabled("Rank " .. rankNum)
            end
        elseif rankNum ~= "" then
            if isOnline then
                imgui.Text("Rank " .. rankNum)
            else
                imgui.TextDisabled("Rank " .. rankNum)
            end
        else
            if isOnline then
                imgui.TextColored(anonColor, "Anon")
            else
                imgui.TextColored(anonColorDim, "Anon")
            end
        end
    end
end

function M.RenderLastSeenCell(friend)
    local presence = friend.presence or {}
    local isOnline = friend.isOnline or false
    
    if isOnline then
        imgui.Text("Now")
    else
        local lastSeenAt = presence.lastSeenAt
        if type(lastSeenAt) ~= "number" then
            lastSeenAt = 0
        end
        if lastSeenAt > 0 then
            imgui.TextDisabled(utils.formatRelativeTime(lastSeenAt))
        else
            imgui.TextDisabled("Unknown")
        end
    end
end

function M.RenderAddedAsCell(friend)
    local friendedAs = friend.friendedAs or ""
    local isOnline = friend.isOnline or false
    
    -- Capitalize the name for display
    local displayName = utils.capitalizeName(friendedAs)
    
    if displayName == "" then
        imgui.TextDisabled("-")
    elseif isOnline then
        imgui.Text(displayName)
    else
        imgui.TextDisabled(displayName)
    end
end

return M
