-- ServerConfig.lua
-- Centralized server URL configuration
-- This is the SINGLE SOURCE OF TRUTH for the default server URL

local M = {}

-- Environment: "production" or "staging"
-- Change this value to switch the plugin between environments.
M.ENVIRONMENT = "staging"

-- Server URLs per environment
M.URLS = {
    production = "https://app.ffxifriendlist.com",
    staging    = "https://app-staging.ffxifriendlist.com",
}

-- Default server URL - derived from the active environment
M.DEFAULT_SERVER_URL = M.URLS[M.ENVIRONMENT] or M.URLS.production

-- WebSocket path (appended to server URL)
M.WS_PATH = "/ws"

-- Get the base URL for the active environment
function M.getBaseUrl()
    return M.URLS[M.ENVIRONMENT] or M.URLS.production
end

-- Build full WebSocket URL from base URL
function M.getWsUrl(baseUrl)
    baseUrl = baseUrl or M.getBaseUrl()
    -- Convert https:// to wss://
    local wsUrl = baseUrl:gsub("^https://", "wss://"):gsub("^http://", "ws://")
    return wsUrl .. M.WS_PATH
end

return M
