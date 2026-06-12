local imgui = require('imgui')
local icons = require('libs.icons')
local utils = require('modules.friendlist.components.helpers.utils')
local InputHelper = require('ui.helpers.InputHelper')
local HoverTooltip = require('ui.widgets.HoverTooltip')
local PresenceStatusPicker = require('ui.widgets.PresenceStatusPicker')
local tagcore = require('core.tagcore')
local nations = require('core.nations')
local IconText = require('ui.helpers.IconText')
local Colors = require('constants.colors')

local M = {}

local DRAG_DROP_TYPE = "FRIEND_RETAG"

local dragState = {
    isDragging = false,
    friendKey = nil,
    friendName = nil,
    hoveredTag = nil,
    dropPending = false
}

function M.getDragState()
    return dragState
end

function M.isDraggingFriend()
    return dragState.isDragging
end

function M.setHoveredTag(tag)
    if dragState.isDragging then
        dragState.hoveredTag = tag
    end
end

function M.clearDragState()
    dragState.isDragging = false
    dragState.friendKey = nil
    dragState.friendName = nil
    dragState.hoveredTag = nil
    dragState.dropPending = false
end

function M.endDrag()
    if dragState.isDragging and dragState.hoveredTag ~= nil then
        dragState.dropPending = true
        return true
    end
    M.clearDragState()
    return false
end

function M.consumePendingDrop()
    if dragState.dropPending then
        local friendKey = dragState.friendKey
        local targetTag = dragState.hoveredTag
        M.clearDragState()
        return friendKey, targetTag
    end
    return nil, nil
end

function M.RenderNameCell(friend, index, state, callbacks, sectionTag)
    local friendName = utils.capitalizeName(utils.getDisplayName(friend))
    local isOnline = friend.isOnline or false
    local isAway = friend.isAway or false
    local isPending = friend.isPending or false
    local uniqueId = (sectionTag or "main") .. "_" .. index
    
    -- Check if friend is muted
    local isMuted = false
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences and friend.friendAccountId then
        isMuted = app.features.preferences:isFriendMuted(friend.friendAccountId)
    end
    
    if not icons.RenderStatusIcon(isOnline, isPending, 16, isAway) then
        local color = isPending and {1.0, 0.8, 0.0, 1.0} or 
                     isAway and {1.0, 0.7, 0.2, 1.0} or
                     isOnline and {0.0, 1.0, 0.0, 1.0} or 
                     {0.5, 0.5, 0.5, 1.0}
        imgui.TextColored(color, isPending and "?" or isAway and "A" or isOnline and "O" or "-")
    end
    imgui.SameLine()
    
    if not isOnline and not isPending then
        imgui.PushStyleColor(ImGuiCol_Text, {0.6, 0.6, 0.6, 1.0})
    end
    
    -- Render name with mute indicator to the right
    local displayText = friendName
    if isMuted then
        displayText = friendName .. " (M)"
    end
    
    local flags = ImGuiSelectableFlags_SpanAllColumns
    if imgui.Selectable(displayText .. "##friend_" .. uniqueId, false, flags) then
        if state.selectedFriendForDetails and state.selectedFriendForDetails.name == friend.name then
            state.selectedFriendForDetails = nil
        else
            state.selectedFriendForDetails = friend
        end
    end
    
    -- Tooltip for muted indicator
    if isMuted and imgui.IsItemHovered() then
        imgui.SetTooltip("Notifications muted for this friend")
    end
    
    if imgui.BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID) then
        local friendKey = tagcore.getFriendKey(friend)
        if friendKey then
            dragState.isDragging = true
            dragState.friendKey = friendKey
            dragState.friendName = friendName
            imgui.Text("Move: " .. friendName)
        end
        imgui.EndDragDropSource()
    elseif dragState.isDragging and dragState.friendKey == tagcore.getFriendKey(friend) then
        M.endDrag()
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
    
    if imgui.BeginPopupContextItem("##friend_context_" .. uniqueId) then
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
    
    local anonColor = Colors.STATUS.ANONYMOUS
    local anonColorDim = Colors.STATUS.ANONYMOUS_DIM
    
    if jobText == "" then
        if isOnline then
            imgui.TextColored(anonColor, "Anonymous")
        else
            imgui.TextColored(anonColorDim, "Anonymous")
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
    
    local anonColor = Colors.STATUS.ANONYMOUS
    local anonColorDim = Colors.STATUS.ANONYMOUS_DIM
    
    if zoneText == "" then
        if isOnline then
            imgui.TextColored(anonColor, "Anonymous")
        else
            imgui.TextColored(anonColorDim, "Anonymous")
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
    -- Convert rank to string if it's a number (server sends numeric rank)
    if type(rank) == "number" then
        rank = tostring(rank)
    elseif type(rank) ~= "string" then
        rank = ""
    end
    
    local rankNum = rank:match("%d+") or ""
    local anonColor = Colors.STATUS.ANONYMOUS
    local anonColorDim = Colors.STATUS.ANONYMOUS_DIM
    
    -- Use nations module for icon lookup (supports both numeric and string values)
    local nationIcon = nations.getIconName(nation)
    
    if nation == nil or nation == -1 or (rankNum == "" and not nationIcon) then
        if isOnline then
            imgui.TextColored(anonColor, "Anonymous")
        else
            imgui.TextColored(anonColorDim, "Anonymous")
        end
    else
        if nationIcon and rankNum ~= "" then
            -- Render icon with proper vertical centering
            IconText.renderIconAligned(nationIcon, IconText.ICON_SIZE.MEDIUM, IconText.SPACING.NORMAL)
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
            -- Has nation but no rank - show nation name
            local nationName = nations.getDisplayName(nation, "Anonymous")
            if isOnline then
                imgui.Text(nationName)
            else
                imgui.TextDisabled(nationName)
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
        local lastSeenRaw = presence.lastSeenAt
        -- Normalize to handle ISO8601 strings from server
        local lastSeenAt = utils.normalizeLastSeen(lastSeenRaw)
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
    
    local displayName = utils.capitalizeName(friendedAs)
    
    if displayName == "" then
        imgui.TextDisabled("-")
    elseif isOnline then
        imgui.Text(displayName)
    else
        imgui.TextDisabled(displayName)
    end
