local imgui = require('imgui')
local tagcore = require('core.tagcore')
local ServerProfiles = require('core.ServerProfiles')

local M = {}

local addState = {
    tagInput = {""},
    realmInput = {""},
    selectedRealmIndex = {0}
}

-- ImGui flag for InputText to return true when Enter is pressed
local ENTER_RETURNS_TRUE = 32 -- ImGuiInputTextFlags_EnterReturnsTrue

-- Helper function to submit the add friend form
local function submitAddFriend(state, tagsFeature, onAddFriend, onAddFriendWithRealm)
    if not state.newFriendName[1] or state.newFriendName[1] == "" then
        return
    end
    
    local app = _G.FFXIFriendListApp
    local notesFeature = app and app.features and app.features.notes
    local note = state.newFriendNote[1] or ""
    
    -- Check if cross-server and realm is selected
    local crossServerEnabled = gConfig and gConfig.crossServerFriendsEnabled
    local selectedRealmId = nil
    
    if crossServerEnabled and addState.selectedRealmIndex[1] > 0 then
        local profiles = ServerProfiles.getAll()
        if profiles and profiles[addState.selectedRealmIndex[1]] then
            selectedRealmId = profiles[addState.selectedRealmIndex[1]].id
        end
    end
    
    -- Call appropriate function based on whether realm is specified
    if selectedRealmId and onAddFriendWithRealm then
        onAddFriendWithRealm(state.newFriendName[1], selectedRealmId, note)
    elseif onAddFriend then
        onAddFriend(state.newFriendName[1], note)
    else
        return
    end
    
    -- Set pending note if provided
    if notesFeature and note ~= "" and notesFeature.setPendingNote then
        notesFeature:setPendingNote(state.newFriendName[1], note)
    end
    
    -- Set pending tag if provided
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
    addState.selectedRealmIndex[1] = 0
end

function M.Render(state, dataModule, onAddFriend, onAddFriendWithRealm)
    local app = _G.FFXIFriendListApp
    local tagsFeature = app and app.features and app.features.tags
    local isConnected = dataModule.IsConnected()
    local crossServerEnabled = gConfig and gConfig.crossServerFriendsEnabled
    
    -- Track if Enter was pressed in any input field
    local enterPressed = false
    
    imgui.AlignTextToFramePadding()
    imgui.Text("Name:")
    imgui.SameLine()
    imgui.PushItemWidth(120)
    if imgui.InputText("##new_friend_name", state.newFriendName, 64, ENTER_RETURNS_TRUE) then
        enterPressed = true
    end
    imgui.PopItemWidth()
    
    -- Show realm selector when cross-server friends are enabled
    if crossServerEnabled and ServerProfiles.isLoaded() then
        imgui.SameLine()
        imgui.AlignTextToFramePadding()
        imgui.Text("Realm:")
        imgui.SameLine()
        imgui.PushItemWidth(100)
        
        -- Build realm list for combo
        local profiles = ServerProfiles.getAll()
        local realmNames = {"(Current)"}
        for _, profile in ipairs(profiles) do
            table.insert(realmNames, profile.name or profile.id)
        end
        local realmNamesStr = table.concat(realmNames, "\0") .. "\0"
        
        if imgui.Combo("##new_friend_realm", addState.selectedRealmIndex, realmNamesStr) then
            -- Selection changed
        end
        imgui.PopItemWidth()
        if imgui.IsItemHovered() then
            imgui.SetTooltip("Select a realm to add a friend from a different server.\nLeave as '(Current)' to use your current realm.")
        end
    end
    
    imgui.SameLine()
    imgui.AlignTextToFramePadding()
    imgui.Text("Note:")
    imgui.SameLine()
    imgui.PushItemWidth(150)
    if imgui.InputText("##new_friend_note", state.newFriendNote, 256, ENTER_RETURNS_TRUE) then
        enterPressed = true
    end
    imgui.PopItemWidth()
    
    if tagsFeature then
        imgui.SameLine()
        imgui.AlignTextToFramePadding()
        imgui.Text("Tag:")
        imgui.SameLine()
        imgui.PushItemWidth(100)
        if imgui.InputText("##new_friend_tag", addState.tagInput, 32, ENTER_RETURNS_TRUE) then
            enterPressed = true
        end
        imgui.PopItemWidth()
        if imgui.IsItemHovered() then
            imgui.SetTooltip("Enter a tag name (leave empty for no tag)")
        end
    end
    
    imgui.SameLine()
    
    if isConnected then
        -- Submit on Enter key or button click
        if enterPressed or imgui.Button("Add Friend##add_btn") then
            submitAddFriend(state, tagsFeature, onAddFriend, onAddFriendWithRealm)
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
