# Ashita v4 ImGui Font Capability Findings

## Date: January 2026

## Summary

**GOOD NEWS: Crisp font size switching IS possible in Ashita v4!**

Ashita preloads 4 fonts at different pixel sizes into the ImGui font atlas. These fonts can be accessed and used with `PushFont`/`PopFont` for crisp, non-scaled font rendering.

## Available Preloaded Fonts

| Vector Index | FontSize | Access Method |
|--------------|----------|---------------|
| 1 | 14px | `imgui.GetIO().Fonts.Fonts[1]` |
| 2 | 18px | `imgui.GetIO().Fonts.Fonts[2]` |
| 3 | 24px | `imgui.GetIO().Fonts.Fonts[3]` |
| 4 | 32px | `imgui.GetIO().Fonts.Fonts[4]` |

## API Availability

### Available Functions
- `imgui.PushFont(font)` - Push a font onto the stack
- `imgui.PopFont()` - Pop the font stack
- `imgui.GetFont()` - Get current font (returns userdata with FontSize, Scale)
- `imgui.GetFontSize()` - Get current font size
- `imgui.SetWindowFontScale(scale)` - Scale current font (still works as fallback)
- `imgui.GetIO()` - Access IO structure
- `imgui.GetIO().Fonts` - Access font atlas (userdata)
- `imgui.GetIO().Fonts.Fonts` - Access font vector (userdata, 1-indexed)
- `imgui.GetIO().Fonts.Fonts.size()` - Get font count

### NOT Available (nil)
- `io.Fonts.AddFontFromFileTTF` - Cannot load custom fonts from Lua
- `io.Fonts.Clear` - Cannot clear font atlas
- `io.Fonts.Build` - Cannot rebuild font atlas
- `io.FontDefault` - Default font reference not exposed

## Implementation Approach

### Recommended: Use Preloaded Fonts

```lua
-- Cache font references once on load
local fontCache = {}
local function initFonts()
    local fonts = imgui.GetIO().Fonts.Fonts
    fontCache[14] = fonts[1]
    fontCache[18] = fonts[2]
    fontCache[24] = fonts[3]
    fontCache[32] = fonts[4]
end

-- In window rendering
local selectedSize = 18 -- User preference
local font = fontCache[selectedSize]
if font then
    imgui.PushFont(font)
end

-- ... render window content ...

if font then
    imgui.PopFont()
end
```

### User Setting

Replace `font_scale` (float 0.5-2.0) with `font_size_px` (enum: 14, 18, 24, 32):

```lua
-- In ThemesTab or settings
local fontSizeOptions = {14, 18, 24, 32}
local currentIndex = -- find index of current size

if imgui.Combo("Font Size", currentIndex, {"14px", "18px", "24px", "32px"}) then
    -- Save new font size preference
end
```

## Cleanup Required

1. Remove `SetWindowFontScale()` calls from all display modules
2. Remove `fontScale` from themes.lua and ThemePersistence.lua
3. Remove `themeFontScale` from display state
4. Add `font_size_px` setting (default: 14)
5. Implement font caching and PushFont/PopFont pattern

## Files to Modify

- `app/features/themes.lua` - Replace fontScale with fontSizePx
- `platform/ashita/persistence/ThemePersistence.lua` - Save/load fontSizePx
- `modules/friendlist/display.lua` - Use PushFont/PopFont
- `modules/friendlist/components/ThemesTab.lua` - Font size selector
- `modules/quickonline/display.lua` - Use PushFont/PopFont
- `modules/notifications/display.lua` - Use PushFont/PopFont
- `modules/noteeditor/display.lua` - Use PushFont/PopFont
- `modules/serverselection/display.lua` - Use PushFont/PopFont
- `modules/friendlist/components/FriendDetailsPopup.lua` - Use PushFont/PopFont

## Probe Location

The diagnostic probe is located at `dev/font_capability_probe.lua` and can be re-run by setting `_G._FFXIFL_DEV_PROBE_FONTS = true` in `FFXIFriendList.lua`.

