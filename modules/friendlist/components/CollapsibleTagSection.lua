local imgui = require('imgui')
local tagcore = require('core.tagcore')
local taggrouper = require('core.taggrouper')
local FriendsTable = require('modules.friendlist.components.FriendsTable')

local M = {}

function M.Render(tagGroup, state, callbacks, renderFriendsTable, idSuffix)
    if not tagGroup then
        return
    end
    
    idSuffix = idSuffix or ""
    
    local tag = tagGroup.tag
    local displayTag = tagGroup.displayTag or tag
    local friends = tagGroup.friends or {}
    local count = #friends
    
    local isUntagged = (tag == "__untagged__")
    local capitalizedTag = isUntagged and displayTag or tagcore.capitalizeTag(displayTag)
    local headerLabel = capitalizedTag .. " (" .. count .. ")"
    
    local app = _G.FFXIFriendListApp
    local tagsFeature = app and app.features and app.features.tags
    local isCollapsed = tagsFeature and tagsFeature:isCollapsed(tag .. idSuffix)
    
    local flags = 0
    if not isCollapsed then
        flags = ImGuiTreeNodeFlags_DefaultOpen
    end
    
    local dragState = FriendsTable.getDragState()
    
    local headerOpen = imgui.CollapsingHeader(headerLabel .. "##tag_" .. tag .. idSuffix, flags)
    
    local hoverFlags = ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
    if dragState.isDragging and imgui.IsItemHovered(hoverFlags) then
        if isUntagged then
            FriendsTable.setHoveredTag("__clear__")
        else
            FriendsTable.setHoveredTag(tag)
        end
    end
    
    local wasOpen = not isCollapsed
    local nowOpen = headerOpen
    if wasOpen ~= nowOpen and tagsFeature then
        tagsFeature:setCollapsed(tag .. idSuffix, not nowOpen)
    end
    
    if headerOpen then
        if count > 0 then
            imgui.BeginGroup()
            
            if renderFriendsTable then
                renderFriendsTable(friends, state, callbacks, tag)
            end
            
            imgui.EndGroup()
            
            if dragState.isDragging and imgui.IsItemHovered(hoverFlags) then
                if isUntagged then
                    FriendsTable.setHoveredTag("__clear__")
                else
                    FriendsTable.setHoveredTag(tag)
                end
            end
        else
            imgui.TextDisabled("  No friends in this group")
        end
    end
end

function M.RenderAllSections(groups, state, callbacks, renderFriendsTable, idSuffix)
    for _, tagGroup in ipairs(groups or {}) do
        M.Render(tagGroup, state, callbacks, renderFriendsTable, idSuffix)
        imgui.Spacing()
    end
    
    local friendKey, targetTag = FriendsTable.consumePendingDrop()
    if friendKey then
        if targetTag == "__clear__" then
            targetTag = nil
        end
        if callbacks and callbacks.onQueueRetag then
            callbacks.onQueueRetag(friendKey, targetTag)
        end
    end
end

return M
