# Real-machine Validation Checklist

For each physical laptop:
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

Production-certify a model profile only after validation on at least two physical units.
