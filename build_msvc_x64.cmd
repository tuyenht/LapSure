@echo off
setlocal
where cmake >nul 2>nul || (echo ERROR: cmake not found & exit /b 1)
where cl >nul 2>nul || (
  echo ERROR: MSVC environment is not initialized.
  echo Open "x64 Native Tools Command Prompt for VS 2022".
  exit /b 1
)
cmake --preset msvc-x64-release || exit /b 1
cmake --build --preset build-msvc-x64-release || exit /b 1
echo BUILD OK: out\build\msvc-x64-release\Release\LapSure.exe
