#!/usr/bin/env python3
"""
Deploy the FFXIFriendList Lua addon into an Ashita installation.

The addon is copied to:   <ashita-root>/addons/FFXIFriendList/
User data is left alone:  <ashita-root>/config/addons/FFXIFriendList/

The "Ashita root" is the folder that CONTAINS addons/, scripts/ and config/
(it also holds Ashita.dll / Ashita-cli.exe). On a Linux/Wine HorizonXI install
that is typically:

    ~/Games/HorizonXI/HorizonXI/Game

On Windows it is typically:

    C:\\HorizonXI\\HorizonXI\\Game   (or  C:\\HorizonXI\\Game )

Resolution order for the root (first hit wins):
    1. --root / --target on the command line
    2. ASHITA_ROOT environment variable
    3. Auto-detection from a list of common locations

Examples:
    python scripts/deploy_lua_addon.py                 # auto-detect, deploy
    python scripts/deploy_lua_addon.py --dry-run       # show what would happen
    python scripts/deploy_lua_addon.py --root "/path/to/Game"
    python scripts/deploy_lua_addon.py --load          # also add auto-load line
    ASHITA_ROOT="/path/to/Game" python scripts/deploy_lua_addon.py
"""

import argparse
import os
import shutil
import sys
from pathlib import Path

ADDON_NAME = "FFXIFriendList"       # folder name under addons/
ADDON_LOAD_NAME = "ffxifriendlist"  # name used by `/addon load ...`

# Names (relative to the repo root) that should NOT be shipped into the addon.
EXCLUDE = {
    "scripts",
    "tests",
    "release",
    ".git",
    ".github",
    ".gitignore",
    ".claude",
    "__pycache__",
    ".DS_Store",
    "Install-FFXIFriendList.bat",
}
EXCLUDE_SUFFIXES = (".pyc",)

# Things inside the TARGET addon folder that must survive a redeploy (in case a
# user keeps per-addon data alongside the code). The canonical user-data location
# is <root>/config/addons/FFXIFriendList, which lives outside the target and is
# never touched, but we guard these too for safety.
PRESERVE_IN_TARGET = {"config", "settings.lua"}


def candidate_roots():
    """Common Ashita roots to probe when not told explicitly. Order matters."""
    home = Path.home()
    return [
        home / "Games" / "HorizonXI" / "HorizonXI" / "Game",
        home / "Games" / "HorizonXI" / "Game",
        Path(r"C:\HorizonXI\HorizonXI\Game"),
        Path(r"C:\HorizonXI\Game"),
        Path(r"C:\Ashita4"),
    ]


def looks_like_ashita_root(path: Path) -> bool:
    """An Ashita root contains addons/ (and normally scripts/ + config/)."""
    return path.is_dir() and (path / "addons").is_dir()


def resolve_root(explicit):
    # 1) explicit CLI value
    if explicit:
        p = Path(explicit).expanduser()
        # Allow pointing at either the root or directly at .../addons
        if p.name.lower() == "addons" and looks_like_ashita_root(p.parent):
            return p.parent
        if looks_like_ashita_root(p):
            return p
        sys.exit(f"ERROR: '{p}' does not look like an Ashita root (no addons/ inside).")

    # 2) environment variable
    env = os.environ.get("ASHITA_ROOT")
    if env:
        p = Path(env).expanduser()
        if looks_like_ashita_root(p):
            return p
        sys.exit(f"ERROR: ASHITA_ROOT='{p}' does not look like an Ashita root.")

    # 3) auto-detect
    for cand in candidate_roots():
        if looks_like_ashita_root(cand):
            return cand

    tried = "\n  ".join(str(c) for c in candidate_roots())
    sys.exit(
        "ERROR: Could not auto-detect an Ashita root. Tried:\n  "
        + tried
        + "\nPass one explicitly with --root \"/path/to/Game\" or set ASHITA_ROOT."
    )


def should_exclude(path: Path) -> bool:
    if path.name in EXCLUDE:
        return True
    return any(path.name.endswith(suffix) for suffix in EXCLUDE_SUFFIXES)


def add_autoload_line(root: Path, dry_run: bool) -> None:
    """Ensure `/addon load ffxifriendlist` is in scripts/default.txt."""
    scripts_dir = root / "scripts"
    default_txt = scripts_dir / "default.txt"
    load_line = f"/addon load {ADDON_LOAD_NAME}"

    existing = ""
    if default_txt.exists():
        existing = default_txt.read_text(encoding="utf-8", errors="ignore")
        if ADDON_LOAD_NAME.lower() in existing.lower():
            print(f"  Auto-load: already present in {default_txt}")
            return

    if dry_run:
        print(f"  Auto-load: would append '{load_line}' to {default_txt}")
        return

    scripts_dir.mkdir(parents=True, exist_ok=True)
    sep = "" if (existing == "" or existing.endswith("\n")) else "\n"
    with default_txt.open("a", encoding="utf-8") as fh:
        fh.write(f"{sep}{load_line}\n")
    print(f"  Auto-load: appended '{load_line}' to {default_txt}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Deploy the FFXIFriendList Lua addon.")
    parser.add_argument(
        "--root", "--target", dest="root", default=None,
        help="Ashita root (folder containing addons/). Defaults to ASHITA_ROOT or auto-detect.",
    )
    parser.add_argument(
        "--load", action="store_true",
        help="Also add '/addon load ffxifriendlist' to scripts/default.txt.",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Show what would be copied without writing anything.",
    )
    args = parser.parse_args()

    addon_src = Path(__file__).resolve().parent.parent  # repo root
    if not (addon_src / "FFXIFriendList.lua").exists():
        sys.exit(f"ERROR: Could not find FFXIFriendList.lua in {addon_src}")

    root = resolve_root(args.root)
    target = root / "addons" / ADDON_NAME

    print(f"Source: {addon_src}")
    print(f"Ashita root: {root}")
    print(f"Target: {target}")
    if args.dry_run:
        print("(dry run - no files will be written)")

    # Clean existing files in the target, preserving any in-target user data.
    if target.exists():
        for item in sorted(target.iterdir()):
            if item.name in PRESERVE_IN_TARGET:
                print(f"  Preserving: {item.name}")
                continue
            if args.dry_run:
                print(f"  Would remove: {item.name}")
                continue
            if item.is_dir():
                shutil.rmtree(item)
            else:
                item.unlink()
    elif not args.dry_run:
        target.mkdir(parents=True, exist_ok=True)

    # Copy the runtime files.
    copied = 0
    for item in sorted(addon_src.iterdir()):
        if should_exclude(item):
            print(f"  Skipping: {item.name}")
            continue
        dest = target / item.name
        if args.dry_run:
            print(f"  Would copy: {item.name}{'/' if item.is_dir() else ''}")
            copied += 1
            continue
        if item.is_dir():
            shutil.copytree(item, dest, dirs_exist_ok=True)
            print(f"  Copied dir: {item.name}/")
        else:
            shutil.copy2(item, dest)
            print(f"  Copied: {item.name}")
        copied += 1

    if args.load:
        add_autoload_line(root, args.dry_run)

    verb = "Would deploy" if args.dry_run else "Deployed"
    print(f"\n{verb} {copied} items to {target}")
    if not args.dry_run:
        print(f"In-game: /addon load {ADDON_LOAD_NAME}")
    print("Done!")


if __name__ == "__main__":
    main()
