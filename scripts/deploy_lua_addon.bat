@echo off
REM Deploy FFXIFriendList Lua addon to HorizonXI addons directory.
REM This is a wrapper that calls the main deploy.py in the repository root.

cd /d "%~dp0..\.."
py deploy.py
pause

