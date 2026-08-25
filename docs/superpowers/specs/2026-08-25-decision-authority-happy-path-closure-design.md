# Round 5.1 — Decision Authority & Happy-Path Closure Design

**Status:** Design approved in principle; implementation not started  
**Branch:** `feature/s01-s04-visual-alignment-v2`  
**Parent candidate:** Run #592 / commit `d7944104b90f6290f8f444de572f06a90d16a676`  
**Purpose:** Close the architectural gap between fail-closed profile provenance and a reachable, evidence-correct purchase decision.

## 1. Problem statement

Round 5 correctly removed mutable portable factory/chassis metadata from acceptance authority. That security fix created a second-order product problem that existing tests did not model end to end:

1. `BuildAuditDecision()` currently forces `INCOMPLETE` whenever a populated chassis profile is not `physical-verified`.
2. `LoadDecisionChassisProfile()` intentionally downgrades mutable disk `physical-verified` to `static-unverified`.
3. The portable package does not currently contain trusted GPU/VRAM or CPU thermal provider binaries, while production stress/coverage can require their evidence.
4. Existing behavioral tests can manufacture an in-memory `physical-verified` profile and omit the production GPU stage, proving scoring behavior but not proving that the real production composition has a reachable healthy-machine path.

The result is an acceptance-path deadlock: a physically healthy laptop can remain permanently `INCOMPLETE` even after the operator has completed every real stimulus available in the session.

This design must preserve the Round 5 security invariant: **no mutable portable file may self-create certification authority or clean acceptance**.

## 2. Goals

Round 5.1 must:

- make `BUY WITH NOTES` reachable for a healthy machine whose session evidence is complete even when the chassis catalog entry is advisory rather than certified;
- reserve clean `BUY` for complete session evidence plus trusted/certified chassis authority and no material warnings;
- keep `REJECT` dominant for critical failures;
- keep `INCOMPLETE` dominant whenever required evidence for the actual machine is missing, unavailable, cancelled or untrusted;
- make required coverage capability-aware instead of creating artificial `NOT TESTED` blockers for hardware that is not present;
- require GPU/VRAM integrity evidence when a discrete GPU is present or explicitly claimed;
- require reliable thermal evidence for purchase-grade stress acceptance;
- keep factory provenance separate from machine-health acceptance;
- add a production-composition reachability regression so CI can prove that the secure path can actually reach every intended verdict;
- keep artifact #592 as historical runtime/security evidence, not as the final formal-acceptance package.

## 3. Non-goals

Round 5.1 will not:

- weaken external-engine SHA-256 verification;
- treat an editable `.profile` file as model certification;
- convert PnP presence into functionality;
- fabricate missing thermal, GPU, port or provider evidence;
- introduce silent runtime downloads;
- redesign the complete UI system;
- solve long-term signed cloud/profile distribution beyond the interfaces required for authority separation;
- claim that one successful laptop session certifies an entire model family.

## 4. Core model: separate session evidence from catalog authority

The decision engine must stop using one string field such as `validationStatus == "physical-verified"` as both a catalog-trust signal and a machine-session completeness signal.

### 4.1 Chassis catalog authority

Introduce an explicit authority concept with values equivalent to:

- `None` — no recognized chassis guidance;
- `Advisory` — mutable/static/heuristic guidance, useful for operator workflow but not certification;
- `Certified` — protected authority from a future authenticated mechanism such as embedded/hash-pinned/signed metadata or another protected installation boundary.

A portable `.profile` file may produce at most `Advisory` authority.

### 4.2 Session port attestation

Introduce session-specific evidence representing the operator's confirmation of the actual machine's physical port inventory and the test result for each required port. This is not reusable model certification.

The attestation must contain, at minimum:

- inspection/session identity;
- model string used for guidance;
- the set of physical ports presented to the operator;
- whether each required port was confirmed present;
- test/stimulus result for each confirmed required port;
- explicit operator confirmation timestamp/state;
- provenance label such as `operator-attested-session`;
- no raw public machine identifier requirement.

The attestation may be initialized from an advisory chassis profile, but the operator must be able to correct the actual port inventory before final completion.

