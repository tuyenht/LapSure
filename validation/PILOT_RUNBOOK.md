# Precision 5560 Pilot Runbook

## Scope
This is the first Beta 0.1.1 runtime-acceptance session. It validates one physical machine and does **not** production-certify the whole model profile. Do not change product code, package contents, trust manifest or profile status during the run.

Use only a portable package that already passed the manual package/provenance checkpoint. Keep the exact source commit, workflow run/job, artifact ID, ZIP SHA-256 and EXE SHA-256 with every pilot record.

## Before the appointment
1. Download the portable ZIP and matching `.zip.sha256` from the **same** GitHub Actions `workflow_dispatch` run.
2. Run `validation/verify_portable_package.ps1 -ZipPath <zip>` and retain the verifier output showing ZIP SHA-256, EXE SHA-256 and commit provenance.
3. Confirm those values match the release evidence recorded on PR #2 before launching LapSure.
4. Copy `SESSION_RECORD_TEMPLATE.md` and `DISCREPANCY_LOG_TEMPLATE.tsv` into a new local evidence folder named with one validation session ID, for example `VAL-YYYYMMDD-5560-01`.
5. Do not commit an unredacted Service Tag, serial number, report or screenshot to the public repository. Keep raw machine evidence in the local validation bundle; publish only reviewed/redacted evidence if needed.
6. Prepare known-good USB-C, Thunderbolt/USB4, SD, display/DP Alt Mode, audio, camera/microphone, network and AC-power stimuli as applicable.
7. Record BIOS, Dell ePSA and independent-comparator versions before testing.
8. Do not add third-party binaries to the verified package unless licensing, provenance and SHA-256 allowlisting are complete.

## Baseline execution order
1. Record machine identity and package hashes before launch.
2. Compare automatic inventory with BIOS and an independent comparator.
3. Complete functional I/O and every required physical port with traceable known-good stimulus.
4. Run Quick first. Review evidence before Standard or Deep.
5. Confirm HTML and JSON are both published, parse successfully, carry the same inspection identity and agree on the final decision/coverage state.
6. Exercise cancellation once and confirm the decision remains incomplete rather than PASS/BUY.
7. Exercise interrupted-journal recovery only when a controlled restart is safe.
8. Save HTML, JSON, screenshots, comparator exports, verifier output and operator notes under the same validation session ID.

## Visual / DPI acceptance matrix
Capture the real executable, not design mocks. Record PASS/FAIL and screenshots for each applicable cell:

| Resolution | Scale | Required checks |
|---|---:|---|
| 1366×768 | 100% | no clipping/overlap; visible primary actions; readable evidence/status |
| 1366×768 | 125% | no clipping/overlap; usable sidebar/table scrolling; visible focus |
| 1366×768 | 150% | no critical control/evidence loss; recovery/scrolling remains usable |
| 1920×1080 | 100% | layout alignment; no excessive truncation; visible status semantics |
| 1920×1080 | 125% | no clipping/overlap; visible focus; readable tables/cards |
| 1920×1080 | 150% | no critical control/evidence loss; navigation remains usable |

For every tested configuration:
- traverse all reachable primary actions with keyboard only;
- verify visible focus and Enter/Space behavior where documented;
- confirm status is not communicated by color alone;
- run Narrator and/or Accessibility Insights where available and record any inaccessible custom-drawn control as a discrepancy rather than assuming accessibility PASS.

## Failure-path matrix
Run only in a controlled validation copy/environment. Preserve evidence and never rerun merely to obtain PASS.

1. **Offline/network unavailable:** LapSure must remain usable; remote/cache factory evidence must not silently become Factory Exact.
2. **Provider unavailable:** missing PowerShell/Event Log/optional provider evidence must surface unavailable/not-tested/incomplete, not confirmed zero/PASS.
3. **No NVIDIA / optional external engine absent:** absence must not create a GPU/VRAM PASS.
4. **Unwritable output root:** report-publication failure must not rewrite the already computed hardware decision; no partial generation may be presented as published.
5. **Corrupt/truncated history copy:** persisted-input replay must fail safely without creating a fabricated prior result. Test only on a copied validation state root.
6. **Cancellation:** exercise cancellation during each stress stage when practical; cancelled evidence must not become PASS.
7. **Interrupted journal:** perform one controlled interruption/restart and verify detection/recovery/archive/discard semantics.
8. **Tampered engine copy:** alter a copied/isolated engine fixture or hash and verify execution is blocked before trust-sensitive use; restore the verified package before continuing normal testing.
9. **WinPE:** when a supported WinPE environment is available, verify explicit fallback/state-root/provider semantics separately. Record `NOT RUN` rather than inferring Windows behavior applies.
10. **Device-disabled cases:** where safe, test disabled camera, disconnected Wi-Fi/Bluetooth stimulus and AC removal; presence alone must not become functional PASS.

## Stop conditions
Stop stress testing immediately for abnormal temperature, odor, swelling, unstable power, repeated display loss, storage errors, a hang or unexpected reboot. Preserve journal/logs and mark the session blocked as appropriate.

## Exit decision
- **Accepted for Beta runtime:** one full workflow completes, package provenance is verified, reports/inspection identity agree, required functional/port evidence exists, visual/DPI acceptance has no blocking discrepancy, no critical false PASS exists, and every material discrepancy has a named reviewer disposition.
- **Profile remains draft:** fewer than two independently validated physical units.
- **Blocked:** crash/hang, unexplained BUY/BUY WITH NOTES with critical unknown evidence, report disagreement, wrong required-port map, package/provenance mismatch, blocking accessibility/visual defect, or any undispositioned critical discrepancy.
