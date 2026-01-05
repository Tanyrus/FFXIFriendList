local imgui = require('imgui')
local UIConstants = require('core.UIConstants')

local M = {}

function M.Render(state, onTabChange, onSaveState)
    local tabs = {"Friends", "General", "Tags", "Notifications", "Controls", "Themes"}
    
    imgui.PushStyleVar(ImGuiStyleVar_FramePadding, UIConstants.FRAME_PADDING)
    
    for i = 0, #tabs - 1 do
        local isSelected = (state.selectedTab == i)
        local tabLabel = tabs[i + 1]
        
        if isSelected then
            imgui.PushStyleColor(ImGuiCol_Button, {0.2, 0.5, 0.8, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.6, 0.9, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.1, 0.4, 0.7, 1.0})
        end
        
        if imgui.Button(tabLabel .. "##tab_" .. i, UIConstants.BUTTON_SIZE) then
            state.selectedTab = i
            if onTabChange then
                onTabChange(i)
            end
            if onSaveState then
                onSaveState()
            end
        end
        
        if isSelected then
            imgui.PopStyleColor(3)
        end
    end
    
    imgui.PopStyleVar()
end

return M
