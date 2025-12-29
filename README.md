### Data Privacy & Terms of Service

**IMPORTANT**: This plugin requires a remote server to function. By using this plugin, you agree to the following terms.

**Disclaimer**: This plugin is community-developed and is not affiliated with or endorsed by Square Enix or any private server operators.

#### Data Collection & Storage

**Server-Side Data**: The following data is stored **server-side** to provide the service:
- Character names and realm identifiers
- Friend relationships and friend requests
- Presence/heartbeat timestamps (used to compute online/offline status)
- Character metadata (job, rank, nation, zone)
- User preferences and display settings
- Account associations (which characters belong to which account)

**Local-Only Data**: The following data is stored **locally on your device** and is never transmitted to the server:
- Local notes about friends (default; stored per-account, shared across characters on the same account)
- Theme preferences
- Debug mode setting
- Notification duration preference

**Notes Storage**: Notes are stored locally on your device and are never transmitted to the server.

**Privacy Controls**: You have full control over what data is shared:
- **Share Online Status**: Control whether you appear online to friends
- **Share Character Data**: Control sharing job/nation/rank information
- **Share Location**: Control sharing your current zone/location
- **Anonymous Mode**: Hide all character data while online

#### Privacy & Data Protection

**No Real-World Personal Information**: The service does not collect real-world personal information (name, email, address, etc.). Only in-game character data is collected.

**Data Usage**: 
- Data is used solely to provide the friend list service
- Data is not used for tracking, advertising, or shared with third parties
- Data is not sold or monetized

**IP Address Handling**: 
- The application does not store or log raw IP addresses
- Infrastructure providers (e.g., Cloudflare, Nginx, hosting) may still process/log IP addresses depending on their configuration
- Rate limiting uses short-lived, pseudonymous tokens derived from IP addresses using a secret-keyed HMAC. Without access to the server secret, recovering the original IP address from a token is computationally infeasible

**Data Retention**: 
- Data is retained as long as your account is active
- You can request account deletion by contacting the service operator
- Deleted accounts and their associated data are permanently removed

**Backups & Service Integrity**: Server-side data (including character data, and preferences) may be backed up for service integrity and disaster recovery. Local notes are not backed up server-side.

#### Service Terms

**Service Availability**:
- The service is provided "as-is" with no guarantees of uptime or availability
- The operator may perform maintenance, updates, or modifications at any time
- Service may be temporarily unavailable due to technical issues or maintenance

**Access & Account Management**:
- The operator may revoke access (API key) for abuse, violations, or operational reasons
- Banned accounts cannot authenticate or use the service
- Account bans propagate to all characters on the account

**User Responsibilities**:
- Do not share your API key with others
- Do not use the service for abusive, illegal, or harmful purposes
- Respect other users' privacy settings
- Report abuse or violations to the service operator

**Limitation of Liability**:
- The service operator is not liable for any loss of data, functionality, or service availability
- The service operator is not responsible for any consequences of using the plugin or service
- Use of the plugin and service is at your own risk

**Acceptance of Terms**:
- Continued use of the plugin constitutes acceptance of these terms
- If you do not agree to these terms, do not use the plugin
- Terms may be updated at any time; continued use after updates constitutes acceptance

---

# FriendList

A comprehensive friend list management plugin for Horizon XI (FFXI) using the Ashita v4 framework. This plugin provides real-time friend status tracking, account-based character linking, notes (local), and more through a modern ImGui interface.

**Version:** 0.9.0
**Author:** Carrott  
**License:** GPL-3.0

## Features

### Core Friend Management
- **Friend List Display**: View all your friends with real-time online/offline status (server-computed)
- **Friend Requests**: Send, accept, reject, or cancel pending friend requests
- **Add Friends**: Easily add new friends via the UI or `/befriend` command
- **Toast Notifications**: Get notified when friends come online or new friend requests arrive

