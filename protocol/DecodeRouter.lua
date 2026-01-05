local MessageTypes = require("protocol.MessageTypes")
local ContractChecks = require("protocol.ContractChecks")

local M = {}

local decoderMap = {}

local function getDecoder(responseType)
    if decoderMap[responseType] then
        return decoderMap[responseType]
    end
    
    local decoder = nil
    if responseType == MessageTypes.ResponseType.FriendsListResponse then
        decoder = require("protocol.Decoders.FriendsList")
    elseif responseType == MessageTypes.ResponseType.FriendRequestsResponse then
        decoder = require("protocol.Decoders.FriendRequests")
    elseif responseType == MessageTypes.ResponseType.HeartbeatResponse then
        decoder = require("protocol.Decoders.Heartbeat")
    elseif responseType == MessageTypes.ResponseType.PreferencesResponse or
           responseType == MessageTypes.ResponseType.PreferencesUpdateResponse then
        decoder = require("protocol.Decoders.Preferences")
    elseif responseType == MessageTypes.ResponseType.NotesListResponse then
        decoder = require("protocol.Decoders.NotesList")
    elseif responseType == MessageTypes.ResponseType.NoteResponse or
           responseType == MessageTypes.ResponseType.NoteUpdateResponse then
        decoder = require("protocol.Decoders.Note")
    elseif responseType == MessageTypes.ResponseType.CharactersListResponse then
        decoder = require("protocol.Decoders.CharactersList")
    elseif responseType == MessageTypes.ResponseType.AuthEnsureResponse then
        decoder = require("protocol.Decoders.AuthEnsure")
    elseif responseType == MessageTypes.ResponseType.SetActiveCharacterResponse then
        -- Reuse AuthEnsure decoder - response structure is compatible
        decoder = require("protocol.Decoders.AuthEnsure")
    elseif responseType == MessageTypes.ResponseType.MeResponse then
        decoder = require("protocol.Decoders.MeResponse")
    elseif responseType == MessageTypes.ResponseType.ServerList then
        decoder = require("protocol.Decoders.ServerList")
    elseif responseType == MessageTypes.ResponseType.BlockedAccountsResponse then
        decoder = require("protocol.Decoders.BlockedAccounts")
    end
    
    if decoder then
        decoderMap[responseType] = decoder
    end
    
    return decoder
end

function M.registerDecoder(responseType, decoderModule)
    decoderMap[responseType] = decoderModule
end

function M.decode(envelope)
    if not envelope or not envelope.type then
        return false, "Invalid envelope: missing type"
    end
    
    local decoder = getDecoder(envelope.type)
    if not decoder then
        return false, "Unknown response type: " .. tostring(envelope.type)
    end
    
    local ok, result = decoder.decode(envelope.payload)
    if not ok then
        local errorMsg = result
        if type(errorMsg) == "table" then
            errorMsg = "Failed to decode payload"
        else
            errorMsg = tostring(errorMsg or "Failed to decode payload")
        end
        return false, errorMsg
    end
    
    ContractChecks.validateResponse(envelope.type, result)
    
    return true, result
end

function M.setDebugMode(enabled)
    ContractChecks.setDebugMode(enabled)
end

return M
