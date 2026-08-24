@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
if exist "%SCRIPT_DIR%build\Release\LapSure.exe" (
    set "ROOT_DIR=%SCRIPT_DIR%"
) else if exist "%SCRIPT_DIR%..\build\Release\LapSure.exe" (
    set "ROOT_DIR=%SCRIPT_DIR%..\"
) else (
    set "ROOT_DIR=%SCRIPT_DIR%"
)

echo Stopping old LapSure if running...
taskkill /F /IM LapSure.exe >nul 2>&1
ping 127.0.0.1 -n 2 >nul

echo Updating binaries from build\Release to bin\LapSure.exe...
if exist "%ROOT_DIR%build\Release\LapSure.exe" (
    copy /Y "%ROOT_DIR%build\Release\LapSure.exe" "%ROOT_DIR%bin\LapSure.exe" >nul
    copy /Y "%ROOT_DIR%build\Release\LapSure.exe" "%ROOT_DIR%LapSure.exe" >nul
)

echo Starting latest LapSure...
if exist "%ROOT_DIR%bin\LapSure.exe" (
    start "" "%ROOT_DIR%bin\LapSure.exe"
) else if exist "%ROOT_DIR%build\Release\LapSure.exe" (
    start "" "%ROOT_DIR%build\Release\LapSure.exe"
)
endlocal