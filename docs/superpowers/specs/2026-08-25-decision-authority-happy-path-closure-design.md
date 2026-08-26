# Round 5.1 A+ — Decision Authority, Capability Truth & Happy-Path Closure

**Status:** Proposed revision A+ — awaiting final approval; implementation not started  
**Branch:** `feature/s01-s04-visual-alignment-v2`  
**Review baseline:** `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6`  
**Parent runtime/security baseline:** Run #592 / commit `d7944104b90f6290f8f444de572f06a90d16a676`  
**Purpose:** Close the acceptance-path deadlock without weakening provenance, capability truth, provider trust or evidence semantics.

## 1. Problem statement

Round 5 correctly removed mutable portable factory/chassis metadata from acceptance authority. That security fix exposed a second-order product problem:

1. scoring currently uses a mutable-text chassis status as both catalog authority and machine-session completeness;
2. production correctly strips portable `physical-verified` authority, making clean acceptance unreachable without another authority source;
3. GPU/VRAM and CPU thermal acceptance can require provider evidence that the portable candidate does not currently have a production-trusted path to obtain;
4. existing tests prove scoring and security components separately, but do not prove that the secure production composition has reachable intended verdicts;
5. capability absence is not yet represented distinctly from capability detection failure/unknown;
6. external-engine SHA verification currently reads its expected hash from a mutable portable manifest, which is not a cryptographic trust root.

Round 5.1 A+ must preserve the governing invariant:

> No mutable portable file, mutable manifest, operator action or unauthenticated network response may create reusable certification authority or trusted provider authority by self-assertion.

## 2. Goals

Round 5.1 A+ must:

- make `BUY WITH NOTES` reachable for a healthy machine with complete trustworthy session evidence even when reusable chassis authority is only advisory;
- reserve clean `BUY` for complete evidence plus protected `Certified` chassis authority and no material warnings;
- keep `REJECT` dominant for critical machine/seller-claim failures;
- keep `INCOMPLETE` dominant for missing, unavailable, unknown, cancelled, inconsistent or untrusted required evidence and for LapSure self-validation/integrity failures;
- represent capability state as `Present`, `AbsentConfirmed` or `Unknown`, never equating unknown with absent;
- derive one immutable, versioned `RequirementSnapshot` for both coverage and decision;
- separate factory/configuration provenance, chassis/catalog authority, session attestation and purchase-health evidence;
- prevent operator corrections from deleting expected coverage silently;
- replace macro-based trust routing with explicit typed decision-facing APIs;
- replace mutable portable engine-manifest authority with a protected/embedded provider trust root before any provider can become purchase-grade evidence;
- close verify/execute TOCTOU and provider dependency-closure risks before enabling elevated external providers;
- version decision/coverage/authority policy in persisted reports;
- add compiled production-composition reachability/security tests;
- keep Run #592 as historical runtime/security/package baseline only.

## 3. Non-goals

Round 5.1 A+ will not:

- weaken fail-closed profile provenance;
- treat editable `.profile`, JSON or manifest text as certification;
- infer functionality from device presence alone;
- fabricate GPU, thermal, port, network or other missing evidence;
- silently download runtime tools;
- allow a test fixture to establish production certification authority;
- claim one tested machine certifies an entire model family;
- redesign all Win32 UI internals unless physical/accessibility smoke proves a blocker;
- complete long-term cloud signing infrastructure beyond interfaces required by the new authority model.

## 4. Decision ordering and truth domains

Decision precedence must be deterministic.

### 4.1 Critical machine or seller-claim failure → `REJECT`

Trusted evidence of a release-defined critical machine failure, functional failure, stability failure, safety failure, material seller misrepresentation or required-port failure produces `REJECT`.

A missing provider or LapSure self-integrity failure must not be misreported as a laptop defect.

### 4.2 LapSure/evidence integrity failure → `INCOMPLETE`

Any failure of LapSure runtime validation, decision-context integrity, required-provider trust, persisted-evidence consistency or other tool-side evidence authority makes the result invalid/incomplete, not `REJECT`.

