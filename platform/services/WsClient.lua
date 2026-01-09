-- WsClient.lua
-- WebSocket client for server-to-client push events
-- Platform layer: owns the WSS connection
-- Implementation: LuaSocket (TCP) + LuaSec (TLS) + pure-Lua RFC6455 framing

local Bit = require("common.Bit")
local WsEventRouter = require("protocol.WsEventRouter")

local M = {}

M.WsClient = {}
M.WsClient.__index = M.WsClient

-- Connection states
M.State = {
    Disconnected = "Disconnected",
    Connecting = "Connecting",
    Connected = "Connected",
    Reconnecting = "Reconnecting",
    Failed = "Failed"
}

-- WebSocket opcodes (RFC 6455 Section 5.2)
local OPCODE = {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
}

-- WebSocket GUID for Sec-WebSocket-Accept validation
local WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

-- Create a new WebSocket client
-- deps: { logger, connection, time? }
function M.WsClient.new(deps)
    local self = setmetatable({}, M.WsClient)
    self.deps = deps or {}
    self.logger = deps.logger
    self.connection = deps.connection
    
    self.state = M.State.Disconnected
    self.url = nil
    self.socket = nil
    self.host = nil
    self.port = nil
    self.path = nil
    
    -- Buffer for partial frame reads
    self.readBuffer = ""
    
    -- Reconnection settings
    self.reconnectAttempts = 0
    self.maxReconnectAttempts = 10
    self.baseReconnectDelayMs = 1000
    self.maxReconnectDelayMs = 60000
    self.nextReconnectAt = nil
    
    -- Event callbacks
    self.onOpen = nil
    self.onMessage = nil
    self.onClose = nil
    self.onError = nil
    
    -- Event router for dispatching decoded events
    self.eventRouter = WsEventRouter.WsEventRouter.new({ logger = self.logger })
    
    -- Track if we received friends_snapshot after reconnect
    self.receivedSnapshot = false
    
    -- Capability check
    self.socketLib = nil
    self.sslLib = nil
    self.wsAvailable = self:_checkWsCapability()
    
    return self
end

-- Check if WebSocket capability is available
function M.WsClient:_checkWsCapability()
    -- Try to require luasocket
    local hasSocket, socket = pcall(require, "socket")
    if not hasSocket then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] LuaSocket not available - WebSocket disabled")
        end
        return false
    end
    self.socketLib = socket
    
    -- Try to require LuaSec (socket.ssl)
    local hasSsl, ssl = pcall(require, "socket.ssl")
    if not hasSsl then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] LuaSec not available - WSS disabled")
        end
        return false
    end
    self.sslLib = ssl
    
    if self.logger and self.logger.info then
        self.logger.info("[WsClient] WebSocket capability available (LuaSocket + LuaSec)")
    end
    return true
end

-- Parse a WebSocket URL: wss://host:port/path
function M.WsClient:_parseUrl(wsUrl)
    local scheme, host, port, path = wsUrl:match("^(wss?)://([^:/]+):?(%d*)(.*)$")
    if not scheme then
        return nil, "Invalid WebSocket URL"
    end
    
    port = tonumber(port) or (scheme == "wss" and 443 or 80)
    path = path ~= "" and path or "/"
    
    return {
        scheme = scheme,
        host = host,
        port = port,
        path = path,
        isSecure = scheme == "wss"
    }
end

-- Generate a random 16-byte WebSocket key
function M.WsClient:_generateKey()
    local bytes = {}
    for _ = 1, 16 do
        bytes[#bytes + 1] = string.char(math.random(0, 255))
    end
    
    -- Base64 encode
    local mime = require("mime")
    return mime.b64(table.concat(bytes))
end

-- Connect to the WebSocket endpoint
function M.WsClient:connect()
    if not self.connection then
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] No connection service available")
        end
        return false
    end
    
    if not self.connection:isConnected() then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] HTTP connection not established, cannot connect WS")
        end
        return false
    end
    
    if self.state == M.State.Connected or self.state == M.State.Connecting then
        return false
    end
    
    self.state = M.State.Connecting
    self.receivedSnapshot = false
    
    -- Build WebSocket URL
    local baseUrl = self.connection:getBaseUrl()
    local wsUrl = baseUrl:gsub("^https://", "wss://"):gsub("^http://", "ws://")
    local Endpoints = require("protocol.Endpoints")
    self.url = wsUrl .. Endpoints.WEBSOCKET
    
    if self.logger and self.logger.info then
        self.logger.info("[WsClient] Connecting to: " .. self.url)
    end
    
    if not self.wsAvailable then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] WebSocket libraries not available - running in HTTP-only mode")
        end
        self.state = M.State.Disconnected
        return false
    end
    
    return self:_connectSocket()
