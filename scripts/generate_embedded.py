#!/usr/bin/env python3
"""
Script to generate embedded resources header
Converts PNG files and WAV sound files to embedded C++ byte arrays
"""

import os
import sys
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    # Image inputs
    online_input = project_root / "assets" / "icons" / "online.png"
    friend_request_input = project_root / "assets" / "icons" / "friend_request.png"
    discord_input = project_root / "assets" / "icons" / "discord.png"
    github_input = project_root / "assets" / "icons" / "github.png"
    heart_input = project_root / "assets" / "icons" / "heart.png"
    sandy_icon_input = project_root / "assets" / "icons" / "sandy_icon.png"
    bastok_icon_input = project_root / "assets" / "icons" / "bastok_icon.png"
    windy_icon_input = project_root / "assets" / "icons" / "windy_icon.png"
    lock_icon_input = project_root / "assets" / "icons" / "lock_icon.png"
    unlock_icon_input = project_root / "assets" / "icons" / "unlock_icon.png"
    
    # Sound inputs
    online_sound_input = project_root / "assets" / "sounds" / "online.wav"
    friend_request_sound_input = project_root / "assets" / "sounds" / "friend-request.wav"
    
    output = project_root / "src" / "Platform" / "Ashita" / "EmbeddedResources.h"
    
    # Check image files
    if not online_input.exists():
        print(f"Error: {online_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not friend_request_input.exists():
        print(f"Error: {friend_request_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not discord_input.exists():
        print(f"Error: {discord_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not github_input.exists():
        print(f"Error: {github_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not heart_input.exists():
        print(f"Error: {heart_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not sandy_icon_input.exists():
        print(f"Error: {sandy_icon_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not bastok_icon_input.exists():
        print(f"Error: {bastok_icon_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not windy_icon_input.exists():
        print(f"Error: {windy_icon_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not lock_icon_input.exists():
        print(f"Error: {lock_icon_input} not found", file=sys.stderr)
        sys.exit(1)
    
    if not unlock_icon_input.exists():
        print(f"Error: {unlock_icon_input} not found", file=sys.stderr)
        sys.exit(1)
    
    # Check sound files (optional - warn if missing but don't fail)
    has_online_sound = online_sound_input.exists()
    has_friend_request_sound = friend_request_sound_input.exists()
    
    if not has_online_sound:
        print(f"Warning: {online_sound_input} not found (sound will not be embedded)")
    
    if not has_friend_request_sound:
        print(f"Warning: {friend_request_sound_input} not found (sound will not be embedded)")
    
    # Read image files
    online_size = online_input.stat().st_size
    friend_request_size = friend_request_input.stat().st_size
    discord_size = discord_input.stat().st_size
    github_size = github_input.stat().st_size
    heart_size = heart_input.stat().st_size
    sandy_icon_size = sandy_icon_input.stat().st_size
    bastok_icon_size = bastok_icon_input.stat().st_size
    windy_icon_size = windy_icon_input.stat().st_size
    lock_icon_size = lock_icon_input.stat().st_size
    unlock_icon_size = unlock_icon_input.stat().st_size
    online_bytes = online_input.read_bytes()
    friend_request_bytes = friend_request_input.read_bytes()
    discord_bytes = discord_input.read_bytes()
    github_bytes = github_input.read_bytes()
    heart_bytes = heart_input.read_bytes()
    sandy_icon_bytes = sandy_icon_input.read_bytes()
    bastok_icon_bytes = bastok_icon_input.read_bytes()
    windy_icon_bytes = windy_icon_input.read_bytes()
    lock_icon_bytes = lock_icon_input.read_bytes()
    unlock_icon_bytes = unlock_icon_input.read_bytes()
    
    # Read sound files (if they exist)
    online_sound_size = 0
    friend_request_sound_size = 0
    online_sound_bytes = b""
    friend_request_sound_bytes = b""
    
    if has_online_sound:
        online_sound_size = online_sound_input.stat().st_size
        online_sound_bytes = online_sound_input.read_bytes()
    
    if has_friend_request_sound:
        friend_request_sound_size = friend_request_sound_input.stat().st_size
        friend_request_sound_bytes = friend_request_sound_input.read_bytes()
    
    # Generate header file
    output_lines = []
    output_lines.append("// EmbeddedResources.h")
    output_lines.append("// Auto-generated embedded resource data")
    output_lines.append(f"// Online image size: {online_size} bytes")
    output_lines.append(f"// Friend request image size: {friend_request_size} bytes")
    output_lines.append(f"// Discord image size: {discord_size} bytes")
    output_lines.append(f"// GitHub image size: {github_size} bytes")
    output_lines.append(f"// Heart image size: {heart_size} bytes")
    output_lines.append(f"// Sandy icon size: {sandy_icon_size} bytes")
    output_lines.append(f"// Bastok icon size: {bastok_icon_size} bytes")
    output_lines.append(f"// Windurst icon size: {windy_icon_size} bytes")
    output_lines.append(f"// Lock icon size: {lock_icon_size} bytes")
    output_lines.append(f"// Unlock icon size: {unlock_icon_size} bytes")
    if has_online_sound:
        output_lines.append(f"// Online sound size: {online_sound_size} bytes")
    if has_friend_request_sound:
        output_lines.append(f"// Friend request sound size: {friend_request_sound_size} bytes")
    output_lines.append("")
    output_lines.append("#ifndef EMBEDDED_RESOURCES_H")
    output_lines.append("#define EMBEDDED_RESOURCES_H")
    output_lines.append("")
    output_lines.append("// Image resources")
    output_lines.append("static const unsigned char g_OnlineImageData[] = {")
    
    # Convert online.png bytes to hex array
    # Format: "    0xXX,\n    0xYY,\n    ..." (similar to PowerShell version)
    online_hex_parts = [f"0x{b:02X}" for b in online_bytes]
    # Join with comma and newline, then add indentation
    online_hex_str = ",\n    ".join(online_hex_parts)
    output_lines.append(f"    {online_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_OnlineImageData_size = {online_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_FriendRequestImageData[] = {")
    
    # Convert friend_request.png bytes to hex array
    friend_request_hex_parts = [f"0x{b:02X}" for b in friend_request_bytes]
    friend_request_hex_str = ",\n    ".join(friend_request_hex_parts)
    output_lines.append(f"    {friend_request_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_FriendRequestImageData_size = {friend_request_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_DiscordImageData[] = {")
    discord_hex_parts = [f"0x{b:02X}" for b in discord_bytes]
    discord_hex_str = ",\n    ".join(discord_hex_parts)
    output_lines.append(f"    {discord_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_DiscordImageData_size = {discord_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_GitHubImageData[] = {")
    github_hex_parts = [f"0x{b:02X}" for b in github_bytes]
    github_hex_str = ",\n    ".join(github_hex_parts)
    output_lines.append(f"    {github_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_GitHubImageData_size = {github_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_HeartImageData[] = {")
    heart_hex_parts = [f"0x{b:02X}" for b in heart_bytes]
    heart_hex_str = ",\n    ".join(heart_hex_parts)
    output_lines.append(f"    {heart_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_HeartImageData_size = {heart_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_SandyIconImageData[] = {")
    sandy_icon_hex_parts = [f"0x{b:02X}" for b in sandy_icon_bytes]
    sandy_icon_hex_str = ",\n    ".join(sandy_icon_hex_parts)
    output_lines.append(f"    {sandy_icon_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_SandyIconImageData_size = {sandy_icon_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_BastokIconImageData[] = {")
    bastok_icon_hex_parts = [f"0x{b:02X}" for b in bastok_icon_bytes]
    bastok_icon_hex_str = ",\n    ".join(bastok_icon_hex_parts)
    output_lines.append(f"    {bastok_icon_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_BastokIconImageData_size = {bastok_icon_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_WindurstIconImageData[] = {")
    windy_icon_hex_parts = [f"0x{b:02X}" for b in windy_icon_bytes]
    windy_icon_hex_str = ",\n    ".join(windy_icon_hex_parts)
    output_lines.append(f"    {windy_icon_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_WindurstIconImageData_size = {windy_icon_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_LockIconImageData[] = {")
    lock_icon_hex_parts = [f"0x{b:02X}" for b in lock_icon_bytes]
    lock_icon_hex_str = ",\n    ".join(lock_icon_hex_parts)
    output_lines.append(f"    {lock_icon_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_LockIconImageData_size = {lock_icon_size};")
    output_lines.append("")
    output_lines.append("static const unsigned char g_UnlockIconImageData[] = {")
    unlock_icon_hex_parts = [f"0x{b:02X}" for b in unlock_icon_bytes]
    unlock_icon_hex_str = ",\n    ".join(unlock_icon_hex_parts)
    output_lines.append(f"    {unlock_icon_hex_str}")
    output_lines.append("};")
    output_lines.append("")
    output_lines.append(f"static const unsigned int g_UnlockIconImageData_size = {unlock_icon_size};")
    output_lines.append("")
    
    # Add sound resources if they exist
    if has_online_sound:
        output_lines.append("// Sound resources")
        output_lines.append("static const unsigned char g_OnlineSoundData[] = {")
        online_sound_hex_parts = [f"0x{b:02X}" for b in online_sound_bytes]
        online_sound_hex_str = ",\n    ".join(online_sound_hex_parts)
        output_lines.append(f"    {online_sound_hex_str}")
        output_lines.append("};")
        output_lines.append("")
        output_lines.append(f"static const unsigned int g_OnlineSoundData_size = {online_sound_size};")
        output_lines.append("")
    
    if has_friend_request_sound:
        output_lines.append("static const unsigned char g_FriendRequestSoundData[] = {")
        friend_request_sound_hex_parts = [f"0x{b:02X}" for b in friend_request_sound_bytes]
        friend_request_sound_hex_str = ",\n    ".join(friend_request_sound_hex_parts)
        output_lines.append(f"    {friend_request_sound_hex_str}")
        output_lines.append("};")
        output_lines.append("")
        output_lines.append(f"static const unsigned int g_FriendRequestSoundData_size = {friend_request_sound_size};")
        output_lines.append("")
    
    output_lines.append("#endif // EMBEDDED_RESOURCES_H")
    output_lines.append("")
    
    # Write output file
    output.write_text("\n".join(output_lines))
    
    resource_list = f"online image ({online_size} bytes), friend request image ({friend_request_size} bytes), discord image ({discord_size} bytes), github image ({github_size} bytes), heart image ({heart_size} bytes), sandy icon ({sandy_icon_size} bytes), bastok icon ({bastok_icon_size} bytes), windurst icon ({windy_icon_size} bytes), lock icon ({lock_icon_size} bytes), unlock icon ({unlock_icon_size} bytes)"
    if has_online_sound:
        resource_list += f", online sound ({online_sound_size} bytes)"
    if has_friend_request_sound:
        resource_list += f", friend request sound ({friend_request_sound_size} bytes)"
    
    print(f"Generated {output} with {resource_list}")

if __name__ == "__main__":
    main()

