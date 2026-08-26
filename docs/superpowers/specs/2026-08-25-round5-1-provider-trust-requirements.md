# Round 5.1B — Provider Trust Requirements

This is a design-support note for the proposed Round 5.1 A+ architecture. It does not authorize provider integration before final A+ approval.

## Trust-root invariant
`tools/engine_manifest.txt` is not a production trust root in a writable portable directory. Purchase-grade provider trust must originate from protected authority, preferably a build-generated allowlist embedded in `LapSure.exe` for the portable Beta.

## Required binding
The protected provider record must bind the logical provider identity to:
- executable SHA-256;
- provider version/build identity where practical;
- private loadable dependency hashes or a bundle digest;
- provenance/license metadata identifier.

## Runtime verification
- canonical provider path under the controlled application/provider root;
- no PATH lookup;
- no reparse/traversal redirection;
- verify the complete provider bundle/dependency closure;
- close the verify/execute replacement race with locked-handle or equivalently protected execution semantics;
- validate provider output contract separately from binary trust;
- fail closed to `INCOMPLETE` when provider trust or output evidence is invalid.

## Test requirements
- mutable engine + mutable `engine_manifest.txt` changed together must still be rejected;
- executable replacement between verification and launch must be blocked;
- private dependency replacement must be blocked;
- malformed/duplicate provider authority records fail closed;
- missing provider when required yields `INCOMPLETE`, never hardware PASS/REJECT;
- CI uses deterministic repository-built provider fixtures and no network download.
