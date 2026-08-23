# Real-machine Validation Checklist

For each physical laptop:
- Record operator, UTC start/end, LapSure commit SHA, artifact ID, ZIP hash and EXE hash.
- Record identifiers for every known-good stimulus device and reference-tool version.
- Record Service Tag, BIOS version and exact hardware.
- Compare inventory with BIOS and an independent reference tool.
- Run Quick, Standard and Deep where practical.
- Confirm EDID native resolution against the actual panel.
- Confirm camera frame, microphone capture and stereo L/R.
- Validate every required chassis-profile port with a known-good device.
- Confirm no missing external engine is reported PASS.
- Cancel one run and confirm incomplete state.
- Reboot after an interrupted stress test and verify journal detection.
- Save HTML + JSON reports.
- Record every false PASS, false FAIL, crash, hang, timeout, wrong label or wrong port map.
- Attach the generated HTML/JSON, screenshots and comparator exports under one validation session ID.
- Run negative cases: missing provider, wrong engine hash, disabled camera, disconnected Wi-Fi, AC removal, cancellation during each stress stage and interrupted journal recovery.
- Require a named reviewer to sign off every discrepancy disposition and final machine result.

Acceptance thresholds:
- No runtime-validation FAIL and no critical UNKNOWN may coexist with BUY / BUY WITH NOTES.
- Every required chassis port is tested with traceable stimulus evidence; zero probes is INCOMPLETE.
- SMART/VRAM malformed output, timeout or non-zero exit is NOT TESTED/FAIL, never PASS.
- HTML and JSON must be non-empty UTF-8, parse successfully and agree on the final decision.

Production-certify a model profile only after validation on at least two physical units.
