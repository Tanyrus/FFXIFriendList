-- NetClient.lua

local HttpTypes = require("platform.HttpTypes")
local HttpHeaders = require("protocol.Encoding.HttpHeaders")
local ServerConfig = require("core.ServerConfig")

local M = {}
M.NetClient = {}
M.NetClient.__index = M.NetClient

function M.NetClient.new(ashitaCore)
    local self = setmetatable({}, M.NetClient)
    self.ashitaCore = ashitaCore
    self.baseUrl = ServerConfig.DEFAULT_SERVER_URL
    self.realmId = ""
    self.sessionId = ""
    self.pendingRequests = {}
    self.nextRequestId = 1
    self:loadServerUrlFromConfig()
    return self
end

function M.NetClient:loadServerUrlFromConfig()
end

function M.NetClient:getBaseUrl()
    return self.baseUrl
end

function M.NetClient:setBaseUrl(url)
    self.baseUrl = url
end

function M.NetClient:getRealmId()
    return self.realmId
end

function M.NetClient:setRealmId(realmId)
    self.realmId = realmId
end

function M.NetClient:getSessionId()
    return self.sessionId
end

function M.NetClient:setSessionId(sessionId)
    self.sessionId = sessionId
end

function M.NetClient:isAvailable()
    return self.ashitaCore ~= nil
end

function M.NetClient:buildHeaders(apiKey, characterName)
    local ctx = HttpHeaders.RequestContext(apiKey, characterName, self.realmId, self.sessionId, "application/json")
    local headers = HttpHeaders.buildHeaderList(ctx)
    
    local headerStr = ""
    for i = 1, #headers do
        headerStr = headerStr .. headers[i][1] .. ": " .. headers[i][2] .. "\r\n"
    end
    return headerStr
end

function M.NetClient:get(url, apiKey, characterName)
    local response = HttpTypes.HttpResponse.new(0, "", "Synchronous get not implemented - use async methods")
    return response
end

function M.NetClient:getPublic(url)
    local response = HttpTypes.HttpResponse.new(0, "", "Synchronous getPublic not implemented - use async methods")
    return response
end

function M.NetClient:post(url, apiKey, characterName, body)
    local response = HttpTypes.HttpResponse.new(0, "", "Synchronous post not implemented - use async methods")
    return response
end

function M.NetClient:patch(url, apiKey, characterName, body)
    local response = HttpTypes.HttpResponse.new(0, "", "Synchronous patch not implemented - use async methods")
    return response
end

function M.NetClient:del(url, apiKey, characterName, body)
    body = body or ""
    local response = HttpTypes.HttpResponse.new(0, "", "Synchronous del not implemented - use async methods")
    return response
end

function M.NetClient:getAsync(url, apiKey, characterName, callback)
    if not self.ashitaCore then
        local response = HttpTypes.HttpResponse.new(0, "", "AshitaCore not available")
        callback(response)
        return
    end
    
    local requestId = self.nextRequestId
    self.nextRequestId = self.nextRequestId + 1
    
    local headers = self:buildHeaders(apiKey, characterName)
    
    self.pendingRequests[requestId] = {
        callback = callback,
        method = "GET",
        url = url,
        headers = headers,
        body = nil
    }
    
    if ashita and ashita.nonBlockingRequests then
        ashita.nonBlockingRequests.request({
            method = "GET",
            url = url,
            headers = headers,
            requestId = requestId
        })
    else
        local response = HttpTypes.HttpResponse.new(0, "", "nonBlockingRequests not available")
        callback(response)
        self.pendingRequests[requestId] = nil
    end
end

function M.NetClient:postAsync(url, apiKey, characterName, body, callback)
    if not self.ashitaCore then
        local response = HttpTypes.HttpResponse.new(0, "", "AshitaCore not available")
        callback(response)
        return
    end
    
    local requestId = self.nextRequestId
    self.nextRequestId = self.nextRequestId + 1
    
    local headers = self:buildHeaders(apiKey, characterName)
    
    self.pendingRequests[requestId] = {
        callback = callback,
        method = "POST",
        url = url,
        headers = headers,
        body = body
    }
    
    if ashita and ashita.nonBlockingRequests then
        ashita.nonBlockingRequests.request({
            method = "POST",
            url = url,
            headers = headers,
            body = body,
            requestId = requestId
        })
    else
        local response = HttpTypes.HttpResponse.new(0, "", "nonBlockingRequests not available")
        callback(response)
        self.pendingRequests[requestId] = nil
    end
end

function M.NetClient:update()
    if not ashita or not ashita.nonBlockingRequests then
        return
    end
    
    ashita.nonBlockingRequests.advance()
    
    local completed = ashita.nonBlockingRequests.getCompleted()
    if completed then
        for i = 1, #completed do
            local result = completed[i]
            local requestId = result.requestId
            local pending = self.pendingRequests[requestId]
            
            if pending then
                local response = HttpTypes.HttpResponse.new(
                    result.statusCode or 0,
                    result.body or "",
                    result.error or ""
                )
                
                if pending.callback then
                    pending.callback(response)
                end
                
                self.pendingRequests[requestId] = nil
            end
        end
    end
end

return M