end

-- Internal socket connection
function M.WsClient:_connectSocket()
    -- Parse URL
    local parsed, err = self:_parseUrl(self.url)
    if not parsed then
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] URL parse failed: " .. tostring(err))
        end
        self.state = M.State.Failed
        self:_scheduleReconnect()
        return false
    end
    
    self.host = parsed.host
    self.port = parsed.port
    self.path = parsed.path
    
    -- Create TCP socket
    local sock, sockErr = self.socketLib.tcp()
    if not sock then
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] TCP socket creation failed: " .. tostring(sockErr))
        end
        self.state = M.State.Failed
        self:_scheduleReconnect()
        return false
    end
    
    -- Set timeout for connect phase (blocking)
    sock:settimeout(10)
    
    -- Connect to server
    local connectOk, connectErr = sock:connect(self.host, self.port)
    if not connectOk then
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] TCP connect failed: " .. tostring(connectErr))
        end
        pcall(function() sock:close() end)
        self.state = M.State.Failed
        self:_scheduleReconnect()
        return false
    end
    
    -- Wrap with TLS if secure
    if parsed.isSecure then
        local sslParams = {
            mode = "client",
            protocol = "any",
            options = {"all", "no_sslv2", "no_sslv3"},
            verify = "none"
        }
        
        local sslSock, sslErr = self.sslLib.wrap(sock, sslParams)
        if not sslSock then
            if self.logger and self.logger.error then
                self.logger.error("[WsClient] SSL wrap failed: " .. tostring(sslErr))
            end
            pcall(function() sock:close() end)
            self.state = M.State.Failed
            self:_scheduleReconnect()
            return false
        end
        
        -- Set SNI
        pcall(function() sslSock:sni(self.host) end)
        
        -- Handshake
        local hsOk, hsErr = sslSock:dohandshake()
        if not hsOk then
            if self.logger and self.logger.error then
                self.logger.error("[WsClient] SSL handshake failed: " .. tostring(hsErr))
            end
            pcall(function() sslSock:close() end)
            self.state = M.State.Failed
            self:_scheduleReconnect()
            return false
        end
        
        sock = sslSock
    end
    
    -- Perform WebSocket upgrade
    local upgradeOk, upgradeErr = self:_performUpgrade(sock)
    if not upgradeOk then
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] WebSocket upgrade failed: " .. tostring(upgradeErr))
        end
        pcall(function() sock:close() end)
        self.state = M.State.Failed
        self:_scheduleReconnect()
        return false
    end
    
    -- Switch to non-blocking for read loop
    sock:settimeout(0)
    
    self.socket = sock
    self.state = M.State.Connected
    self.reconnectAttempts = 0
    self.readBuffer = ""
    
    if self.logger and self.logger.info then
        self.logger.info("[WsClient] Connected to WebSocket")
    end
    
    if self.onOpen then
        self.onOpen()
    end
    
    return true
end

