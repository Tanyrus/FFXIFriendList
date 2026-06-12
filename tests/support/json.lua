-- tests/support/json.lua
-- Compact pure-Lua JSON encoder/decoder used ONLY by the test harness as a
-- stand-in for Ashita's `json` library. Adapted from the well-known rxi/json.lua
-- (MIT). Behaviour-compatible for the encode(table)/decode(str) calls the addon
-- makes via protocol/Json.lua.

local json = {}

-- ---------------------------------------------------------------- encode -----
local encode

local escape_char_map = {
    ["\\"] = "\\\\", ["\""] = "\\\"", ["\b"] = "\\b", ["\f"] = "\\f",
    ["\n"] = "\\n", ["\r"] = "\\r", ["\t"] = "\\t",
}

local function escape_char(c)
    return escape_char_map[c] or string.format("\\u%04x", c:byte())
end

local function encode_nil() return "null" end

local function encode_string(val)
    return '"' .. val:gsub('[%z\1-\31\\"]', escape_char) .. '"'
end

local function encode_number(val)
    if val ~= val or val <= -math.huge or val >= math.huge then
        error("unexpected number value '" .. tostring(val) .. "'")
    end
    -- integers without trailing .0
    if math.type and math.type(val) == "integer" then
        return string.format("%d", val)
    end
    if val == math.floor(val) and math.abs(val) < 1e15 then
        return string.format("%d", val)
    end
    return string.format("%.14g", val)
end

