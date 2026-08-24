# Component Catalog

| ID | Component | Purpose |
|---|---|---|
| C01 | App Shell | Window chrome/content/sidebar/status architecture |
| C02 | Sidebar | Grouped navigation and active state |
| C03 | Page Header | Screen title, context, optional help/actions |
| C04 | Status Badge | Canonical state label/icon/color |
| C05 | Metric Card | One evidenced metric or concise domain summary |
| C06 | Progress & Coverage | Real progress or evidence coverage |
| C07 | Guided Stepper | Orchestrator stage progression |
| C08 | Evidence Row | Expandable evidence/result/source row |
| C09 | Data Table | Dense structured technical information |
| C10 | Next Action Panel | Current next-best action and reason |
| C11 | Dialog & Confirmation | Modal interaction/safety/destructive confirmation |
| C12 | Empty/Error/Unsupported State | Explicit non-happy states |

Each component has a dedicated normative contract under `components/`.
Screens should reference these IDs rather than inventing screen-specific variants.
