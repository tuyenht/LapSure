# LapSure Architecture

## Design goal
One native diagnostic core should run on normal Windows and, where capabilities exist, on custom x64 WinPE. Optional Windows-only providers enhance evidence without becoming hard dependencies.

## High-level flow
```text
UI / Test Orchestrator
        |
        +-- Inventory & Identity
        +-- Factory / Chassis Profiles
        +-- Health Providers
        +-- Stress & Stability
        +-- Telemetry
        +-- Functional I/O
        +-- Port & Power Stimulus Tests
        |
   Decision Engine
        |
   HTML + JSON Reports
```

## Core modules
- `inventory` — CPU/RAM/GPU/storage/battery identity
- `edid` — native panel EDID identity/timing
- `forensics` — BIOS/mainboard/security/event evidence
- `engines` / `trust` — external-tool adapters and hash allowlist
- `stress` / `journal` — controlled stress and interruption evidence
- `telemetry` / `sensors` — runtime telemetry and optional trusted providers
- `functional` / `functional_io` — display/input/audio/camera/network workflows
- `port_power` — physical-port stimulus and USB4/Thunderbolt/AC evidence
- `chassis_profile` — data-driven model-specific physical-port requirements
- `orchestrator` — stage progression and next-best action
- `scoring` — confidence-aware decision model
- `report` — HTML/JSON evidence output
- `runtime_validation` — build/provider/runtime acceptance evidence

## Trust boundary
External engines are not trusted by path/name alone. `engine_manifest.txt` pins reviewed SHA-256 values. Unavailable or hash-mismatched engines must not execute and must not generate PASS.

## WinPE policy
The core favors Win32/SetupAPI/COM APIs. WinPE images vary by optional packages and drivers; missing support becomes `UNSUPPORTED`/`NOT TESTED`, not hardware failure.

## Workflow integrity
Automatic audit runs separately from interactive operator steps. Interactive results are blocked until the automatic snapshot completes; updates are synchronized and regenerate decision/report.

## Evidence model
Identity, factory expectation, health, functionality, stability, historical evidence, coverage and confidence are kept separate. This avoids conflating “changed component” with “bad component” or “detected” with “verified”.