### 4.3 Missing required machine evidence → `INCOMPLETE`

Required evidence that is not run, cancelled, unknown, unavailable, untrusted or internally inconsistent keeps the decision `INCOMPLETE`.

### 4.4 Complete machine evidence + advisory chassis → `BUY WITH NOTES`

A healthy machine may reach `BUY WITH NOTES` when all capability-aware required evidence is complete and its actual session port inventory is fully attested/tested, even if reusable chassis/catalog authority is only advisory.

### 4.5 Complete machine evidence + certified chassis → `BUY`

Clean `BUY` additionally requires protected `Certified` chassis authority and no material purchase-relevant warning.

## 5. Capability truth model

Boolean presence is insufficient for conditional acceptance.

Introduce an explicit state equivalent to:

- `Present` — trusted/native evidence establishes the capability is present;
- `AbsentConfirmed` — the relevant detection contract completed successfully and establishes absence;
- `Unknown` — detection failed, was unsupported, was incomplete or cannot establish presence/absence.

Invariant:

> `Unknown` must never be treated as `AbsentConfirmed`.

The model applies first to discrete GPU and should be used for other conditional domains where correctness requires it: battery, touch, camera, Wi-Fi, Bluetooth and similar hardware.

## 6. Seller-claim precedence

Seller claims participate before optional-capability waiver.

For a claimed dGPU:

1. if trusted inventory shows a conflicting/missing claimed dGPU and policy classifies the mismatch critical, produce `REJECT`;
2. otherwise, if dGPU presence is established or remains required by a non-conflicting claim, GPU/VRAM integrity evidence is required;
3. provider absence cannot hide a proven seller mismatch by downgrading it to `INCOMPLETE`.

The same principle applies to other material seller claims.

## 7. Chassis authority model

Introduce explicit authority equivalent to:

- `None`;
- `Advisory`;
- `Certified`.

### 7.1 Portable input ceiling

Mutable portable `.profile` data, heuristic model matching and operator-entered catalog text may produce at most `Advisory`.

### 7.2 Certified authority must be minted by a trusted resolver

`Certified` must not be a public caller-settable field whose value alone creates authority.

Production authority must be created only by a decision-facing resolver that validates a protected trust source. Possible future sources include embedded/hash-pinned/signed metadata or another protected installation boundary.

Raw parsers must never return production `Certified` authority.

UI/operator actions must never mint reusable `Certified` authority.

Tests that need a certified case must use an explicit test-only trusted-authority fixture/factory that cannot be invoked by production runtime paths.

## 8. Factory authority remains separate

Factory profile authority answers configuration provenance/comparison questions only.

Missing authenticated factory truth does not by itself make a machine-health decision incomplete when all purchase-safety requirements are complete.

Seller-claim comparison remains separate from factory authority.

## 9. Session-specific physical port attestation

Session attestation is evidence for the current inspected machine only; it is not reusable model certification.

### 9.1 Preserve expected and observed inventories separately

Do not mutate/delete the expected advisory inventory when the operator observes something different.

Persist at least:

- expected/advisory inventory;
- observed/attested inventory;
- per-port presence confirmation;
- per-port requiredness for this `RequirementSnapshot`;
- stimulus/test result;
- discrepancy status when expected and observed differ;
- operator correction reason;
- timestamp;
- current inspection/session identity;
- provenance such as `operator-attested-session`.

### 9.2 No edit-away coverage

If an expected required port is not observed, record an explicit discrepancy such as `EXPECTED_PORT_NOT_OBSERVED`; do not silently remove it and recompute a smaller denominator.

Operator additions/removals/corrections must be auditable and must not gain reusable catalog authority.

### 9.3 No cross-session promotion

Attestation from history/reopened reports is untrusted persisted evidence until explicitly validated and must not automatically become current-session acceptance evidence.

## 10. Versioned `RequirementSnapshot`

Create one immutable requirement snapshot after evidence normalization and seller-claim comparison.

