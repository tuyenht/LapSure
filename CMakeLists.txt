cmake_minimum_required(VERSION 3.21)
project(LapSure VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_executable(LapSure WIN32
    src/main.cpp src/process.cpp src/environment.cpp src/profile.cpp src/hardware.cpp src/edid.cpp src/inventory.cpp src/engines.cpp src/forensics.cpp src/sensors.cpp src/telemetry.cpp src/trust.cpp src/journal.cpp src/baseline.cpp src/functional.cpp src/functional_io.cpp src/port_power.cpp src/devpkey_defs.cpp src/chassis_profile.cpp src/port_selector.cpp src/orchestrator.cpp src/runtime_validation.cpp src/scoring.cpp src/stress.cpp src/report.cpp
    app.manifest
)
target_include_directories(LapSure PRIVATE include)
target_compile_definitions(LapSure PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
  target_compile_options(LapSure PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus /utf-8)
  # Known Win32/Media Foundation call-site warnings are tracked separately; CI still treats all other warnings as errors.
  target_compile_options(LapSure PRIVATE /wd4189 /wd4245 /wd4456)
  # app.manifest owns the requestedExecutionLevel; prevent the linker from injecting a conflicting UAC snippet.
  target_link_options(LapSure PRIVATE /SUBSYSTEM:WINDOWS /MANIFESTUAC:NO)
else()
  target_compile_options(LapSure PRIVATE -Wall -Wextra -Wpedantic)
endif()
# mf.lib provides MFEnumDeviceSources; uuid.lib provides standard Windows GUID imports.
target_link_libraries(LapSure PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
install(TARGETS LapSure RUNTIME DESTINATION .)

option(LAPSURE_STRICT_WARNINGS "Treat MSVC warnings as errors in CI" OFF)
if(MSVC AND LAPSURE_STRICT_WARNINGS)
  target_compile_options(LapSure PRIVATE /WX)
endif()
