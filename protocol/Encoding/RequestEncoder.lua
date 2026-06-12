-- RequestEncoder.lua
-- Request message encoding

local Json = require("protocol.Json")

local M = {}

-- Normalize character/realm fields for HTTP payloads
local function normalizeCharacterName(name)
    if not name then return "" end
    local trimmed = tostring(name):match("^%s*(.-)%s*$") or ""
    return string.lower(trimmed)
end

local function normalizeRealmId(realmId)
    if not realmId then return "unknown" end
    local trimmed = tostring(realmId):match("^%s*(.-)%s*$") or ""
    if trimmed == "" then return "unknown" end
    return string.lower(trimmed)
end
-- ============================================================================
-- New Server Endpoints (friendlist-server)
-- ============================================================================

-- Encode Register request (for /api/auth/register endpoint)
-- Creates a new account with the given character (no auth required)
-- Body: {"characterName": "...", "realmId": "..."}
function M.encodeRegister(characterName, realmId)
    local body = {
        characterName = characterName,
        realmId = realmId or "unknown"
    }
    return Json.encode(body)
end

-- Encode AddCharacter request (for /api/auth/add-character endpoint)
-- Adds a character to the authenticated account
-- Body: {"name": "...", "realmId": "..."}
function M.encodeAddCharacter(characterName, realmId)
    local body = {
        name = characterName,
        realmId = realmId or "unknown"
    }
    return Json.encode(body)
end

-- Encode SetActiveCharacter request (for /api/auth/set-active endpoint)
-- Sets the active character for the account
-- Body: {"characterId": "uuid"}
function M.encodeSetActiveCharacter(characterId)
    local body = {
        characterId = characterId
    }
    return Json.encode(body)
end

-- Encode SendFriendRequest for new server (for /api/friends/request endpoint)
-- Body: {"characterName": "...", "realmId": "..."}
function M.encodeNewSendFriendRequest(characterName, realmId)
    local body = {
        characterName = normalizeCharacterName(characterName),
        realmId = normalizeRealmId(realmId)
    }
    return Json.encode(body)
end

-- Encode Block request (for /api/block endpoint)
-- Body: {"characterName": "...", "realmId": "..."}
function M.encodeBlock(characterName, realmId)
    local body = {
        characterName = normalizeCharacterName(characterName),
        realmId = normalizeRealmId(realmId)
    }
    return Json.encode(body)
end

-- Encode PresenceUpdate request (for /api/presence/update endpoint)
-- Body: { job?, subJob?, jobLevel?, subJobLevel?, zone?, nation?, rank?, isAnonymous? }
-- Server expects:
--   job: string (e.g., "WHM", "SMN") - main job only
--   subJob: string (e.g., "BLM", "WHM") - sub job only  
--   jobLevel: number (1-99)
--   subJobLevel: number (0-49)
--   nation: string (e.g., "San d'Oria", "Bastok", "Windurst")
--   rank: number (1-10)
function M.encodePresenceUpdate(presence)
    local body = {}
    
    -- Only send job info if we have valid levels (>= 1)
    local hasValidJob = (presence.mainJobLevel and presence.mainJobLevel > 0) or (presence.jobLevel and presence.jobLevel > 0)
    
    if hasValidJob then
        -- Use mainJob numeric ID if available, otherwise extract from job string
        if presence.mainJob then
            -- Convert numeric job ID to abbreviation
            local jobNames = {
                "NON", "WAR", "MNK", "WHM", "BLM", "RDM", "THF", "PLD", "DRK", "BST",
                "BRD", "RNG", "SAM", "NIN", "DRG", "SMN", "BLU", "COR", "PUP", "DNC",
                "SCH", "GEO", "RUN"
            }
            body.job = jobNames[presence.mainJob + 1] or "NON"
        elseif presence.job and presence.job ~= "" then
            -- Fallback: extract from formatted string
            local jobAbbr = presence.job:match("^(%u+)") or ""
            if jobAbbr ~= "" then
                body.job = jobAbbr
            end
        end
        
        -- Send job levels
        if presence.mainJobLevel and presence.mainJobLevel > 0 then
            body.jobLevel = presence.mainJobLevel
        elseif presence.jobLevel and presence.jobLevel > 0 then
            body.jobLevel = presence.jobLevel
        end
        
        -- SubJob and level
        if presence.subJob then body.subJob = presence.subJob end
        if presence.subJobLevel and presence.subJobLevel > 0 then
            body.subJobLevel = presence.subJobLevel
        end
    end
    
    if presence.zone then body.zone = presence.zone end
    
    -- Nation: convert number to string
    -- 0 = San d'Oria, 1 = Bastok, 2 = Windurst
    if presence.nation ~= nil then
        local nationNames = {
            [0] = "San d'Oria",
            [1] = "Bastok",
            [2] = "Windurst"
        }
        if type(presence.nation) == "number" then
            body.nation = nationNames[presence.nation] or "Unknown"
        else
            body.nation = tostring(presence.nation)
        end
    end
    
    -- Rank: extract number from "Rank X" string
    if presence.rank ~= nil then
        if type(presence.rank) == "string" then
            local rankNum = presence.rank:match("(%d+)")
            body.rank = tonumber(rankNum) or 1
        elseif type(presence.rank) == "number" then
            body.rank = presence.rank
        end
    end
    
    if presence.isAnonymous ~= nil then body.isAnonymous = presence.isAnonymous end
    return Json.encode(body)
end

-- Encode Heartbeat request (for /api/presence/heartbeat endpoint)
-- Body: { timestamp?: string, characterId?: string }
function M.encodeHeartbeat(timestamp, characterId)
    local body = {}
    if timestamp then body.timestamp = timestamp end
    if characterId then body.characterId = characterId end
    return Json.encode(body)
end

-- Encode UpdatePreferences request (for PATCH /api/preferences endpoint)
-- Body: { presenceStatus?, shareLocation?, shareJobWhenAnonymous? }
function M.encodeUpdatePreferences(prefs)
    local body = {}
    if prefs.presenceStatus then body.presenceStatus = prefs.presenceStatus end
    if prefs.shareLocation ~= nil then body.shareLocation = prefs.shareLocation end
    if prefs.shareJobWhenAnonymous ~= nil then body.shareJobWhenAnonymous = prefs.shareJobWhenAnonymous end
    return Json.encode(body)
end

return M

