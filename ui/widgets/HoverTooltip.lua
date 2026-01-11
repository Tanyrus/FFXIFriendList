local imgui = require('imgui')
local utils = require('modules.friendlist.components.helpers.utils')
local icons = require('libs.icons')
local tagcore = require('core.tagcore')

local M = {}

local TOOLTIP_CONSTANTS = {
    LABEL_WIDTH = 95,
    VALUE_UNKNOWN = "Unknown",
    VALUE_ANON = "Anon",
}

local NATION_ICON_NAMES = {
    [0] = "nation_sandoria",
    [1] = "nation_bastok",
    [2] = "nation_windurst",
    -- String keys for server-side string values
    ["San d'Oria"] = "nation_sandoria",
    ["Bastok"] = "nation_bastok",
    ["Windurst"] = "nation_windurst"
}

local function getValueOrAnon(val)
    if val == nil then return TOOLTIP_CONSTANTS.VALUE_ANON end
    if type(val) == "function" then return TOOLTIP_CONSTANTS.VALUE_ANON end
    if type(val) == "string" and val == "" then return TOOLTIP_CONSTANTS.VALUE_ANON end
    return val
end

local function getValueOrUnknown(val)
    if val == nil then return TOOLTIP_CONSTANTS.VALUE_UNKNOWN end
    if type(val) == "function" then return TOOLTIP_CONSTANTS.VALUE_UNKNOWN end
    if type(val) == "string" and val == "" then return TOOLTIP_CONSTANTS.VALUE_UNKNOWN end
    return val
end

function M.Render(friend, settings, forceAll)
    if not friend then return end
    
    local presence = friend.presence or {}
    local isOnline = friend.isOnline or false
    local showAll = forceAll or false
    
    imgui.BeginTooltip()
    
    imgui.Text(utils.capitalizeName(friend.name or TOOLTIP_CONSTANTS.VALUE_UNKNOWN))
    imgui.Separator()
    
    if showAll or (settings and settings.showJob) then
        local job = getValueOrAnon(presence.job)
        imgui.TextDisabled("Job:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        imgui.Text(tostring(job))
    end
    
    if showAll or (settings and settings.showZone) then
        local zone = getValueOrAnon(presence.zone)
        imgui.TextDisabled("Zone:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        imgui.Text(tostring(zone))
    end
    
    if showAll or (settings and settings.showNationRank) then
        local nationIconName = NATION_ICON_NAMES[presence.nation]
        
        imgui.TextDisabled("Nation:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        
        -- If nation is hidden, show Anon (ignore rank)
        if not nationIconName then
            imgui.Text(TOOLTIP_CONSTANTS.VALUE_ANON)
        else
            -- Nation is visible - show icon and rank
            icons.RenderIcon(nationIconName, 14, 14)
            
            local rank = presence.rank or ""
            if type(rank) ~= "string" then rank = tostring(rank) end
            local rankNum = rank:match("%d+") or ""
            
            if rankNum ~= "" and rankNum ~= "0" then
                imgui.SameLine(0, 4)
                imgui.Text("Rank " .. rankNum)
            end
        end
    end
    
    if showAll or (settings and settings.showLastSeen) then
        imgui.TextDisabled("Last Seen:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        if isOnline then
            imgui.Text("Now")
        else
            local lastSeenAt = presence.lastSeenAt or 0
            if type(lastSeenAt) == "number" and lastSeenAt > 0 then
                imgui.Text(utils.formatRelativeTime(lastSeenAt))
            else
                imgui.Text(TOOLTIP_CONSTANTS.VALUE_UNKNOWN)
            end
        end
    end
    
    if showAll or (settings and settings.showFriendedAs) then
        local friendedAs = friend.friendedAs or friend.friendedAsName
        imgui.TextDisabled("Added As:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        if friendedAs and friendedAs ~= "" then
            imgui.Text(utils.capitalizeName(friendedAs))
        else
            imgui.Text(TOOLTIP_CONSTANTS.VALUE_UNKNOWN)
        end
    end
    
    local app = _G.FFXIFriendListApp
    local tagsFeature = app and app.features and app.features.tags
    if tagsFeature then
        local friendKey = tagcore.getFriendKey(friend)
        local tag = tagsFeature:getTagForFriend(friendKey)
        imgui.TextDisabled("Tag:")
        imgui.SameLine(TOOLTIP_CONSTANTS.LABEL_WIDTH)
        if tag and tag ~= "" then
            imgui.Text(tagcore.capitalizeTag(tag))
        else
            imgui.Text("General")
        end
    end
    
    imgui.EndTooltip()
end

return M

