-- Cross-platform "open this URL in the default browser" helper.
-- Opening a browser legitimately needs a shell (there is no Ashita API for it),
-- but it must be cross-platform and live in one place — previously TopBar had a
-- portable version while two Discord buttons hardcoded Windows-only `start`.

local M = {}

function M.open(url)
    if not url or url == "" or not os.execute then
        return false
    end
    local cmd
    if package.config:sub(1, 1) == "\\" then
        cmd = 'start "" "' .. url .. '"'
    else
        cmd = 'xdg-open "' .. url .. '" 2>/dev/null || open "' .. url .. '"'
    end
    os.execute(cmd)
    return true
end

return M
