@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          LapSure - Native Win32 Release Build
echo =======================================================
echo.

:: 1. Check if cl.exe and cmake are already in PATH
where cl >nul 2>nul
if %errorlevel% equ 0 goto :CHECK_CMAKE

:: 2. Auto-detect Visual Studio 2022 / 2019 / BuildTools vcvars64.bat
set "VCVARS="
for %%p in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist %%p (
        set "VCVARS=%%~p"
        goto :FOUND_VCVARS
    )
)

:FOUND_VCVARS
if defined VCVARS (
    echo [INFO] Dang nap moi truong MSVC x64 tu:
    echo        !VCVARS!
    call "!VCVARS!" >nul 2>nul
) else (
    echo [CANH BAO] Khong tim thay Visual Studio C++ (vcvars64.bat) mac dinh.
)

:CHECK_CMAKE
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo.
    echo [LOI] Chua cai dat CMake hoac CMake chua duoc them vao PATH.
    echo Vui long cai dat CMake tai: https://cmake.org/download/
    if exist "bin\LapSure.exe" (
        echo.
        echo [GOI Y] Ban da co file thuc thi build san tai: bin\LapSure.exe
        echo        Co the mo truc tiep file do de su dung ngay!
    )
    echo.
    pause
    exit /b 1
)

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

:: Copy to root and bin
if exist build\Release\LapSure.exe (
    copy /Y build\Release\LapSure.exe .\LapSure.exe >nul
    if not exist bin mkdir bin
    copy /Y build\Release\LapSure.exe bin\LapSure.exe >nul
    echo.
    echo =======================================================
    echo [THANH CONG] Da bien dich xong LapSure.exe!
    echo Vi tri file: %CD%\LapSure.exe
    echo =======================================================
)

echo.
pause
