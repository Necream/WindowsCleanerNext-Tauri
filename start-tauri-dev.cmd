@echo off
setlocal

set "ROOT=%~dp0"
if not exist "%ROOT%tmp" mkdir "%ROOT%tmp"

start "WindowsCleanerNext-Tauri Dev" /D "%ROOT%" cmd /k "npm run tauri -- dev 1>tmp\tauri-dev.stdout.log 2>tmp\tauri-dev.stderr.log"
