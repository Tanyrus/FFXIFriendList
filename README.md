
<p align="center">
  <a href="https://discord.gg/JXsx7zf3Dx">
    <img src="assets/discord_banner.png" alt="Join the FFXI Friend List Discord">
  </a>
</p>

## Data Privacy & Terms of Service

**IMPORTANT**: This plugin requires a remote server to function. By using it, you agree to the following terms. This plugin is community-developed and is not affiliated with or endorsed by Square Enix or any private server operators. The service processes only in-game character data required to provide friend list functionality, including character name and realm, friend relationships and requests, presence and heartbeat timestamps used to compute online status and last seen, character metadata such as job, nation, rank, and zone when sharing is enabled, account associations, and server-synced preferences. Friend notes are stored locally and never transmitted to the server. No real-world personal information such as name, email, or address is collected. Data is used only to operate the service, is not sold, tracked, advertised, or shared with third parties, raw IP addresses are not stored by the application though infrastructure providers may process them, server-side data may be backed up for reliability, and data is retained while your account remains active with account deletion available upon request. The service is provided “as-is” with no uptime guarantees, access may be revoked for abuse or operational reasons, you are responsible for protecting your API key, and use of the plugin and service is at your own risk. Continued use constitutes acceptance of these terms, which may change over time.

### Donations & Monetization

No donations, payments, subscriptions, or other financial contributions are accepted or solicited for this plugin or its associated service. This project is non-commercial and provided purely as a community service.

# FriendList

A comprehensive friend list management plugin for Horizon XI (FFXI) using the Ashita v4 framework. This plugin provides real-time friend status tracking, account-based character linking, notes (local), and more through a modern ImGui interface.

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

## Commands

### Main Commands

- **`/fl`** - Toggle the friend list window (show/hide)
- **`/fl help`** or **`/fl h`** - Display help information
- **`/befriend <name>`** - Send a friend request to the specified player

## Installation

### Prerequisites

1. **Ashita v4**: This plugin requires Ashita v4 framework

### Installation Steps

1. Download the latest release of FriendList plugin
2. Extract the plugin files to your Ashita plugins directory:
3. Ensure the plugin DLL (`FFXIFriendList.dll`) is in the plugins directory
4. Launch Ashita and the plugin will load automatically

## Configuration

### Configuration Files

Configuration files are stored in:
- `HorizonXI\Game\config\FFXIFriendList\`

### Realm Detection

**To set your realm**: The plugin will request your server on first load. If your server is not listed, submit an issue on Github and I will add it to the server.

### Local Data

Local-only data (not synced to server):
- Local notes (default; stored locally per-account, shared across characters on the same account)
- Theme preference
- Notification duration

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
2. Right-click on a friend amd select "Edit Note" (or just left click)
3. Enter your note in the editor
4. Click "Save"

**Note Storage**:
- **Local notes** (default): Notes are stored locally on your device (per-account) and are not synced to the server. Notes are shared across all characters on the same account.

### Switching Characters

1. Log out and log in with a different character
2. Plugin automatically detects character change
3. Server handles account association automatically
4. Friend list and notes are shared if characters are on the same account

**Note**: The server automatically merges accounts if you switch to a character that belongs to a different account. Notes are stored per-account and are shared across all characters on the same account (local notes are local-only).

## Plugin Not Loading

- **Check Ashita version**: Ensure you're using Ashita v4
- **Check plugin location**: Verify `FFXIFriendList.dll` is in the `plugins` directory
- **Check Ashita logs**: Look for error messages in Ashita's log files

## Support

### Reporting Issues

When reporting issues, please include:
- Plugin version (0.9.6)
- Ashita version
- Realm (Horizon, Eden, Nasomi, etc.)
- Steps to reproduce the issue
- Screenshots (if applicable)

## License

This project is licensed under the GNU General Public License v3 (GPLv3). You can find the full license text [here](LICENSE).

**Key aspects of GPLv3:**
*   **Free Software:** Grants users the freedom to run, study, modify, and share the software [7, 10].
*   **Strong Copyleft:** Any software derived from or linked with this code must also be released under GPLv3 [1, 4, 5].
*   **Patent Protection:** Explicitly protects users from patent claims by contributors [9, 10].

## Acknowledgments

- Uses the Ashita v4 plugin framework
- Powered by ImGui for the user interface
