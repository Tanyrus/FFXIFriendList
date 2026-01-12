--[[
* Visibility Matrix Module - Display Layer
* Renders the grid UI for per-friend per-character visibility control
]]

local imgui = require('imgui')

local M = {}

-- Window state
local windowState = {
    isOpen = false,
    size = { 800, 600 },
    scrollY = 0,
    scrollX = 0,
    headerHeight = 60,
    rowHeight = 25,
    columnWidth = 120,
    friendColumnWidth = 200,
}

-- UI colors
local colors = {
    visibleCell = { 0.2, 0.8, 0.2, 1.0 },      -- Green
    hiddenCell = { 0.8, 0.2, 0.2, 1.0 },       -- Red
    headerBg = { 0.15, 0.15, 0.15, 1.0 },
    rowEven = { 0.08, 0.08, 0.08, 1.0 },
    rowOdd = { 0.12, 0.12, 0.12, 1.0 },
    activeCharacterHeader = { 0.2, 0.4, 0.8, 1.0 }, -- Blue for active character
}

function M.Initialize(settings)
    -- Load saved window state if available
    if settings and settings.visibilityMatrixWindow then
        windowState.isOpen = settings.visibilityMatrixWindow.isOpen or false
        windowState.size = settings.visibilityMatrixWindow.size or windowState.size
    end
end

function M.IsWindowOpen()
    return windowState.isOpen
end

function M.SetWindowOpen(open)
    windowState.isOpen = open or false
end

function M.ToggleWindow()
    windowState.isOpen = not windowState.isOpen
end

function M.SetHidden(hidden)
    if hidden then
        windowState.isOpen = false
    end
end

function M.DrawWindow(settings, dataModule)
    if not windowState.isOpen then return end
    
    local snapshot = dataModule.GetSnapshot()
    local isFetching = dataModule.IsFetching()
    local isUpdating = dataModule.IsUpdating()
    local lastError = dataModule.GetLastError()
    local pendingCount = dataModule.GetPendingUpdateCount()
    local friendFilter = dataModule.GetFriendFilter()
    
    imgui.SetNextWindowSize(windowState.size, ImGuiCond_FirstUseEver)
    
    local visible, opened = imgui.Begin('Per-Friend Visibility Matrix###vismatrix', true, ImGuiWindowFlags_NoCollapse)
    
    if not opened then
        windowState.isOpen = false
    end
    
    if visible then
        -- Header with controls
        M.DrawHeader(dataModule, snapshot, isFetching, isUpdating, lastError, pendingCount, friendFilter)
        
        imgui.Separator()
        imgui.Spacing()
        
        -- Main content
        if not snapshot then
            if isFetching then
                imgui.Text('Loading visibility matrix...')
            else
                imgui.Text('No data loaded. Click Refresh to fetch.')
                if imgui.Button('Refresh Now') then
                    dataModule.Fetch()
                end
            end
        else
            M.DrawMatrix(dataModule, snapshot, friendFilter)
        end
        
        windowState.size = imgui.GetWindowSize()
    end
    
    imgui.End()
end

function M.DrawHeader(dataModule, snapshot, isFetching, isUpdating, lastError, pendingCount, friendFilter)
    -- Status line
    if lastError then
        imgui.TextColored({ 1, 0.3, 0.3, 1 }, 'Error: ' .. tostring(lastError))
    elseif isFetching then
        imgui.Text('Fetching...')
    elseif isUpdating then
        imgui.Text('Saving changes...')
    elseif snapshot then
        local charCount = #(snapshot.characters or {})
        local friendCount = #(snapshot.friends or {})
        local entryCount = #(snapshot.matrix or {})
        imgui.Text(string.format('%d characters × %d friends (%d explicit settings)', 
            charCount, friendCount, entryCount))
        
        if imgui.IsItemHovered() and friendCount > 0 then
            local tooltip = 'Friends: '
            for i, f in ipairs(snapshot.friends) do
                if i > 1 then tooltip = tooltip .. ', ' end
                tooltip = tooltip .. (f.displayName or 'Unknown')
                if i >= 5 then
                    tooltip = tooltip .. '...'
                    break
                end
            end
            imgui.SetTooltip(tooltip)
        end
    end
    
    imgui.SameLine()
    
    -- Show pending count
    if pendingCount > 0 then
        imgui.TextColored({ 1, 1, 0, 1 }, string.format(' [%d unsaved]', pendingCount))
    end
    
    -- Buttons
    imgui.SameLine()
    imgui.SetCursorPosX(imgui.GetWindowWidth() - 250)
    
    if imgui.Button('Refresh') then
        dataModule.Fetch()
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip('Reload visibility matrix from server')
    end
    
    imgui.SameLine()
    
    if pendingCount > 0 then
        if imgui.Button('Save Now') then
            dataModule.SaveNow()
        end
        if imgui.IsItemHovered() then
            imgui.SetTooltip('Save all pending changes immediately')
        end
    else
        imgui.BeginDisabled()
        imgui.Button('Save Now')
        imgui.EndDisabled()
    end
    
    -- Search filter
    imgui.Text('Filter Friends:')
    imgui.SameLine()
    imgui.SetNextItemWidth(200)
    local changed, newFilter = imgui.InputText('##friendFilter', friendFilter, 256)
    if changed then
        dataModule.SetFriendFilter(newFilter)
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip('Search/filter friend names')
    end
