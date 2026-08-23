@echo off
setlocal
where cmake >nul 2>nul || (echo ERROR: CMake not found & exit /b 1)
if not exist build mkdir build
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
copy /Y build\Release\LapSure.exe .\LapSure.exe >nul
powershell -NoProfile -Command "$p='LapSure.exe'; Write-Host ('Built: '+(Resolve-Path $p)); Get-Item $p | Select Name,Length,LastWriteTime"
echo.
echo IMPORTANT: Release uses static MSVC CRT (/MT) for portable Windows/WinPE deployment.