Inputs include:

- normalized observed capabilities;
- seller claim;
- inspection mode;
- chassis guidance/authority;
- policy versions;
- release-defined mandatory domains.

The snapshot records each domain as at least:

- `Required`;
- `NotApplicable`;
- optionally `ConditionalBlocked` when requiredness cannot be resolved because capability state is `Unknown`.

Both `BuildCoverageContract()` and `BuildAuditDecision()` must consume the same snapshot. Neither may independently recompute requiredness from mutable report contents.

## 11. Policy versioning

Persist explicit version fields with every report/bundle, equivalent to:

- `decisionPolicyVersion`;
- `coveragePolicyVersion`;
- `authorityPolicyVersion`.

Reports must remain interpretable after later policy changes.

A policy version must change when verdict semantics or requiredness semantics materially change.

## 12. GPU/VRAM requiredness

GPU/VRAM integrity is required when:

- discrete GPU state is `Present`; or
- a non-rejected seller claim still requires a discrete GPU.

If dGPU state is `Unknown`, the system may not waive GPU evidence merely because the GPU list is empty.

If dGPU state is `AbsentConfirmed` and there is no unresolved dGPU claim, GPU/VRAM stress is `NotApplicable` and must not create artificial incompleteness.

When GPU/VRAM is required, missing/untrusted/cancelled/invalid GPU stress evidence keeps the decision `INCOMPLETE`.

## 13. Thermal requiredness

Any purchase-grade verdict after CPU sustained-load execution requires contract-valid trusted CPU thermal evidence.

Current Quick/Standard/Deep modes all execute CPU load, so absent trusted CPU thermal evidence prevents `BUY` and `BUY WITH NOTES` in those modes.

Quick may still be used for runtime smoke, but it cannot close purchase acceptance without the required thermal evidence.

No synthetic or inferred package temperature is permitted.

## 14. Provider trust root — portable Beta

`tools/engine_manifest.txt` is configuration/provenance text, not a production trust root.

Before enabling an external provider for purchase-grade evidence, expected provider identity must come from a protected authority, preferably an allowlist generated at build time and embedded/compiled into `LapSure.exe` for the portable Beta.

The protected provider record should bind at least:

- logical provider name;
- expected SHA-256 of executable;
- expected provider version/build identity where practical;
- expected hashes/identities of private loadable dependencies or a bundle digest;
- provenance/license metadata identifier.

`engine_manifest.txt` may remain human-readable package metadata, but changing it must not change runtime trust authority.

## 15. Provider dependency closure

Hashing only the top-level `.exe` is insufficient when the provider can load writable private DLLs/sidecars/configuration that affect execution.

The trust design must either:

- verify the complete private provider bundle/dependency set; or
- execute from a release-controlled provider layout whose loadable dependency boundary is explicitly constrained and verified.

Provider output parsing/contract validation remains separate from binary trust.

## 16. Verify/execute TOCTOU closure

Re-verification immediately before process launch is necessary but not sufficient if a writable provider file can be replaced between hashing and `CreateProcess`.

Round 5.1B must implement and test a Windows-safe execution boundary, with preference for:

- opening provider artifacts with sharing semantics that prevent replacement/write/delete during verification/execution;
- hashing the exact opened artifact/bundle;
- retaining the protective handle(s) across process creation where Windows semantics support it;

or an equivalently protected isolated execution root that is copied, ACL-protected and reverified before launch.

The final implementation choice must have a regression/failure-injection test; no design may assume the race is negligible because the app is elevated.

## 17. Production typed architecture

Remove trust-sensitive preprocessor substitution such as `#define LoadFactoryProfile ...` and `#define LoadChassisProfile ...` from the production decision route.

Introduce explicit typed services/components equivalent to:

- `EvidenceNormalizer`;
- `ObservedCapabilities`;
- `DecisionProfileResolver`;
- `CapabilityRequirementPolicy`;
- `RequirementSnapshot`;
- `SessionPortAttestation`;
- immutable `DecisionContext`.