### 4.3 Factory authority remains separate

Factory profile authority continues to answer configuration provenance/comparison questions only. Missing authenticated factory truth must not by itself make a healthy machine `INCOMPLETE` when all purchase-safety domains are complete.

Seller-claim mismatch remains independent evidence and may still create a critical `REJECT` when the seller's explicit claim conflicts with observed hardware.

## 5. Verdict semantics

Decision ordering must be explicit and deterministic.

### 5.1 REJECT

`REJECT` wins when any release-defined critical hardware, functional, stability, port/power, seller-claim or runtime-integrity failure is present.

No authority or completeness upgrade can override a critical failure.

### 5.2 INCOMPLETE

`INCOMPLETE` is required when any capability-aware required domain for this machine/session is:

- not run;
- cancelled;
- unavailable where the domain is mandatory;
- untrusted where trusted evidence is required;
- missing required operator stimulus;
- missing required session port attestation;
- internally inconsistent;
- blocked by runtime-validation failure.

### 5.3 BUY WITH NOTES

`BUY WITH NOTES` is permitted only when:

- all capability-aware required machine/session evidence is complete;
- there is no critical failure;
- port/power evidence is complete for the operator-attested actual port inventory;
- runtime validation passes;
- chassis authority is `Advisory` or there are non-critical warnings/known limitations that must remain visible.

Required reason text for the advisory-chassis path must state that the specific machine's ports were manually attested/tested in this session and that the model profile itself is not certified.

### 5.4 BUY

`BUY` requires all `BUY WITH NOTES` evidence conditions plus:

- chassis authority is `Certified`;
- no material warning remains that policy classifies as purchase-relevant;
- no required coverage limitation remains.

This means Round 5.1 may make `BUY WITH NOTES` reachable before a signed/certified chassis catalog exists, while preserving a stronger meaning for clean `BUY`.

## 6. Capability-aware requiredness

Coverage must be derived from detected/claimed capabilities, not from a fixed stage list.

### 6.1 GPU/VRAM

GPU/VRAM integrity is required when either:

- a discrete GPU is detected by trusted/native inventory; or
- the seller explicitly claims a discrete GPU.

If a discrete GPU is required and the trusted GPU stress engine is absent, hash-invalid, cancelled or cannot produce contract-valid evidence, the decision remains `INCOMPLETE`.

If no discrete GPU is present or claimed, the production stress plan must not create a required `GPU / VRAM integrity = NOT TESTED` blocker merely because the optional engine is absent.

### 6.2 Thermal evidence

A purchase-grade verdict (`BUY` or `BUY WITH NOTES`) requires trusted thermal evidence for every session that executes the CPU sustained-load stage. Because Quick, Standard and Deep all execute CPU load in the current product, missing trusted CPU thermal evidence keeps the purchase decision `INCOMPLETE` in all three modes.

Quick may still be used as a runtime/smoke workflow when the thermal provider is unavailable, but that smoke result cannot close formal purchase acceptance.

Round 5.1 must not invent CPU package temperature. The trusted provider must return contract-valid package-temperature evidence, or formal purchase acceptance remains `INCOMPLETE`.

This requirement is explicit and must not be inferred merely from whether a provider binary happens to exist.

### 6.3 Other optional capabilities

The same pattern applies to touch, battery, camera, Bluetooth, Wi-Fi and other conditional domains: hardware that is not present must not create a missing-evidence blocker, while detected/claimed required hardware must be tested to the level defined by the coverage contract.

## 7. Trusted diagnostic provider strategy

Provider integration is a release input, not a reason to weaken coverage.

### 7.1 GPU engine

The current supported integration contract for `memtest_vulkan.exe` may be retained only if a specific reviewed binary is deliberately supplied with:

- reviewed license/distribution compatibility;
- exact SHA-256 in `tools/engine_manifest.txt`;
- package inclusion only when the expected binary and non-empty matching manifest entry are both present;
- launch-time re-verification through the existing trust boundary;
- provenance recorded in the generated package metadata.

No third-party binary is to be downloaded automatically at runtime.

