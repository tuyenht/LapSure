# Round 5.1 A+ — Tranche Map

Normative architecture:
`docs/superpowers/specs/2026-08-25-decision-authority-happy-path-closure-design.md`

This file is a review map only. It does not authorize implementation before the A+ design receives final approval.

## 5.1A — Decision Authority Core
- authority model and trusted resolver boundary
- `Present / AbsentConfirmed / Unknown` capability truth
- seller-claim precedence
- immutable/versioned `RequirementSnapshot`
- session port attestation with anti-edit-away semantics
- `DecisionContext`
- one requiredness source for coverage + scoring
- advisory-chassis `BUY WITH NOTES`
- runtime-validation semantics
- remove macro trust routing
- production-path reachability tests

## 5.1B — Trusted Provider Execution
- provider license/provenance review
- embedded/protected provider trust root
- dependency/bundle closure
- TOCTOU-safe verify/execute boundary
- GPU/thermal provider integration
- deterministic security/failure-injection tests
- package provider provenance

## 5.1C — Acceptance & Release Closure
- report/UI authority/capability/policy semantics
- DPI/accessibility matrix correction
- exact package/provenance candidate
- short Precision smoke
- full physical acceptance
- main branch protection/equivalent ruleset
- final review, Ready/Merge, final-main package

Use `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6` as the logical Round 5.1 review baseline and bounded compare ranges for each tranche.
