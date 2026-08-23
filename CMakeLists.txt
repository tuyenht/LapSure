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
  # app.manifest owns the requestedExecutionLevel; prevent the linker from injecting a conflicting UAC snippet.
  target_link_options(LapSure PRIVATE /SUBSYSTEM:WINDOWS /MANIFESTUAC:NO)
else()
  target_compile_options(LapSure PRIVATE -Wall -Wextra -Wpedantic)
endif()
# mf.lib provides MFEnumDeviceSources; uuid.lib provides standard Windows GUID imports.
target_link_libraries(LapSure PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
install(TARGETS LapSure RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
  add_executable(LapSureBehaviorTests
      tests/behavioral_tests.cpp
      src/scoring.cpp src/chassis_profile.cpp src/port_power.cpp src/devpkey_defs.cpp
      src/stress.cpp src/process.cpp src/telemetry.cpp src/sensors.cpp src/trust.cpp
      src/journal.cpp src/baseline.cpp src/hardware.cpp)
  target_sources(LapSureBehaviorTests PRIVATE src/engines.cpp)
  target_sources(LapSureBehaviorTests PRIVATE
      src/environment.cpp src/profile.cpp src/edid.cpp src/inventory.cpp src/forensics.cpp
      src/functional.cpp src/functional_io.cpp src/orchestrator.cpp src/runtime_validation.cpp src/report.cpp)
  target_include_directories(LapSureBehaviorTests PRIVATE include)
  target_compile_definitions(LapSureBehaviorTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(LapSureBehaviorTests PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
  if(MSVC)
    target_compile_options(LapSureBehaviorTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  endif()
  add_test(NAME LapSureBehaviorTests COMMAND LapSureBehaviorTests)
endif()

option(LAPSURE_STRICT_WARNINGS "Treat MSVC warnings as errors in CI" OFF)
if(MSVC AND LAPSURE_STRICT_WARNINGS)
  target_compile_options(LapSure PRIVATE /WX)
endif()
