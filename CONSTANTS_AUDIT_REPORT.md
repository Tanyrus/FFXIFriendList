# FFXIFriendList Addon - Constants Audit Report

**Generated:** 2026-01-09  
**Scope:** Lua addon only (FFXIFriendList/)

---

## Executive Summary

This audit identified **45+ scattered constants**, **12 major duplication hotspots**, and **8 values that should be user-configurable**. The current codebase has partial centralization (`core/UIConstants.lua`, `core/TimingConstants.lua`, `core/NetworkConstants.lua`, `core/ServerConfig.lua`) but significant gaps remain.

---

## 1. High-Risk Constants (Likely to Change)

| ID | Risk | Effort | Literal Value | Location(s) | Why Risky | Proposed Home |
|----|------|--------|---------------|-------------|-----------|---------------|
| H1 | High | S | `"https://api2.horizonfriendlist.com"` | [ServerConfig.lua#L9](core/ServerConfig.lua#L9), [settings.lua#L136](libs/settings.lua#L136), [FFXIFriendList.lua#L136](FFXIFriendList.lua#L136) | URL may change between releases (already changed from api to api2) | `core/ServerConfig.lua` (SINGLE source - fix duplicates) |
| H2 | High | M | `350` (window width) | [serverselection/display.lua#L130](modules/serverselection/display.lua#L130), [quickonline/display.lua#L871,879](modules/quickonline/display.lua#L871) | Repeated for server selection popup sizing | `constants/ui.lua` → `WINDOW_SIZES.SERVER_SELECTION` |
| H3 | High | M | `{420, 320}` | [quickonline/display.lua#L233,238](modules/quickonline/display.lua#L233) | Default window size repeated | `constants/ui.lua` → `WINDOW_SIZES.QUICK_ONLINE` |
| H4 | High | M | `{500, 400}` | [noteeditor/display.lua#L161,164](modules/noteeditor/display.lua#L161) | Note editor default size | `constants/ui.lua` → `WINDOW_SIZES.NOTE_EDITOR` |
| H5 | High | S | `"online.wav"`, `"friend-request.wav"` | [FFXIFriendList.lua#L448-449](FFXIFriendList.lua#L448), [notifications/display.lua#L210,212](modules/notifications/display.lua#L210) | Sound filenames duplicated | `constants/assets.lua` → `SOUNDS.FRIEND_ONLINE`, `SOUNDS.FRIEND_REQUEST` |
| H6 | High | M | `"Select Server##serverselection"` | [serverselection/display.lua#L152](modules/serverselection/display.lua#L152), [quickonline/display.lua#L900](modules/quickonline/display.lua#L900), [friendlist/display.lua#L1176](modules/friendlist/display.lua#L1176) | Window title/ID repeated 3x | `constants/ui.lua` → `WINDOW_IDS.SERVER_SELECTION` |
| H7 | Med | M | Toast duration `8000` | [notifications/display.lua#L14](modules/notifications/display.lua#L14), [TimingConstants.lua#L20](core/TimingConstants.lua#L20) | Already in TimingConstants but not used everywhere | Centralize in `TimingConstants` |
| H8 | Med | S | WS Event types (string literals) | [WsEnvelope.lua#L59-68](protocol/WsEnvelope.lua#L59), [App.lua#L73-75](app/App.lua#L73) | Event names hardcoded in multiple places | `protocol/WsEnvelope.lua` `EventType` table (already exists, enforce usage) |

---

## 2. Duplication Hotspots

| ID | Severity | Literal | Count | Files | Proposed Fix |
|----|----------|---------|-------|-------|--------------|
| D1 | Critical | `{0.0, 0.0, 0.0, 0.0}` (transparent color) | 8+ | [quickonline/display.lua#L256-261,336](modules/quickonline/display.lua#L256) | `constants/colors.lua` → `COLORS.TRANSPARENT` |
| D2 | Critical | `{0.3, 0.3, 0.3, 1.0}` (disabled button gray) | 6+ | [quickonline/display.lua#L412-414](modules/quickonline/display.lua#L412), [serverselection/display.lua#L317-319](modules/serverselection/display.lua#L317) | `constants/colors.lua` → `COLORS.BUTTON_DISABLED` |
| D3 | High | `120` (PushItemWidth) | 5+ | [AddFriendSection.lua#L17](modules/friendlist/components/AddFriendSection.lua#L17), [TagSelectDropdown.lua#L81](modules/friendlist/components/TagSelectDropdown.lua#L81), [TagManager.lua#L24](modules/friendlist/components/TagManager.lua#L24) | `UIConstants.INPUT_WIDTH_SMALL = 120` (already exists, use it) |
| D4 | High | `150` (PushItemWidth) | 4+ | [AddFriendSection.lua#L25](modules/friendlist/components/AddFriendSection.lua#L25), [TagSelectDropdown.lua#L25](modules/friendlist/components/TagSelectDropdown.lua#L25), [PrivacyTab.lua#L347](modules/friendlist/components/PrivacyTab.lua#L347) | `UIConstants.INPUT_WIDTH_MEDIUM = 150` |
| D5 | High | `0.95` (background alpha) | 5+ | [themes/data.lua#L58](modules/themes/data.lua#L58), [quickonline/display.lua#L692](modules/quickonline/display.lua#L692), [notifications/display.lua#L387](modules/notifications/display.lua#L387), [friendlist/display.lua#L84](modules/friendlist/display.lua#L84) | `constants/ui.lua` → `DEFAULTS.BACKGROUND_ALPHA` |
| D6 | High | `0.6` (sound volume default) | 4+ | [models.lua#L113](core/models.lua#L113), [NotificationsTab.lua#L25,26,35](modules/friendlist/components/NotificationsTab.lua#L25) | `constants/defaults.lua` → `DEFAULTS.SOUND_VOLUME` |
| D7 | Med | ImGuiWindowFlags hex fallbacks | 14 | [notifications/display.lua#L17-27](modules/notifications/display.lua#L17) | Create `constants/imgui_flags.lua` or rely on globals |
| D8 | Med | `{1.0, 1.0, 1.0, 1.0}` (white color) | 20+ | Multiple theme buffers | `constants/colors.lua` → `COLORS.WHITE` |
| D9 | Med | Status colors (online/offline/away) | 3+ | [HelpTab.lua#L88-98](modules/friendlist/components/HelpTab.lua#L88), [quickonline/display.lua#L639-643](modules/quickonline/display.lua#L639) | `constants/colors.lua` → `STATUS_COLORS.ONLINE/OFFLINE/AWAY` |
| D10 | Med | `200` (child height, toast margin, input height) | 6+ | Various | Context-specific - group by purpose |
| D11 | Med | Reconnect delays `1000`/`60000` | 2 | [WsClient.lua#L57-58](platform/services/WsClient.lua#L57) vs [TimingConstants.lua#L14-15](core/TimingConstants.lua#L14) | Consolidate in `TimingConstants` |
| D12 | Low | Toast states `"ENTERING"/"VISIBLE"/"EXITING"/"COMPLETE"` | 2 | [app/features/notifications.lua#L18-23](app/features/notifications.lua#L18), [modules/notifications/display.lua#L30-35](modules/notifications/display.lua#L30) | Single definition in `app/features/notifications.lua`, import in display |

---

## 3. Inappropriate Constants (Should Be User Settings)

| ID | Value | Current Location | Why User-Configurable | Proposed Home |
|----|-------|------------------|----------------------|---------------|
| I1 | `8000` (notification duration) | [notifications/display.lua#L14](modules/notifications/display.lua#L14) | Users want shorter/longer notifications | Already in prefs (`notificationDuration`), use it |
| I2 | `10.0`/`15.0` (notification position) | [notifications/display.lua#L12-13](modules/notifications/display.lua#L12) | Users may want different screen positions | Already in prefs (`notificationPositionX/Y`), ensure defaults centralized |
| I3 | `0.6` (sound volume) | [models.lua#L113](core/models.lua#L113) | Volume preference | Already in prefs (`notificationSoundVolume`), centralize default |
| I4 | Auto-open on start | [settings.lua#L94](libs/settings.lua#L94) | User preference | Already a setting, properly exposed |
| I5 | Column widths `120.0/100.0/etc` | [friendlist/display.lua#L101-107](modules/friendlist/display.lua#L101) | User may resize columns | Already persisted, needs default in constants |
| I6 | Window sizes (all) | Multiple display.lua files | User resizes windows | Persisted in settings, defaults in constants |
| I7 | Sound cooldowns `10000/5000` | [notificationsoundpolicy.lua#L9-10](core/notificationsoundpolicy.lua#L9) | Power users may want to adjust | Consider adding to advanced settings |
| I8 | Theme background/text alpha | [display.lua#L84-85](modules/friendlist/display.lua#L84) | Part of theme customization | Already in themes feature |

---

## 4. Inconsistent Naming / Units

| ID | Issue | Examples | Fix |
|----|-------|----------|-----|
| U1 | ms vs seconds naming | `HEARTBEAT_INTERVAL_MS` vs `REQUEST_CLEANUP_TIMEOUT_SECONDS` | Standardize: all timing in MS, suffix `_MS` |
| U2 | Pixel sizes without units | `120`, `150`, `200` for widths | Document as pixels in constant name or comment |
| U3 | Alpha range documentation | `0.95`, `0.6` - is this 0-1 or 0-255? | Standardize to 0.0-1.0, document |
| U4 | Color format inconsistency | `{r, g, b, a}` vs `{0.5, 0.5, 0.5, 1.0}` | Table format ok, but named fields (`{r=0.5, g=0.5}`) would be clearer |
| U5 | Window ID patterns | `##quickonline_main` vs `##serverselection` | Standardize: `##<module>_<window>` |

---

## 5. Recommended Constants/Config Architecture

### Directory Structure (No Mega-Files)

```
FFXIFriendList/
├── constants/                    # NEW: Compile-time constants
│   ├── init.lua                  # Re-exports all constants for convenience
│   ├── ui.lua                    # Window sizes, spacing, z-order
│   ├── colors.lua                # Color palette, status colors, theme tokens
│   ├── assets.lua                # Icon names, sound filenames, texture paths
│   ├── timings.lua               # Re-export from core/TimingConstants + additions
│   ├── protocol.lua              # Re-export WS event types, endpoints
│   └── limits.lua                # Max lengths, caps, thresholds
├── core/
│   ├── UIConstants.lua           # KEEP (mature, migrate gradients to constants/ui)
│   ├── TimingConstants.lua       # KEEP (mature)
│   ├── NetworkConstants.lua      # KEEP (mature)
│   ├── ServerConfig.lua          # KEEP (single source for server URL)
│   └── ...
├── libs/
│   └── settings.lua              # User preferences (persisted)
└── ...
```

### Access Patterns

```lua
-- From any module:
local UIConst = require("constants.ui")
local Colors = require("constants.colors")
local Assets = require("constants.assets")
local Timings = require("core.TimingConstants")  -- Already exists
local Limits = require("core.util").Limits       -- Already exists

-- Or via single import:
local C = require("constants")  -- aggregator
```

### Design Principles

1. **Domain grouping**: Constants grouped by what they describe, not where used
2. **No circular imports**: constants/ requires nothing from app/ or modules/
3. **Prefer small files**: Each ~50-100 lines max
4. **Named tables for enums**: `COLORS.ONLINE = {0, 1, 0, 1}` not raw literals
5. **Document units**: `WIDTH_PX`, `DURATION_MS`, `ALPHA_01`
6. **User-tunable → settings.lua**: If end-user should change it, it's a setting

---

## 6. Refactor Plan (Incremental PRs)

### PR1: Introduce Constants Architecture (LOW RISK)
- Create `constants/` directory with stub files
- Create `constants/ui.lua` with window sizes, spacing
- Create `constants/colors.lua` with transparent, white, status colors
- Create `constants/assets.lua` with sound/icon filenames
- Create `constants/init.lua` aggregator
- **NO behavioral changes - just adds new files**

### PR2: Migrate Top Duplicates (MEDIUM RISK)
- Replace hardcoded `{0.0, 0.0, 0.0, 0.0}` with `Colors.TRANSPARENT`
- Replace hardcoded `{0.3, 0.3, 0.3, 1.0}` with `Colors.BUTTON_DISABLED`
- Replace window size literals with `UI.WINDOW_SIZES.*`
- Replace sound filenames with `Assets.SOUNDS.*`
- Fix ServerConfig duplication in settings.lua and FFXIFriendList.lua

### PR3: Consolidate Timing Constants (LOW RISK)
- Use `TimingConstants.NOTIFICATION_DEFAULT_DURATION_MS` everywhere
- Remove duplicate `DEFAULT_DURATION_MS = 8000` in notifications/display.lua
- Align WsClient reconnect delays with TimingConstants

### PR4: Standardize Window IDs (LOW RISK)
- Create `UI.WINDOW_IDS` table
- Replace string literals for window titles/IDs

### PR5: Cleanup & Deprecation (LOW RISK)
- Add deprecation comments to old patterns
- Update documentation

---

## 7. Implementation Status

### ✅ PR1 - Constants Architecture (COMPLETE)

**New Files Created:**

| File | Purpose |
|------|---------|
| `constants/init.lua` | Aggregator re-exporting all constant modules |
| `constants/ui.lua` | Window sizes, spacing, input widths, button sizes, animation timing |
| `constants/colors.lua` | TRANSPARENT, WHITE, STATUS colors, BUTTON states, TOAST colors |
| `constants/assets.lua` | Sound filenames, icon names, paths |
| `constants/limits.lua` | Character name max, friend list size, network limits |

### ✅ PR2 - Top Duplicate Migrations (COMPLETE)

**Files Modified:**

| File | Changes |
|------|---------|
| `modules/notifications/display.lua` | Added imports for TimingConstants, UIConst, Colors, Assets; replaced hardcoded sound filenames with `Assets.SOUNDS.*` |
| `modules/quickonline/display.lua` | Added UIConst, Colors imports; replaced `{0.0,0.0,0.0,0.0}` with `Colors.TRANSPARENT`; replaced `{0.3,0.3,0.3,1.0}` with `Colors.BUTTON.DISABLED` |
| `modules/serverselection/display.lua` | Added UIConst, Colors imports; replaced window size `{350,0}` with `UIConst.WINDOW_SIZES.SERVER_SELECTION`; replaced disabled button colors |
| `libs/settings.lua` | Added `ServerConfig = require('core.ServerConfig')`; replaced hardcoded URL with `ServerConfig.DEFAULT_SERVER_URL` |
| `FFXIFriendList.lua` | Added ServerConfig import; replaced hardcoded URL; added Assets import; replaced sound filenames with `Assets.SOUNDS.*` |

### 🔜 PR3-PR5 - Remaining Work

See refactor plan above. The remaining migrations are lower priority and can be addressed incrementally.

---

## 8. Quick Reference - Using the New Constants

```lua
-- Import individual modules
local UIConst = require("constants.ui")
local Colors = require("constants.colors")
local Assets = require("constants.assets")
local Limits = require("constants.limits")

-- Or use the aggregator
local C = require("constants")
-- Then: C.ui.WINDOW_SIZES.*, C.colors.TRANSPARENT, etc.

-- Examples
imgui.SetNextWindowSize(UIConst.WINDOW_SIZES.SERVER_SELECTION, ...)
imgui.PushStyleColor(ImGuiCol_WindowBg, Colors.TRANSPARENT)
playSound(Assets.SOUNDS.FRIEND_ONLINE)
if #name > Limits.NAME_LENGTH.MAX_CHARACTER_NAME then ... end
```