-- Perform HTTP upgrade to WebSocket
function M.WsClient:_performUpgrade(sock)
    -- Get auth headers for WebSocket upgrade
    local characterName = ""
    if self.connection and self.connection.getCharacterName then
        characterName = self.connection:getCharacterName()
    end
    local headers = self.connection:getHeaders(characterName)
    
    -- Generate WebSocket key
    local wsKey = self:_generateKey()
    
    -- Build upgrade request
    local hostHeader = self.host
    if self.port ~= 80 and self.port ~= 443 then
        hostHeader = hostHeader .. ":" .. self.port
    end
    
    local request = string.format(
        "GET %s HTTP/1.1\r\n" ..
        "Host: %s\r\n" ..
        "Upgrade: websocket\r\n" ..
        "Connection: Upgrade\r\n" ..
        "Sec-WebSocket-Key: %s\r\n" ..
        "Sec-WebSocket-Version: 13\r\n",
        self.path, hostHeader, wsKey
    )
    
    -- Add auth headers (important for server auth)
    if headers["Authorization"] then
        request = request .. "Authorization: " .. headers["Authorization"] .. "\r\n"
    end
    if headers["X-Character-Name"] then
        request = request .. "X-Character-Name: " .. headers["X-Character-Name"] .. "\r\n"
    end
    if headers["X-Realm-Id"] then
        request = request .. "X-Realm-Id: " .. headers["X-Realm-Id"] .. "\r\n"
    end
    
    request = request .. "\r\n"
    
    -- Send upgrade request
    local sent, sendErr = sock:send(request)
    if not sent then
        return false, "Failed to send upgrade request: " .. tostring(sendErr)
    end
    
    -- Read response (blocking, with timeout)
    sock:settimeout(10)
    
    local statusLine, err = sock:receive("*l")
    if not statusLine then
        return false, "Failed to receive response: " .. tostring(err)
    end
    
    -- Check for HTTP 101 Switching Protocols
    if not statusLine:match("^HTTP/1.1 101") then
        return false, "Upgrade failed: " .. statusLine
    end
    
    -- Read headers until empty line
    local sawUpgrade = false
    local sawConnection = false
    local acceptKey = nil
    
    while true do
        local line = sock:receive("*l")
        if not line then
            return false, "Connection closed during upgrade"
        end
        if line == "" then
            break
        end
        
        local name, value = line:match("^([^:]+):%s*(.*)$")
        if name then
            local lowerName = name:lower()
            if lowerName == "upgrade" and value:lower() == "websocket" then
                sawUpgrade = true
            elseif lowerName == "connection" and value:lower():find("upgrade") then
                sawConnection = true
            elseif lowerName == "sec-websocket-accept" then
                acceptKey = value
            end
        end
    end
    
    if not sawUpgrade or not sawConnection then
        return false, "Missing required upgrade headers"
    end
    
    -- Sec-WebSocket-Accept validation is optional in MVP (no SHA1 available)
    -- We log the accept key for debugging but don't validate
    if self.logger and self.logger.debug and acceptKey then
        self.logger.debug("[WsClient] Sec-WebSocket-Accept: " .. acceptKey)
    end
    
    return true
end

-- Disconnect from WebSocket
function M.WsClient:disconnect()
    if self.socket then
        -- Send close frame
        pcall(function()
            self:_sendFrame(OPCODE.CLOSE, "")
        end)
        pcall(function() self.socket:close() end)
        self.socket = nil
    end
    
    self.state = M.State.Disconnected
    self.nextReconnectAt = nil
    self.readBuffer = ""
    
    if self.logger and self.logger.info then
        self.logger.info("[WsClient] Disconnected")
    end
    
    if self.onClose then
        self.onClose("User disconnect")
    end
end

-- Tick - called each frame to process messages and handle reconnection
function M.WsClient:tick(dtSeconds)
    local now = self:_getTime()
    
    -- Handle reconnection
    if self.nextReconnectAt and now >= self.nextReconnectAt then
        self.nextReconnectAt = nil
        if self.logger and self.logger.info then
            self.logger.info("[WsClient] Attempting reconnect...")
        end
        self:connect()
    end
    
    -- Process incoming messages if connected
    if self.state == M.State.Connected and self.socket then
        self:_processMessages()
    end
end

-- Process incoming WebSocket messages (non-blocking)
function M.WsClient:_processMessages()
    if not self.socket then return end
    
    -- Try to read available data
    local data, err, partial = self.socket:receive(4096)
    local chunk = data or partial
    
    if chunk and #chunk > 0 then
        self.readBuffer = self.readBuffer .. chunk
    end
    
    -- Handle errors
    if err and err ~= "timeout" and err ~= "wantread" then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] Receive error: " .. tostring(err))
        end
        self:_handleDisconnect()
        return
    end
    
    -- Process complete frames in buffer
    while #self.readBuffer >= 2 do
        local frame, consumed, frameErr = self:_parseFrame()
        if frameErr then
            if self.logger and self.logger.warn then
                self.logger.warn("[WsClient] Frame parse error: " .. tostring(frameErr))
            end
            self:_handleDisconnect()
            return
        end
        
        if not frame then
            -- Need more data
            break
        end
        
        -- Remove consumed bytes from buffer
        self.readBuffer = self.readBuffer:sub(consumed + 1)
        
        -- Handle frame by opcode
        self:_handleFrame(frame)
    end
