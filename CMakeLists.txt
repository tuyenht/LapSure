cmake_minimum_required(VERSION 3.21)
project(LapSure VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_executable(LapSure WIN32
    src/main.cpp src/ui_renderer.cpp src/ui_state.cpp src/ui_components.cpp src/ui_shell_dynamic.cpp src/ui_screens.cpp src/ui_screens_s01_s04_v2.cpp src/ui_screens_s02_s09_v2.cpp src/ui_screens_s10_s15_v2.cpp src/ui_screens_s12_s14_v2.cpp src/ui_screens_s16_s21_v2.cpp src/ui_screens_s22_s23_v2.cpp src/session_history.cpp src/process.cpp src/environment.cpp src/profile.cpp src/cloud_lookup.cpp src/hardware.cpp src/edid.cpp src/inventory.cpp src/engines.cpp src/forensics.cpp src/sensors.cpp src/telemetry.cpp src/trust.cpp src/journal.cpp src/baseline.cpp src/functional.cpp src/functional_io.cpp src/acquisition.cpp src/port_power.cpp src/devpkey_defs.cpp src/chassis_profile.cpp src/port_selector.cpp src/orchestrator.cpp src/runtime_validation.cpp src/scoring.cpp src/stress.cpp src/report.cpp
    resources/resource.rc
    app.manifest
)
# Keep legacy screen implementations compiled only as explicitly named fallback symbols.
# Runtime callers use the canonical names implemented by the evidence-bound v2 translation units.
set_source_files_properties(src/ui_screens.cpp PROPERTIES
  COMPILE_DEFINITIONS "RenderScreenS01_Overview=RenderScreenS01_Overview_Legacy;RenderScreenS02_NewSession=RenderScreenS02_NewSession_Legacy;RenderScreenS03_SellerClaim=RenderScreenS03_SellerClaim_Legacy;RenderScreenS04_AutoAudit=RenderScreenS04_AutoAudit_Legacy;RenderScreenS05_Functional=RenderScreenS05_Functional_Legacy;RenderScreenS06_PhysicalSafety=RenderScreenS06_PhysicalSafety_Legacy;RenderScreenS07_PortsPower=RenderScreenS07_PortsPower_Legacy;RenderScreenS08_StressStability=RenderScreenS08_StressStability_Legacy;RenderScreenS09_BatteryPower=RenderScreenS09_BatteryPower_Legacy;RenderScreenS10_Storage=RenderScreenS10_Storage_Legacy;RenderScreenS11_Memory=RenderScreenS11_Memory_Legacy;RenderScreenS12_Display=RenderScreenS12_Display_Legacy;RenderScreenS13_AudioCamera=RenderScreenS13_AudioCamera_Legacy;RenderScreenS14_Network=RenderScreenS14_Network_Legacy;RenderScreenS15_SystemInfo=RenderScreenS15_SystemInfo_Legacy;RenderScreenS16_FactoryCompare=RenderScreenS16_FactoryCompare_Legacy;RenderScreenS17_EvidenceLibrary=RenderScreenS17_EvidenceLibrary_Legacy;RenderScreenS18_FinalReport=RenderScreenS18_FinalReport_Legacy;RenderScreenS19_ExportShare=RenderScreenS19_ExportShare_Legacy;RenderScreenS20_LogsEvents=RenderScreenS20_LogsEvents_Legacy;RenderScreenS21_Settings=RenderScreenS21_Settings_Legacy;RenderScreenS22_SessionHistory=RenderScreenS22_SessionHistory_Legacy;RenderScreenS23_InterruptedRecovery=RenderScreenS23_InterruptedRecovery_Legacy"
)
# Keep the legacy shell drawing code available as layout-compatible fallback helpers only.
# Canonical DrawSidebar/DrawAppShellFooter are provided by ui_shell_dynamic.cpp so runtime metadata
# can never come from stale literals or heuristic readiness counts.
set_source_files_properties(src/ui_components.cpp PROPERTIES
  COMPILE_DEFINITIONS "DrawSidebar=DrawSidebar_Legacy;DrawAppShellFooter=DrawAppShellFooter_Legacy"
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
target_link_libraries(LapSure PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm winhttp mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
install(TARGETS LapSure RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
  add_executable(LapSureBehaviorTests
      tests/behavioral_tests.cpp
      src/scoring.cpp src/chassis_profile.cpp src/port_power.cpp src/devpkey_defs.cpp
      src/stress.cpp src/process.cpp src/telemetry.cpp src/sensors.cpp src/trust.cpp
      src/journal.cpp src/session_history.cpp src/baseline.cpp src/hardware.cpp)
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
