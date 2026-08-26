# Round 5.1A Task 4.1 — Pre-Task-5 Authority Hardening

**Status:** Approved implementation checkpoint following post-Task-4 audit  
**Branch:** `feature/s01-s04-visual-alignment-v2`  
**Parent:** Round 5.1 A+ normative architecture  
**Purpose:** Close production-integration gaps discovered after Task 4 scoped GREEN before `RequirementSnapshot` can make advisory-session evidence acceptance-authoritative.

## 1. Why Task 4.1 exists

Task 4 proved the core `SessionPortAttestation` contract: stable expected IDs, anti-edit-away after initialization, explicit discrepancy, and stale-confirmation revocation. Post-GREEN production-path audit found two P1 gaps that are not contradictions of Task 4 core semantics:

1. the expected denominator can still be reduced *before* attestation initialization by mutable portable chassis metadata;
2. the guided UI can stop after a single successful probe because progression is gated by `PortPowerSummary.overall` instead of remaining expected-port attestation work.

Task 5 must not freeze or persist these transitional semantics.

## 2. Protected expected-port baseline

For currently supported Precision pilot models, required expected-port IDs are release-defined in compiled code. The initial protected pilot scope is:

- Dell Precision 5560;
- Dell Precision 5570;
- Dell Precision 7670.

Mutable portable chassis profiles remain advisory overlays only. They may improve display labels, side, connector and capability guidance, but they may not:

- delete a protected required port;
- change a protected required port to optional;
- replace a protected expected ID;
- create a smaller acceptance denominator.

Portable-only extra ports may remain visible as advisory/optional guidance, but do not gain release-defined requiredness merely from mutable text.

For models without a protected release baseline, Task 5 must not infer purchase-grade port completeness from a mutable profile denominator. Such requiredness remains unresolved/conditional until a protected baseline or an independently complete observed-inventory contract exists.

## 3. Guided-port production route

The canonical guided flow uses `SessionPortAttestation` as the progression authority.

- The visible Ports & Power primary action routes through the model-aware guided command.
- Guided continuation checks `RequiredPortsRemaining(SessionPortAttestation)`, not `PortPowerSummary.overall`.
- `PortPowerSummary` remains observed probe/power presentation evidence, not expected-denominator authority.
- The orchestrator's port progress uses session attestation rather than mutable `ChassisProfile::ports[].tested`.
- Stable `expectedPortId` is preferred for compatibility mirrors. Label fallback is allowed only for genuinely generic/ad-hoc results with no expected ID.

## 4. Attestation state transitions

Attestation state is monotonic only while evidence remains valid.

- `AbsentConfirmed` or `Unknown` invalidates any previous completed test/verdict for that expected port.
- A later stable-ID completed probe that establishes presence clears stale absence discrepancy/correction state.
- Session confirmation is revoked whenever any required expected port ceases to be complete.

## 5. Requirement helper semantics

`ConditionalBlocked` is acceptance-blocking. Any helper used by coverage/scoring must therefore treat both `Required` and `ConditionalBlocked` as needing resolution/evidence. `NotApplicable` is the only non-blocking disposition.

Task 5 may rename helpers for clarity, but no call site may equate `ConditionalBlocked` with optional/not-required.

## 6. Factory provenance guard for Task 5

`factoryExact` means exact identity/configuration matching; it is not authentication. Task 5 must not pass `factoryExact` as the authentication boolean to `DecisionProfileResolver`.

Decision-facing factory authority must derive from explicit trusted provenance (`TrustedExact()` / authenticated provenance evidence) carried through a typed field or equivalent resolver input. Exact-but-unauthenticated data remains at most Advisory.

## 7. Verdict precedence regression

Task 4.1 adds an explicit regression that a trusted critical machine/stability failure remains `REJECT` even if runtime validation also fails. Tool/runtime failure may invalidate issuance, but it must not rewrite a proven critical laptop defect into a non-machine verdict.

Task 5 will implement the full ordered lattice from the normative design.

## 8. TDD gates

RED must demonstrate at least:

1. mutable Precision profile can currently shrink/demote the expected denominator;
2. `ConditionalBlocked` is currently not treated as blocking by the requirement helper;
3. attestation correction transitions retain stale state;
4. critical hardware failure can currently be overwritten by runtime `INCOMPLETE`;
5. Ports & Power primary routing / guided continuation / orchestrator still depend on generic or mutable-mirror state.

GREEN must prove all of the above are closed while preserving Task 2–4 assertions and historical security/profile regressions.

## 9. Scope boundary

Task 4.1 does not implement Task 5 scoring migration, Task 6 macro removal, provider trust, package generation, physical pilot, release or merge. PR #2 remains Draft.
