#!/usr/bin/env python3
"""
Script to convert MP3 sound files to WAV format
Requires ffmpeg to be installed and in PATH
"""

import os
import sys
import subprocess
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    sounds_dir = project_root / "assets" / "sounds"
    
    # Check for ffmpeg
    try:
        subprocess.run(["ffmpeg", "-version"], 
                      capture_output=True, 
                      check=True)
        ffmpeg_available = True
    except (subprocess.CalledProcessError, FileNotFoundError):
        ffmpeg_available = False
    
    # Convert online.mp3 to online.wav
    online_mp3 = sounds_dir / "online.mp3"
    online_wav = sounds_dir / "online.wav"
    
    if online_wav.exists():
        print("  [OK] online.wav already exists, skipping conversion")
    elif online_mp3.exists():
        if not ffmpeg_available:
            print("WARNING: ffmpeg not found in PATH. Skipping online.mp3 conversion.")
            print(f"  WAV file must exist at: {online_wav}")
        else:
            print("Converting online.mp3 to online.wav...")
            result = subprocess.run(
                ["ffmpeg", "-i", str(online_mp3), 
                 "-acodec", "pcm_u8", "-ar", "22050", "-ac", "1", "-y", str(online_wav)],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print("  [OK] Converted online.mp3 to online.wav")
            else:
                print(f"  [ERROR] Failed to convert online.mp3: {result.stderr}")
                sys.exit(1)
    else:
        print(f"WARNING: online.mp3 not found at {online_mp3}")
    
    # Convert friend-request.mp3 to friend-request.wav
    friend_request_mp3 = sounds_dir / "friend-request.mp3"
    friend_request_wav = sounds_dir / "friend-request.wav"
    
    if friend_request_wav.exists():
        print("  [OK] friend-request.wav already exists, skipping conversion")
    elif friend_request_mp3.exists():
        if not ffmpeg_available:
            print("WARNING: ffmpeg not found in PATH. Skipping friend-request.mp3 conversion.")
            print(f"  WAV file must exist at: {friend_request_wav}")
        else:
            print("Converting friend-request.mp3 to friend-request.wav...")
            result = subprocess.run(
                ["ffmpeg", "-i", str(friend_request_mp3),
                 "-acodec", "pcm_u8", "-ar", "22050", "-ac", "1", "-y", str(friend_request_wav)],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print("  [OK] Converted friend-request.mp3 to friend-request.wav")
            else:
                print(f"  [ERROR] Failed to convert friend-request.mp3: {result.stderr}")
                sys.exit(1)
    else:
        print(f"WARNING: friend-request.mp3 not found at {friend_request_mp3}")
    
    print("\nConversion complete!")
    print("WAV files are ready for embedding.")

if __name__ == "__main__":
    main()

