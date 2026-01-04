local imgui = require('imgui')
local utils = require('modules.friendlist.components.helpers.utils')

local M = {}

function M.Render(friend, state, callbacks)
    if friend.friendedAs and friend.friendedAs ~= "" and friend.friendedAs ~= friend.name then
        imgui.Text("Friended As: " .. utils.capitalizeName(friend.friendedAs))
        imgui.Separator()
    end
    
    if imgui.MenuItem("View Details") then
        state.selectedFriendForDetails = friend
    end
    
    imgui.Separator()
    
    if imgui.MenuItem("Remove Friend") then
        if callbacks.onRemoveFriend then
            callbacks.onRemoveFriend(friend.name)
        end
    end
end

return M
