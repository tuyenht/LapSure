# Round 5.1A — Decision Semantics Contract

This note supports the proposed Round 5.1 A+ architecture and is not implementation authorization.

## Verdict precedence
1. Critical trusted machine/seller-claim failure → `REJECT`.
2. LapSure self-integrity/runtime-validation/provider-trust failure → `INCOMPLETE` / invalid result.
3. Missing/unknown/untrusted required machine evidence → `INCOMPLETE`.
4. Complete evidence + advisory chassis + complete session port attestation → `BUY WITH NOTES`.
5. Complete evidence + protected certified chassis + no material warnings → `BUY`.

## Capability state
Conditional hardware uses `Present`, `AbsentConfirmed`, `Unknown`. `Unknown` never waives a requirement.

## Requirement snapshot
Coverage and decision consume the same immutable/versioned `RequirementSnapshot`; neither recomputes requiredness independently.

## Port attestation
Expected/advisory inventory and observed/attested inventory are preserved separately. Operator corrections never silently delete expected required coverage and always produce an auditable discrepancy/reason.

## Authority
Portable profiles, UI actions and operator attestation may not mint reusable `Certified` authority. Production `Certified` authority must originate from a protected resolver source. Tests use test-only trusted authority fixtures.

## Policy versioning
Reports persist decision, coverage and authority policy versions used to produce the verdict.
