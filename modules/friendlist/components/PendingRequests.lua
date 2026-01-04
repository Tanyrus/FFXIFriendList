local imgui = require('imgui')
local utils = require('modules.friendlist.components.helpers.utils')

local M = {}

function M.Render(state, dataModule, callbacks)
    local incomingRequests = dataModule.GetIncomingRequests()
    local outgoingRequests = dataModule.GetOutgoingRequests()
    
    local headerLabel = "Pending Requests"
    local isOpen = imgui.CollapsingHeader(headerLabel, state.pendingRequestsExpanded and ImGuiTreeNodeFlags_DefaultOpen or 0)
    
    if isOpen ~= state.pendingRequestsExpanded then
        state.pendingRequestsExpanded = isOpen
        if callbacks.onSaveState then
            callbacks.onSaveState()
        end
    end
    
    if not isOpen then
        return
    end
    
    imgui.Text("Incoming:")
    if #incomingRequests > 0 then
        for i, request in ipairs(incomingRequests) do
            imgui.AlignTextToFramePadding()
            imgui.Text(utils.capitalizeName(request.name or "Unknown"))
            imgui.SameLine()
            if imgui.Button("Accept##incoming_" .. i) then
                if callbacks.onAcceptRequest then
                    callbacks.onAcceptRequest(request.id or request.name)
                end
            end
            imgui.SameLine()
            if imgui.Button("Reject##incoming_" .. i) then
                if callbacks.onRejectRequest then
                    callbacks.onRejectRequest(request.id or request.name)
                end
            end
        end
    else
        imgui.TextDisabled("  None")
    end
    
    imgui.Spacing()
    
    imgui.Text("Outgoing:")
    if #outgoingRequests > 0 then
        for i, request in ipairs(outgoingRequests) do
            imgui.AlignTextToFramePadding()
            imgui.Text(utils.capitalizeName(request.name or "Unknown"))
            imgui.SameLine()
            if imgui.Button("Cancel##outgoing_" .. i) then
                if callbacks.onCancelRequest then
                    callbacks.onCancelRequest(request.id or request.name)
                end
            end
        end
    else
        imgui.TextDisabled("  None")
    end
end

return M
