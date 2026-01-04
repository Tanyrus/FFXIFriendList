--[[
* libs/packets.lua
* Packet parsing functions (called from entry file for routing)
* All packet parsing functions return parsed packet table or nil
]]--

local M = {}

-- Helper: Read string from packet data
-- Note: Ashita packet data is a string, use string.byte() to get bytes
local function readString(data, offset, length)
    if not data or type(data) ~= "string" or offset < 0 or length <= 0 then
        return nil, offset
    end
    if offset + length > #data then
        return nil, offset
    end
    
    local str = ""
    for i = offset + 1, offset + length do  -- Lua strings are 1-indexed
        local byte = string.byte(data, i)
        if byte == 0 then
            break  -- Null terminator
        end
        str = str .. string.char(byte)
    end
    
    return str, offset + length
end

-- Helper: Read uint8 from packet data
-- Note: Ashita packet data is a string, use string.byte() to get bytes
local function readUint8(data, offset)
    if not data or type(data) ~= "string" or offset < 0 or offset >= #data then
        return nil, offset
    end
    return string.byte(data, offset + 1), offset + 1  -- Lua strings are 1-indexed
end

-- Helper: Read uint16 (little-endian) from packet data
-- Note: Ashita packet data is a string, use string.byte() to get bytes
local function readUint16(data, offset)
    if not data or type(data) ~= "string" or offset < 0 or offset + 1 >= #data then
        return nil, offset
    end
    local low = string.byte(data, offset + 1)   -- Lua strings are 1-indexed
    local high = string.byte(data, offset + 2)
    return low + (high * 256), offset + 2
end

-- Helper: Read uint32 (little-endian) from packet data
-- Note: Ashita packet data is a string, use string.byte() to get bytes
local function readUint32(data, offset)
    if not data or type(data) ~= "string" or offset < 0 or offset + 3 >= #data then
        return nil, offset
    end
    local b1 = string.byte(data, offset + 1)   -- Lua strings are 1-indexed
    local b2 = string.byte(data, offset + 2)
    local b3 = string.byte(data, offset + 3)
    local b4 = string.byte(data, offset + 4)
    return b1 + (b2 * 256) + (b3 * 65536) + (b4 * 16777216), offset + 4
end

-- Parse zone packet (0x00A) - Zone change
function M.ParseZonePacket(e)
    if not e or e.id ~= 0x00A or e.size < 4 then
        return nil
    end
    
    local data = e.data
    local offset = 0
    
    local zoneId, err = readUint16(data, offset)
    if not zoneId then return nil end
    
    return {
        type = "zone",
        zoneId = zoneId
    }
end

-- Generic packet parser (returns basic info if no specific parser exists)
function M.ParseGenericPacket(e)
    if not e then
        return nil
    end
    
    return {
        type = "generic",
        id = e.id,
        size = e.size
    }
end

return M
