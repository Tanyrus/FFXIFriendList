local ServerListCore = require("core.serverlistcore")
local Endpoints = require("protocol.Endpoints")

local M = {}

M.ServerList = {}
M.ServerList.__index = M.ServerList

local DEFAULT_SERVER_URL = "https://api.horizonfriendlist.com"

function M.ServerList.new(deps)
    local self = setmetatable({}, M.ServerList)
    self.deps = deps or {}
    
    self.servers = {}
    self.selected = nil
    self.state = "idle"
    self.lastError = nil
    self.lastUpdatedAt = nil
    
    if _G.gConfig and _G.gConfig.data and _G.gConfig.data.serverSelection then
        local serverSel = _G.gConfig.data.serverSelection
        if serverSel.savedServerId and serverSel.savedServerId ~= "" then
            self.selected = ServerListCore.ServerInfo.new(
                serverSel.savedServerId,
                serverSel.savedServerId,
                serverSel.savedServerBaseUrl or DEFAULT_SERVER_URL,
                serverSel.savedServerId,
                false
            )
        end
    end
    
    return self
end

function M.ServerList:getState()
    return {
        servers = self.servers,
        selected = self.selected,
        state = self.state,
        lastUpdatedAt = self.lastUpdatedAt,
        lastError = self.lastError
    }
end

function M.ServerList:tick(dtSeconds)
end

local function getTime(self)
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

function M.ServerList:refresh()
    if not self.deps.net then
        return false
    end
    
    self.state = "loading"
    self.lastError = nil
    
    local url = DEFAULT_SERVER_URL .. Endpoints.SERVERS
    
    local requestId = self.deps.net.request({
        url = url,
        method = "GET",
        headers = {},
        body = nil,
        callback = function(success, response)
            self:handleRefreshResponse(success, response)
        end
    })
    
    return requestId ~= nil
end

function M.ServerList:handleRefreshResponse(success, response)
    if not success then
        self.state = "error"
        local errorMsg = response
        if type(errorMsg) == "table" then
            errorMsg = "Network error"
        else
            errorMsg = tostring(errorMsg or "Network error")
        end
        self.lastError = { type = "NetworkError", message = errorMsg }
        return
    end
    
    local Json = require("protocol.Json")
    local ok, data = Json.decode(response)
    if not ok then
        self.state = "error"
        local errorMsg = data
        if type(errorMsg) == "table" then
            errorMsg = "Failed to decode JSON"
        else
            errorMsg = tostring(errorMsg or "Failed to decode JSON")
        end
        self.lastError = { type = "DecodeError", message = errorMsg }
        return
    end
    
    local result = { servers = {} }
    if data and data.servers and type(data.servers) == "table" then
        result.servers = data.servers
    end
    
    self.servers = {}
    if result.servers then
        for _, serverData in ipairs(result.servers) do
            local server = ServerListCore.ServerInfo.new(
                serverData.id,
                serverData.name,
                serverData.baseUrl,
                serverData.realmId,
                true
            )
            table.insert(self.servers, server)
        end
    end
    
    if self.selected and self.selected.id then
        local savedServerId = self.selected.id
        local matchedServer = nil
        for _, server in ipairs(self.servers) do
            if server.id == savedServerId then
                matchedServer = server
                break
            end
        end
        
        if matchedServer then
            self.selected = matchedServer
            if _G.gConfig and _G.gConfig.data and _G.gConfig.data.serverSelection then
                _G.gConfig.data.serverSelection.savedServerId = matchedServer.id
                _G.gConfig.data.serverSelection.savedServerBaseUrl = matchedServer.baseUrl
            end
        end
    end
    
    self.state = "idle"
    self.lastError = nil
    self.lastUpdatedAt = getTime(self)
end

function M.ServerList:selectServer(serverId)
    for _, server in ipairs(self.servers) do
        if server.id == serverId then
            self.selected = server
            return true
        end
    end
    return false
end

function M.ServerList:getServers()
    return self.servers
end

function M.ServerList:getSelected()
    return self.selected
end

function M.ServerList:isServerSelected()
    return self.selected ~= nil
end

return M
