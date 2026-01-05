local imgui = require('imgui')
local NotificationSoundPolicy = require('core.notificationsoundpolicy')
local ThemeHelper = require('libs.themehelper')
local SoundPlayer = require('platform.services.SoundPlayer')

local M = {}

local TOAST_SPACING = 10.0
local DEFAULT_POSITION_X = 10.0
local DEFAULT_POSITION_Y = 15.0
local DEFAULT_DURATION_MS = 8000

local TOAST_WINDOW_FLAGS = bit.bor(
    ImGuiWindowFlags_NoTitleBar or 0x00000001,
    ImGuiWindowFlags_NoResize or 0x00000002,
    ImGuiWindowFlags_NoMove or 0x00000004,
    ImGuiWindowFlags_NoScrollbar or 0x00000008,
    ImGuiWindowFlags_NoScrollWithMouse or 0x00000010,
    ImGuiWindowFlags_NoCollapse or 0x00000020,
    ImGuiWindowFlags_AlwaysAutoResize or 0x00000040,
    ImGuiWindowFlags_NoSavedSettings or 0x00002000,
    ImGuiWindowFlags_NoFocusOnAppearing or 0x00001000,
    ImGuiWindowFlags_NoNav or 0x00040000,
    ImGuiWindowFlags_NoInputs or 0x00020000
)

local ToastState = {
    ENTERING = "ENTERING",
    VISIBLE = "VISIBLE",
    EXITING = "EXITING",
    COMPLETE = "COMPLETE"
}

local ToastType = {
    FriendOnline = "FriendOnline",
    FriendOffline = "FriendOffline",
    FriendRequestReceived = "FriendRequestReceived",
    FriendRequestAccepted = "FriendRequestAccepted",
    FriendRequestRejected = "FriendRequestRejected",
    MailReceived = "MailReceived",
    Error = "Error",
    Info = "Info",
    Warning = "Warning",
    Success = "Success"
}

local state = {
    initialized = false,
    hidden = false,
    positionX = DEFAULT_POSITION_X,
    positionY = DEFAULT_POSITION_Y,
    soundPolicy = nil,
    lastToastIds = {},
    activeToasts = {}
}

function M.Initialize(settings)
    state.initialized = true
    state.settings = settings or {}
    state.soundPolicy = NotificationSoundPolicy.NotificationSoundPolicy.new()
    state.activeToasts = {}
    
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        if prefs then
            state.positionX = prefs.notificationPositionX or DEFAULT_POSITION_X
            state.positionY = prefs.notificationPositionY or DEFAULT_POSITION_Y
        end
    end
end

function M.UpdateVisuals(settings)
    state.settings = settings or {}
    
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        if prefs then
            state.positionX = prefs.notificationPositionX or DEFAULT_POSITION_X
            state.positionY = prefs.notificationPositionY or DEFAULT_POSITION_Y
        end
    end
end

function M.SetHidden(hidden)
    state.hidden = hidden or false
end

local startClock = os.clock()
local startTime = os.time()

local function getCurrentTimeMs()
    local clockElapsed = os.clock() - startClock
    return (startTime * 1000) + (clockElapsed * 1000)
end

local function getDurationFromPrefs()
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        if prefs and prefs.notificationDuration ~= nil then
            local duration = tonumber(prefs.notificationDuration)
            if duration and duration > 0 then
                return duration * 1000
            end
        end
    end
    return DEFAULT_DURATION_MS
end

local function getToastColor(toastType)
    if toastType == ToastType.FriendOnline then
        return {0.2, 1.0, 0.2, 1.0}
    elseif toastType == ToastType.FriendOffline then
        return {0.8, 0.8, 0.8, 1.0}
    elseif toastType == ToastType.FriendRequestReceived or toastType == ToastType.FriendRequestAccepted then
        return {0.2, 0.6, 1.0, 1.0}
    elseif toastType == ToastType.FriendRequestRejected then
        return {1.0, 0.6, 0.2, 1.0}
    elseif toastType == ToastType.MailReceived then
        return {1.0, 0.8, 0.2, 1.0}
    elseif toastType == ToastType.Error then
        return {1.0, 0.2, 0.2, 1.0}
    elseif toastType == ToastType.Warning then
        return {1.0, 0.8, 0.0, 1.0}
    elseif toastType == ToastType.Success then
        return {0.2, 0.8, 0.2, 1.0}
    else
        return {0.8, 0.8, 1.0, 1.0}
    end
