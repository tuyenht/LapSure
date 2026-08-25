cmake_minimum_required(VERSION 3.21)
project(LapSure VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_executable(LapSure WIN32
    src/main_round5.cpp src/ui_renderer.cpp src/ui_state.cpp src/ui_components.cpp src/ui_shell_dynamic.cpp src/ui_screens_s01_s04_v2.cpp src/ui_screens_s02_s09_v2.cpp src/ui_screens_s10_s15_v2.cpp src/ui_screens_s12_s14_v2.cpp src/ui_screens_s16_s21_v2.cpp src/ui_screen_s20_round5.cpp src/ui_screens_s22_s23_v2.cpp src/session_history.cpp src/process.cpp src/environment.cpp src/profile.cpp src/cloud_lookup.cpp src/hardware.cpp src/edid.cpp src/inventory.cpp src/engines.cpp src/forensics.cpp src/sensors.cpp src/telemetry.cpp src/trust.cpp src/journal.cpp src/baseline.cpp src/functional.cpp src/functional_io.cpp src/acquisition.cpp src/port_power.cpp src/devpkey_defs.cpp src/chassis_profile.cpp src/port_selector.cpp src/port_attestation.cpp src/orchestrator.cpp src/runtime_validation.cpp src/decision_context.cpp src/decision_policy.cpp src/scoring.cpp src/stress.cpp src/report.cpp src/report_publication.cpp
    resources/resource.rc
    app.manifest
)
# Historical legacy screen symbol mapping remains documented here for source-level
# migration checks, but src/ui_screens.cpp is no longer part of the production target.
set_source_files_properties(src/ui_screens.cpp PROPERTIES
  COMPILE_DEFINITIONS "RenderScreenS01_Overview=RenderScreenS01_Overview_Legacy;RenderScreenS02_NewSession=RenderScreenS02_NewSession_Legacy;RenderScreenS03_SellerClaim=RenderScreenS03_SellerClaim_Legacy;RenderScreenS04_AutoAudit=RenderScreenS04_AutoAudit_Legacy;RenderScreenS05_Functional=RenderScreenS05_Functional_Legacy;RenderScreenS06_PhysicalSafety=RenderScreenS06_PhysicalSafety_Legacy;RenderScreenS07_PortsPower=RenderScreenS07_PortsPower_Legacy;RenderScreenS08_StressStability=RenderScreenS08_StressStability_Legacy;RenderScreenS09_BatteryPower=RenderScreenS09_BatteryPower_Legacy;RenderScreenS10_Storage=RenderScreenS10_Storage_Legacy;RenderScreenS11_Memory=RenderScreenS11_Memory_Legacy;RenderScreenS12_Display=RenderScreenS12_Display_Legacy;RenderScreenS13_AudioCamera=RenderScreenS13_AudioCamera_Legacy;RenderScreenS14_Network=RenderScreenS14_Network_Legacy;RenderScreenS15_SystemInfo=RenderScreenS15_SystemInfo_Legacy;RenderScreenS16_FactoryCompare=RenderScreenS16_FactoryCompare_Legacy;RenderScreenS17_EvidenceLibrary=RenderScreenS17_EvidenceLibrary_Legacy;RenderScreenS18_FinalReport=RenderScreenS18_FinalReport_Legacy;RenderScreenS19_ExportShare=RenderScreenS19_ExportShare_Legacy;RenderScreenS20_LogsEvents=RenderScreenS20_LogsEvents_Legacy;RenderScreenS21_Settings=RenderScreenS21_Settings_Legacy;RenderScreenS22_SessionHistory=RenderScreenS22_SessionHistory_Legacy;RenderScreenS23_InterruptedRecovery=RenderScreenS23_InterruptedRecovery_Legacy"
)
# Round 5 provides a conservative canonical S20 implementation. Rename the grouped
# v2 S20 symbol so unavailable Event Log providers cannot surface zero as evidence.
set_source_files_properties(src/ui_screens_s16_s21_v2.cpp PROPERTIES
  COMPILE_DEFINITIONS "RenderScreenS20_LogsEvents=RenderScreenS20_LogsEvents_Round5Legacy"
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
      src/functional.cpp src/functional_io.cpp src/acquisition.cpp src/orchestrator.cpp src/runtime_validation.cpp src/report.cpp src/report_publication.cpp)
  target_include_directories(LapSureBehaviorTests PRIVATE include)
  target_compile_definitions(LapSureBehaviorTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(LapSureBehaviorTests PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm winhttp mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
  if(MSVC)
    target_compile_options(LapSureBehaviorTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  endif()
  add_test(NAME LapSureBehaviorTests COMMAND LapSureBehaviorTests)

  add_executable(LapSureReportPublicationTests
      tests/report_publication_tests.cpp
      src/report.cpp src/report_publication.cpp src/session_history.cpp src/journal.cpp
      src/scoring.cpp src/chassis_profile.cpp src/port_power.cpp src/runtime_validation.cpp
      src/orchestrator.cpp src/functional.cpp src/process.cpp src/trust.cpp src/hardware.cpp src/baseline.cpp
      src/environment.cpp src/profile.cpp src/cloud_lookup.cpp src/edid.cpp src/inventory.cpp
      src/forensics.cpp src/functional_io.cpp src/acquisition.cpp src/devpkey_defs.cpp)
  target_include_directories(LapSureReportPublicationTests PRIVATE include)
  target_compile_definitions(LapSureReportPublicationTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX LAPSURE_ENABLE_TEST_HOOKS)
  target_link_libraries(LapSureReportPublicationTests PRIVATE user32 gdi32 comctl32 advapi32 shell32 shlwapi ole32 oleaut32 wbemuuid setupapi cfgmgr32 iphlpapi powrprof bcrypt winmm winhttp mf mfplat mfreadwrite mfuuid wlanapi bthprops uuid)
  if(MSVC)
    target_compile_options(LapSureReportPublicationTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  endif()
  add_test(NAME LapSureReportPublicationTests COMMAND LapSureReportPublicationTests)

  add_executable(LapSureSessionHistoryTests
      tests/session_history_tests.cpp
      src/session_history.cpp src/journal.cpp)
  target_include_directories(LapSureSessionHistoryTests PRIVATE include)
  target_compile_definitions(LapSureSessionHistoryTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX LAPSURE_ENABLE_TEST_HOOKS)
  if(MSVC)
    target_compile_options(LapSureSessionHistoryTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  endif()
  add_test(NAME LapSureSessionHistoryTests COMMAND LapSureSessionHistoryTests)

  add_executable(LapSureTrustSecurityTests tests/trust_security_tests.cpp src/trust.cpp src/process.cpp)
  target_include_directories(LapSureTrustSecurityTests PRIVATE include)
  target_compile_definitions(LapSureTrustSecurityTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(LapSureTrustSecurityTests PRIVATE bcrypt shell32)
  if(MSVC)
    target_compile_options(LapSureTrustSecurityTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  else()
    target_compile_options(LapSureTrustSecurityTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME LapSureTrustSecurityTests COMMAND LapSureTrustSecurityTests)

  add_executable(LapSureProcessSecurityTests tests/process_security_tests.cpp src/process.cpp src/trust.cpp)
  target_include_directories(LapSureProcessSecurityTests PRIVATE include)
  target_compile_definitions(LapSureProcessSecurityTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(LapSureProcessSecurityTests PRIVATE bcrypt shell32)
  if(MSVC)
    target_compile_options(LapSureProcessSecurityTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  else()
    target_compile_options(LapSureProcessSecurityTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME LapSureProcessSecurityTests COMMAND LapSureProcessSecurityTests)

  add_executable(LapSureProfileProvenanceTests
      tests/profile_provenance_security_tests.cpp
      src/profile.cpp src/chassis_profile.cpp)
  target_include_directories(LapSureProfileProvenanceTests PRIVATE include)
  target_compile_definitions(LapSureProfileProvenanceTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  if(MSVC)
    target_compile_options(LapSureProfileProvenanceTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  else()
    target_compile_options(LapSureProfileProvenanceTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME LapSureProfileProvenanceTests COMMAND LapSureProfileProvenanceTests)

  add_executable(LapSureDecisionAuthorityTests
      tests/decision_authority_tests.cpp
      src/decision_context.cpp src/decision_policy.cpp src/port_attestation.cpp src/scoring.cpp src/chassis_profile.cpp)
  target_include_directories(LapSureDecisionAuthorityTests PRIVATE include)
  target_compile_definitions(LapSureDecisionAuthorityTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX LAPSURE_ENABLE_TEST_HOOKS)
  if(MSVC)
    target_compile_options(LapSureDecisionAuthorityTests PRIVATE /W4 /WX /permissive- /EHsc /Zc:__cplusplus /utf-8)
  else()
    target_compile_options(LapSureDecisionAuthorityTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME LapSureDecisionAuthorityTests COMMAND LapSureDecisionAuthorityTests)
endif()

option(LAPSURE_STRICT_WARNINGS "Treat MSVC warnings as errors in CI" OFF)
if(MSVC AND LAPSURE_STRICT_WARNINGS)
  target_compile_options(LapSure PRIVATE /WX)
endif()