Raw loaders remain advisory/tooling parsers and cannot mint authority.

## 18. Report/UI semantics

Expose the distinctions rather than hiding them.

Recommended report/UI fields:

- Factory authority: Unknown / Advisory / Authenticated;
- Chassis authority: None / Advisory / Certified;
- Physical port evidence: Not attested / Partial / Session-attested complete;
- Capability state for conditional hardware: Present / Absent confirmed / Unknown;
- Required capability coverage: Complete / Partial with explicit missing domains;
- policy versions used for the decision.

For advisory-chassis `BUY WITH NOTES`, reason text must clearly say that this specific machine's ports were attested/tested while the reusable chassis catalog is not certified.

Session attestation must never be labeled as model certification.

## 19. Production-path reachability/security tests

A compiled suite must exercise the same typed production composition, not manufacture an impossible privileged state.

Minimum cases:

1. healthy test-only trusted certified authority + complete evidence → `BUY`;
2. healthy advisory chassis + complete current-session port attestation → `BUY WITH NOTES`;
3. advisory chassis + expected required port absent/untested → `INCOMPLETE`;
4. operator attempts to delete an expected required port → discrepancy retained, no clean acceptance;
5. dGPU `Present` + trusted GPU provider unavailable → `INCOMPLETE`;
6. dGPU `Unknown` + provider unavailable → `INCOMPLETE`, never waived as absent;
7. dGPU `AbsentConfirmed` + no claim → GPU test `NotApplicable` and no artificial blocker;
8. claimed dGPU contradicted by trusted inventory → critical seller mismatch `REJECT` where policy defines it critical;
9. CPU load + thermal evidence unavailable/untrusted → `INCOMPLETE`;
10. mutable chassis file self-declares certified/physical-verified → authority remains `Advisory`;
11. unauthenticated factory provenance absent while purchase-safety evidence is complete → factory absence alone does not block;
12. critical hardware/functional/stability failure → `REJECT`;
13. LapSure runtime/self-validation failure → `INCOMPLETE`/invalid result, not machine `REJECT`;
14. mutable portable engine manifest changed together with engine → provider still rejected because authority is embedded/protected;
15. provider executable/dependency replacement at the execution boundary → launch blocked;
16. coverage and decision consume the same requirement snapshot and report the same requiredness/reasons;
17. persisted policy versions round-trip through JSON/HTML/session history where applicable.

## 20. Implementation decomposition

Round 5.1 is intentionally split into three reviewable tranches.

### Round 5.1A — Decision Authority Core

Scope:

- authority types/resolver boundary;
- `Present / AbsentConfirmed / Unknown` capability truth;
- seller-claim precedence;
- `RequirementSnapshot` + policy versions;
- session port attestation with anti-edit-away semantics;
- immutable `DecisionContext`;
- one requiredness source for coverage + scoring;
- `BUY WITH NOTES` advisory-session path;
- runtime-validation semantics;
- remove macro trust routing;
- production-path RED/GREEN reachability tests.

5.1A does **not** enable third-party GPU/thermal binaries.

### Round 5.1B — Trusted Provider Execution

Scope:

- license/provenance review for selected GPU/thermal providers;
- embedded/protected provider trust root;
- full provider dependency/bundle closure;
- TOCTOU-safe Windows execution boundary;
- exact provider/package provenance;
- deterministic security/failure-injection tests;
- integration of trusted GPU/thermal evidence.

No silent runtime download.

### Round 5.1C — Acceptance & Release Closure

Scope:

- report/UI authority/capability/policy semantics;
- validation matrix and DPI/accessibility correction;
- exact package/provenance candidate;
- short Precision smoke;
- full formal physical acceptance after smoke closure;
- main branch protection/equivalent ruleset;
- final review/Ready/Merge/final-main package.

## 21. Review baseline and PR strategy

PR #2 is already a large historical hardening PR. Do not create unnecessary branch churn solely to simulate a sub-PR when the current workflow is scoped to PRs targeting `main`.

