local M = {}

M.API_PREFIX = "/api"

M.AUTH = {
    ENSURE = "/api/auth/ensure"
}

M.HEARTBEAT = "/api/heartbeat"

M.FRIENDS = {
    LIST = "/api/friends",
    REQUESTS = "/api/friends/requests",
    SEND_REQUEST = "/api/friends/requests/request",
    ACCEPT = "/api/friends/requests/accept",
    REJECT = "/api/friends/requests/reject",
    CANCEL = "/api/friends/requests/cancel",
    VISIBILITY = "/api/friends/visibility"
}

M.CHARACTERS = {
    LIST = "/api/characters",
    ACTIVE = "/api/characters/active",
    STATE = "/api/characters/state",
    PRIVACY = "/api/characters/privacy"
}

M.PREFERENCES = "/api/preferences"

M.NOTES = {
    LIST = "/api/notes"
}

M.BLOCK = {
    LIST = "/api/blocked",
    ADD = "/api/block"
}

M.SERVERS = "/api/servers"

function M.friendDelete(friendName)
    local encodedName = friendName:gsub("([^%w%-%.%_])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
    return "/api/friends/" .. encodedName
end

function M.noteGet(friendName)
    local encodedName = friendName:gsub("([^%w%-%.%_])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
    return "/api/notes/" .. encodedName
end

function M.noteUpdate(friendName)
    return M.noteGet(friendName)
end

function M.noteDelete(friendName)
    return M.noteGet(friendName)
end

function M.friendVisibilityDelete(friendName)
    local encodedName = friendName:gsub("([^%w%-%.%_])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
    return "/api/friends/" .. encodedName .. "/visibility"
end

function M.blockRemove(blockedAccountId)
    return "/api/block/" .. tostring(blockedAccountId)
end

return M

