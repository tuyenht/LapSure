# S07 — Cổng & Nguồn

`components: [C01,C02,C03,C04,C08,C09,C10,C11,C12]`  
`visual: ../references/approved/S07_PORTS_POWER.jpg`

## User outcome
Verify actual physical connectors/power using guided known-good stimulus.

## Objects
Model-aware chassis visualization; required/optional ports; before/after stimulus; PnP/location/bus evidence; USB4/TB deltas; AC/power; known-good device guidance.

## Data
`PortPowerSummary`, `PortProbeResult`, `PowerProbeResult`, `ChassisProfile`.

## Invariant
Controller presence never certifies a physical port. Exact link rate is not guessed. Draft/generic chassis profile confidence is disclosed.

## Acceptance
Every required port has explicit tested/not-tested/unsupported state and evidence source; user sees the next port/action.
