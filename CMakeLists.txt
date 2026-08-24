cmake_minimum_required(VERSION 3.21)
project(LapSure VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_executable(LapSure WIN32
    src/main.cpp src/ui_renderer.cpp src/ui_state.cpp src/ui_components.cpp src/ui_screens.cpp src/ui_screens_s01_s04_v2.cpp src/process.cpp src/environment.cpp src/profile.cpp src/cloud_lookup.cpp src/hardware.cpp src/edid.cpp src/inventory.cpp src/engines.cpp src/forensics.cpp src/sensors.cpp src/telemetry.cpp src/trust.cpp src/journal.cpp src/baseline.cpp src/functional.cpp src/functional_io.cpp src/acquisition.cpp src/port_power.cpp src/devpkey_defs.cpp src/chassis_profile.cpp src/port_selector.cpp src/orchestrator.cpp src/runtime_validation.cpp src/scoring.cpp src/stress.cpp src/report.cpp
    resources/resource.rc
    app.manifest
)
# Keep the legacy S01/S04 implementation compiled only as explicitly named fallback symbols.
# Runtime callers keep the canonical names, which are implemented by ui_screens_s01_s04_v2.cpp.
set_source_files_properties(src/ui_screens.cpp PROPERTIES
  COMPILE_DEFINITIONS "RenderScreenS01_Overview=RenderScreenS01_Overview_Legacy;RenderScreenS04_AutoAudit=RenderScreenS04_AutoAudit_Legacy"
)
# The v2 translation unit intentionally keeps one internal physical-safety evaluator for the next
# screen-composition pass. Suppress only MSVC C4505 in this file; all other /W4 warnings remain errors in CI.
if(MSVC)
  set_source_files_properties(src/ui_screens_s01_s04_v2.cpp PROPERTIES COMPILE_OPTIONS "/wd4505")
endif()
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
target_link_libraries(LapSure PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm winhttp mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
add_custom_command(TARGET LapSure POST_BUILD
    COMMAND powershell -Command "if (Test-Path '$<TARGET_FILE:LapSure>') { Copy-Item '$<TARGET_FILE:LapSure>' '${CMAKE_SOURCE_DIR}/bin/LapSure.exe' -Force -ErrorAction SilentlyContinue; Copy-Item '$<TARGET_FILE:LapSure>' '${CMAKE_SOURCE_DIR}/LapSure.exe' -Force -ErrorAction SilentlyContinue }"
    COMMENT "Syncing LapSure.exe to bin/ and root directory"
)
install(TARGETS LapSure RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
  add_executable(LapSureBehaviorTests
      tests/behavioral_tests.cpp
      src/scoring.cpp src/chassis_profile.cpp src/port_power.cpp src/devpkey_defs.cpp
      src/stress.cpp src/process.cpp src/telemetry.cpp src/sensors.cpp src/trust.cpp
      src/journal.cpp src/baseline.cpp src/hardware.cpp)
  target_sources(LapSureBehaviorTests PRIVATE src/engines.cpp src/cloud_lookup.cpp)
  target_sources(LapSureBehaviorTests PRIVATE
      src/environment.cpp src/profile.cpp src/edid.cpp src/inventory.cpp src/forensics.cpp
      src/functional.cpp src/functional_io.cpp src/acquisition.cpp src/orchestrator.cpp src/runtime_validation.cpp src/report.cpp)
  target_include_directories(LapSureBehaviorTests PRIVATE include)
  target_compile_definitions(LapSureBehaviorTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(LapSureBehaviorTests PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm winhttp mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
  if(MSVC)
    target_compile_options(LapSureBehaviorTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  endif()
  add_test(NAME LapSureBehaviorTests COMMAND LapSureBehaviorTests)
endif()

option(LAPSURE_STRICT_WARNINGS "Treat MSVC warnings as errors in CI" OFF)
if(MSVC AND LAPSURE_STRICT_WARNINGS)
  target_compile_options(LapSure PRIVATE /WX)
endif()
