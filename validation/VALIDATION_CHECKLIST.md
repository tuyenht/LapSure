# Real-machine Validation Checklist

Follow `PILOT_RUNBOOK.md` for the first Precision 5560 session. Use only a portable package from a successful manual package/provenance checkpoint, verify it before launch, and keep all raw evidence under one local validation session ID.

## Package / identity gate
- Record operator, reviewer, UTC start/end, machine model and local Service Tag/serial evidence.
- Record exact LapSure source commit, workflow run ID, job ID and artifact ID.
- Record ZIP SHA-256 and EXE SHA-256 from `verify_portable_package.ps1` output.
- Confirm package commit provenance matches the intended PR #2 head for the dispatch.
- Do not publish unredacted Service Tags, serials, raw reports or screenshots to the public repo.

## Automatic inventory and evidence
- Record BIOS version and exact hardware configuration.
- Compare inventory with BIOS and an independent reference tool; record comparator name/version.
- Confirm EDID native resolution against the actual panel.
- Confirm factory-profile provenance is explicit; cloud/cache advisory evidence never silently becomes Factory Exact.
- Confirm missing/malformed/timed-out/untrusted/unsupported required evidence remains NOT TESTED/UNAVAILABLE/INCOMPLETE rather than PASS.
- Confirm no missing optional external engine is reported PASS.

## Functional / physical validation
- Confirm camera frame, microphone capture and stereo L/R with real stimulus.
- Validate Wi-Fi/Bluetooth functionality with known-good interaction rather than radio presence alone.
- Validate every required chassis-profile port with an identified known-good device.
- Record required ports tested / required total; zero required probes is INCOMPLETE.
- Record AC/battery behavior and charger/stimulus identity where applicable.

## Stress / lifecycle
- Run Quick, then Standard and Deep where safe/practical.
- Cancel at least one run and confirm incomplete/cancelled state.
- Where practical, exercise cancellation during each stress stage and preserve evidence.
- Perform one controlled interrupted-session/restart test and verify journal detection/recovery semantics.
- Stop immediately for abnormal heat, odor, swelling, unstable power, repeated display loss, storage errors, hang or unexpected reboot.

## Report / persistence contract
- Save the published HTML and JSON from the same `bundle-*` generation.
- Confirm both files are non-empty, valid/parseable and belong to the same inspection identity.
- Confirm HTML and JSON agree on final decision and material coverage state.
- Confirm `session_history.tsv` references the published generation correctly.
- Confirm no `.staging-*` generation is presented as published.
- Exercise an unwritable output root in a controlled test and verify publication failure does not rewrite hardware truth or expose a partial bundle as complete.
- Exercise a copied corrupt/truncated history/state fixture and verify replay fails safely without fabricating a prior result.

## Negative / unavailable cases
- Offline/no-network operation.
- Missing provider / Event Log query unavailable.
- No NVIDIA or optional trusted engine absent.
- Wrong/tampered engine hash in an isolated validation copy; trusted execution must be blocked.
- Disabled camera.
- Disconnected Wi-Fi/Bluetooth stimulus.
- AC removal when safe.
- Interrupted journal recovery.
- WinPE state-root/provider fallback when a supported WinPE environment is available; otherwise record `NOT RUN`.

## Visual / accessibility acceptance
Capture real executable screenshots and result for:
- 1366×768 @ 100%, 125%, 150%.
- 1920×1080 @ 100%, 125%, 150%.

For each applicable cell:
- no blocking clipping/overlap or inaccessible primary action;
- sidebar/table scrolling remains usable when content exceeds viewport;
- keyboard traversal reaches documented primary actions;
- visible focus is present;
- Enter/Space activation matches the visible action contract;
- status is not color-only;
- Narrator and/or Accessibility Insights is exercised where available; inaccessible custom controls are logged as discrepancies rather than presumed accessible.

## Evidence bundle
- Attach verifier output, HTML, JSON, screenshots, BIOS/ePSA evidence, comparator exports, stress journal/logs and operator notes.
- Record every false PASS, false FAIL, crash, hang, timeout, wrong label, wrong port map, inaccessible control or report disagreement.
- Require a named reviewer to disposition every material discrepancy.

## Acceptance thresholds
- No runtime-validation FAIL and no critical UNKNOWN may coexist with BUY / BUY WITH NOTES.
- Every required chassis port has traceable stimulus evidence; otherwise decision remains incomplete as required by contract.
- SMART/VRAM malformed output, timeout or non-zero exit is NOT TESTED/FAIL, never PASS.
- HTML/JSON inspection identity and final decision agree.
- No critical false PASS exists.
- No package/provenance mismatch exists.
- No blocking visual/accessibility discrepancy remains undispositioned.

## Release gates
- One reviewed full-machine session may satisfy **Beta runtime acceptance for that machine/session**.
- It does not certify the whole model or automatically change a chassis profile to `physical-verified`.
- Profile certification requires at least two independently validated physical units of that model with every material discrepancy disposition signed off.