### 7.2 CPU thermal provider

The `lhm_bridge.exe` path remains subject to the same release discipline: reviewed source/binary provenance, licensing, explicit package inclusion, manifest hash pinning and launch-time verification.

Until such a provider is supplied, a Precision session whose acceptance policy requires reliable CPU thermal evidence must remain `INCOMPLETE`.

### 7.3 Deterministic CI fixtures

CI must not depend on downloading third-party binaries. Reachability/security tests should use repository-built deterministic fixture executables with generated test manifests inside isolated temporary package roots. These fixtures prove production composition and trust semantics without pretending to validate real GPU/thermal hardware.

## 8. Production architecture

### 8.1 Remove decision routing by preprocessor substitution

The production app should stop depending on `#define LoadFactoryProfile ...` / `#define LoadChassisProfile ...` substitution as the trust boundary.

Introduce explicit typed decision-facing APIs/services. Exact names may vary, but the architecture should contain equivalents of:

- `DecisionProfileResolver` — returns factory/chassis data plus authority;
- `CapabilityRequirementPolicy` — determines which domains/stages are mandatory for the observed/claimed machine;
- `SessionPortAttestation` — session-local physical-port inventory/stimulus truth;
- `DecisionContext` — immutable inputs consumed by coverage/scoring.

Raw loaders remain available to tooling/tests only as advisory parsers.

### 8.2 Scoring consumes authority, not mutable strings

`BuildAuditDecision()` must not compare portable text such as `validationStatus == "physical-verified"` to decide acceptance authority.

It should consume explicit typed authority and completeness values from `DecisionContext`.

### 8.3 Coverage and decision share one requirement policy

`BuildCoverageContract()` and `BuildAuditDecision()` must derive requiredness from the same `CapabilityRequirementPolicy` so the UI/report cannot say a domain is optional while scoring silently blocks on it, or vice versa.

## 9. Report and UI semantics

Reports and UI must expose the distinction rather than hiding it.

Recommended visible fields:

- **Factory authority:** Unknown / Advisory / Authenticated;
- **Chassis authority:** None / Advisory / Certified;
- **Physical port evidence:** Not attested / Partially attested / Session-attested complete;
- **Required capability coverage:** Complete / Partial with explicit missing domains.

For `BUY WITH NOTES` caused only by advisory chassis authority, the reason must be explicit and non-alarming:

> All required ports on this specific machine were operator-attested and passed stimulus testing. The reusable model/chassis catalog entry is advisory, not certified.

The UI must never label session attestation as model certification.

## 10. Production-path reachability tests

A new compiled suite must exercise the same decision-safe composition used by production rather than constructing an impossible privileged in-memory state.

Minimum required cases:

1. **Healthy certified path** → `BUY`.
2. **Healthy advisory chassis + complete session port attestation** → `BUY WITH NOTES`.
3. **Advisory chassis + one required port untested** → `INCOMPLETE`.
4. **Discrete GPU detected + trusted GPU engine unavailable** → `INCOMPLETE`.
5. **No discrete GPU present/claimed + GPU engine unavailable** → GPU stage not required and must not create artificial incompleteness.
6. **CPU load executed + trusted thermal evidence unavailable** → `INCOMPLETE`.
7. **Mutable chassis file self-declares certified/physical-verified** → authority remains `Advisory`.
8. **Critical hardware/functional/stability failure** → `REJECT` regardless of authority.
9. **Factory provenance unavailable but all purchase-safety evidence complete** → does not alone force `INCOMPLETE`.
10. **Seller claim mismatch that policy marks critical** → `REJECT` independent of factory authority.

The suite must verify both final verdict and the reason/coverage fields that explain it.

## 11. Physical-pilot strategy

Artifact #592 is preserved as a **runtime/security smoke baseline**. It is not promoted as the final formal-acceptance candidate after this architectural finding.

Round 5.1 validation sequence:

1. source/unit policy tests;
2. strict MSVC `/W4 /WX` compile/link;
3. all existing security/process/report/history tests;
4. production-path reachability suite;
5. inventory transactional preflight;
6. package/provenance checkpoint on the exact Round 5.1 head;
7. one short physical smoke session on a Precision machine;
8. only after smoke closure, full formal physical acceptance matrix.

