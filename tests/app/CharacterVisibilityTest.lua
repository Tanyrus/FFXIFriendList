--[[
* Character Visibility Feature Tests
* Tests for the Per-Character Visibility feature
]]--

local M = {}

-- Mock dependencies
local function createMockDeps()
    local mockNet = {
        lastRequest = nil,
        request = function(options)
            mockNet.lastRequest = options
            -- Simulate async callback
            if options.callback then
                -- Default success response
                options.callback(true, '{"success":true,"data":{"characters":[]}}')
            end
        end
    }
    
    local mockLog = {
        messages = {},
        info = function(msg) table.insert(mockLog.messages, {level = "info", msg = msg}) end,
        error = function(msg) table.insert(mockLog.messages, {level = "error", msg = msg}) end,
        debug = function(msg) table.insert(mockLog.messages, {level = "debug", msg = msg}) end
    }
    
    local mockTime = function()
        return os.time() * 1000
    end
    
    return {
        net = mockNet,
        log = mockLog,
        time = mockTime
    }
end

-- Test: CharacterVisibility.new creates valid instance
local function testCharacterVisibilityNew()
    local CharacterVisibility = require("app.features.characterVisibility")
    local deps = createMockDeps()
    
    local feature = CharacterVisibility.CharacterVisibility.new(deps)
    
    assert(feature ~= nil, "CharacterVisibility.new should return instance")
    assert(type(feature.getState) == "function", "Should have getState method")
    assert(type(feature.fetch) == "function", "Should have fetch method")
    assert(type(feature.setVisibility) == "function", "Should have setVisibility method")
    assert(type(feature.toggleVisibility) == "function", "Should have toggleVisibility method")
    assert(type(feature.isCharacterVisible) == "function", "Should have isCharacterVisible method")
    
    print("[PASS] testCharacterVisibilityNew")
end

-- Test: getState returns correct structure
local function testGetState()
    local CharacterVisibility = require("app.features.characterVisibility")
    local deps = createMockDeps()
    
    local feature = CharacterVisibility.CharacterVisibility.new(deps)
    local state = feature:getState()
    
    assert(type(state) == "table", "getState should return table")
    assert(type(state.characters) == "table", "state.characters should be table")
    assert(type(state.isLoading) == "boolean", "state.isLoading should be boolean")
    assert(state.isLoading == false, "Initial isLoading should be false")
    
    print("[PASS] testGetState")
end

-- Test: _updateFromServer populates characters correctly
local function testUpdateFromServer()
    local CharacterVisibility = require("app.features.characterVisibility")
    local deps = createMockDeps()
    
    local feature = CharacterVisibility.CharacterVisibility.new(deps)
    
    local serverData = {
        {
            characterId = "char-1",
            characterName = "MainChar",
            realmId = "horizon",
            shareVisibility = true
        },
        {
            characterId = "char-2",
            characterName = "AltChar",
            realmId = "horizon",
            shareVisibility = false
        }
    }
    
    feature:_updateFromServer(serverData)
    
    local characters = feature:getCharacters()
    assert(#characters == 2, "Should have 2 characters")
    
    -- Characters should be sorted by name
    assert(characters[1].characterName == "AltChar", "First should be AltChar (alphabetically)")
    assert(characters[2].characterName == "MainChar", "Second should be MainChar")
    
    -- Visibility should match
    assert(characters[1].shareVisibility == false, "AltChar should be hidden")
    assert(characters[2].shareVisibility == true, "MainChar should be visible")
    
    print("[PASS] testUpdateFromServer")
end

-- Test: isCharacterVisible returns correct value
local function testIsCharacterVisible()
    local CharacterVisibility = require("app.features.characterVisibility")
    local deps = createMockDeps()
    
    local feature = CharacterVisibility.CharacterVisibility.new(deps)
    
    feature:_updateFromServer({
        {characterId = "visible-char", characterName = "Visible", realmId = "test", shareVisibility = true},
        {characterId = "hidden-char", characterName = "Hidden", realmId = "test", shareVisibility = false}
    })
    
    assert(feature:isCharacterVisible("visible-char") == true, "Visible char should be visible")
    assert(feature:isCharacterVisible("hidden-char") == false, "Hidden char should not be visible")
    assert(feature:isCharacterVisible("unknown-char") == true, "Unknown char should default to visible")
    
    print("[PASS] testIsCharacterVisible")
end

-- Test: _setLocalVisibility updates local state
local function testSetLocalVisibility()
    local CharacterVisibility = require("app.features.characterVisibility")
    local deps = createMockDeps()
    
    local feature = CharacterVisibility.CharacterVisibility.new(deps)
    
    feature:_updateFromServer({
        {characterId = "char-1", characterName = "TestChar", realmId = "test", shareVisibility = true}
    })
    
    assert(feature:isCharacterVisible("char-1") == true, "Initially visible")
    
    feature:_setLocalVisibility("char-1", false)
    
    assert(feature:isCharacterVisible("char-1") == false, "Should be hidden after update")
    
    print("[PASS] testSetLocalVisibility")
end

-- Run all tests
function M.runAll()
    print("========================================")
    print("Character Visibility Feature Tests")
    print("========================================")
    
    local success, err = pcall(function()
        testCharacterVisibilityNew()
        testGetState()
        testUpdateFromServer()
        testIsCharacterVisible()
        testSetLocalVisibility()
    end)
    
    if success then
        print("========================================")
        print("ALL TESTS PASSED")
        print("========================================")
    else
        print("========================================")
        print("TEST FAILED: " .. tostring(err))
        print("========================================")
    end
    
    return success
end

return M