### Friend Information
- **Job Information**: View friends' current job and level (when sharing enabled)
- **Rank Display**: See friends' nation rank (when sharing enabled)
- **Nation Information**: Display which nation friends belong to (when sharing enabled)
- **Zone/Location**: Track which zone friends are currently in (when sharing enabled)
- **Last Seen**: See when friends were last online (durable, server-computed)
- **Linked Characters**: See all characters linked to a friend's account (server-provided)

### Notes System
- **Local Notes**: Notes are stored locally on your device (not synced to server)
- **Note Editor**: Rich text editor for friend notes with character count
- **Per-Account**: Notes are stored per-account and shared across all characters on the same account

### Account & Character Management
- **Automatic Account Association**: Server automatically associates characters to accounts
- **Character Switching**: Server handles character switching and account merging automatically
- **Alt Awareness**: See all linked characters for friends (server-provided)
- **Identity Recovery**: If API key is lost, server can recover identity via character name + realm

### Privacy & Preferences
- **Share Online Status**: Control whether you appear online to friends
- **Share Character Data**: Control sharing job/nation/rank information
- **Share Location**: Control sharing your current zone/location
- **Anonymous Mode**: Hide all character data while online
- **Preferences Sync**: Preferences are synced to the server across all characters (per-account)
- **Immediate Apply**: Preference changes apply immediately (no save button needed)
- **Notification Duration**: Customize how long toast notifications are displayed

### Customization
- **Themes**: 4 built-in themes plus support for custom themes
  - Warm Brown
  - Modern Dark
  - Green Nature
  - Purple Mystic
- **Custom Themes**: Create and save your own color schemes
- **Transparency Controls**: Adjust window and text transparency
- **Font Scaling**: Customize font size
- **Column Visibility**: Show/hide columns (Job, Rank, Nation, Zone, Last Seen)
- **Column Resizing**: Resize table columns to your preference
- **Color Customization**: Fine-tune colors for all UI elements

### Technical Features

#### Memory Usage Reporting

The plugin provides memory usage statistics to help understand resource consumption:

**Commands:**
- `/fl stats` - Display memory usage report in chat
- `/fl debug memory` - Alternative command for memory stats

**Debug UI:**
- In debug builds, memory stats are also shown in the Debug Settings section of the Options window

**What the stats represent:**
- **Friends**: Memory used by friend list data structures (friend entries and status information)
- **Notes**: Memory used by stored notes (local or server-synced)
- **Themes**: Memory used by custom theme definitions
- **Notifications**: Memory used by active toast notifications
- **Icons/Textures**: Estimated memory for loaded icon textures

**What the stats do NOT represent:**
- These are approximate estimates based on container sizes and known data structures
- They do NOT measure total process memory or OS-level allocations
- They do NOT include memory used by ImGui, Direct3D, or other system libraries
- They are best-effort estimates for plugin-owned data structures only

The reported values are rounded to the nearest KB and are intended for relative comparisons and debugging, not precise memory accounting.
- **Real-time Updates**: Automatic polling for friend status and requests
- **Presence Heartbeat**: Lightweight presence updates every 10 seconds (sole writer for online status)
- **Full Data Refresh**: Complete data refresh every 60 seconds
- **Player Data Tracking**: Automatic tracking of your job, rank, nation, and zone
- **Packet Detection**: Zone detection via game packets to update player zone
- **Non-blocking Operations**: All network operations are asynchronous and never block the game
- **Server-Authoritative**: Server computes all derived state (online, last seen, visibility)

## Server-Authoritative Design

This plugin follows a **server-authoritative** architecture where the server is the single source of truth for all derived state and identity decisions.

### What the Plugin Reports

The plugin only reports **facts** to the server:
- Current character name
- Realm identifier (detected from sentinel files)
- Session identifier
- Character state (job, zone, nation, rank, isAnonymous)
- User preferences
- Heartbeat timestamps

### What the Server Decides

