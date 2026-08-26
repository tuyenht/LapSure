# Round 5.1C — Acceptance & Release Closure Contract

This note supports the proposed Round 5.1 A+ architecture and is not implementation authorization.

## Preconditions
5.1C does not begin until 5.1A decision semantics and 5.1B trusted-provider execution are compiled-green at their bounded review checkpoints.

## Acceptance order
1. strict Windows regression/security checkpoint;
2. exact package/provenance candidate;
3. short Precision physical smoke;
4. keyboard/focus/accessibility smoke;
5. representative DPI/display matrix;
6. full physical acceptance only when no unresolved P0/P1 remains;
7. branch/release governance closure;
8. final review, Ready/Merge, final-main package checkpoint.

## Display/DPI matrix
- 1366×768 at 100/125/150% compatibility viewport;
- 1920×1200 at 100/125/150% Precision FHD+ class;
- 3840×2400 at 150/200/225% where hardware permits;
- 1920×1080 optional regression only.

## Governance
Before merge, protect `main` or apply an equivalent ruleset: PR-based integration, no force push/delete, conversation resolution, and required checks compatible with Draft/cost-control policy.

## Evidence
Run #592 remains historical runtime/security baseline only. A new exact candidate is required after production changes. One physical session validates that session, not an entire model family.
