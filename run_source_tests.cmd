@echo off
python tests\source_sanity.py
if errorlevel 1 exit /b 1
python tests\accuracy_gate_sanity.py
if errorlevel 1 exit /b 1
python tests\alpha61_integrity_sanity.py
if errorlevel 1 exit /b 1
python tests\alpha62_functional_io_sanity.py
if errorlevel 1 exit /b 1
python tests\alpha63_port_power_sanity.py
if errorlevel 1 exit /b 1
python tests\alpha7_orchestrator_sanity.py
if errorlevel 1 exit /b 1
python tests\alpha71_chassis_profile_sanity.py
if errorlevel 1 exit /b 1
python tests\beta01_build_validation_sanity.py
if errorlevel 1 exit /b 1
python tests\repo_integration_sanity.py
if errorlevel 1 exit /b 1
python tests\phase_a_ui_sanity.py
if errorlevel 1 exit /b 1
python tests\s01_s04_dynamic_binding_sanity.py
if errorlevel 1 exit /b 1
python tests\s02_s09_dynamic_binding_sanity.py
if errorlevel 1 exit /b 1
python tests\s10_s15_p0_evidence_sanity.py
if errorlevel 1 exit /b 1
python tests\s12_s14_evidence_sanity.py
if errorlevel 1 exit /b 1
