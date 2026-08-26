@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          LapSure - Local Build Only
echo =======================================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found.
    echo Install CMake 3.25+ and Visual Studio 2022 Desktop C++ / Build Tools.
    echo This script never downloads or substitutes a prebuilt LapSure.exe.
    echo Use sync_release.cmd only when you explicitly want a verified release artifact.
    exit /b 1
)

where cl >nul 2>nul
if not errorlevel 1 goto :BUILD

set "VCVARS="
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if not defined VCVARS (
    echo [ERROR] Visual Studio 2022 C++ x64 toolchain was not found.
    echo No release binary was downloaded and no existing executable was replaced.
    exit /b 1
)

echo [INFO] Loading MSVC x64 environment from: !VCVARS!
call "!VCVARS!" >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Could not initialize the MSVC x64 environment.
    exit /b 1
)

:BUILD
echo [INFO] Configuring source with preset msvc-x64-release...
cmake --preset msvc-x64-release
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo [INFO] Building Release from the current source tree...
cmake --build --preset build-msvc-x64-release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

set "BUILT_EXE=out\build\msvc-x64-release\Release\LapSure.exe"
if not exist "%BUILT_EXE%" (
    echo [ERROR] Build command succeeded but %BUILT_EXE% was not produced.
    exit /b 1
)

copy /Y "%BUILT_EXE%" ".\LapSure.exe" >nul
if not exist bin mkdir bin
copy /Y "%BUILT_EXE%" "bin\LapSure.exe" >nul

echo.
echo =======================================================
echo [SUCCESS] LapSure.exe was built from this source tree.
echo Source output: %CD%\%BUILT_EXE%
echo Convenience copies: %CD%\LapSure.exe and %CD%\bin\LapSure.exe
echo =======================================================
exit /b 0
