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
python tests\bluetooth_functional_semantics_sanity.py
if errorlevel 1 exit /b 1
python tests\s16_s21_evidence_sanity.py
if errorlevel 1 exit /b 1
python tests\s22_s23_persistence_recovery_sanity.py
if errorlevel 1 exit /b 1
python tests\production_hardening_round1_sanity.py
if errorlevel 1 exit /b 1
python tests\production_hardening_round2_sanity.py
if errorlevel 1 exit /b 1
python tests\production_hardening_round3_interactions_sanity.py
if errorlevel 1 exit /b 1
python tests\production_hardening_keyboard_dispatch_sanity.py
if errorlevel 1 exit /b 1
python tests\s11_primary_action_sanity.py
if errorlevel 1 exit /b 1
python tests\production_hardening_hidden_controls_sanity.py
if errorlevel 1 exit /b 1
python tests\round4_process_trust_sanity.py
if errorlevel 1 exit /b 1
python tests\production_security_hardening_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_inspection_identity_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_report_publication_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_app_entry_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_session_history_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_trust_process_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_cloud_privacy_sanity.py
if errorlevel 1 exit /b 1
python tests\round5_product_truth_hygiene_sanity.py
if errorlevel 1 exit /b 1
python tests\ci_cost_control_policy.py
if errorlevel 1 exit /b 1
