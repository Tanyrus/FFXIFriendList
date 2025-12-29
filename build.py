#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional


# =========================
# Defaults
# =========================
DEFAULT_GENERATOR = "Ninja"
DEFAULT_CONFIG = "Release"
DEFAULT_JOBS = 12
DEFAULT_DEPLOY = True

DEFAULT_ASHITA_SDK_DIR = Path(r"C:\HorizonXI\HorizonXI\Game\plugins\sdk")
DEFAULT_DEPLOY_DIR = Path(r"C:\HorizonXI\HorizonXI\Game\plugins")


# =========================
# Color helpers
# =========================
class C:
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    CYAN = "\033[0;36m"
    NC = "\033[0m"


def colorize(s: str, c: str) -> str:
    return f"{c}{s}{C.NC}"


def run(cmd: List[str], *, cwd: Path, env: Optional[Dict[str, str]] = None, check: bool = True) -> subprocess.CompletedProcess:
    print(f"\n> {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=str(cwd), env=env, check=check)


def which(exe: str) -> Optional[str]:
    return shutil.which(exe)


def md5_file(path: Path) -> str:
    h = hashlib.md5()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def stop_mspdbsrv() -> None:
    subprocess.run(["cmd.exe", "/c", "taskkill /IM mspdbsrv.exe /F >nul 2>&1"], check=False)
    time.sleep(0.5)


def _rmtree_onerror(func, path, exc_info):
    try:
        os.chmod(path, 0o666)
    except Exception:
        pass
    try:
        func(path)
    except Exception:
        pass


def delete_dir_with_retries(path: Path, retries: int = 3) -> None:
    if not path.exists():
        return

    for attempt in range(1, retries + 1):
        try:
            stop_mspdbsrv()

            # Best effort: clear attrs on the whole tree first
            for root, dirs, files in os.walk(path, topdown=False):
                for name in files:
                    fp = Path(root) / name
                    try:
                        os.chmod(fp, 0o666)
                    except Exception:
                        pass
                for name in dirs:
                    dp = Path(root) / name
                    try:
                        os.chmod(dp, 0o777)
                    except Exception:
                        pass

            shutil.rmtree(path, onerror=_rmtree_onerror)
            return
        except Exception as e:
            if attempt == retries:
                raise
            print(f"[RETRY {attempt}/{retries}] Failed to delete {path}: {e}")
            time.sleep(0.75 * attempt)


def find_vswhere() -> Optional[Path]:
    p = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    return p if p.exists() else None


def vswhere_install_path() -> Optional[Path]:
    vswhere = find_vswhere()
    if not vswhere:
        return None
    try:
        out = subprocess.check_output(
            [
                str(vswhere),
                "-latest",
                "-products",
                "*",
                "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property",
                "installationPath",
            ],
            text=True,
        ).strip()
        if not out:
            return None
        p = Path(out)
        return p if p.exists() else None
    except Exception:
        return None


def find_vsdevcmd() -> Optional[Path]:
    install = vswhere_install_path()
    if install:
        p = install / "Common7" / "Tools" / "VsDevCmd.bat"
        if p.exists():
            return p

    candidates = [
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
    ]
    for c in candidates:
        p = Path(c)
        if p.exists():
            return p
    return None


def capture_msvc_env_x86() -> Dict[str, str]:
    """
    Bootstraps an MSVC x86 environment using VsDevCmd.bat, then returns env vars.
    """
    vsdevcmd = find_vsdevcmd()
    if not vsdevcmd:
        raise RuntimeError("Could not find VsDevCmd.bat. Install VS 2022 Build Tools (Desktop development with C++).")

    # IMPORTANT: only quote the bat path; no outer quoting that Git Bash mangles.
    cmd_str = f'cmd.exe /d /s /c call "{vsdevcmd}" -arch=x86 -host_arch=x64 >nul && set'
    proc = subprocess.run(cmd_str, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    if proc.returncode != 0:
        print(colorize("\n[ERROR] Failed to initialize MSVC environment via VsDevCmd.bat", C.RED))
        print(f"  VsDevCmd: {vsdevcmd}")
        print("  cmd.exe output:")
        print("------------------------------------------------------------")
        print(proc.stdout)
        print("------------------------------------------------------------")
        raise RuntimeError(f"VsDevCmd.bat failed with exit code {proc.returncode}")

    env = dict(os.environ)
    for line in proc.stdout.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            if re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", k):
                env[k] = v
    return env


def ensure_tools(generator: str) -> None:
    if not which("cmake"):
        raise RuntimeError("cmake not found on PATH. Install CMake and check 'Add CMake to PATH'.")
    if generator.lower() == "ninja" and not which("ninja"):
        raise RuntimeError("ninja not found on PATH. Install Ninja or use --generator VisualStudio.")
    if not which("ctest"):
        # ctest usually comes with CMake, but keep this explicit.
        raise RuntimeError("ctest not found on PATH. Ensure CMake is installed and on PATH.")


def detect_tests(build_dir: Path, generator: str, no_tests: bool, build_tests_flag: bool) -> bool:
    if no_tests:
        return False
    if build_tests_flag:
        return True

    if generator.lower() == "ninja":
        bn = build_dir / "build.ninja"
        if bn.exists():
            try:
                return "FFXIFriendList_tests" in bn.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                return False
        return False
        return (build_dir / "FFXIFriendList_tests.vcxproj").exists()


def run_ctest_quiet(*, root: Path, build_dir_name: str, config: str, env: Dict[str, str]) -> tuple[int, str]:
    """
    Run ctest but only print a compact summary.
    Returns (rc, raw_output).
    """
    # -C is harmless for Ninja and needed for VS multi-config
    cmd = ["ctest", "--test-dir", build_dir_name, "-C", config]
    proc = subprocess.run(cmd, cwd=str(root), env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout or ""
    rc = proc.returncode

    # Preferred summary line in your output looks like:
    # "100% tests passed, 0 tests failed out of 320"
    m = re.search(r"(\d+)%\s+tests\s+passed,\s+(\d+)\s+tests\s+failed\s+out\s+of\s+(\d+)", out, re.IGNORECASE)
    if m:
        pct, failed, total = m.group(1), int(m.group(2)), m.group(3)
        if failed == 0 and rc == 0:
            print(colorize(f"[CTEST] {pct}% tests passed (0 failed out of {total})", C.GREEN))
        else:
            print(colorize(f"[CTEST] {pct}% tests passed ({failed} failed out of {total})", C.RED))
    else:
        # Fallback: just show that it finished
        if rc == 0:
            print(colorize("[CTEST] All tests passed.", C.GREEN))
        else:
            print(colorize("[CTEST] Some tests failed.", C.RED))

    # If failures, print only the failure list section (no per-test spam)
    if rc != 0:
        idx = out.lower().find("the following tests failed")
        if idx != -1:
            print(colorize("\nFailed tests:", C.RED))
            # Print from that section onwards, but stop before any huge logs if present
            failed_section = out[idx:].strip()
            # Keep it readable (usually short)
            print(failed_section)
        else:
            # If ctest didn't emit the standard section, show last ~200 lines for debugging
            tail = "\n".join(out.splitlines()[-200:])
            print(colorize("\n[CTEST] Output tail:", C.YELLOW))
            print(tail)

    return rc, out


def main() -> int:
    ap = argparse.ArgumentParser()

    ap.add_argument("--generator", choices=["VisualStudio", "Ninja"], default=DEFAULT_GENERATOR)
    ap.add_argument("--config", choices=["Debug", "Release"], default=DEFAULT_CONFIG)
    ap.add_argument("--jobs", type=int, default=DEFAULT_JOBS)

    ap.add_argument("--clean", action="store_true")

    # default deploy ON, allow --no-deploy
    ap.add_argument("--deploy", action=argparse.BooleanOptionalAction, default=DEFAULT_DEPLOY)

    ap.add_argument("--no-tests", action="store_true")
    ap.add_argument("--build-tests", action="store_true")
    ap.add_argument("--reconfigure", action="store_true")

    args = ap.parse_args()

    root = Path(__file__).resolve().parent
    os.chdir(root)

    ashita_sdk_dir = DEFAULT_ASHITA_SDK_DIR
    if not ashita_sdk_dir.exists() or not (ashita_sdk_dir / "Ashita.h").exists():
        print(colorize("[ERROR] Ashita SDK not found at hardcoded path:", C.RED))
        print(f"        {ashita_sdk_dir}")
        print("        Expected to find Ashita.h in that directory.")
        return 1

    build_dir = root / "build"
    cmake_cache = build_dir / "CMakeCache.txt"
    dll_name = "FFXIFriendList.dll"

    deploy_dir = DEFAULT_DEPLOY_DIR
    configure_ran = False
    build_compiled = False
    deploy_copied = False
    deploy_pending = False

    try:
        ensure_tools(args.generator)
    except Exception as e:
        print(colorize(f"[ERROR] {e}", C.RED))
        return 1

    if args.clean:
        if build_dir.exists():
            print(f"Cleaning build directory: {build_dir}")
            delete_dir_with_retries(build_dir, retries=3)
            print(colorize("  [OK] Build directory cleaned", C.GREEN))
        else:
            print("Build directory does not exist, nothing to clean.")

    try:
        msvc_env = capture_msvc_env_x86()
    except Exception as e:
        print(colorize(f"[ERROR] {e}", C.RED))
        return 1

    # Configure
    needs_configure = args.reconfigure or not cmake_cache.exists()
    
    # Check if cached build type matches requested config (for Ninja generator)
    if not needs_configure and args.generator == "Ninja" and cmake_cache.exists():
        try:
            cache_content = cmake_cache.read_text(encoding="utf-8", errors="ignore")
            # Look for CMAKE_BUILD_TYPE in cache
            for line in cache_content.splitlines():
                if line.startswith("CMAKE_BUILD_TYPE:STRING="):
                    cached_type = line.split("=", 1)[1].strip()
                    if cached_type != args.config:
                        print(colorize(f"[WARNING] Cached build type is '{cached_type}' but requested '{args.config}'", C.YELLOW))
                        print(colorize("         Configuration mismatch detected - reconfigure required", C.YELLOW))
                        if not args.reconfigure:
                            print(colorize("         Use --reconfigure or --clean to fix this", C.YELLOW))
                            print(colorize("         Auto-reconfiguring now...", C.YELLOW))
                        needs_configure = True
                        break
        except Exception as e:
            # If we can't read the cache, just reconfigure
            print(colorize(f"[WARNING] Could not read CMake cache: {e}", C.YELLOW))
            print(colorize("         Reconfiguring to be safe...", C.YELLOW))
            needs_configure = True
    
    if needs_configure:
        build_dir.mkdir(parents=True, exist_ok=True)
        configure_ran = True

        if args.generator == "Ninja":
            print("Configuring (Ninja x86)...")
            run(
                [
                    "cmake",
                    "-S",
                    ".",
                    "-B",
                    "build",
                    "-G",
                    "Ninja",
                    f"-DCMAKE_BUILD_TYPE={args.config}",
                    f"-DASHITA_SDK_DIR={ashita_sdk_dir.as_posix()}",
                ],
                cwd=root,
                env=msvc_env,
            )
        else:
            print("Configuring (Visual Studio 2022 Win32)...")
            run(
                ["cmake", "-S", ".", "-B", "build", "-G", "Visual Studio 17 2022", "-A", "Win32", f"-DASHITA_SDK_DIR={ashita_sdk_dir.as_posix()}"],
                cwd=root,
                env=msvc_env,
            )
    else:
        # Verify the cached configuration matches what we're building
        if args.generator == "Ninja":
            try:
                cache_content = cmake_cache.read_text(encoding="utf-8", errors="ignore")
                for line in cache_content.splitlines():
                    if line.startswith("CMAKE_BUILD_TYPE:STRING="):
                        cached_type = line.split("=", 1)[1].strip()
                        if cached_type == args.config:
                            print(f"CMake cache exists with matching build type ({args.config}), skipping configure")
                        else:
                            print(colorize(f"[WARNING] Cache has '{cached_type}' but building '{args.config}' - this may cause issues", C.YELLOW))
                            print(colorize("         Use --reconfigure to fix", C.YELLOW))
                        break
            except Exception:
                print("CMake cache exists, skipping configure (use --reconfigure to force)")
        else:
            # Visual Studio is multi-config, so --config is used at build time
            print("CMake cache exists, skipping configure (use --reconfigure to force)")
            print(f"Note: Building {args.config} configuration (Visual Studio supports multiple configs)")

    # Embedded resources
    # Note: Embedded resources are now generated automatically by CMake during build
    # This section is kept for manual generation if needed, but CMake will handle it
    # embedded_py = root / "scripts" / "generate_embedded.py"
    # if embedded_py.exists():
    #     print("Generating embedded resources...")
    #     subprocess.run(
    #         [sys.executable, str(embedded_py)],
    #         cwd=str(root),
    #         check=False,
    #     )
    # else:
    #     print(colorize(f"  [WARNING] Embedded resources script not found: {embedded_py}", C.YELLOW))

    # Build
    print(f"Building ({args.config})...")
    targets = ["FFXIFriendList"]
    if detect_tests(build_dir, args.generator, args.no_tests, args.build_tests):
        print("  Also building test target...")
        targets.append("FFXIFriendList_tests")

    for t in targets:
        cmd = ["cmake", "--build", "build", "--target", t]
        if args.generator == "VisualStudio":
            cmd += ["--config", args.config]
        if args.jobs > 0:
            cmd += ["--parallel", str(args.jobs)]

        print(f"  Building target: {t}")
        proc = subprocess.run(cmd, cwd=str(root), env=msvc_env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        sys.stdout.write(proc.stdout or "")

        if proc.returncode != 0:
            print(colorize(f"[ERROR] Build failed for target: {t}", C.RED))
            return proc.returncode

        if re.search(r"(Compiling|Linking|Building|Rebuilding|\bcl\.exe\b|\blink\.exe\b|\[[0-9]+/[0-9]+\])", proc.stdout or "", re.IGNORECASE):
            build_compiled = True

    print(colorize("  [OK] Build completed (compiled/linked)", C.GREEN) if build_compiled else "  [OK] Build completed (up to date)")

    # DLL path
    if args.generator == "Ninja":
        dll_path = build_dir / "bin" / dll_name
    else:
        dll_path = build_dir / "bin" / args.config / dll_name

    if dll_path.exists():
        print("Copying DLL to repo root...")
        shutil.copy2(dll_path, root / dll_name)
        print(colorize(f"  [OK] DLL copied to: {root / dll_name}", C.GREEN))
    else:
        print(colorize(f"  [WARNING] DLL not found at expected path: {dll_path}", C.YELLOW))

    # Tests (quiet summary only)
    tests_ran = False
    tests_ok = None  # None=skipped, True=pass, False=fail
    if not args.no_tests:
        test_exe = (build_dir / "bin" / "FFXIFriendList_tests.exe") if args.generator == "Ninja" else (build_dir / "bin" / args.config / "FFXIFriendList_tests.exe")
        if test_exe.exists():
            tests_ran = True
            print(f"Running tests ({args.config})...")
            rc, _ = run_ctest_quiet(root=root, build_dir_name="build", config=args.config, env=msvc_env)
            tests_ok = (rc == 0)
            # If tests fail, treat as failure exit (keeps CI honest)
            if rc != 0:
                return rc
        else:
            print(colorize("Test executable not found, skipping tests (use --build-tests to build tests)", C.YELLOW))
    else:
        print("Skipping tests (--no-tests)")

    # Deploy
    if args.deploy:
        if not dll_path.exists():
            print(colorize(f"[ERROR] DLL not found: {dll_path}", C.RED))
            return 1

        if not deploy_dir.exists():
            print(colorize(f"[ERROR] Deploy directory does not exist: {deploy_dir}", C.RED))
            return 1

        deploy_path = deploy_dir / dll_name

        needs_copy = True
        if deploy_path.exists():
            if md5_file(dll_path) == md5_file(deploy_path):
                needs_copy = False
                print("DLL unchanged (hash matches), skipping deploy")
            else:
                print("DLL changed (hash differs), deploying...")
        else:
            print("DLL not found at destination, deploying...")

        if needs_copy:
            copied = False
            last_err = None
            for _ in range(1, 41):  # ~10s total at 250ms intervals
                try:
                    shutil.copy2(dll_path, deploy_path)
                    copied = True
                    break
                except PermissionError as e:
                    last_err = e
                    time.sleep(0.25)

            if copied:
                deploy_copied = True
                print(colorize("  [OK] Deployed successfully", C.GREEN))
            else:
                pending_path = deploy_dir / f"{dll_name}.pending"
                try:
                    shutil.copy2(dll_path, pending_path)
                    deploy_pending = True
                    print(colorize(f"  [WARNING] Deploy skipped (DLL in use). Wrote pending file: {pending_path}", C.YELLOW))
                    print("            Unload the plugin / close the game, then replace the DLL with the pending file.")
                except Exception as e:
                    print(colorize(f"  [WARNING] Deploy skipped (DLL in use) and could not write pending file: {e}", C.YELLOW))
                if last_err is not None:
                    print(colorize(f"            Last deploy error: {last_err}", C.YELLOW))

    # Summary
    print("\n========================================")
    print("Build Summary")
    print("========================================")
    print(f"  Configure: {'Ran' if configure_ran else 'Skipped (cache exists)'}")
    print(f"  Build:     {'Compiled files' if build_compiled else 'Up to date (no changes)'}")

    if tests_ran:
        if tests_ok:
            print(f"  Tests:     {colorize('PASS', C.GREEN)}")
        else:
            print(f"  Tests:     {colorize('FAIL', C.RED)}")
    else:
        print("  Tests:     Skipped")

    if args.deploy:
        if deploy_copied:
            print(f"  Deploy:    {colorize('DEPLOYED', C.GREEN)}")
        elif deploy_pending:
            print(f"  Deploy:    {colorize('PENDING (DLL in use)', C.YELLOW)}")
        else:
            print("  Deploy:    Skipped (unchanged)")
    else:
        print("  Deploy:    Skipped (--no-deploy)")
    print("========================================")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
