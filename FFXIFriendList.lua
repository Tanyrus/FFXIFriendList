--[[
* FFXIFriendList Entry File
* Registers callbacks, routes packets, manages modules
* Follows plan: config loading, module registry, packet routing, network polling
]]--

addon.name = 'FFXIFriendList'
addon.author = 'Tanyrus'
addon.version = '0.9.9'
addon.desc = 'Friend list addon for FFXI'

-- Set up package.path (absolute requires only)
local addonPath = string.match(debug.getinfo(1, "S").source:sub(2), "(.*[/\\])")
if addonPath then
    -- Support nested modules (e.g., libs.dkjson.dkjson)
    -- Lua's require converts dots to path separators automatically
    -- Pattern ?.lua will match libs/dkjson/dkjson.lua when requiring libs.dkjson.dkjson
    package.path = addonPath .. "?.lua;" .. addonPath .. "?/init.lua;" .. package.path
end

local installPathForLibs = AshitaCore:GetInstallPath():gsub('\\$', '')

-- Add Ashita's libs directory (for socket, settings, json, imgui, etc.)
local libsPath = installPathForLibs .. "\\addons\\libs\\?.lua"
package.path = libsPath .. ";" .. package.path

-- Also add socket subdirectory for socket.url, socket.ssl, etc.
local socketPath = installPathForLibs .. "\\addons\\libs\\socket\\?.lua"
package.path = socketPath .. ";" .. package.path

-- Load common (provides :args() method for strings)
require('common')

-- Load imgui (required before d3d_present registration)
require('imgui')

-- Load core modules
-- config module removed - using libs.settings directly
local moduleRegistry = require('core.moduleregistry')
local settings = require('libs.settings')
local ServerConfig = require('core.ServerConfig')

-- Expose moduleRegistry globally for window close policy
_G.moduleRegistry = moduleRegistry

-- Load app
local App = require('app.App')

-- Load libs
local packets = require('libs.packets')
local net = require('libs.net')
local storageLib = require('libs.storage')
local netWrapper = require('libs.netwrapper')
local icons = require('libs.icons')

-- Load handlers
local inputHandler = require('handlers.input')
local controllerInputHandler = require('handlers.controller_input')

-- Load UI close handling (centralized ESC/controller close)
local closeInputHandler = require('ui.input.close_input')

-- Load sound player service
local SoundPlayer = require('platform.services.SoundPlayer')

-- Load diagnostics runner
local DiagnosticsRunner = require('app.diagnostics.DiagnosticsRunner')

-- Simple services (no platform dependencies)
local SessionManager = {}
SessionManager.new = function()
    local self = { sessionId = "" }
    function self:getSessionId() return self.sessionId end
    function self:setSessionId(id) self.sessionId = id end
    return self
end

local RealmDetector = {}
RealmDetector.new = function(ashitaCore)
    local self = { ashitaCore = ashitaCore, realmId = "notARealServer" }
    function self:getRealmId() return self.realmId end
    function self:setRealmId(id) self.realmId = id end
    return self
end

-- Global config (read-only for modules)
gConfig = nil
gAdjustedSettings = nil

-- App instance
local app = nil
local initialized = false
local installPath = nil

-- Diagnostics runner
local diagRunner = nil

-- Update timing (for network polling - now per-frame for faster response)
local lastUpdateTime = 0

