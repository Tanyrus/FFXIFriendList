local imgui = require('imgui')

local M = {}

function M.Render(state, dataModule, onAddFriend)
    imgui.AlignTextToFramePadding()
    imgui.Text("Name:")
    imgui.SameLine()
    imgui.PushItemWidth(150)
    imgui.InputText("##new_friend_name", state.newFriendName, 64)
    imgui.PopItemWidth()
    
    imgui.SameLine()
    imgui.AlignTextToFramePadding()
    imgui.Text("Note:")
    imgui.SameLine()
    imgui.PushItemWidth(250)
    imgui.InputText("##new_friend_note", state.newFriendNote, 256)
    imgui.PopItemWidth()
    
    imgui.SameLine()
    
    local isConnected = dataModule.IsConnected()
    if isConnected then
        if imgui.Button("Add Friend##add_btn") then
            if state.newFriendName[1] and state.newFriendName[1] ~= "" then
                if onAddFriend then
                    local note = state.newFriendNote[1] or ""
                    onAddFriend(state.newFriendName[1], note)
                    state.newFriendName[1] = ""
                    state.newFriendNote[1] = ""
                end
            end
        end
    else
        imgui.PushStyleColor(ImGuiCol_Button, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.3, 0.3, 1.0})
        imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.3, 0.3, 0.3, 1.0})
        imgui.Button("Add Friend##add_btn")
        imgui.PopStyleColor(3)
    end
end

return M
