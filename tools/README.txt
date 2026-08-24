Optional expert engines. The main app must still run if these are absent or blocked.

- smartctl.exe: optional deep SATA/NVMe SMART JSON enrichment.
- nvidia-smi.exe: optional bundled NVIDIA telemetry adapter.
- memtest_vulkan.exe: optional VRAM integrity adapter under tools\gpu.
- lhm_bridge.exe: optional CPU sensor adapter under tools\sensors.

Security rules:
- PATH-discovered copies are not trusted.
- A bundled engine is disabled until its reviewed SHA-256 is explicitly configured in engine_manifest.txt.
- Empty, malformed, duplicate or mismatched allowlist entries fail closed.
- Runtime launch re-verifies the engine and uses its canonical path.
- Do not place reviewed engines or the allowlist in a user-writable elevated deployment and call that production-trusted; use the release security policy.

Do not redistribute third-party binaries until their licenses/redistribution terms have been reviewed.
