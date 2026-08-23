# LapSure Coverage Contract

LapSure is the single user-facing workflow for laptop verification. It may use Windows-native providers and reviewed, internally managed adapters, but the operator should not need to run or reconcile separate diagnostic applications.

“Complete” means that every required coverage domain has validated evidence in the report. It does not mean that LapSure invents vendor-private values which firmware or drivers do not expose.

Each HTML and JSON report records, per domain:

- status: `COMPLETE` or `PARTIAL`;
- whether the domain is required for the current device;
- evidence sources;
- the exact missing evidence when incomplete.

The required domains are system identity, memory, storage identity and health, graphics, display, stability, thermals/throttling, functional devices, physical ports/power, and runtime/report integrity. Battery is required only when a battery is present.

An acceptance verdict is prohibited while any required domain is incomplete. Optional deep enrichment (for example a controller-specific SMART log) may remain unavailable when a validated native provider already satisfies the domain's minimum evidence contract; the limitation remains visible in detailed findings.

Interactive checks such as speakers, display defects, keyboard feel, and physical connector stimulus remain part of LapSure's guided workflow. They require operator action or a known-good peripheral, but never a second software tool.

Provider precedence is:

1. documented Windows-native APIs;
2. reviewed and hash-pinned adapters distributed inside the LapSure portable package;
3. guided human/physical stimulus with evidence recorded by LapSure.

Unknown, unsupported, permission-denied, contradictory, and stale results must remain explicit. They must never be converted into PASS.