end

local function getSoundType(toastType)
    if toastType == ToastType.FriendOnline then
        return NotificationSoundPolicy.NotificationSoundType.FriendOnline
    elseif toastType == ToastType.FriendRequestReceived or toastType == ToastType.FriendRequestAccepted then
        return NotificationSoundPolicy.NotificationSoundType.FriendRequest
    else
        return NotificationSoundPolicy.NotificationSoundType.Unknown
    end
end

local function maybePlaySound(toast)
    if not toast or type(toast) ~= "table" then
        return
    end
    
    local app = _G.FFXIFriendListApp
    if not app or not app.features or not app.features.preferences then
        return
    end
    
    local prefs = app.features.preferences:getPrefs()
    if not prefs then
        return
    end
    
    local soundsEnabled = prefs.notificationSoundsEnabled
    if soundsEnabled == false or soundsEnabled == "false" then
        return
    end
    
    local toastType = toast.type
    if not toastType then
        return
    end
    
    local soundType = getSoundType(toastType)
    if soundType == NotificationSoundPolicy.NotificationSoundType.Unknown then
        return
    end
    
    local soundOnOnline = prefs.soundOnFriendOnline
    local soundOnRequest = prefs.soundOnFriendRequest
    
    if soundType == NotificationSoundPolicy.NotificationSoundType.FriendOnline then
        if soundOnOnline == false or soundOnOnline == "false" then
            return
        end
    end
    if soundType == NotificationSoundPolicy.NotificationSoundType.FriendRequest then
        if soundOnRequest == false or soundOnRequest == "false" then
            return
        end
    end
    
    local currentTime = getCurrentTimeMs()
    if not state.soundPolicy:shouldPlay(soundType, currentTime) then
        return
    end
    
    local soundFile = nil
    if soundType == NotificationSoundPolicy.NotificationSoundType.FriendOnline then
        soundFile = "online.wav"
    elseif soundType == NotificationSoundPolicy.NotificationSoundType.FriendRequest then
        soundFile = "friend-request.wav"
    end
    
    if soundFile then
        SoundPlayer.playSound(soundFile, 1.0, nil)
    end
end

local FADE_OUT_PERCENT = 0.20
local Y_ANIMATION_SPEED = 8.0

local function updateToast(toast, currentTime, prefsDurationMs)
    local elapsed = currentTime - (toast.createdAt or 0)
    local duration = prefsDurationMs
    if duration <= 0 then
        duration = DEFAULT_DURATION_MS
    end
    
    toast.duration = duration
    
    local fadeStartTime = duration * (1.0 - FADE_OUT_PERCENT)
    
    if toast.state == ToastState.ENTERING then
        toast.state = ToastState.VISIBLE
        toast.alpha = 1.0
    elseif toast.state == ToastState.VISIBLE then
        if toast.userDismissed then
            toast.state = ToastState.EXITING
        elseif elapsed >= fadeStartTime then
            toast.state = ToastState.EXITING
        end
    elseif toast.state == ToastState.EXITING then
        local fadeDuration = duration * FADE_OUT_PERCENT
        local fadeElapsed = elapsed - fadeStartTime
        
        if toast.userDismissed then
            fadeElapsed = (currentTime - (toast.dismissTime or currentTime))
            fadeDuration = 200
        end
        
        local fadeProgress = fadeElapsed / fadeDuration
        
        if fadeProgress >= 1.0 then
            toast.alpha = 0.0
            toast.state = ToastState.COMPLETE
        else
            toast.alpha = math.max(0.0, 1.0 - fadeProgress)
        end
    end
end

