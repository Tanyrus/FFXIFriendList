-- RealmDetector.lua

local M = {}
M.RealmDetector = {}
M.RealmDetector.__index = M.RealmDetector

function M.RealmDetector.new(ashitaCore)
    local self = setmetatable({}, M.RealmDetector)
    self.ashitaCore = ashitaCore
    self.cachedRealmId = ""
    return self
end

function M.RealmDetector:detectRealm()
    if not self.ashitaCore then
        return ""
    end
    
    if self.cachedRealmId ~= "" then
        return self.cachedRealmId
    end
    
    return "horizon"
end

function M.RealmDetector:getRealmId()
    if self.cachedRealmId == "" then
        self.cachedRealmId = self:detectRealm()
    end
    return self.cachedRealmId
end

function M.RealmDetector:setRealmId(realmId)
    self.cachedRealmId = realmId
end

return M

