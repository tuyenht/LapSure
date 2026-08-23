# S09 — Pin & Năng lượng

`components: [C01,C02,C03,C04,C05,C08,C10,C12]`  
`visual: ../references/approved/S09_BATTERY_POWER.jpg`

## User outcome
Explain battery capacities/wear and controlled power evidence without invented adapter data.

## Objects
Presence/status; design/full capacity; wear when derivable; cycle count; charge state; timed charge/discharge evidence; adapter evidence/confidence.

## Data
`BatteryInfo`, `PowerProbeResult`, battery audit findings.

## Invariant
Wear only from valid capacities. Adapter wattage is UNKNOWN unless trusted evidence proves it. Short uncontrolled samples do not become runtime prediction.

## Acceptance
Named metrics only; unavailable state for missing capacity/cycle/wattage; evidence source visible.
