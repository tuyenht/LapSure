# LapSure Visual References

Images define visual hierarchy/layout/style only and never authorize fake diagnostic data.

Precedence: Coverage Contract → Product Spec → Architecture → UI Master Spec → State/Data contracts → Screen contract → Component contract → Known Deviations → Approved visual.

## Canonical repository visual source

- `CONTACT_SHEET.jpg` — compact global overview.
- `LapSure_UI_Visual_Reference_Pack.zip` — **canonical complete visual pack in the repository**. It contains `approved/Sxx_*.jpg` entries plus archived concepts.
- `approved/` — optional directly promoted previews; a screen does not require a duplicated direct file when the canonical packed member exists.
- `archive/` — superseded concepts, never implementation authority unless explicitly requested.
- `MANIFEST.yaml` — machine-readable screen/spec/packed-image mapping.

For detailed per-screen inspection, an agent should extract `LapSure_UI_Visual_Reference_Pack.zip` into a temporary working directory and use the member named by `MANIFEST.yaml`. Do not commit extracted duplicates unless explicitly required.

The downloadable specification bundle distributed outside the repository additionally contains source PNG originals for long-term reference.

AI-generated text/numbers inside a mockup may be illustrative or imperfect; written contracts remain normative for behavior and semantics.
