local M = {}

local activeController = nil
local activeControllerName = nil

-- Global handle binding function to be called by controller files
_G.FFXIFriendList_HandleBinding = function(buttonName, newState)
    return M.HandleBinding(buttonName, newState)
end

function M.HandleBinding(buttonName, newState)
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.preferences then
        return
    end

    local prefs = app.features.preferences:getPrefs()
    local boundButton = prefs.flistBindButton
    
    -- Trigger on button press (newState == true)
    if boundButton and boundButton ~= '' and buttonName == boundButton and newState == true then
        local chatManager = AshitaCore:GetChatManager()
        if chatManager then
            chatManager:QueueCommand(-1, '/flist')
        end
        return true
    end
end

function M.Initialize()
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        local layout = prefs.controllerLayout
        if layout then
            M.LoadController(layout)
        else
            M.LoadController('xinput')
        end
    else
        M.LoadController('xinput')
    end
end

function M.Update(dt)
    -- No pending command logic needed for simple /flist bind
end

function M.LoadController(name)
    if activeControllerName == name and activeController then
        return -- Already loaded
    end

    local path = string.format('%s/addons/%s/controllers/%s.lua', AshitaCore:GetInstallPath(), 'FFXIFriendList', name)
    if not ashita.fs.exists(path) then
        print('[FFXIFriendList] Controller file not found: ' .. path)
        return
    end

    local f, err = loadfile(path)
    if not f then
        print('[FFXIFriendList] Error loading controller file: ' .. err)
        return
    end

    local success, result = pcall(f)
    if not success then
        print('[FFXIFriendList] Error executing controller file: ' .. result)
        return
    end

    activeController = result
    activeControllerName = name
end

function M.HandleDirectInput(e)
    if activeController and activeController.HandleDirectInput then
        activeController:HandleDirectInput(e)
    end
end

function M.HandleXInput(e)
    if activeController and activeController.HandleXInput then
        activeController:HandleXInput(e)
    end
end

function M.GetActiveController()
    return activeController
end

function M.GetActiveControllerName()
    return activeControllerName
end

function M.GetAvailableControllers()
    local path = string.format('%s/addons/%s/controllers/', AshitaCore:GetInstallPath(), 'FFXIFriendList')
    local controllers = {}
    if not (ashita.fs.exists(path)) then
        ashita.fs.create_directory(path)
    end
    local contents = ashita.fs.get_directory(path, '.*\\.lua')
    for _, file in pairs(contents) do
        file = string.sub(file, 1, -5)
        if file ~= 'init' then
            table.insert(controllers, file)
        end
    end
    return controllers
end

function M.GetActiveControllerButtons()
    if activeController and activeController.Buttons then
        return activeController.Buttons
    end
    return {}
end

return M