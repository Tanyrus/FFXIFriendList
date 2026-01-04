# FFXIFriendList v0.9.9 - Lua Addon Release

**Release Date:** January 2026  
**Compatibility:** Ashita v4

---

## Highlights

This release marks a complete rewrite of FFXIFriendList from a C++ plugin to a **native Lua addon**. All features from the C++ version have been ported with full behavioral parity, plus several bug fixes and quality-of-life improvements.

### Why Lua?

- **Easier to modify** - No compilation required; edit and reload
- **Better compatibility** - Native Ashita v4 addon format
- **Simpler installation** - Just copy the addon folder
- **Community contributions** - Lower barrier for fixes and features

---

## What's New

### Improved Friend Details

- **Single-click** now opens the Friend Details popup (previously required double-click)
- Friend Details popup now includes an integrated note editor
- All fields show "Unknown" for missing data instead of blank

---

## Bug Fixes

| Issue | Fix |
|-------|-----|
| Notification position broken on first load | Default position now correctly set to (10, 15) |
| Notification duration slider had no effect | Duration now properly reads from preferences |
| Notifications stacked incorrectly after expiring | Position resets correctly when all toasts expire |
| Note timestamps showed "unknown" | Notes now display formatted dates (YYYY-MM-DD HH:MM:SS) |
| Toast notifications had unwanted title bar | Fixed to render without title bar or close button |
| Opening note editor crashed the addon | Fixed function ordering that caused the crash |
| Individual sound tests didn't play | Fixed sound playback for single sound tests |

---

## Breaking Changes

### Installation Location Changed

| Aspect | C++ Plugin (Old) | Lua Addon (New) |
|--------|------------------|-----------------|
| Type | Plugin DLL | Addon folder |
| Location | `plugins/FFXIFriendList.dll` | `addons/FFXIFriendList/` |
| Load | Auto-loads with Ashita | `/addon load FFXIFriendList` |

### Configuration Not Migrated

- C++ plugin configuration is **not** automatically migrated
- Settings will be recreated on first run
- Local notes are stored separately and will not persist

### Controller Support Removed

- Controller B button to close windows is no longer available
- Ashita Lua does not expose XInput for controller detection
- Use ESC key or click the X button to close windows

---

## Upgrade Guide

### Step 1: Remove the C++ Plugin

1. Close FFXI completely
2. Navigate to your Ashita installation folder
3. Delete the old plugin DLL from the `plugins/` folder:
   - `plugins/FFXIFriendList.dll` (or `plugins/XIFriendList.dll`)
4. Update your startup script (`scripts/default.txt` or your character script):
   - **Remove** the old plugin load line: `/load ffxifriendlist`
   - **Add** the new addon load line: `/addon load ffxifriendlist`

> **Note:** The old `/load` command loaded plugins. The new `/addon load` command loads Lua addons.

### Step 2: Install the Lua Addon

1. Download `FFXIFriendList-0.9.9.zip` from the release
2. Extract the `FFXIFriendList` folder to `Ashita/addons/`
3. Your folder structure should be: `Ashita/addons/FFXIFriendList/FFXIFriendList.lua`

### Step 3: Load the Addon

1. Launch FFXI through Ashita
2. Run `/addon load FFXIFriendList`
3. The Server Selection window will appear - select your server
4. Enter your character's API key when prompted

### Step 4: (Optional) Auto-Load on Startup

Add to your Ashita default script or character script:
```
/addon load FFXIFriendList
```

---

## All Features

All features from the C++ version are fully supported:

### Friend Management
- Friend list with real-time online/offline status
- Send, accept, reject, and cancel friend requests
- Add friends via UI or `/befriend <name>` command
- Toast notifications for friends coming online and new requests

### Friend Information
- Job and level display (when sharing enabled)
- Nation and rank display (when sharing enabled)
- Current zone/location (when sharing enabled)
- Last seen timestamp (server-computed)
- Linked characters per account

### Notes System
- Local notes stored per-account
- Rich note editor with character count
- Notes shared across all characters on same account

### Privacy Controls
- Share online status toggle
- Share character data toggle
- Share location toggle
- Alt visibility controls
- Preferences synced to server

### Customization
- 4 built-in themes (Classic, Modern Dark, Green Nature, Purple Mystic)
- Custom theme creation and saving
- Column visibility toggles
- Notification position and duration settings

### Quick Online View
- Minimal window showing only online friends
- Hover for details, SHIFT+hover for all fields

---

## Known Differences from C++

These differences are architectural and do not affect normal usage:

| Difference | Reason |
|------------|--------|
| Network latency +16-50ms | Lua uses non-blocking polling vs C++ blocking calls |
| Max 8 concurrent requests | Lua request queue limit vs C++ unlimited threads |
| Status icons are colored bullets | Simpler implementation; same visual effect |
| No controller close button | XInput not available in Ashita Lua |

---

## Troubleshooting

### Addon Won't Load

1. Check Ashita console for Lua errors
2. Verify `FFXIFriendList.lua` exists in `addons/FFXIFriendList/`
3. Ensure all addon files are present (not just the main file)

### Connection Issues

1. Verify your server is reachable
2. Check console for network errors
3. Try `/fl connect` manually
4. Re-enter API key if prompted

### Menu Detection Not Working

The addon normally auto-opens when you open FFXI's friend list menu. If this doesn't work:

1. Use `/fl menutoggle` as a manual fallback
2. Enable debug mode: `/fl menudebug on`
3. Check status: `/fl menudebug status`

---

## Commands Reference

| Command | Description |
|---------|-------------|
| `/fl` | Toggle main window |
| `/fl show` | Show main window |
| `/fl hide` | Hide main window |
| `/fl connect` | Connect to server |
| `/fl disconnect` | Disconnect from server |
| `/fl refresh` | Refresh friend list |
| `/fl notify` | Show test notification |
| `/fl soundtest <type>` | Test sounds (online, request, all) |
| `/fl menutoggle` | Manual window toggle |
| `/fl menudebug <cmd>` | Debug menu detection |
| `/fl help` | Show all commands |
| `/befriend <name>` | Send friend request |

---

## Credits

- Original C++ Plugin and Lua Port: Tanyrus
- Ashita v4: Atom0s and the Ashita Development Team
- Async Networking: [nonBlockingRequests](https://github.com/loonsies/nonBlockingRequests) by loonsies

---

## Full Changelog

**[View all changes on GitHub](../../compare/main...lua-conversion)**

### Summary
- Complete rewrite from C++ to Lua (~416,000 lines of C++ replaced with ~30,000 lines of Lua)
- 423 files changed
- Full behavioral parity with C++ version
- 7 bug fixes
- 4 new commands
- Improved Friend Details UX

---

## Support

- **Discord:** [Join the FFXI Friend List Discord](https://discord.gg/JXsx7zf3Dx)
- **Issues:** Report bugs on GitHub Issues

