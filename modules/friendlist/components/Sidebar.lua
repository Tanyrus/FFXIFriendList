local imgui = require('imgui')
local FontManager = require('app.ui.FontManager')

local M = {}

function M.Render(state, dataModule, onSaveState)
    local incomingCount = 0
    if dataModule and dataModule.GetIncomingRequestsCount then
        incomingCount = dataModule.GetIncomingRequestsCount()
    end
    
    local requestsLabel = "Requests"
    if incomingCount > 0 then
        requestsLabel = "Requests (" .. incomingCount .. ")"
    end
    
    local tabs = {"Friends", requestsLabel, "General", "Privacy", "Tags", "Notifications", "Controls", "Themes"}
    
    local s = FontManager.scaled
    local framePadding = {s(10), s(8)}
    local buttonSize = {s(164), s(32)}
    
    imgui.PushStyleVar(ImGuiStyleVar_FramePadding, framePadding)
    
    for i = 0, #tabs - 1 do
        local isSelected = (state.selectedTab == i)
        local tabLabel = tabs[i + 1]
        
        if isSelected then
            imgui.PushStyleColor(ImGuiCol_Button, {0.2, 0.5, 0.8, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonHovered, {0.3, 0.6, 0.9, 1.0})
            imgui.PushStyleColor(ImGuiCol_ButtonActive, {0.1, 0.4, 0.7, 1.0})
        end
        
        if imgui.Button(tabLabel .. "##tab_" .. i, buttonSize) then
            state.selectedTab = i
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
