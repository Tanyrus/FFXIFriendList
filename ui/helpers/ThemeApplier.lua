-- Shared addon-theme push/pop used by every window's render entry point.
-- Centralizes the pcall-guarded ThemeHelper handshake that was previously
-- copy-pasted across the friendlist and quickonline modules (and was missing
-- entirely in quickonline's server-not-detected window, causing a nil-call crash).
--
-- This covers ONLY the addon theme. Overlay/transparency styling (quickonline's
-- compact view) is a separate concern and stays in that module.

local ThemeHelper = require('libs.themehelper')

local M = {}

-- Push the current addon theme. Returns a token to hand back to pop().
-- No-op (returns false) when the Ashita default theme is selected (index -2)
-- or the themes feature is unavailable.
function M.apply(logTag)
    local themePushed = false
    local app = _G.FFXIFriendListApp
    if app and app.features and app.features.themes then
        local success, err = pcall(function()
            local themesFeature = app.features.themes
            local themeIndex = themesFeature:getThemeIndex()

            if themeIndex ~= -2 then
                local themeColors = themesFeature:getCurrentThemeColors()
                local backgroundAlpha = themesFeature:getBackgroundAlpha()
                local textAlpha = themesFeature:getTextAlpha()

                if themeColors then
                    themePushed = ThemeHelper.pushThemeStyles(themeColors, backgroundAlpha, textAlpha)
                end
            end
        end)
        if not success then
            if app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[" .. (logTag or "ThemeApplier") .. "] Theme application failed: " .. tostring(err))
            end
        end
    end
    return themePushed
end

-- Pop a theme previously pushed by apply(). Safe to call with a false token.
function M.pop(themePushed, logTag)
    if themePushed then
        local success, err = pcall(ThemeHelper.popThemeStyles)
        if not success then
            local app = _G.FFXIFriendListApp
            if app and app.deps and app.deps.logger and app.deps.logger.error then
                app.deps.logger.error("[" .. (logTag or "ThemeApplier") .. "] Theme pop failed: " .. tostring(err))
            end
        end
    end
end

return M
