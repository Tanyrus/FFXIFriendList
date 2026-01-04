-- UrlBuilder.lua
-- URL/path/query construction helpers

local M = {}

-- URL encode a string (percent encoding)
local function urlEncode(str)
    if not str then
        return ""
    end
    
    local result = {}
    for i = 1, #str do
        local c = str:sub(i, i)
        local byte = string.byte(c)
        if (byte >= 48 and byte <= 57) or  -- 0-9
           (byte >= 65 and byte <= 90) or   -- A-Z
           (byte >= 97 and byte <= 122) or  -- a-z
           c == '-' or c == '_' or c == '.' or c == '~' then
            table.insert(result, c)
        else
            table.insert(result, string.format("%%%02X", byte))
        end
    end
    return table.concat(result)
end

-- Build query string from parameters
function M.buildQueryString(params)
    if not params or #params == 0 then
        return ""
    end
    
    local parts = {}
    for i = 1, #params do
        local key, value = params[i][1], params[i][2]
        if value ~= nil then
            table.insert(parts, urlEncode(key) .. "=" .. urlEncode(tostring(value)))
        end
    end
    
    if #parts == 0 then
        return ""
    end
    
    return "?" .. table.concat(parts, "&")
end

-- Build URL with path and optional query string
function M.buildUrl(baseUrl, path, queryParams)
    local url = baseUrl
    if not url:match("/$") and path and not path:match("^/") then
        url = url .. "/"
    end
    if path then
        url = url .. path
    end
    if queryParams and #queryParams > 0 then
        url = url .. M.buildQueryString(queryParams)
    end
    return url
end

return M

