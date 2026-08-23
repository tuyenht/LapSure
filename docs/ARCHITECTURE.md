# LapSure Architecture

## Design goal
One native diagnostic core should run on normal Windows and, where capabilities exist, custom x64 WinPE. Optional Windows-only providers enhance evidence without becoming hard dependencies.

## High-level flow
```text
Providers / Inventory / Functional / Stress / Port stimulus
                    |
              Evidence Model
             (AuditReport etc.)
                    |
          Decision + Orchestrator
                    |
          Presentation Mapping
                    |
      Reusable Native UI Components
                    |
              Screens S01–S23
                    |
             HTML + JSON Reports
```

## Core modules
- inventory
- edid
- forensics
- engines/trust
- stress/journal
- telemetry/sensors
- functional/functional_io
- port_power
- chassis_profile
- orchestrator
- scoring
- report
- runtime_validation

## Trust boundary
External engines are SHA-256 pinned. Missing/hash-mismatched engines do not execute and cannot generate PASS.

## WinPE policy
Missing packages/drivers/capabilities become `UNSUPPORTED`/`NOT TESTED`, not hardware failure.

## Workflow integrity
Automatic audit is separate from interactive operator steps. Interactive result recording is gated until the required automatic snapshot exists.

## Evidence model
Identity, factory expectation, health, functionality, stability, historical evidence, coverage and confidence remain separate.

## Presentation-layer architecture

### Dependency direction
`Provider → Evidence Model → Decision/Orchestrator → Presentation Mapping → UI`

The UI:
- MAY read model/evidence and request operations.
- MAY format/derive explicitly documented values.
- MUST NOT invent evidence.
- MUST NOT convert uncertainty to PASS.
- MUST NOT perform slow provider work inside paint/layout.
- MUST keep provider/environment failures visible.
- MUST use shared state/data/component contracts from `docs/ui/`.

Prefer a presentation/view-model mapper that normalizes status wording, availability and unit formatting without mutating underlying diagnostic truth. Screen renderers should depend on shared native components/tokens, not duplicate semantic state logic.
