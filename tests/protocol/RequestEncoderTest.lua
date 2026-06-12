-- RequestEncoderTest.lua
-- Tests the LIVE request encoders (the ones app/features actually call). Each
-- returns a JSON body string for the new server endpoints; we decode and assert
-- the wire fields. The retired legacy encoders + MessageTypes were removed.

local RequestEncoder = require("protocol.Encoding.RequestEncoder")
local Json = require("protocol.Json")

local function assert(condition, message)
    if not condition then
        error("ASSERTION FAILED: " .. (message or "unknown"))
    end
end

local function decodeBody(encoded)
    assert(type(encoded) == "string", "encoder should return a JSON string")
    local ok, body = Json.decode(encoded)
    assert(ok, "encoder output should be valid JSON")
    return body
end

local function testEncodeRegister()
    local body = decodeBody(RequestEncoder.encodeRegister("Tarutaru", "horizon"))
    assert(body.characterName == "Tarutaru", "characterName passed through")
    assert(body.realmId == "horizon", "realmId passed through")
    -- realmId defaults to "unknown" when absent.
    local body2 = decodeBody(RequestEncoder.encodeRegister("Tarutaru"))
    assert(body2.realmId == "unknown", "missing realmId defaults to unknown")
    print("✓ testEncodeRegister passed")
end

local function testEncodeAddCharacter()
    local body = decodeBody(RequestEncoder.encodeAddCharacter("Bob", "horizon"))
    assert(body.name == "Bob", "add-character uses 'name' (not characterName)")
    assert(body.realmId == "horizon", "realmId passed through")
    print("✓ testEncodeAddCharacter passed")
end

local function testEncodeSetActiveCharacter()
    local body = decodeBody(RequestEncoder.encodeSetActiveCharacter("uuid-123"))
    assert(body.characterId == "uuid-123", "characterId passed through")
    print("✓ testEncodeSetActiveCharacter passed")
end

local function testEncodeBlockNormalizes()
    -- Block (and the new friend-request) lowercase/trim name and realm.
    local body = decodeBody(RequestEncoder.encodeBlock("  MixedCase  ", "  Horizon  "))
    assert(body.characterName == "mixedcase", "characterName lowercased and trimmed")
    assert(body.realmId == "horizon", "realmId lowercased and trimmed")
    -- Empty realm normalizes to "unknown".
    local body2 = decodeBody(RequestEncoder.encodeBlock("Name", ""))
    assert(body2.realmId == "unknown", "empty realmId normalizes to unknown")
    print("✓ testEncodeBlockNormalizes passed")
end

local function testEncodeNewSendFriendRequest()
    local body = decodeBody(RequestEncoder.encodeNewSendFriendRequest("FriendName", "Horizon"))
    assert(body.characterName == "friendname", "characterName normalized")
    assert(body.realmId == "horizon", "realmId normalized")
    print("✓ testEncodeNewSendFriendRequest passed")
end

local function testEncodePresenceUpdateFull()
    local body = decodeBody(RequestEncoder.encodePresenceUpdate({
        mainJob = 3,            -- WHM (index+1 into the abbrev table)
        mainJobLevel = 99,
        subJob = "BLM",
        subJobLevel = 49,
        zone = "San d'Oria",
        nation = 1,             -- Bastok
        rank = "Rank 10",
        isAnonymous = false,
    }))
    assert(body.job == "WHM", "numeric mainJob maps to abbreviation")
    assert(body.jobLevel == 99, "jobLevel encoded")
    assert(body.subJob == "BLM", "subJob encoded")
    assert(body.subJobLevel == 49, "subJobLevel encoded")
    assert(body.zone == "San d'Oria", "zone encoded")
    assert(body.nation == "Bastok", "numeric nation maps to name")
    assert(body.rank == 10, "rank extracted from 'Rank 10'")
    assert(body.isAnonymous == false, "isAnonymous encoded")
    print("✓ testEncodePresenceUpdateFull passed")
end

local function testEncodePresenceUpdateOmitsJobWithoutLevel()
    -- No valid job level -> job fields are omitted entirely.
    local body = decodeBody(RequestEncoder.encodePresenceUpdate({
        job = "WAR",
        zone = "Bastok Markets",
        isAnonymous = true,
    }))
    assert(body.job == nil, "job omitted when no level present")
    assert(body.jobLevel == nil, "jobLevel omitted")
    assert(body.zone == "Bastok Markets", "zone still encoded")
    assert(body.isAnonymous == true, "isAnonymous still encoded")
    print("✓ testEncodePresenceUpdateOmitsJobWithoutLevel passed")
end

local function testEncodeHeartbeat()
    local body = decodeBody(RequestEncoder.encodeHeartbeat("2026-06-11T00:00:00Z", "cid"))
    assert(body.timestamp == "2026-06-11T00:00:00Z", "timestamp encoded")
    assert(body.characterId == "cid", "characterId encoded")
    -- Empty heartbeat is a valid empty object.
    local body2 = decodeBody(RequestEncoder.encodeHeartbeat())
    assert(body2.timestamp == nil and body2.characterId == nil, "empty heartbeat omits both")
    print("✓ testEncodeHeartbeat passed")
end

local function testEncodeUpdatePreferences()
    -- shareLocation=false must be preserved (uses ~= nil, not truthiness).
    local body = decodeBody(RequestEncoder.encodeUpdatePreferences({
        presenceStatus = "away",
        shareLocation = false,
        shareJobWhenAnonymous = true,
    }))
    assert(body.presenceStatus == "away", "presenceStatus encoded")
    assert(body.shareLocation == false, "shareLocation=false preserved")
    assert(body.shareJobWhenAnonymous == true, "shareJobWhenAnonymous encoded")
    print("✓ testEncodeUpdatePreferences passed")
end

local function runAllTests()
    print("Running RequestEncoder tests...")
    testEncodeRegister()
    testEncodeAddCharacter()
    testEncodeSetActiveCharacter()
    testEncodeBlockNormalizes()
    testEncodeNewSendFriendRequest()
    testEncodePresenceUpdateFull()
    testEncodePresenceUpdateOmitsJobWithoutLevel()
    testEncodeHeartbeat()
    testEncodeUpdatePreferences()
    print("All RequestEncoder tests passed!")
end

if arg and arg[0] and arg[0]:match("RequestEncoderTest") then
    runAllTests()
end

return {
    runAllTests = runAllTests
}