The server makes all **decisions**:
- **Identity resolution**: Whether a character exists, belongs to an account, or needs to be created
- **Account association**: Which account a character belongs to
- **Character merging**: Automatically merging accounts when switching to a character on a different account
- **Online/offline state**: Computed from heartbeat timestamps + TTL (2 minutes) + privacy settings
- **Last seen**: Durable timestamp computed from heartbeat history
- **Visibility rules**: What data to show based on privacy settings (shareOnlineStatus, shareCharacterData, shareLocation)

### What the Plugin Never Does

The plugin **never attempts to**:
- Merge accounts or link characters independently
- Infer online status from local data
- Calculate last seen timestamps
- Recover identities without server assistance
- Make visibility filtering decisions

**Plugin behavior**: The plugin displays only what the server returns. It does not calculate or infer any derived state.

## Commands

### Main Commands

- **`/fl`** - Toggle the friend list window (show/hide)
- **`/fl help`** or **`/fl h`** - Display help information
- **`/befriend <name>`** - Send a friend request to the specified player

## Installation

### Prerequisites

1. **Ashita v4**: This plugin requires Ashita v4 framework
2. **Horizon XI or other supported realms**: Designed for Horizon XI, Eden, Nasomi, Catseye, Gaia, or LevelDown99
3. **Windows**: Currently only supports Windows (32-bit build required for FFXI)

### Installation Steps

1. Download the latest release of FriendList plugin
2. Extract the plugin files to your Ashita plugins directory:
3. Ensure the plugin DLL (`FFXIFriendList.dll`) is in the plugins directory
4. Launch Ashita and the plugin will load automatically

### First-Time Setup

1. Open the friend list window using `/fl`
2. The plugin will automatically:
   - Detect your realm from sentinel files (or default to Horizon)
   - Connect to the FriendList server
   - Register your character (server creates account + character if new)
   - Receive your API key from the server
3. You can start adding friends immediately!

## Configuration

### Configuration Files

Configuration files are stored in:
- `HorizonXI\Game\config\FFXIFriendList\`

### Realm Detection

The plugin detects which realm/server you're playing on via **sentinel files** in the config directory:
- **Location**: `config/FFXIFriendList/`
- **Supported sentinel filenames** (case-sensitive, no extension):
  - `Nasomi` → realm: `nasomi`
  - `Eden` → realm: `eden`
  - `Catseye` → realm: `catseye`
  - `Horizon` → realm: `horizon`
  - `Gaia` → realm: `gaia`
  - `LevelDown99` → realm: `leveldown99`
- **Default**: If no sentinel file is found, defaults to `horizon`
- **Priority**: If multiple sentinel files exist, priority is: Nasomi > Eden > Catseye > Horizon > Gaia > LevelDown99

**To set your realm**: Create an empty file (no extension) with the realm name in `config/FFXIFriendList/`

### Main Configuration

The main config file (`ffxifriendlist.ini`) contains:
- **Debug Mode**: Enable/disable debug logging (messages appear in Debug Log window)
- **Theme**: Selected theme name
- **Column Visibility**: Show/hide individual columns
- **API Key**: Server authentication key (auto-generated by server, do not modify manually)
- **Notification Duration**: How long toast notifications are displayed (seconds)

### Preferences (Server-Synced)

These preferences are stored on the server and sync across all your characters (per-account):
- Share Online Status
- Share Character Data
- Share Location
- Column Visibility settings

**Note**: Preference changes apply immediately (no save button needed).

### Local Data

Local-only data (not synced to server):
- Local notes (default; stored locally per-account, shared across characters on the same account)
- Debug mode setting
- Theme preference
- Notification duration

## Server Connection

### Server Information

- **Server**: Operated by Tanyrus
- **Endpoint**: `https://api.horizonfriendlist.com`
- **Protocol**: REST API (HTTPS)
- **Authentication**: API key (generated by server on first registration)

### Data Collection

The plugin connects to the FriendList server to provide the following features:
- Friend list synchronization
- Friend request management
- Automatic account + character association
- Preference synchronization (per-account)
- Presence tracking (heartbeat-based)

### API Key