end

function M.DrawMatrix(dataModule, snapshot, friendFilter)
    local characters = snapshot.characters or {}
    local friends = snapshot.friends or {}
    
    if #friends > 0 then
        imgui.TextDisabled('Friends:')
        imgui.SameLine()
        local friendNames = {}
        for i = 1, math.min(3, #friends) do
            table.insert(friendNames, friends[i].displayName or '?')
        end
        imgui.Text(table.concat(friendNames, ', '))
    end
    imgui.Spacing()
    
    if #characters == 0 then
        imgui.TextColored({1, 0.5, 0, 1}, 'No characters found.')
        imgui.TextWrapped('This account has no characters registered.')
        return
    end
    
    if #friends == 0 then
        imgui.TextColored({1, 0.5, 0, 1}, 'No friends found.')
        imgui.TextWrapped('Add friends first before configuring per-friend visibility.')
        imgui.Spacing()
        imgui.TextDisabled('Note: You need bidirectional friendships (both added each other).')
        return
    end
    
    -- Filter friends
    local filteredFriends = {}
    local filterLower = string.lower(friendFilter or '')
    for _, friend in ipairs(friends) do
        if filterLower == '' or string.find(string.lower(friend.displayName or ''), filterLower, 1, true) then
            table.insert(filteredFriends, friend)
        end
    end
    
    if #filteredFriends == 0 then
        imgui.Text('No friends match filter.')
        return
    end
    
    -- Calculate total width needed
    local totalWidth = windowState.friendColumnWidth + (#characters * windowState.columnWidth)
    local availWidth = imgui.GetContentRegionAvail()
    
    -- Use ImGui table for better compatibility
    if imgui.BeginTable('VisibilityMatrixTable', #characters + 1, ImGuiTableFlags_Borders + ImGuiTableFlags_RowBg + ImGuiTableFlags_ScrollX + ImGuiTableFlags_ScrollY) then
        -- Setup columns
        imgui.TableSetupColumn('Friend', ImGuiTableColumnFlags_WidthFixed, windowState.friendColumnWidth)
        for i, char in ipairs(characters) do
            imgui.TableSetupColumn(char.characterName, ImGuiTableColumnFlags_WidthFixed, windowState.columnWidth)
        end
        imgui.TableHeadersRow()
        
        -- Draw rows (one per friend)
        for i, friend in ipairs(filteredFriends) do
            imgui.TableNextRow()
            
            -- Friend name column
            imgui.TableSetColumnIndex(0)
            imgui.Text(friend.displayName or 'Unknown')
            
            -- Visibility cells
            for j, char in ipairs(characters) do
                imgui.TableSetColumnIndex(j)
                
                local isVisible = dataModule.GetVisibility(char.characterId, friend.accountId)
                
                imgui.PushID('cell_' .. char.characterId .. '_' .. friend.accountId)
                
                local checked = {isVisible}
                if imgui.Checkbox('##vis', checked) then
                    dataModule.ToggleVisibility(char.characterId, friend.accountId)
                end
                
                if imgui.IsItemHovered() then
                    local status = checked[1] and 'CAN see' or 'CANNOT see'
                    imgui.SetTooltip(string.format('%s %s %s when online',
                        friend.displayName,
                        status,
                        char.characterName))
                end
                
                imgui.PopID()
            end
        end
        
        imgui.EndTable()
    end
end

function M.DrawColumnHeaders(characters)
    local cursorY = imgui.GetCursorPosY()
    
    -- Friend column header (LEFT SIDE - ROWS)
    imgui.SetCursorPosY(cursorY)
    imgui.PushStyleColor(ImGuiCol_Button, colors.headerBg)
    imgui.Button('Friend Name\n(rows below)', { windowState.friendColumnWidth, windowState.headerHeight })
    imgui.PopStyleColor()
    if imgui.IsItemHovered() then
        imgui.SetTooltip('Each row below represents one of your friends')
    end
    
    -- Character column headers (TOP - COLUMNS)
    for i, char in ipairs(characters) do
        imgui.SameLine()
        
        -- Highlight active character
        if char.isActive then
            imgui.PushStyleColor(ImGuiCol_Button, colors.activeCharacterHeader)
        else
            imgui.PushStyleColor(ImGuiCol_Button, colors.headerBg)
        end
        
        local label = char.characterName
        if char.isActive then
            label = label .. '\n(Active)'
        else
            label = label .. '\n(Your Alt)'
        end
        
        imgui.Button(label, { windowState.columnWidth, windowState.headerHeight })
        
        imgui.PopStyleColor()
        
        if imgui.IsItemHovered() then
            imgui.SetTooltip(string.format('Your character: %s @ %s%s\nColumn shows what this friend can see of this character', 
                char.characterName, 
                char.realmId,
                char.isActive and '\n(Currently Active)' or ''))
        end
    end
end

function M.DrawFriendRow(dataModule, friend, characters, rowIndex)
    local cursorY = imgui.GetCursorPosY()
    local isEven = (rowIndex % 2) == 0
    
    -- Draw row background
    local drawList = imgui.GetWindowDrawList()
    local rowColor = isEven and colors.rowEven or colors.rowOdd
    local cursorScreenPos = imgui.GetCursorScreenPos()
    local rowWidth = windowState.friendColumnWidth + (#characters * windowState.columnWidth)
    
    drawList:AddRectFilled(
        cursorScreenPos,
        { cursorScreenPos[1] + rowWidth, cursorScreenPos[2] + windowState.rowHeight },
        imgui.GetColorU32(rowColor)
    )
    
    -- Friend name column
    imgui.SetCursorPosY(cursorY)
    imgui.Text(friend.displayName or 'Unknown')
    if imgui.IsItemHovered() and friend.realmId then
        imgui.SetTooltip(friend.displayName .. ' @ ' .. friend.realmId)
    end
    
    -- Visibility toggle cells
    for i, char in ipairs(characters) do
        imgui.SameLine()
        
        local isVisible = dataModule.GetVisibility(char.characterId, friend.accountId)
        local cellLabel = isVisible and '✓ Visible' or '✗ Hidden'
        local cellColor = isVisible and colors.visibleCell or colors.hiddenCell
        
        imgui.PushID('cell_' .. char.characterId .. '_' .. friend.accountId)
        imgui.PushStyleColor(ImGuiCol_Button, cellColor)
        imgui.PushStyleColor(ImGuiCol_ButtonHovered, {
            cellColor[1] * 1.2,
            cellColor[2] * 1.2,
            cellColor[3] * 1.2,
            cellColor[4]
        })
        imgui.PushStyleColor(ImGuiCol_ButtonActive, {
            cellColor[1] * 0.8,
            cellColor[2] * 0.8,
            cellColor[3] * 0.8,
            cellColor[4]
        })
        
        if imgui.Button(cellLabel, { windowState.columnWidth - 5, windowState.rowHeight - 2 }) then
            dataModule.ToggleVisibility(char.characterId, friend.accountId)
        end
        
        imgui.PopStyleColor(3)
        imgui.PopID()
        
        if imgui.IsItemHovered() then
            local status = isVisible and 'can see' or 'CANNOT see'
            imgui.SetTooltip(string.format('%s %s %s when online\n\nClick to toggle',
                friend.displayName,
                status,
                char.characterName))
        end
    end
end

function M.UpdateVisuals(settings)
    -- Update colors or styles based on settings/theme if needed
end

function M.DrawMatrixInline(dataModule)
    local snapshot = dataModule.GetSnapshot()
    local isFetching = dataModule.IsFetching()
    local isUpdating = dataModule.IsUpdating()
    local lastError = dataModule.GetLastError()
    local pendingCount = dataModule.GetPendingUpdateCount()
    local friendFilter = dataModule.GetFriendFilter()
    
    -- Status
    if lastError then
        imgui.TextColored({ 1, 0.3, 0.3, 1 }, 'Error: ' .. tostring(lastError))
        imgui.Spacing()
        if imgui.Button('Retry') then
            dataModule.Fetch()
        end
        return
    elseif isFetching then
        imgui.Text('Loading...')
        imgui.TextDisabled('(Fetching from server...)')
        return
    end
    
    if not snapshot then
        imgui.Text('Click Refresh to load visibility matrix.')
        if imgui.Button('Refresh') then
            dataModule.Fetch()
        end
        return
    end
    
    -- Refresh button
    if imgui.Button('Refresh') then
        dataModule.Fetch()
    end
    if imgui.IsItemHovered() then
        imgui.SetTooltip('Reload from server')
    end
    
    -- Show pending count
    if pendingCount > 0 then
        imgui.SameLine()
        imgui.TextColored({ 1, 1, 0, 1 }, string.format('[Auto-saving %d changes...]', pendingCount))
    end
    
    imgui.Spacing()
    
    -- Draw the matrix
    M.DrawMatrix(dataModule, snapshot, friendFilter)
end

function M.Cleanup()
    windowState.isOpen = false
end

return M
