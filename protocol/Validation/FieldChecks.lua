-- FieldChecks.lua
-- Field-level validation helpers (presence/type/range)

local Limits = require("constants.limits")

local M = {}

-- Constants (re-exported from centralized limits)
M.MAX_FRIEND_LIST_SIZE = Limits.MAX_FRIEND_LIST_SIZE
M.MAX_CHARACTER_NAME_LENGTH = Limits.CHARACTER_NAME_MAX

-- Check if character name is valid
function M.validateCharacterName(name)
    if not name or name == "" then
        return false, "Character name is required"
    end
    
    if #name > M.MAX_CHARACTER_NAME_LENGTH then
        return false, "Character name too long"
    end
    
    -- Allow alphanumeric, space, dash, underscore
    for i = 1, #name do
        local c = name:sub(i, i)
        local byte = string.byte(c)
        local isAlnum = (byte >= 48 and byte <= 57) or  -- 0-9
                        (byte >= 65 and byte <= 90) or  -- A-Z
                        (byte >= 97 and byte <= 122)    -- a-z
        if not isAlnum and c ~= " " and c ~= "-" and c ~= "_" then
            return false, "Character name contains invalid characters"
        end
    end
    
    return true, nil
end

-- Check if friend list size is valid
function M.validateFriendListSize(count)
    if count > M.MAX_FRIEND_LIST_SIZE then
        return false, "Friend list size exceeds maximum"
    end
    return true, nil
end

return M