- Your API key is **automatically generated by the server** when you first register
- The API key is **per-character** (stored per-character in the database) but functionally **account-scoped** for authentication (any API key from the same account can authenticate any character on that account)
- The key is stored locally in your configuration file
- **Do not share your API key** - it provides access to your account
- **Recovery**: If you lose your API key, the server can recover it via `POST /api/auth/ensure` with character name + realm (no API key required)

### What Data is Collected

The server stores the following data:
- Character name (in-game identifier)
- Realm identifier
- Friend relationships (per-account)
- Friend requests (pending, accepted, rejected)
- Account associations (which characters belong to which account)
- Preference settings (per-account)
- Presence/heartbeat timestamps (per-character, used to compute online/offline)
- Last seen timestamps (durable, computed from heartbeat history)
- Current zone/location (if sharing enabled)
- Job/rank/nation information (if sharing enabled)

## Authentication & Identity Flow

### Initial Registration

1. Plugin detects character name and realm
2. Plugin calls `POST /api/auth/ensure` with character name + realm (no API key)
3. Server checks if character exists:
   - **If new**: Creates account + character, returns new API key
   - **If exists**: Returns existing API key (recovery)
   - **If banned**: Returns `403 API_KEY_REVOKED`
4. Plugin stores API key locally

### Character Switching

1. Plugin detects character change
2. Plugin calls `POST /api/characters/active` with new character name + realm
3. Server decides:
   - **Character on same account**: Sets as active character
   - **Character on different account**: Auto-merges accounts, sets as active
   - **Character doesn't exist**: Auto-creates character, attaches to account
4. Plugin updates stored identity based on server response

### API Key Recovery

If local data is deleted or API key is lost:
1. Plugin calls `POST /api/auth/ensure` with character name + realm (no API key)
2. Server checks if character exists
3. If exists and not banned → server returns existing API key
4. Plugin stores recovered API key

### Banned Account Behavior

If account is banned:
- Server rejects all authentication requests with `403 API_KEY_REVOKED`
- Plugin displays message: "API key revoked; contact plugin owner."
- Plugin cannot connect to server

## Usage Examples

### Adding a Friend

1. Open the friend list window (`/fl`)
2. Click "Add New Friend" button
3. Enter the friend's character name
4. Press Enter or click "Add Friend"
5. A friend request will be sent

Alternatively, use the command:
```
/befriend PlayerName
```

### Adding a Note

1. Open the friend list window
2. Right-click on a friend
3. Select "Edit Note"
4. Enter your note in the editor
5. Click "Save"

**Note Storage**:
- **Local notes** (default): Notes are stored locally on your device (per-account) and are not synced to the server. Notes are shared across all characters on the same account.

### Switching Characters

1. Log out and log in with a different character
2. Plugin automatically detects character change
3. Server handles account association automatically
4. Friend list and notes are shared if characters are on the same account

**Note**: The server automatically merges accounts if you switch to a character that belongs to a different account. Notes are stored per-account and are shared across all characters on the same account (local notes are local-only).

## Friend List Behavior

### Online/Offline Status

- **Online**: Friend's last heartbeat was within 2 minutes AND `shareOnlineStatus: true`
- **Offline**: Friend's last heartbeat was more than 2 minutes ago OR `shareOnlineStatus: false`
- **Computed by**: Server (plugin displays server-provided `isOnline` field)
- **Plugin behavior**: Plugin never calculates online status locally

### Last Seen

- **Source**: Server-computed from durable heartbeat timestamps
- **Display**: Only shown when friend is offline AND `shareOnlineStatus: true`
- **Durability**: Last seen survives plugin reloads and server restarts
- **Plugin behavior**: Plugin displays server-provided `lastSeenAt` field

### Display Names

- **"Name" column**: Current active character name (server-provided)
- **"Friended As" column**: Immutable label from when friendship was created
- **Linked Characters**: Array of all character names for this friend's account (server-provided)

### Visibility Filtering