end

-- Parse a WebSocket frame from the buffer
-- Returns: frame, bytesConsumed, error
function M.WsClient:_parseFrame()
    local buf = self.readBuffer
    if #buf < 2 then
        return nil, 0
    end
    
    local b1 = buf:byte(1)
    local b2 = buf:byte(2)
    
    local fin = Bit.band(b1, 0x80) ~= 0
    local opcode = Bit.band(b1, 0x0F)
    local masked = Bit.band(b2, 0x80) ~= 0
    local payloadLen = Bit.band(b2, 0x7F)
    
    local headerLen = 2
    
    -- Extended payload length
    if payloadLen == 126 then
        if #buf < 4 then return nil, 0 end
        payloadLen = buf:byte(3) * 256 + buf:byte(4)
        headerLen = 4
    elseif payloadLen == 127 then
        if #buf < 10 then return nil, 0 end
        -- 8-byte length (we only use lower 32 bits for practical purposes)
        payloadLen = 0
        for i = 3, 10 do
            payloadLen = payloadLen * 256 + buf:byte(i)
        end
        headerLen = 10
    end
    
    -- Mask (server frames should NOT be masked, but handle defensively)
    local maskLen = 0
    local mask = nil
    if masked then
        maskLen = 4
        if #buf < headerLen + 4 then return nil, 0 end
        mask = buf:sub(headerLen + 1, headerLen + 4)
    end
    
    local totalLen = headerLen + maskLen + payloadLen
    if #buf < totalLen then
        return nil, 0  -- Need more data
    end
    
    -- Extract payload
    local payload = buf:sub(headerLen + maskLen + 1, headerLen + maskLen + payloadLen)
    
    -- Unmask if necessary
    if masked and mask then
        local unmasked = {}
        for i = 1, #payload do
            local j = ((i - 1) % 4) + 1
            unmasked[i] = string.char(Bit.bxor(payload:byte(i), mask:byte(j)))
        end
        payload = table.concat(unmasked)
    end
    
    return {
        fin = fin,
        opcode = opcode,
        payload = payload
    }, totalLen
end

