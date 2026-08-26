@echo off
setlocal enabledelayedexpansion

if "%~2"=="" (
    echo Usage: sync_release.cmd ^<release-tag^> ^<expected-sha256^>
    echo Example: sync_release.cmd v0.1.1-beta 0123456789abcdef...
    echo.
    echo This is NOT a source build. It downloads one explicit GitHub release artifact,
    echo verifies its SHA-256, then installs the verified executable locally.
    exit /b 2
)

set "RELEASE_TAG=%~1"
set "EXPECTED_SHA256=%~2"
set "ARCHIVE=out\release-sync\LapSure-windows-x64-portable.zip"
set "EXTRACTED=out\release-sync\extracted"

where gh >nul 2>nul
if errorlevel 1 (
    echo [ERROR] GitHub CLI ^(gh^) was not found.
    exit /b 1
)
where powershell >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Windows PowerShell was not found.
    exit /b 1
)

powershell -NoProfile -NonInteractive -Command "$h='%EXPECTED_SHA256%'; if($h -notmatch '^[0-9A-Fa-f]{64}$'){exit 1}"
if errorlevel 1 (
    echo [ERROR] expected-sha256 must contain exactly 64 hexadecimal characters.
    exit /b 2
)

if exist "out\release-sync" rmdir /S /Q "out\release-sync"
mkdir "out\release-sync" >nul 2>nul

echo [INFO] Downloading explicit release tag %RELEASE_TAG% ...
gh release download "%RELEASE_TAG%" --repo tuyenht/LapSure --pattern "LapSure-windows-x64-portable.zip" --dir "out\release-sync" --clobber
if errorlevel 1 (
    echo [ERROR] Release download failed. No executable was installed.
    exit /b 1
)
if not exist "%ARCHIVE%" (
    echo [ERROR] Expected archive was not downloaded.
    exit /b 1
)

for /f "usebackq delims=" %%H in (`powershell -NoProfile -NonInteractive -Command "(Get-FileHash -LiteralPath '%ARCHIVE%' -Algorithm SHA256).Hash"`) do set "ACTUAL_SHA256=%%H"
if /I not "!ACTUAL_SHA256!"=="%EXPECTED_SHA256%" (
    echo [ERROR] SHA-256 mismatch. No executable was installed.
    echo Expected: %EXPECTED_SHA256%
    echo Actual:   !ACTUAL_SHA256!
    del /Q "%ARCHIVE%" >nul 2>nul
    exit /b 1
)

echo [INFO] SHA-256 verified. Extracting release artifact...
powershell -NoProfile -NonInteractive -Command "Expand-Archive -LiteralPath '%ARCHIVE%' -DestinationPath '%EXTRACTED%' -Force"
if errorlevel 1 (
    echo [ERROR] Verified archive could not be extracted.
    exit /b 1
)
if not exist "%EXTRACTED%\LapSure.exe" (
    echo [ERROR] Verified archive does not contain LapSure.exe at the expected path.
    exit /b 1
)

if not exist bin mkdir bin
copy /Y "%EXTRACTED%\LapSure.exe" "bin\LapSure.exe" >nul
copy /Y "%EXTRACTED%\LapSure.exe" ".\LapSure.exe" >nul

echo [SUCCESS] Verified release %RELEASE_TAG% installed locally.
echo Archive SHA-256: !ACTUAL_SHA256!
exit /b 0
