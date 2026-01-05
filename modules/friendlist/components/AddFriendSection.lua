local imgui = require('imgui')
local tagcore = require('core.tagcore')

local M = {}

local addState = {
    tagInput = {""}
}

function M.Render(state, dataModule, onAddFriend)
    local app = _G.FFXIFriendListApp
    local tagsFeature = app and app.features and app.features.tags
    
    imgui.AlignTextToFramePadding()
    imgui.Text("Name:")
    imgui.SameLine()
    imgui.PushItemWidth(120)
    imgui.InputText("##new_friend_name", state.newFriendName, 64)
    imgui.PopItemWidth()
    
    imgui.SameLine()
    imgui.AlignTextToFramePadding()
    imgui.Text("Note:")
    imgui.SameLine()
    imgui.PushItemWidth(150)
    imgui.InputText("##new_friend_note", state.newFriendNote, 256)
    imgui.PopItemWidth()
    
    if tagsFeature then
        imgui.SameLine()
        imgui.AlignTextToFramePadding()
        imgui.Text("Tag:")
        imgui.SameLine()
        imgui.PushItemWidth(100)
        imgui.InputText("##new_friend_tag", addState.tagInput, 32)
        imgui.PopItemWidth()
        if imgui.IsItemHovered() then
            imgui.SetTooltip("Enter a tag name (leave empty for no tag)")
        end
    end
    
    imgui.SameLine()
    
    local isConnected = dataModule.IsConnected()
    if isConnected then
        if imgui.Button("Add Friend##add_btn") then
            if state.newFriendName[1] and state.newFriendName[1] ~= "" then
                if onAddFriend then
                    local note = state.newFriendNote[1] or ""
                    onAddFriend(state.newFriendName[1], note)
                    
                    if tagsFeature then
                        local inputTag = tagcore.normalizeTag(addState.tagInput[1])
                        if inputTag then
                            local tagOrder = tagsFeature:getAllTags() or {}
                            local existingTag = tagcore.findExistingTag(tagOrder, inputTag)
                            local tagToUse = existingTag or tagcore.capitalizeTag(inputTag)
                            tagsFeature:setPendingTag(state.newFriendName[1], tagToUse)
                        end
                    end
                    
                    state.newFriendName[1] = ""
                    state.newFriendNote[1] = ""
                    addState.tagInput[1] = ""
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
