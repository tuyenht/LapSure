# LapSure Production Hardening Round 4 Design

## Purpose
Close the remaining release-blocking gaps found in the production audit of PR #2 without adding new product scope. The implementation must preserve LapSure's evidence-first contracts and native C++20/Win32 architecture.

## Normative constraints
- Evidence before verdict; uncertainty never becomes PASS.
- Required coverage incomplete prohibits an acceptance verdict.
- Presence is not functionality.
- Factory/seller mismatch is not hardware-health failure.
- Provider/environment failures remain explicit.
- UI paint/layout must not perform slow provider work.
- External engines may execute only through a fail-closed trust boundary.
- Windows/WinPE graceful degradation is required.

## 1. Interaction dispatch hardening
The current window procedure must not derive behavior from a global focus index alone. Keyboard and mouse activation shall resolve a screen-specific action first, then invoke exactly the operation represented by the visible CTA.

S11 is required by its screen contract to expose a reachable primary action for starting/rerunning the online RAM/stress workflow. Its visible C10 action must therefore have a real hit-test and keyboard route. When RAM evidence already exists, the action may navigate to the Stress/Evidence view rather than silently rerun diagnostics.

Acceptance rules:
- No `focusIndex == 2` or equivalent global shortcut may call `StartAudit()` independently of the current screen.
- Every visible enabled primary CTA S01-S23 either has a real operation or is rendered non-interactive.
- S11 has one unambiguous primary action.

## 2. Elevated process and engine trust boundary
`RunProcessCapture` shall support an explicit executable path (`lpApplicationName`) plus a separately quoted argument vector. Elevated launches must not rely on Windows command-line executable resolution for security-sensitive providers.

System PowerShell shall resolve from the canonical Windows system path rather than PATH/SearchPath. Optional bundled engines shall be verified immediately before launch, and the exact verified canonical path shall be the path executed.

`VerifyEngine` shall:
- reject absolute/rooted/traversal paths;
- canonicalize within the application root;
- reject reparse-point escape/reparse components in the engine path;
- require exactly one manifest entry for the logical engine name;
- require an exact 64-hex SHA-256;
- return the verified canonical path;
- fail closed for missing, malformed, duplicate, replaced or mismatched engine artifacts.

The allowlist remains disabled by default when hashes are empty.

## 3. Durable session, history and recovery state
Inspection identity must exist before stress begins. Introduce a session identifier at inspection/report scope and use it for report stems, history and stress journal correlation. If a legacy report lacks an inspection id, generate a collision-resistant GUID when a new inspection starts; historical deserialization must not invent acceptance evidence.

Recovery journal storage must use a persistent state/output root selected by the same policy that protects reports in WinPE; it must not implicitly depend on `appDir/reports` when that directory can be volatile.

Journal write/complete/discard failures must be observable evidence. A failure to establish recovery durability cannot create PASS and must reduce publication readiness.

History index loading shall be bounded and schema-validated. Persisted verdict/status strings are untrusted input: invalid records render as corrupt/untrusted and cannot appear as a trusted BUY/COMPLETE session. In-memory history changes become visible only after the replacement index is successfully persisted.

## 4. Report publication transaction
Hardware `AuditDecision` and report publication readiness are separate concerns. A storage/persistence failure must not rewrite the underlying hardware truth; instead it blocks publication/export of an acceptance result.

HTML and JSON shall be written as a staged bundle using one session identity. The final bundle is considered publishable only when both files are successfully materialized and the corresponding history record is durably committed. Partial artifacts remain incomplete and are not exposed as a trusted final report.

## 5. Evidence presentation cleanup
Runtime log severity/state is informational unless the producer explicitly supplies warning/failure semantics; default log state must not render as GOOD/PASS.

If the Event Log provider did not successfully query, WHEA/Disk/stornvme/Display/BugCheck counts shall render unavailable rather than zero.

## 6. Release hygiene
The feature branch must be reconciled with current `main` before merge. The obsolete write-enabled one-shot S01/S04 workflow on `main` must not survive the final merge. Legacy S01-S23 renderer fallbacks should be removed from the production executable after canonical v2 routing is verified.

## 7. Validation strategy
All bug fixes follow RED -> GREEN tests. Add behavioral/security regression coverage for:
- screen-aware keyboard dispatch and S11 CTA;
- explicit executable launch/argument quoting;
- duplicate manifest/reparse/path replacement cases;
- persistent journal root and journal failure semantics;
- bounded/validated history records and transactional update behavior;
- report bundle publication gating;
- Event Log unavailable/default runtime-log presentation semantics.

Final merge gate:
1. all source sanity tests pass;
2. strict MSVC x64 Release build passes with warnings as errors;
3. behavioral + trust security CTests pass;
4. inventory-only preflight passes;
5. portable package/integrity gate passes;
6. no temporary write-enabled workflow remains;
7. real-machine validation remains explicitly required before declaring production-ready.