end



function M.RenderGroupTable(friends, state, callbacks, sectionTag)
    if not friends or #friends == 0 then
        return
    end
    
    local columnOrder = state.columnOrder or {"Name", "Job", "Zone", "Nation/Rank", "Last Seen", "Added As"}
    local columnWidths = state.columnWidths or {}
    
    local columnVisibilityMap = {
        Name = true,
        Job = state.columnVisible and state.columnVisible.job,
        Zone = state.columnVisible and state.columnVisible.zone,
        ["Nation/Rank"] = state.columnVisible and state.columnVisible.nationRank,
        ["Last Seen"] = state.columnVisible and state.columnVisible.lastSeen,
        ["Added As"] = state.columnVisible and state.columnVisible.addedAs
    }
    
    local visibleColumns = {}
    for _, colName in ipairs(columnOrder) do
        if columnVisibilityMap[colName] then
            table.insert(visibleColumns, colName)
        end
    end
    
    local tableId = "##friends_table_" .. (sectionTag or "default")
    if imgui.BeginTable(tableId, #visibleColumns, 0) then
        for _, colName in ipairs(visibleColumns) do
            local width = columnWidths[colName] or 100.0
            imgui.TableSetupColumn(colName, ImGuiTableColumnFlags_WidthFixed, width)
        end
        
        for i, friend in ipairs(friends) do
            imgui.TableNextRow()
            
            for _, colName in ipairs(visibleColumns) do
                imgui.TableNextColumn()
                
                if colName == "Name" then
                    M.RenderNameCell(friend, i, state, callbacks, sectionTag)
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

return M
