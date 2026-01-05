local tagcore = require("core.tagcore")

local M = {}

M.Tags = {}
M.Tags.__index = M.Tags

function M.Tags.new(deps)
    local self = setmetatable({}, M.Tags)
    self.deps = deps or {}
    
    self.friendTags = {}
    self.tagOrder = { "Favorite" }
    self.collapsedTags = {}
    self.pendingTags = {}
    self.pendingRetags = {}
    self.dirty = false
    
    return self
end

function M.Tags:getState()
    return {
        tagOrder = self.tagOrder,
        collapsedTags = self.collapsedTags,
        tagCount = #self.tagOrder
    }
end

function M.Tags:load()
    if gConfig then
        if gConfig.friendTags and type(gConfig.friendTags) == "table" then
            self.friendTags = gConfig.friendTags
        end
        if gConfig.tagOrder and type(gConfig.tagOrder) == "table" and #gConfig.tagOrder > 0 then
            self.tagOrder = gConfig.tagOrder
        end
        if gConfig.collapsedTags and type(gConfig.collapsedTags) == "table" then
            self.collapsedTags = gConfig.collapsedTags
        end
    end
    self.dirty = false
    return true
end

function M.Tags:save()
    local settings = require("libs.settings")
    if gConfig then
        gConfig.friendTags = self.friendTags
        gConfig.tagOrder = self.tagOrder
        gConfig.collapsedTags = self.collapsedTags
    end
    settings.save()
    self.dirty = false
    return true
end

function M.Tags:getTagForFriend(friendKey)
    if not friendKey then
        return nil
    end
    local key = string.lower(tostring(friendKey))
    return self.friendTags[key]
end

function M.Tags:setTagForFriend(friendKey, tag)
    if not friendKey then
        return false
    end
    
    local key = string.lower(tostring(friendKey))
    local normalized = tagcore.normalizeTag(tag)
    
    if normalized then
        if not tagcore.tagExistsInOrder(self.tagOrder, normalized) then
            self:createTag(normalized)
        end
        self.friendTags[key] = normalized
    else
        self.friendTags[key] = nil
    end
    
    self.dirty = true
    return true
end

function M.Tags:createTag(tag)
    local normalized = tagcore.normalizeTag(tag)
    if not normalized then
        return false
    end
    
    if tagcore.tagExistsInOrder(self.tagOrder, normalized) then
        return false
    end
    
    table.insert(self.tagOrder, normalized)
    self.dirty = true
    return true
end

function M.Tags:deleteTag(tag)
    local normalized = tagcore.normalizeTag(tag)
    if not normalized then
        return false
    end
    
    local index = tagcore.findTagIndex(self.tagOrder, normalized)
    if not index then
        return false
    end
    
    table.remove(self.tagOrder, index)
    
    for friendKey, friendTag in pairs(self.friendTags) do
        if tagcore.tagsEqual(friendTag, normalized) then
            self.friendTags[friendKey] = nil
        end
    end
    
    local lowerTag = string.lower(normalized)
    self.collapsedTags[lowerTag] = nil
    
    self.dirty = true
    return true
end

function M.Tags:renameTag(oldTag, newTag)
    local oldNorm = tagcore.normalizeTag(oldTag)
    local newNorm = tagcore.normalizeTag(newTag)
    
    if not oldNorm or not newNorm then
        return false
    end
    
    if tagcore.tagsEqual(oldNorm, newNorm) then
        local index = tagcore.findTagIndex(self.tagOrder, oldNorm)
        if index then
            self.tagOrder[index] = newNorm
            self.dirty = true
        end
        return true
    end
    
    if tagcore.tagExistsInOrder(self.tagOrder, newNorm) then
        return false
    end
    
    local index = tagcore.findTagIndex(self.tagOrder, oldNorm)
    if not index then
        return false
    end
    
    self.tagOrder[index] = newNorm
    
    for friendKey, friendTag in pairs(self.friendTags) do
        if tagcore.tagsEqual(friendTag, oldNorm) then
            self.friendTags[friendKey] = newNorm
        end
    end
    
    local oldLower = string.lower(oldNorm)
    local newLower = string.lower(newNorm)
    if self.collapsedTags[oldLower] ~= nil then
        self.collapsedTags[newLower] = self.collapsedTags[oldLower]
        self.collapsedTags[oldLower] = nil
    end
    
    self.dirty = true
    return true