A physical smoke may legitimately end `INCOMPLETE` if required trusted GPU/thermal providers have not yet been integrated; that result validates fail-closed behavior but does not close purchase-decision acceptance.

## 12. DPI and accessibility acceptance correction

The pilot matrix must represent the actual Precision display families instead of relying primarily on 16:9 desktop modes.

Required matrix should include:

- compatibility viewport: 1366×768 at 100/125/150%;
- Precision FHD+ native class: 1920×1200 at 100/125/150%;
- Precision UHD+ native class: 3840×2400 at 150/200/225% where hardware permits;
- 1920×1080 retained only as an optional compatibility regression.

Accessibility smoke must happen before the full expensive hardware matrix. Custom-drawn primary controls must have a demonstrable keyboard/focus/accessibility path; failures are recorded as discrepancies rather than inferred PASS.

Full UI Automation implementation is outside this design unless the smoke shows that the current accessibility contract is not reachable.

## 13. Documentation and package-state policy

Live release state must not be embedded in static README text in a way that becomes stale inside a frozen artifact.

Use:

- `README.md` / product docs for stable policy and architecture;
- GitHub Issue/PR metadata for current gate state;
- generated `BUILD_INFO.txt` / package provenance for exact artifact state;
- validation records for physical evidence.

The package verifier must continue to bind the artifact to an exact commit.

Package slimming into separate runtime and validation kits is a post-Round-5.1 optimization unless package content itself blocks pilot usability or trust.

## 14. Repository/release governance

Before final merge to `main`:

- `main` must have branch protection or an equivalent ruleset that prevents accidental direct/force pushes and requires PR-based integration;
- required checks must be configured so they do not defeat the existing Draft/cost-control policy;
- PR #2 remains Draft until Round 5.1 package plus physical acceptance permits Ready;
- no merge/rebase/squash decision may invalidate exact provenance without an explicit final-main package checkpoint.

## 15. Migration plan

Implementation should proceed in narrow, independently testable slices:

1. introduce authority/requirement/session-attestation data types without changing verdicts;
2. add RED reachability tests that demonstrate the current deadlock;
3. route production through typed decision-facing APIs and remove macro trust substitution;
4. make coverage/scoring capability-aware;
5. implement advisory-session-attested `BUY WITH NOTES` semantics;
6. integrate provider packaging only after provenance/licensing review;
7. update report/UI reason fields and validation matrix;
8. run strict compiled/security checkpoint;
9. create a new exact package/provenance candidate;
10. run smoke pilot, then full physical acceptance.

Each slice must preserve the existing fail-closed profile provenance regression.

## 16. Acceptance criteria for Round 5.1 design implementation

Round 5.1 is implementation-complete only when all of the following are true:

- no mutable portable profile can grant `Certified` authority;
- a healthy advisory-chassis machine with complete session-specific port evidence can reach `BUY WITH NOTES` in the production composition;
- clean `BUY` remains reserved for certified authority plus complete evidence;
- discrete-GPU requiredness is capability-aware and missing trusted GPU stress evidence blocks acceptance;
- absent discrete GPU does not create a fake required GPU stage;
- CPU-load sessions cannot produce a purchase-grade verdict without trusted thermal evidence;
- coverage and decision consume the same requirement policy;
- production path no longer relies on preprocessor substitution for trust-sensitive loaders;
- compiled reachability/security tests cover all intended verdict classes;
- reports explain authority, session attestation and missing capability evidence without conflating them;
- #592 remains historical/smoke evidence and a new package is created for formal acceptance;
- `main` release governance is protected before final merge.

## 17. Design decision

Adopt **Decision Authority & Happy-Path Closure**:

> A specific laptop that has complete, trustworthy session evidence may reach `BUY WITH NOTES` even when its reusable chassis profile is only advisory. Clean `BUY` requires certified chassis authority. Missing required dGPU/thermal evidence remains `INCOMPLETE`; security gates are not relaxed to manufacture a positive verdict.