local function renderToast(toast, targetY)
    if toast.state == ToastState.COMPLETE then
        return 0
    end
    
    toast.frameCount = (toast.frameCount or 0) + 1
    local isFirstFrame = toast.frameCount <= 2
    
    if not toast.currentY then
        toast.currentY = targetY
    end
    
    local yDiff = targetY - toast.currentY
    if math.abs(yDiff) > 0.5 then
        toast.currentY = toast.currentY + yDiff * Y_ANIMATION_SPEED * 0.016
    else
        toast.currentY = targetY
    end
    
    local posX = state.positionX
    local posY = toast.currentY
    
    local windowId = "##Toast_" .. tostring(toast.id or 0)
    
    imgui.SetNextWindowPos({posX, posY}, ImGuiCond_Always)
    
    local alpha = toast.alpha or 1.0
    if isFirstFrame then
        alpha = 0.0
    end
    
    imgui.PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0)
    imgui.PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0)
    imgui.PushStyleVar(ImGuiStyleVar_WindowPadding, {10, 8})
    imgui.PushStyleColor(ImGuiCol_WindowBg, {0.1, 0.1, 0.1, 0.9 * alpha})
    
    local beginResult = imgui.Begin(windowId, true, TOAST_WINDOW_FLAGS)
    if not beginResult then
        imgui.PopStyleColor(1)
        imgui.PopStyleVar(3)
        imgui.End()
        return 60
    end
    
    -- Apply font scale
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.themes then
        local fontScale = app.features.themes:getFontScale() or 1.0
        imgui.SetWindowFontScale(fontScale)
    end
    
    local color = getToastColor(toast.type or ToastType.Info)
    
    imgui.PushStyleColor(ImGuiCol_Text, {color[1], color[2], color[3], alpha})
    imgui.Text(toast.title or "")
    imgui.PopStyleColor()
    
    imgui.PushStyleColor(ImGuiCol_Text, {1.0, 1.0, 1.0, alpha * 0.9})
    imgui.Text(toast.message or "")
    imgui.PopStyleColor()
    
    local windowHeight = 60
    local _, height = imgui.GetWindowSize()
    if height and height > 0 then
        windowHeight = height
    end
    
    toast.lastHeight = windowHeight
    
    imgui.End()
    imgui.PopStyleColor(1)
    imgui.PopStyleVar(3)
    
    return windowHeight
end

function M.DrawWindow(settings, dataModule)
    if not state.initialized or state.hidden then
        return
    end
    
    if not dataModule then
        return
    end
    
    local rawToasts = dataModule.GetToasts()
    if not rawToasts then
        rawToasts = {}
    end
    
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.preferences then
        local prefs = app.features.preferences:getPrefs()
        if prefs then
            state.positionX = prefs.notificationPositionX or DEFAULT_POSITION_X
            state.positionY = prefs.notificationPositionY or DEFAULT_POSITION_Y
        end
    end
    
    local currentTime = getCurrentTimeMs()
    local prefsDurationMs = getDurationFromPrefs()
    
    local seenIds = {}
    for i, toast in ipairs(rawToasts) do
        local toastId = toast.id or i
        seenIds[toastId] = true
        
        if not state.lastToastIds[toastId] then
            toast.state = ToastState.VISIBLE
            toast.alpha = 1.0
            if not toast.createdAt or toast.createdAt == 0 then
                toast.createdAt = currentTime
            end
            if not toast.duration or toast.duration == 0 then
                toast.duration = prefsDurationMs
            end
            maybePlaySound(toast)
            state.lastToastIds[toastId] = true
        end
        
        updateToast(toast, currentTime, prefsDurationMs)
    end
    
    for id, _ in pairs(state.lastToastIds) do
        if not seenIds[id] then
            state.lastToastIds[id] = nil
        end
    end
    
    local activeToasts = {}
    for _, toast in ipairs(rawToasts) do
        if toast.state ~= ToastState.COMPLETE then
            table.insert(activeToasts, toast)
        end
    end
    
    if #activeToasts == 0 then
        return
    end
    
    local themePushed = false
    if app and app.features and app.features.themes then
        local themesFeature = app.features.themes
        local themeIndex = themesFeature:getThemeIndex()
        
        if themeIndex ~= -2 then
            local success, err = pcall(function()
                local themeColors = themesFeature:getCurrentThemeColors()
                local backgroundAlpha = themesFeature:getBackgroundAlpha()
                local textAlpha = themesFeature:getTextAlpha()
                
                if themeColors then
                    themePushed = ThemeHelper.pushThemeStyles(themeColors, backgroundAlpha, textAlpha)
                end
            end)
        end
    end
    
    local targetY = state.positionY
    for i = 1, #activeToasts do
        local toast = activeToasts[i]
        local toastHeight = toast.lastHeight or 60
        renderToast(toast, targetY)
        if toast.state ~= ToastState.EXITING then
            targetY = targetY + toastHeight + TOAST_SPACING
        end
    end
    
    if themePushed then
        pcall(ThemeHelper.popThemeStyles)
    end
end

function M.Cleanup()
    state.initialized = false
    state.lastToastIds = {}
    state.activeToasts = {}
end

return M