Use commit `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6` as the logical Round 5.1 review baseline.

Review each tranche using bounded compare ranges from that baseline/tranche checkpoint instead of re-reviewing the entire PR history.

PR #2 remains Draft until Round 5.1 plus physical acceptance closes.

## 22. Physical-pilot strategy

Artifact #592 remains a runtime/security/package smoke baseline, not the final formal-acceptance candidate.

Validation order after implementation:

1. 5.1A source/unit/reachability/security tests;
2. strict MSVC `/W4 /WX` checkpoint;
3. 5.1B provider trust/security tests and integration when providers are ready;
4. inventory transactional preflight;
5. exact package/provenance checkpoint;
6. short Precision smoke;
7. accessibility smoke before expensive full display matrix;
8. full formal physical acceptance only if no unresolved P0/P1 blocker.

If provider integration is incomplete, smoke may legitimately remain `INCOMPLETE`; that validates fail-closed runtime behavior but cannot close purchase acceptance.

## 23. DPI/accessibility acceptance

Use Precision-representative targets:

- 1366×768 at 100/125/150% for compatibility viewport;
- 1920×1200 at 100/125/150% for FHD+ Precision class;
- 3840×2400 at 150/200/225% where hardware permits for UHD+ Precision class;
- 1920×1080 as optional compatibility regression, not the primary Precision target.

Run keyboard/focus/accessibility smoke before the full matrix. Accessibility failures are discrepancies, never inferred PASS.

Full UI Automation work is conditional on smoke evidence unless independently elevated to a blocker.

## 24. Documentation/package-state policy

Use:

- README/product docs for stable policy;
- GitHub PR/issues for live gate state;
- generated `BUILD_INFO.txt`/provenance for exact artifact state;
- validation records for physical evidence.

Do not embed volatile current-run instructions in frozen runtime packages as if they were timeless policy.

Package slimming remains post-5.1 unless package content creates a usability/trust blocker.

## 25. Repository/release governance

Before final merge:

- protect `main` or apply an equivalent ruleset;
- prevent force push and branch deletion;
- require PR-based integration;
- require conversation resolution where applicable;
- configure checks without defeating Draft/cost-control policy;
- preserve exact provenance across merge strategy and run a final-main package checkpoint.

## 26. Acceptance criteria

Round 5.1 is complete only when:

- mutable portable profile data cannot grant `Certified` authority;
- `Certified` can only be minted from a protected production authority source;
- capability `Unknown` cannot be treated as absent;
- seller-claim critical mismatch precedence is deterministic;
- operator port correction cannot silently shrink required coverage;
- coverage and scoring share one immutable `RequirementSnapshot`;
- decision/coverage/authority policy versions are persisted;
- healthy advisory-chassis + complete session evidence can reach `BUY WITH NOTES`;
- clean `BUY` remains reserved for protected certified authority + complete evidence;
- LapSure self-integrity failures yield incomplete/invalid results rather than laptop rejection;
- dGPU/thermal requiredness remains fail-closed;
- purchase-grade external-provider trust is rooted outside mutable portable manifest text;
- provider dependency closure and verify/execute race are addressed and tested before enabling elevated providers;
- compiled production reachability/security tests cover intended verdicts and trust failures;
- no previous profile-provenance regression is weakened;
- a new exact package is generated after production changes;
- short smoke precedes full physical acceptance;
- `main` governance is protected before merge.

## 27. Proposed design decision

Adopt **Round 5.1 A+ — Decision Authority, Capability Truth & Happy-Path Closure**:

> A specific laptop with complete trustworthy session evidence may reach `BUY WITH NOTES` when reusable chassis guidance is only advisory. Clean `BUY` requires protected certified chassis authority. Capability unknown is never absence. Missing required dGPU/thermal evidence remains `INCOMPLETE`. Machine failures and LapSure self-integrity failures remain separate truths. External provider trust must be anchored outside mutable portable manifest text before provider evidence can become purchase-grade authority.

This revision is proposed for final approval before implementation planning.