local function encode_table(val)
    local out = {}
    -- determine array vs object
    local n = 0
    for _ in pairs(val) do n = n + 1 end
    local arrayLen = #val
    if arrayLen > 0 and arrayLen == n then
        for i = 1, arrayLen do
            out[i] = encode(val[i])
        end
        return "[" .. table.concat(out, ",") .. "]"
    elseif n == 0 then
        return "[]" -- empty table encodes as array (matches typical Lua JSON)
    else
        local parts = {}
        for k, v in pairs(val) do
            if type(k) ~= "string" and type(k) ~= "number" then
                error("invalid table key type: " .. type(k))
            end
            parts[#parts + 1] = encode_string(tostring(k)) .. ":" .. encode(v)
        end
        return "{" .. table.concat(parts, ",") .. "}"
    end
end

local encode_map = {
    ["nil"] = encode_nil,
    ["table"] = encode_table,
    ["string"] = encode_string,
    ["number"] = encode_number,
    ["boolean"] = function(v) return tostring(v) end,
}

encode = function(val)
    local t = type(val)
    local f = encode_map[t]
    if f then return f(val) end
    error("unexpected type '" .. t .. "'")
end

function json.encode(val)
    return encode(val)
end

-- ---------------------------------------------------------------- decode -----
local parse

local function create_set(...)
    local res = {}
    for _, v in ipairs({ ... }) do res[v] = true end
    return res
end

local space_chars  = create_set(" ", "\t", "\r", "\n")
local delim_chars  = create_set(" ", "\t", "\r", "\n", "]", "}", ",")
local escape_chars = create_set("\\", "/", '"', "b", "f", "n", "r", "t", "u")
local literals     = create_set("true", "false", "null")

local literal_map = { ["true"] = true, ["false"] = false, ["null"] = nil }

local function next_char(str, idx, set, negate)
    for i = idx, #str do
        if set[str:sub(i, i)] ~= negate then return i end
    end
    return #str + 1
end

local function decode_error(str, idx, msg)
    error(string.format("%s at position %d", msg, idx))
end

local function parse_string(str, i)
    local res = ""
    local j = i + 1
    local k = j
    while j <= #str do
        local x = str:byte(j)
        if x < 32 then
            decode_error(str, j, "control character in string")
        elseif x == 92 then -- backslash
            res = res .. str:sub(k, j - 1)
            j = j + 1
            local c = str:sub(j, j)
            if c == "u" then
                local hex = str:sub(j + 1, j + 4)
                if not hex:find("^[%da-fA-F][%da-fA-F][%da-fA-F][%da-fA-F]$") then
                    decode_error(str, j, "invalid unicode escape")
                end
                local n = tonumber(hex, 16)
                -- encode as UTF-8 (BMP only)
                if n < 0x80 then
                    res = res .. string.char(n)
                elseif n < 0x800 then
                    res = res .. string.char(0xC0 + math.floor(n / 0x40), 0x80 + (n % 0x40))
                else
                    res = res .. string.char(
                        0xE0 + math.floor(n / 0x1000),
                        0x80 + (math.floor(n / 0x40) % 0x40),
                        0x80 + (n % 0x40))
                end
                j = j + 4
            else
                if not escape_chars[c] then
                    decode_error(str, j, "invalid escape char '" .. c .. "'")
                end
                local em = { ["\\"] = "\\", ["/"] = "/", ['"'] = '"', b = "\b",
                             f = "\f", n = "\n", r = "\r", t = "\t" }
                res = res .. (em[c] or c)
            end
            k = j + 1
        elseif x == 34 then -- closing quote
            res = res .. str:sub(k, j - 1)
            return res, j + 1
        end
        j = j + 1
    end
    decode_error(str, i, "expected closing quote for string")
end

local function parse_number(str, i)
    local x = next_char(str, i, delim_chars)
    local s = str:sub(i, x - 1)
    local n = tonumber(s)
    if not n then decode_error(str, i, "invalid number '" .. s .. "'") end
    return n, x
end

local function parse_literal(str, i)
    local x = next_char(str, i, delim_chars)
    local word = str:sub(i, x - 1)
    if not literals[word] then decode_error(str, i, "invalid literal '" .. word .. "'") end
    return literal_map[word], x
end

local function parse_array(str, i)
    local res = {}
    local n = 1
    i = i + 1
    while true do
        local x
        i = next_char(str, i, space_chars, true)
        if str:sub(i, i) == "]" then i = i + 1; break end
        x, i = parse(str, i)
        res[n] = x
        n = n + 1
        i = next_char(str, i, space_chars, true)
        local c = str:sub(i, i)
        i = i + 1
        if c == "]" then break end
        if c ~= "," then decode_error(str, i, "expected ']' or ','") end
    end
    return res, i
end

local function parse_object(str, i)
    local res = {}
    i = i + 1
    while true do
        local key, val
        i = next_char(str, i, space_chars, true)
        if str:sub(i, i) == "}" then i = i + 1; break end
        if str:sub(i, i) ~= '"' then decode_error(str, i, "expected string key") end
        key, i = parse_string(str, i)
        i = next_char(str, i, space_chars, true)
        if str:sub(i, i) ~= ":" then decode_error(str, i, "expected ':'") end
        i = next_char(str, i + 1, space_chars, true)
        val, i = parse(str, i)
        res[key] = val
        i = next_char(str, i, space_chars, true)
        local c = str:sub(i, i)
        i = i + 1
        if c == "}" then break end
        if c ~= "," then decode_error(str, i, "expected '}' or ','") end
    end
    return res, i
end

local char_func_map = {
    ['"'] = parse_string,
    ["0"] = parse_number, ["1"] = parse_number, ["2"] = parse_number,
    ["3"] = parse_number, ["4"] = parse_number, ["5"] = parse_number,
    ["6"] = parse_number, ["7"] = parse_number, ["8"] = parse_number,
    ["9"] = parse_number, ["-"] = parse_number,
    ["t"] = parse_literal, ["f"] = parse_literal, ["n"] = parse_literal,
    ["["] = parse_array, ["{"] = parse_object,
}

parse = function(str, idx)
    local chr = str:sub(idx, idx)
    local f = char_func_map[chr]
    if f then return f(str, idx) end
    decode_error(str, idx, "unexpected character '" .. chr .. "'")
end

function json.decode(str)
    if type(str) ~= "string" then error("expected argument of type string") end
    local res, idx = parse(str, next_char(str, 1, space_chars, true))
    idx = next_char(str, idx, space_chars, true)
    if idx <= #str then decode_error(str, idx, "trailing garbage") end
    return res
end

return json
