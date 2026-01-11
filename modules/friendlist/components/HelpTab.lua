local imgui = require('imgui')

local M = {}

-- State for character list section
M.state = {
    charactersExpanded = false
}

function M.Render(state, dataModule, callbacks)
    M.RenderWelcomeSection()
    imgui.Spacing()
    M.RenderAccountInfoSection()
    imgui.Spacing()
    M.RenderCommandsSection()
    imgui.Spacing()
    M.RenderFriendRequestsSection()
    imgui.Spacing()
    M.RenderPresenceStatusSection()
    imgui.Spacing()
    M.RenderSettingsSection()
end

function M.RenderWelcomeSection()
    imgui.TextColored({0.4, 0.8, 1.0, 1.0}, "Welcome to FFXIFriendList!")
    imgui.Spacing()
    imgui.TextWrapped("This addon lets you manage a cross-server friend list for Final Fantasy XI. " ..
        "Stay connected with friends across different private servers, see when they're online, and send messages.")
end

function M.RenderAccountInfoSection()
    if imgui.CollapsingHeader("Your Characters") then
        imgui.Indent()
        
        local app = _G.FFXIFriendListApp
        local characters = {}
        local currentCharName = nil
        
        -- Get current character name from connection
        if app and app.features and app.features.connection then
            currentCharName = app.features.connection:getCharacterName()
        end
        
        -- Try to get characters from altVisibility first (it has them after visibility fetch)
        if app and app.features and app.features.altVisibility then
            characters = app.features.altVisibility:getCharacters() or {}
        end
        
        if #characters == 0 then
            -- Show current character at minimum
            if currentCharName and currentCharName ~= "" then
                local displayName = currentCharName:sub(1, 1):upper() .. currentCharName:sub(2):lower()
                imgui.TextColored({0.0, 1.0, 0.0, 1.0}, "* " .. displayName)
                imgui.SameLine()
                imgui.TextDisabled("(current)")
            else
                imgui.TextDisabled("Not connected")
            end
            imgui.Spacing()
            imgui.TextDisabled("Open the Privacy tab's \"Alt Online Visibility\" section to load all your characters.")
        else
            -- Show all characters
            for _, charInfo in ipairs(characters) do
                local charName = charInfo.characterName or "Unknown"
                local displayName = charName:sub(1, 1):upper() .. charName:sub(2):lower()
                
                local isCurrent = currentCharName and charName:lower() == currentCharName:lower()
                local isActive = charInfo.isActive
                
                if isCurrent then
                    imgui.TextColored({0.0, 1.0, 0.0, 1.0}, "* " .. displayName)
                    imgui.SameLine()
                    imgui.TextDisabled("(current)")
                elseif isActive then
                    imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "  " .. displayName)
                    imgui.SameLine()
                    imgui.TextDisabled("(active)")
                else
                    imgui.Text("  " .. displayName)
                end
            end
            
            imgui.Spacing()
            imgui.TextDisabled("Characters linked to your account.")
        end
        
        imgui.Unindent()
    end
end

function M.RenderCommandsSection()
    if imgui.CollapsingHeader("Commands", ImGuiTreeNodeFlags_DefaultOpen) then
        imgui.Indent()
        
        imgui.TextColored({0.9, 0.9, 0.6, 1.0}, "/fl")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.Text("Open the main Friend List window")  
       
        imgui.TextColored({0.9, 0.9, 0.6, 1.0}, "/flist")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.Text("Open the Compact Friend List window")

        imgui.TextColored({0.9, 0.9, 0.6, 1.0}, "/befriend <name> <tag-optional>")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.Text("Sends a friend request to the specified player with an optional tag")

        imgui.Unindent()
    end
end

function M.RenderFriendRequestsSection()
    if imgui.CollapsingHeader("Friend Requests", ImGuiTreeNodeFlags_DefaultOpen) then
        imgui.Indent()
        
        imgui.TextWrapped("To add a friend:")
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextWrapped("Type the character name in the \"Add Friend\" box")
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextWrapped("Click \"Add Friend\" to send them a friend request")
        
        imgui.Spacing()
        
        imgui.TextWrapped("To accept incoming requests:")
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextWrapped("Go to the \"Requests\" tab")
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextWrapped("Click \"Accept\" next to any pending request")
        
        imgui.Spacing()
        
        imgui.TextDisabled("Note: Both players must have the addon installed on the same server.")
        
        imgui.Unindent()
    end
end

function M.RenderPresenceStatusSection()
    if imgui.CollapsingHeader("Presence Status", ImGuiTreeNodeFlags_DefaultOpen) then
        imgui.Indent()
        
        imgui.TextWrapped("Control how you appear to friends in the Privacy tab:")
        imgui.Spacing()
        
        imgui.TextColored({0.0, 1.0, 0.0, 1.0}, "Online")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("You appear online with full status visible")
        
        imgui.TextColored({1.0, 0.7, 0.2, 1.0}, "Away")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("You appear online but marked as Away")
        
        imgui.TextColored({0.5, 0.5, 0.5, 1.0}, "Invisible")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("You appear offline to all friends")
        
        imgui.Unindent()
    end
end

function M.RenderSettingsSection()
    if imgui.CollapsingHeader("Settings & Tips") then
        imgui.Indent()
        
        imgui.TextWrapped("Useful settings can be found in these tabs:")
        imgui.Spacing()
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "General")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Window columns, hover tooltips, and Lock All Windows toggle")
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "Privacy")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Presence status, location sharing, alt visibility")
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "Tags")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Organize friends into groups")
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "Notifications")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Configure friend online alerts and sounds")
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "Controls")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Keyboard and controller bindings")
        
        imgui.Bullet()
        imgui.SameLine()
        imgui.TextColored({0.7, 0.9, 1.0, 1.0}, "Themes")
        imgui.SameLine()
        imgui.TextDisabled(" - ")
        imgui.SameLine()
        imgui.TextWrapped("Customize the look and feel")
        
        imgui.Unindent()
    end
end

return M