-- Generate session ID
local function generateSessionId()
    local chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    local id = ""
    for i = 1, 32 do
        local idx = math.random(1, #chars)
        id = id .. chars:sub(idx, idx)
    end
    return id
end

-- Function to create directory (passed to config/storage modules)
local function createDirectory(path)
    return ashita.fs.create_directory(path)
end

-- Initialize addon
ashita.events.register('load', 'ffxifriendlist_load', function()
    -- Get Ashita install path (for reference only)
    installPath = AshitaCore:GetInstallPath():gsub('\\$', '')
    
    -- Initialize settings system (saves to config/addons/FFXIFriendList/settings.lua)
    -- Uses Ashita-style Lua table format, account-wide (not per-character)
    -- API keys are obtained from auth/ensure endpoint and saved here
    gConfig = settings.initialize()
    
    -- Ensure data structure exists
    if not gConfig.data then
        gConfig.data = {}
    end
    if not gConfig.data.apiKeys then
        gConfig.data.apiKeys = {}
    end
    if not gConfig.data.serverSelection then
        gConfig.data.serverSelection = {
            savedServerId = "",
            savedServerBaseUrl = ServerConfig.DEFAULT_SERVER_URL,
            detectedServerShownOnce = false
        }
    end
    if not gConfig.data.preferences then
        gConfig.data.preferences = {}
    end
    
    -- Store addon path for sound/icon loading
    gConfig.addonPath = addonPath
    
    -- Create custom asset override folders (sounds/, images/)
    settings.ensureCustomAssetFolders()
    
    -- Initialize icon manager with addon path
    icons.Initialize(addonPath)
    icons.PreloadAll()
    
    -- Create adjusted settings from settings data
    gAdjustedSettings = {}
    if gConfig and gConfig.data and gConfig.data.preferences then
        gAdjustedSettings = gConfig.data.preferences
    end
    
    -- Initialize simple services
    local sessionManager = SessionManager.new()
    local realmDetector = RealmDetector.new(ashitaCore)
    
    -- Set up session
    local sessionId = generateSessionId()
    sessionManager:setSessionId(sessionId)
    
    -- Set up realm
    local realmId = realmDetector:getRealmId()
    
    -- Create storage (uses persistence.lua)
    local storage = storageLib.create(installPath, createDirectory)
    
    -- Create network wrapper (uses libs/net.lua)
    local netClient = netWrapper.create(sessionManager, realmDetector, nil)
    
    -- Build deps table (logging disabled for production - enable for debugging)
    local deps = {
        logger = {
            debug = function(msg) return nil end,  -- Silent
            info = function(msg) return nil end,   -- Silent
            error = function(msg) 
                print("[FFXIFriendList ERROR] " .. tostring(msg))
                return nil
            end,
            warn = function(msg) return nil end,   -- Silent
            echo = function(msg) return nil end    -- Silent
        },
        net = netClient,
        storage = storage,
        session = sessionManager,
        realmDetector = realmDetector,
        time = function() return os.time() * 1000 end
    }
    
    -- Create app (all features use new(deps) signature)
    app = App.create(deps)
    if app then
        -- Update net wrapper with connection feature
        if app.features.connection then
            netClient.connection = app.features.connection
        end
        
        local _ = App.initialize(app)
        
        -- Set global app instance for modules to access
        _G.FFXIFriendListApp = app
        
        -- Create diagnostics runner
        diagRunner = DiagnosticsRunner.new({
            logger = deps.logger,
            net = netClient,
            connection = app.features.connection,
            wsClient = app.features.wsClient
        })
        
        -- Enable diagnostics if DebugMode is set
        if gConfig and gConfig.DebugMode then
            diagRunner:setEnabled(true)
        end
    end
    
    -- Initialize handlers
    local _ = inputHandler.initialize()
    local _ = controllerInputHandler.Initialize()
    
    -- Load and register UI modules
    local friendListModule = require('modules.friendlist')
    local optionsModule = require('modules.options')
    local notificationsModule = require('modules.notifications')
    
    -- Register modules in registry
    local _ = moduleRegistry.Register('friendList', {
        module = friendListModule,
        settingsKey = 'friendListSettings',
        configKey = 'showFriendList',
        hasSetHidden = true
    })
    
    local _ = moduleRegistry.Register('options', {
        module = optionsModule,
        settingsKey = 'optionsSettings',
        configKey = 'showOptions',
        hasSetHidden = true
    })
    
    local _ = moduleRegistry.Register('notifications', {
        module = notificationsModule,
        settingsKey = 'notificationsSettings',
        configKey = 'showNotifications',
        hasSetHidden = true
    })
    
    local quickOnlineModule = require('modules.quickonline')
    local _ = moduleRegistry.Register('quickOnline', {
        module = quickOnlineModule,
        settingsKey = 'quickOnlineSettings',
        configKey = 'showQuickOnline',
        hasSetHidden = true
    })
    
    -- NoteEditor module
    local noteEditorModule = require('modules.noteeditor')
    local _ = moduleRegistry.Register('noteEditor', {
        module = noteEditorModule,
        settingsKey = 'noteEditorSettings',
        configKey = 'showNoteEditor',
        hasSetHidden = true
    })
    
    -- ServerSelection module - DISABLED: Now handled by friendlist and quickonline modules
    -- local serverSelectionModule = require('modules.serverselection')
    -- local _ = moduleRegistry.Register('serverSelection', {
    --     module = serverSelectionModule,
    --     settingsKey = 'serverSelectionSettings',
    --     configKey = 'showServerSelection',
    --     hasSetHidden = true
    -- })
    
    -- Themes functionality is now integrated into the main window's Themes tab
    -- No separate themes window module needed
    
    -- Initialize modules
    local _ = moduleRegistry.InitializeAll(gAdjustedSettings)
    
    initialized = true
    if deps.logger and deps.logger.info then
        deps.logger.info("Initialized")
    end
    
    -- Ensure windows start closed (modules may have set them during initialization)
    if gConfig then
        gConfig.showFriendList = false
        gConfig.showQuickOnline = false
    end
    
    -- Auto-show friend list window if server not configured (triggers server selection popup)
    if app and app.features and app.features.serverlist then
        local serverSelected = app.features.serverlist:isServerSelected()
        if not serverSelected then
            if gConfig then
                gConfig.showFriendList = true
            end
        end
    end
    
    -- Auto-open windows on game start if enabled
    if gConfig then
        if gConfig.friendListSettings and gConfig.friendListSettings.autoOpenOnStart then
            gConfig.showFriendList = true
        end
        if gConfig.quickOnlineSettings and gConfig.quickOnlineSettings.autoOpenOnStart then
            gConfig.showQuickOnline = true
        end
    end
    
    return nil
end)

-- Unload addon
ashita.events.register('unload', 'ffxifriendlist_unload', function()
    if not initialized then
        return
    end
    
    -- Cleanup handlers
    local _ = inputHandler.cleanup()
    
    -- Cleanup modules
    local _ = moduleRegistry.CleanupAll()
    
    -- Release app (save state via deps.storage)
    if app then
        local _ = App.release(app)
        app = nil
    end
    
    -- Clear global app instance
    _G.FFXIFriendListApp = nil
    
    -- Save settings
    if gConfig then
        settings.save()
    end
    
    initialized = false
    return nil
end)

-- Render callback
ashita.events.register('d3d_present', 'ffxifriendlist_present', function()
    if not initialized then
        return
    end
    
    -- Update timing
    local now = os.clock()
    local deltaTime = now - lastUpdateTime
    lastUpdateTime = now
    
    -- Poll network every frame for faster response (60Hz+)
    -- processAll() drains all completions in one call, so this is efficient
    net.poll()
    net.cleanup()
    
    -- Update app (tick with deltaTime in seconds)
    if app then
        App.tick(app, deltaTime)  -- deltaTime is already in seconds from os.clock()
    end
    
    -- Don't render or process input until character is loaded
    if not App.isGameReady() then
        return nil
    end
    
    -- Update menu detection (polling method, every frame but throttled internally)
    if initialized then
        inputHandler.update(deltaTime)
    end
    
    -- Process close key before window draws
    -- Must run BEFORE render so close is processed before window draws
    if initialized then
        closeInputHandler.update()
    end
    
    -- Render modules via registry
    -- TODO: Get event system active state when game state module exists
    local eventSystemActive = false
    
    -- Server selection is now handled as a popup within the friend list window
    -- (no longer auto-opened as a standalone window)
    
    for name, _ in pairs(moduleRegistry.GetAll()) do
        local _ = moduleRegistry.RenderModule(name, gConfig, gAdjustedSettings, eventSystemActive)  -- Capture return value
    end
    
    -- Explicitly return nil
    return nil
end)

-- Controller Input Callbacks
ashita.events.register('dinput_button', 'ffxifriendlist_dinput_button', function(e)
    if initialized then
        controllerInputHandler.HandleDirectInput(e)
    end
end)

ashita.events.register('xinput_button', 'ffxifriendlist_xinput_button', function(e)
    if initialized then
        controllerInputHandler.HandleXInput(e)
    end
end)

-- XInput state polling (more reliable than xinput_button on some systems)
ashita.events.register('xinput_state', 'ffxifriendlist_xinput_state', function(e)
    if initialized then
        controllerInputHandler.HandleXInputState(e)
    end
end)

-- Packet routing (entry file routes packets directly)
ashita.events.register('packet_in', 'ffxifriendlist_packet', function(e)
    if not initialized then
        return false
    end
    
    -- Route packets using libs/packets.lua
    local parsedPacket = nil
    
    if e.id == 0x00A then
        parsedPacket = packets.ParseZonePacket(e)
        if parsedPacket and app and app.features.friends then
            -- Schedule zone change handling (processed after debounce delay)
            local _ = app.features.friends:scheduleZoneChange(parsedPacket)
        end
    else
        -- Generic packet (for future expansion)
        parsedPacket = packets.ParseGenericPacket(e)
    end
    
    -- Return false to let other addons handle the packet
    return false
end)

local Assets = require('constants.assets')

local function M_playSoundTest(soundType)
    local sounds = {
        online = Assets.SOUNDS.FRIEND_ONLINE,
        request = Assets.SOUNDS.FRIEND_REQUEST
    }
    
    if soundType == "all" then
        for name, file in pairs(sounds) do
            print("[FFXIFriendList] Playing sound: " .. name .. " -> " .. file)
            SoundPlayer.playSound(file, 1.0, nil)
        end
        return
    end
    
    local soundFile = sounds[soundType]
    if not soundFile then
        print("[FFXIFriendList] Unknown sound type: " .. tostring(soundType))
        print("[FFXIFriendList] Available: online, request, all")
        return
    end
    
    print("[FFXIFriendList] Playing sound: " .. soundType .. " -> " .. soundFile)
    SoundPlayer.playSound(soundFile, 1.0, nil)
end

ashita.events.register('command', 'ffxifriendlist_command', function(e)
    if not e.command then
        return
    end
    
    local command_args = e.command:lower():args()
    
    if #command_args == 0 then
        return
    end
    
    -- Handle /fl command (toggle friend list window)
    if command_args[1] == "/fl" then
        e.blocked = true
        
        -- Only proceed if initialized
        if not initialized then
            return
        end
        
        -- If there are subcommands, handle them
        if #command_args > 1 then
            local subcmd = command_args[2]
            
            -- /fl debug - toggle debug window (if implemented)
            if subcmd == "debug" then
                -- TODO: Implement debug window toggle
                return
            end
            
            -- /fl perf - print performance summary (if implemented)
            if subcmd == "perf" then
                -- TODO: Implement performance summary
                return
            end
            
            -- /fl stats - print memory stats (if implemented)
            if subcmd == "stats" then
                -- TODO: Implement memory stats
                return
            end
            
            if subcmd == "notify" then
                if app and app.features and app.features.notifications then
                    app.features.notifications:showTestNotification()
                end
                return
            end
            
            if subcmd == "soundtest" then
                local soundType = command_args[3] or "online"
                M_playSoundTest(soundType)
                return
            end
            
            if subcmd == "menutoggle" then
                if gConfig then
                    gConfig.showFriendList = not gConfig.showFriendList
                    moduleRegistry.CheckVisibility(gConfig)
                    print("[FFXIFriendList] Window " .. (gConfig.showFriendList and "opened" or "closed") .. " via menutoggle")
                end
                return
            end
            
            if subcmd == "menudebug" then
                local arg = command_args[3]
                if arg == "on" then
                    gConfig.menuDebugEnabled = true
                    print("[FFXIFriendList] Menu debug ENABLED")
                    local menuState = inputHandler.getState()
                    if menuState.pGameMenu then
                        print("[FFXIFriendList] Pattern scan OK: 0x" .. string.format("%X", menuState.pGameMenu))
                    else
                        print("[FFXIFriendList] Pattern scan FAILED - detection unavailable")
                    end
                elseif arg == "off" then
                    gConfig.menuDebugEnabled = false
                    print("[FFXIFriendList] Menu debug DISABLED")
                elseif arg == "status" then
                    local menuState = inputHandler.getState()
                    print("[FFXIFriendList] Menu Detection Status:")
                    print("  detectionMode: " .. tostring(menuState.detectionMode))
                    if menuState.pGameMenu then
                        print("  pGameMenu: 0x" .. string.format("%X", menuState.pGameMenu))
                    else
                        print("  pGameMenu: nil (pattern scan failed)")
                    end
                    print("  Pointer chain:")
                    print("    subPointer: 0x" .. string.format("%X", menuState.pointerChain.subPointer or 0))
                    print("    subValue: 0x" .. string.format("%X", menuState.pointerChain.subValue or 0))
                    print("    menuHeader: 0x" .. string.format("%X", menuState.pointerChain.menuHeader or 0))
                    print("  currentMenuName: \"" .. tostring(menuState.lastMenuName) .. "\"")
                    print("  normalizedName: \"" .. tostring(menuState.normalizedMenuName) .. "\"")
                    print("  inFriendListMenu: " .. tostring(menuState.inFriendListMenu))
                    print("  toListTriggered: " .. tostring(menuState.toListTriggered))
                    print("  windowOpened: " .. tostring(menuState.windowOpened))
                    print("  stableFlistmaiCount: " .. tostring(menuState.stableFlistmaiCount))
                    print("  stableExitCount: " .. tostring(menuState.stableExitCount))
                    print("  Ashita API:")
                    print("    GetIsMenuOpen available: " .. tostring(menuState.ashitaApiAvailable))
                    if menuState.ashitaApiAvailable then
                        print("    GetIsMenuOpen value: " .. tostring(menuState.ashitaMenuOpen))
                    end
                elseif arg == "dump" then
                    local ringBuffer = inputHandler.getRingBuffer()
                    print("[FFXIFriendList] Menu Name Ring Buffer (oldest->newest):")
                    if #ringBuffer == 0 then
                        print("  (empty)")
                    else
                        for i, entry in ipairs(ringBuffer) do
                            print(string.format("  %02d: \"%s\" -> \"%s\"", i, entry.raw or "", entry.normalized or ""))
                        end
                    end
                elseif arg == "hexdump" then
                    local menuState = inputHandler.getState()
                    if menuState.pointerChain.menuHeader and menuState.pointerChain.menuHeader ~= 0 then
                        local hex = inputHandler.getHexDump(menuState.pointerChain.menuHeader + 0x46, 32)
                        print("[FFXIFriendList] Hex dump at menuHeader+0x46:")
                        print("  " .. hex)
                    else
                        print("[FFXIFriendList] No valid menuHeader pointer to dump")
                    end
                else
                    print("[FFXIFriendList] Usage: /fl menudebug <on|off|status|dump|hexdump>")
                    print("  on     - Enable debug logging (throttled)")
                    print("  off    - Disable debug logging")
                    print("  status - Show current detection state and pointer chain")
                    print("  dump   - Show ring buffer of recent menu names")
                    print("  hexdump - Hex dump around menuHeader for debugging")
                end
                return
            end
            
            if subcmd == "help" then
                print("[FFXIFriendList] Commands:")
                print("  /fl - Toggle friend list window")
                print("  /flist or /qo - Toggle quick online window")
                print("  /fl menutoggle - Manually toggle window (fallback for menu detection)")
                print("  /fl menudebug <on|off|status|dump|hexdump> - Menu detection debug")
                print("  /fl notify - Show test notification")
                print("  /fl soundtest <type> - Test sounds (online, request, all)")
                print("  /fl block <name> - Block a player from sending friend requests")
                print("  /fl unblock <name> - Unblock a player")
                print("  /fl blocked - List all blocked players")
                print("  /fl diag <cmd> - Run diagnostics (requires DebugMode=true)")
                print("  /befriend <name> [tag] - Send friend request with optional tag")
                return
            end
            
            if subcmd == "diag" then
                if diagRunner then
                    -- Collect remaining args
                    local diagArgs = {}
                    for i = 3, #command_args do
                        table.insert(diagArgs, command_args[i])
                    end
                    diagRunner:handleCommand(diagArgs)
                else
                    print("[FFXIFriendList] Diagnostics not available")
                end
                return
            end
            
            if subcmd == "debug" then
                -- Toggle debug mode which enables diagnostics
                if gConfig then
                    gConfig.DebugMode = not gConfig.DebugMode
                    if diagRunner then
                        diagRunner:setEnabled(gConfig.DebugMode)
                    end
                    print("[FFXIFriendList] Debug mode " .. (gConfig.DebugMode and "ENABLED" or "DISABLED"))
                end
                return
            end
            
            if subcmd == "block" then
                local targetName = command_args[3]
                if not targetName or targetName == "" then
                    print("[FFXIFriendList] Usage: /fl block <name>")
                    return
                end
                
                if app and app.features and app.features.blocklist then
                    app.features.blocklist:block(targetName, function(success, result)
                        if success then
                            if result and result.alreadyBlocked then
                                print("[FFXIFriendList] " .. targetName .. " was already blocked")
                            else
                                print("[FFXIFriendList] Blocked " .. targetName)
                            end
                        else
                            print("[FFXIFriendList] Failed to block " .. targetName .. ": " .. tostring(result))
                        end
                    end)
                else
                    print("[FFXIFriendList] Blocklist feature not available")
                end
                return
            end
            
            if subcmd == "unblock" then
                local targetName = command_args[3]
                if not targetName or targetName == "" then
                    print("[FFXIFriendList] Usage: /fl unblock <name>")
                    return
                end
                
                if app and app.features and app.features.blocklist then
                    app.features.blocklist:unblockByName(targetName, function(success, result)
                        if success then
                            if result and not result.wasBlocked then
                                print("[FFXIFriendList] " .. targetName .. " was not blocked")
                            else
                                print("[FFXIFriendList] Unblocked " .. targetName)
                            end
                        else
                            print("[FFXIFriendList] Failed to unblock " .. targetName .. ": " .. tostring(result))
                        end
                    end)
                else
                    print("[FFXIFriendList] Blocklist feature not available")
                end
                return
            end
            
            if subcmd == "blocked" then
                if app and app.features and app.features.blocklist then
                    local blocked = app.features.blocklist:getBlockedList()
                    if #blocked == 0 then
                        print("[FFXIFriendList] No blocked players")
                    else
                        print("[FFXIFriendList] Blocked players (" .. #blocked .. "):")
                        for _, entry in ipairs(blocked) do
                            local name = entry.displayName or "Unknown"
                            if #name > 0 then
                                name = string.upper(string.sub(name, 1, 1)) .. string.sub(name, 2)
                            end
                            print("  - " .. name)
                        end
                    end
                else
                    print("[FFXIFriendList] Blocklist feature not available")
                end
                return
            end
        end
        
        -- No subcommand or unknown subcommand - toggle friend list window
        if gConfig then
            -- Initialize showFriendList if it doesn't exist (default to true)
            if gConfig.showFriendList == nil then
                gConfig.showFriendList = true
            end
            
            -- Toggle showFriendList config
            gConfig.showFriendList = not gConfig.showFriendList
            
            -- Update module visibility via registry
            moduleRegistry.CheckVisibility(gConfig)
            
            -- Also ensure the module's window is open when showing
            local friendListEntry = moduleRegistry.Get('friendList')
            if friendListEntry and friendListEntry.module and friendListEntry.module.display then
                -- Force window to be visible if we're showing it
                if gConfig.showFriendList and friendListEntry.module.display.SetWindowOpen then
                    friendListEntry.module.display.SetWindowOpen(true)
                end
            end
            
            -- Trigger refresh if opening
            if gConfig.showFriendList and app and app.features.friends then
                local _ = app.features.friends:refresh()
            end
            
        end
        
        return
    end
    
    -- Handle /flist and /qo commands (toggle quick online window)
    if command_args[1] == "/flist" or command_args[1] == "/qo" then
        e.blocked = true
        
        if not initialized then
            return
        end
        
        if gConfig then
            -- Initialize showQuickOnline if it doesn't exist
            if gConfig.showQuickOnline == nil then
                gConfig.showQuickOnline = true
            else
                gConfig.showQuickOnline = not gConfig.showQuickOnline
            end
            
            -- Update module visibility
            moduleRegistry.CheckVisibility(gConfig)
            
            -- Trigger refresh if opening
            if gConfig.showQuickOnline and app and app.features.friends then
                local _ = app.features.friends:refresh()
            end
        end
        
        return
    end
    
    -- Handle /befriend <name> [tag] command
    if command_args[1] == "/befriend" and #command_args > 1 then
        e.blocked = true
        
        local friendName = command_args[2]
        local tagName = nil
        
        if #command_args > 2 then
            local tagParts = {}
            for i = 3, #command_args do
                table.insert(tagParts, command_args[i])
            end
            tagName = table.concat(tagParts, " ")
        end
        
        local originalCmd = e.command
        local nameStart = originalCmd:find(command_args[2], 1, true)
        if nameStart then
            local restOfCmd = originalCmd:sub(nameStart)
            local spacePos = restOfCmd:find(" ")
            if spacePos then
                friendName = restOfCmd:sub(1, spacePos - 1)
                tagName = restOfCmd:sub(spacePos + 1):match("^%s*(.-)%s*$")
                if tagName == "" then
                    tagName = nil
                end
            else
                friendName = restOfCmd
            end
        end
        
        if friendName and friendName ~= "" and app and app.features.friends then
            local _ = app.features.friends:addFriend(friendName)
            
            if tagName and app.features.tags then
                app.features.tags:setPendingTag(friendName, tagName)
            end
        end
        
        return
    end
end)

