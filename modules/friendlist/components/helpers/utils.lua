local M = {}

function M.capitalizeName(name)
    if not name or name == "" then
        return name or ""
    end
    
    local result = ""
    local capitalizeNext = true
    
    for i = 1, #name do
        local char = string.sub(name, i, i)
        if char == " " then
            capitalizeNext = true
            result = result .. char
        else
            if capitalizeNext then
                result = result .. string.upper(char)
                capitalizeNext = false
            else
                result = result .. string.lower(char)
            end
        end
    end
    
    return result
end

function M.formatRelativeTime(timestampMs)
    if type(timestampMs) ~= "number" or timestampMs <= 0 then
        return "Unknown"
    end
    
    local now = os.time()
    local diff = now - (timestampMs / 1000)
    
    if diff < 60 then
        return "Just now"
    elseif diff < 3600 then
        return math.floor(diff / 60) .. "m ago"
    elseif diff < 86400 then
        return math.floor(diff / 3600) .. "h ago"
    elseif diff < 604800 then
        return math.floor(diff / 86400) .. "d ago"
    else
        return os.date("%m/%d/%Y", timestampMs / 1000)
    end
end

return M

