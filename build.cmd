@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          LapSure - Native Win32 Build ^& Sync
echo =======================================================
echo.

:: 1. Check if cl.exe is in PATH
where cl >nul 2>nul
if %errorlevel% equ 0 goto :CHECK_CMAKE

:: 2. Auto-detect Visual Studio 2022 / 2019 / BuildTools
set "VCVARS="
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if defined VCVARS (
    echo [INFO] Nap moi truong MSVC x64 tu: !VCVARS!
    call "!VCVARS!" >nul 2>nul
)

:CHECK_CMAKE
where cmake >nul 2>nul
if %errorlevel% equ 0 goto :LOCAL_COMPILE

:: ------------------------------------------------------------------
:: If NO local MSVC/CMake compiler found: Fallback to GitHub Release Sync
:: ------------------------------------------------------------------
echo [THONG BAO] May hien tai chua cai dat CMake hoac Visual Studio C++.
echo [INFO] Dang tu dong cap nhat ban dung moi nhat tu GitHub Release...
echo.

where gh >nul 2>nul
if %errorlevel% equ 0 (
    if not exist "out\download" mkdir "out\download"
    gh release download --pattern "LapSure-windows-x64-portable.zip" --dir "out\download" --clobber
    if %errorlevel% equ 0 (
        echo [INFO] Dang giai nen va dong bo vao bin\LapSure.exe...
        powershell -NoProfile -Command "Expand-Archive -LiteralPath out\download\LapSure-windows-x64-portable.zip -DestinationPath bin -Force; Copy-Item bin\LapSure.exe .\LapSure.exe -Force"
        echo.
        echo =======================================================
        echo [THANH CONG] Da dong bo ban LapSure.exe moi nhat!
        echo Vi tri file: %CD%\bin\LapSure.exe
        echo =======================================================
        echo.
        pause
        exit /b 0
    )
)

echo [LOI] Khong the tai ban dung tu dong.
echo Vui long cai dat Visual Studio 2022 (Desktop C++) hoac CMake.
echo Hoac tai file zip truc tiep tai: https://github.com/tuyenht/LapSure/releases
echo.
pause
exit /b 1

:LOCAL_COMPILE
echo [INFO] Dang cau hinh CMake x64 Release...
if not exist build mkdir build
cmake -S . -B build -A x64
if %errorlevel% neq 0 (
    echo [LOI] Cau hinh CMake that bai.
    pause
    exit /b 1
)

echo.
echo [INFO] Dang bien dich LapSure.exe (Release)...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [LOI] Bien dich that bai.
    pause
    exit /b 1
)

if exist build\Release\LapSure.exe (
    copy /Y build\Release\LapSure.exe .\LapSure.exe >nul
    if not exist bin mkdir bin
    copy /Y build\Release\LapSure.exe bin\LapSure.exe >nul
    echo.
    echo =======================================================
    echo [THANH CONG] Da bien dich xong LapSure.exe!
    echo Vi tri file: %CD%\LapSure.exe va %CD%\bin\LapSure.exe
    echo =======================================================
)

echo.
pause