-- Handle a parsed WebSocket frame
function M.WsClient:_handleFrame(frame)
    if frame.opcode == OPCODE.TEXT then
        if self.logger and self.logger.debug then
            self.logger.debug("[WsClient] Received TEXT frame: " .. #frame.payload .. " bytes")
        end
        
        -- Route through event router
        if self.eventRouter then
            self.eventRouter:route(frame.payload)
        end
        
        -- Check if this is a friends_snapshot for reconnect handling
        if frame.payload:find('"type":"friends_snapshot"') then
            self.receivedSnapshot = true
        end
        
        if self.onMessage then
            self.onMessage(frame.payload)
        end
        
    elseif frame.opcode == OPCODE.PING then
        if self.logger and self.logger.debug then
            self.logger.debug("[WsClient] Received PING, sending PONG")
        end
        self:_sendFrame(OPCODE.PONG, frame.payload)
        
    elseif frame.opcode == OPCODE.PONG then
        if self.logger and self.logger.debug then
            self.logger.debug("[WsClient] Received PONG")
        end
        
    elseif frame.opcode == OPCODE.CLOSE then
        local code = 0
        local reason = ""
        if #frame.payload >= 2 then
            code = frame.payload:byte(1) * 256 + frame.payload:byte(2)
            reason = frame.payload:sub(3)
        end
        if self.logger and self.logger.info then
            self.logger.info(string.format("[WsClient] Received CLOSE: code=%d reason=%s", code, reason))
        end
        self:_handleDisconnect()
    end
end

-- Send a WebSocket frame (client frames MUST be masked per RFC 6455)
function M.WsClient:_sendFrame(opcode, payload)
    if not self.socket then return false end
    
    payload = payload or ""
    local len = #payload
    
    -- Generate random 4-byte mask
    local mask = string.char(
        math.random(0, 255), math.random(0, 255),
        math.random(0, 255), math.random(0, 255)
    )
    
    -- Build header
    local header = string.char(0x80 + opcode)  -- FIN + opcode
    
    if len <= 125 then
        header = header .. string.char(0x80 + len)  -- MASK bit + length
    elseif len <= 65535 then
        header = header .. string.char(0x80 + 126)
        header = header .. string.char(math.floor(len / 256), len % 256)
    else
        -- 8-byte length
        header = header .. string.char(0x80 + 127)
        for i = 7, 0, -1 do
            header = header .. string.char(math.floor(len / (256^i)) % 256)
        end
    end
    
    -- Mask payload
    local masked = {}
    for i = 1, len do
        local j = ((i - 1) % 4) + 1
        masked[i] = string.char(Bit.bxor(payload:byte(i), mask:byte(j)))
    end
    
    local frame = header .. mask .. table.concat(masked)
    
    local sent, err = self.socket:send(frame)
    if not sent then
        if self.logger and self.logger.warn then
            self.logger.warn("[WsClient] Send failed: " .. tostring(err))
        end
        return false
    end
    
    return true
end

-- Handle unexpected disconnect
function M.WsClient:_handleDisconnect()
    if self.socket then
        pcall(function() self.socket:close() end)
        self.socket = nil
    end
    
    self.state = M.State.Reconnecting
    self.readBuffer = ""
    
    if self.logger and self.logger.warn then
        self.logger.warn("[WsClient] Connection lost, entering reconnect mode")
    end
    
    if self.onClose then
        self.onClose("Connection lost")
    end
    
    self:_scheduleReconnect()
end

-- Schedule a reconnection attempt with exponential backoff and jitter
function M.WsClient:_scheduleReconnect()
    if self.reconnectAttempts >= self.maxReconnectAttempts then
        self.state = M.State.Failed
        if self.logger and self.logger.error then
            self.logger.error("[WsClient] Max reconnect attempts (" .. self.maxReconnectAttempts .. ") reached")
        end
        return
    end
    
    self.reconnectAttempts = self.reconnectAttempts + 1
    
    -- Exponential backoff with jitter (+/- 20%)
    local delay = self.baseReconnectDelayMs * (2 ^ (self.reconnectAttempts - 1))
    delay = math.min(delay, self.maxReconnectDelayMs)
    
    -- Add jitter
    local jitter = delay * 0.2 * (math.random() * 2 - 1)  -- +/- 20%
    delay = math.floor(delay + jitter)
    
    self.nextReconnectAt = self:_getTime() + delay
    
    if self.logger and self.logger.info then
        self.logger.info(string.format("[WsClient] Reconnect attempt %d/%d in %dms",
            self.reconnectAttempts, self.maxReconnectAttempts, delay))
    end
end

-- Register a handler with the event router
function M.WsClient:registerHandler(eventType, handler)
    if self.eventRouter then
        -- The router uses setHandler for a single handler
        -- We need to adapt this to support multiple event types
        -- For now, we use the existing setHandler approach
    end
end

-- Get current state
function M.WsClient:getState()
    return self.state
end

-- Check if connected
function M.WsClient:isConnected()
    return self.state == M.State.Connected
end

-- Check if WebSocket is available
function M.WsClient:isAvailable()
    return self.wsAvailable
end

-- Check if we've received a snapshot since last connect
function M.WsClient:hasReceivedSnapshot()
    return self.receivedSnapshot
end

-- Get current time in milliseconds
function M.WsClient:_getTime()
    if self.deps.time then
        return self.deps.time()
    end
    return os.time() * 1000
end

-- Set message handler (for direct message callbacks)
function M.WsClient:setOnMessage(handler)
    self.onMessage = handler
end

-- Set open handler
function M.WsClient:setOnOpen(handler)
    self.onOpen = handler
end

-- Set close handler
function M.WsClient:setOnClose(handler)
    self.onClose = handler
end

-- Set error handler
function M.WsClient:setOnError(handler)
    self.onError = handler
end

return M
