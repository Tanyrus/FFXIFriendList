#!/usr/bin/env python3
"""
Deploy FFXIFriendList Lua addon to HorizonXI addons directory.

This script deploys the Lua addon to the target path:
C:\HorizonXI\HorizonXI\Game\addons\FFXIFriendList

Note: Use the main deploy.py in the repository root instead.
This script is kept for backwards compatibility.
"""

import subprocess
import sys
from pathlib import Path

def main():
    # Find and run the main deploy.py script from repo root
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent.parent
    main_deploy = repo_root / "deploy.py"
    
    if main_deploy.exists():
        subprocess.run([sys.executable, str(main_deploy)], cwd=str(repo_root))
    else:
        print(f"ERROR: Main deploy script not found at: {main_deploy}")
        sys.exit(1)

if __name__ == "__main__":
    main()

