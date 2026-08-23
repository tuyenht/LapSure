# Precision 5560 Pilot Runbook

## Scope
This is the first Beta 0.1.1 runtime-acceptance session. It validates one machine and does not production-certify the whole model profile. Do not change product code or profile status during the run.

## Before the appointment
1. Download the portable ZIP and matching `.zip.sha256` from the same GitHub Actions run.
2. Run `validation/verify_portable_package.ps1 -ZipPath <zip>` and retain its output.
3. Copy `SESSION_RECORD_TEMPLATE.md` and `DISCREPANCY_LOG_TEMPLATE.tsv` into a new folder named with one session ID, for example `VAL-20260823-5560-01`.
4. Prepare known-good USB-C, Thunderbolt/USB4, SD, display/DP Alt Mode, audio and AC-power stimuli as applicable.
5. Record versions of BIOS, Dell ePSA and the independent comparator. Do not add third-party binaries to the package unless licensing and SHA-256 allowlisting are complete.

## Execution order
1. Record identity and hashes before launching LapSure.
2. Compare automatic inventory with BIOS and an independent comparator.
3. Complete functional I/O and every required physical port.
4. Run Quick first. Review evidence before Standard or Deep.
5. Exercise cancellation once and confirm the decision remains incomplete.
6. Exercise interrupted-journal recovery only when a reboot is safe.
7. Run the negative cases in `VALIDATION_CHECKLIST.md`.
8. Save HTML, JSON, screenshots, comparator exports and operator notes under the same session ID.

## Stop conditions
Stop stress testing immediately for abnormal temperature, odor, swelling, unstable power, repeated display loss, storage errors, a hang, or an unexpected reboot. Preserve the journal and logs; do not rerun merely to obtain a PASS.

## Exit decision
- **Accepted for Beta runtime:** one full workflow completes, reports agree, no critical false PASS exists, and every discrepancy has a reviewer disposition.
- **Profile remains draft:** fewer than two independently validated physical units.
- **Blocked:** crash/hang, unexplained BUY with critical unknown evidence, report disagreement, wrong required-port map, or an undispositioned critical discrepancy.