Server filters character data based on privacy settings:
- **If offline**: All data hidden (job, zone, nation, rank = null)
- **If `shareCharacterData: false`**: Job, nation, rank = null
- **If `shareLocation: false`**: Zone = null (even if online and sharing character data)
- **If `isAnonymous: true`**: All data hidden (overrides other settings)

**Plugin behavior**: Plugin displays only fields returned by server. Fields returned as `null` should not be displayed.

## Troubleshooting

### Plugin Not Loading

- **Check Ashita version**: Ensure you're using Ashita v4
- **Check plugin location**: Verify `FFXIFriendList.dll` is in the `plugins` directory
- **Check Ashita logs**: Look for error messages in Ashita's log files

### DLL Load Failure (LoadLibraryA Error)

If you see a "Load failure for LoadLibraryA" error:

- **Missing Visual C++ Redistributable**: The DLL requires the Visual C++ Redistributable (x86) to be installed
  - Download: [Visual C++ 2015-2022 Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe)
  - **Important**: Install the **x86 (32-bit)** version, not x64
- **Rebuild Required**: If you're building from source, rebuild with the updated CMakeLists.txt which uses static runtime linking
  - See `DLL_LOAD_FAILURE_TROUBLESHOOTING.md` for detailed instructions

### Cannot Connect to Server

- **Check internet connection**: Ensure you have an active internet connection
- **Check firewall**: Ensure Ashita/FFXI is allowed through your firewall
- **Check server status**: The server may be temporarily unavailable
- **Check debug logs**: Enable debug mode to see detailed connection logs
- **Check realm detection**: Verify sentinel file exists in `config/FFXIFriendList/` if not using default

### Friends Not Showing

- **Wait for sync**: Friend list syncs every 60 seconds, wait a moment
- **Check API key**: Ensure your API key is valid (should be auto-generated by server)
- **Check debug logs**: Enable debug mode to see sync status
- **Check server connection**: Verify plugin is connected to server

### Notes Not Saving

- **Check permissions**: Ensure the plugin has write permissions to the config directory (for local notes)
- **Check debug logs**: Enable debug mode to see save errors

### UI Not Appearing

- **Check window visibility**: Try toggling the window with `/fl`
- **Check window position**: Windows may be off-screen; reset window positions in Options
- **Check theme**: Try switching themes in case of rendering issues

### Performance Issues

- **Reduce polling frequency**: Not configurable, but the plugin uses efficient polling (10s heartbeat, 60s refresh)
- **Disable debug mode**: Debug logging can impact performance
- **Check network**: Slow network connections may cause delays

### Friend Shows Offline When Online

- **Check heartbeat**: Verify plugin is sending heartbeat every 10 seconds
- **Check privacy**: Friend may have `shareOnlineStatus: false`
- **Check server time**: Verify server time is correct (affects TTL calculations)
- **Wait for sync**: Status updates every 10 seconds (heartbeat) or 60 seconds (full refresh)

## Support

### Getting Help

- **Debug Mode**: Enable debug mode in Options to see detailed logs
- **Debug Log Window**: View debug messages in the Debug Log window (accessible from Options)
- **Check Logs**: Review Ashita log files for error messages

### Reporting Issues

When reporting issues, please include:
- Plugin version (0.9.0)
- Ashita version
- Realm (Horizon, Eden, Nasomi, etc.)
- Error messages from debug logs
- Steps to reproduce the issue
- Screenshots (if applicable)

## License

This project is licensed under the GNU General Public License v3 (GPLv3). You can find the full license text [here](LICENSE).

**Key aspects of GPLv3:**
*   **Free Software:** Grants users the freedom to run, study, modify, and share the software [7, 10].
*   **Strong Copyleft:** Any software derived from or linked with this code must also be released under GPLv3 [1, 4, 5].
*   **Patent Protection:** Explicitly protects users from patent claims by contributors [9, 10].

## Acknowledgments

- Built for the Horizon XI community
- Uses the Ashita v4 plugin framework
- Powered by ImGui for the user interface