end

function M.Tags:reorderTags(newOrder)
    if not newOrder or type(newOrder) ~= "table" then
        return false
    end
    
    local validOrder = {}
    for _, tag in ipairs(newOrder) do
        local normalized = tagcore.normalizeTag(tag)
        if normalized and tagcore.tagExistsInOrder(self.tagOrder, normalized) then
            local alreadyAdded = false
            for _, existing in ipairs(validOrder) do
                if tagcore.tagsEqual(existing, normalized) then
                    alreadyAdded = true
                    break
                end
            end
            if not alreadyAdded then
                table.insert(validOrder, normalized)
            end
        end
    end
    
    for _, tag in ipairs(self.tagOrder) do
        local inNew = false
        for _, newTag in ipairs(validOrder) do
            if tagcore.tagsEqual(tag, newTag) then
                inNew = true
                break
            end
        end
        if not inNew then
            table.insert(validOrder, tag)
        end
    end
    
    self.tagOrder = validOrder
    self.dirty = true
    return true
end

function M.Tags:moveTagUp(tag)
    local index = tagcore.findTagIndex(self.tagOrder, tag)
    if not index or index <= 1 then
        return false
    end
    
    self.tagOrder[index], self.tagOrder[index - 1] = self.tagOrder[index - 1], self.tagOrder[index]
    self.dirty = true
    return true
end

function M.Tags:moveTagDown(tag)
    local index = tagcore.findTagIndex(self.tagOrder, tag)
    if not index or index >= #self.tagOrder then
        return false
    end
    
    self.tagOrder[index], self.tagOrder[index + 1] = self.tagOrder[index + 1], self.tagOrder[index]
    self.dirty = true
    return true
end

function M.Tags:getAllTags()
    return self.tagOrder
end

function M.Tags:isCollapsed(tag)
    local normalized = tagcore.normalizeTag(tag)
    if not normalized then
        normalized = "__untagged__"
    end
    local key = string.lower(normalized)
    return self.collapsedTags[key] == true
end

function M.Tags:setCollapsed(tag, collapsed)
    local normalized = tagcore.normalizeTag(tag)
    if not normalized then
        normalized = "__untagged__"
    end
    local key = string.lower(normalized)
    self.collapsedTags[key] = collapsed and true or nil
    self.dirty = true
end

function M.Tags:setPendingTag(friendName, tag)
    if not friendName or friendName == "" then
        return false
    end
    
    local key = string.lower(friendName)
    local normalized = tagcore.normalizeTag(tag)
    
    if normalized then
        if not tagcore.tagExistsInOrder(self.tagOrder, normalized) then
            self:createTag(normalized)
        end
        self.pendingTags[key] = normalized
    else
        self.pendingTags[key] = nil
    end
    
    return true
end

function M.Tags:consumePendingTag(friendName)
    if not friendName or friendName == "" then
        return nil
    end
    
    local key = string.lower(friendName)
    local tag = self.pendingTags[key]
    self.pendingTags[key] = nil
    return tag
end

function M.Tags:queueRetag(friendKey, newTag)
    if not friendKey then
        return false
    end
    
    table.insert(self.pendingRetags, {
        friendKey = friendKey,
        newTag = newTag
    })
    return true
end

function M.Tags:flushRetagQueue()
    if #self.pendingRetags == 0 then
        return false
    end
    
    for _, retag in ipairs(self.pendingRetags) do
        self:setTagForFriend(retag.friendKey, retag.newTag)
    end
    
    self.pendingRetags = {}
    self:save()
    return true
end

function M.Tags:hasPendingRetags()
    return #self.pendingRetags > 0
end

return M

