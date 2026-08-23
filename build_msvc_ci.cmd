@echo off
setlocal
where cmake >nul 2>nul || exit /b 1
where cl >nul 2>nul || exit /b 1
cmake --preset msvc-x64-ci || exit /b 1
cmake --build --preset build-msvc-x64-ci || exit /b 1
echo STRICT MSVC BUILD OK
