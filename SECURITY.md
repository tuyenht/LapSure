# Security Policy

LapSure executes low-level diagnostic workflows and may invoke optional external diagnostic engines. Reports can contain model, serial/service-tag and hardware-identifying information and should be handled accordingly.

## External engine policy
- No silent runtime downloads.
- Diagnostic engines are executed only from reviewed bundled paths under the LapSure application directory.
- PATH-discovered `smartctl.exe`, `nvidia-smi.exe`, or similarly named binaries are never trusted merely because they exist.
- `VerifyEngine()` canonicalizes the candidate under `tools/`, rejects traversal/reparse paths, requires one unambiguous manifest entry, validates an exact 64-hex SHA-256, and fails closed on missing/empty/malformed/duplicate entries.
- Every external engine must be re-verified immediately before launch. Launch should use an explicit canonical application path rather than relying on Windows command-line executable resolution.
- Dynamic external-tool arguments must use Windows argument quoting; raw provider output must not be concatenated as executable syntax.
- Missing or blocked providers remain NOT TESTED / UNSUPPORTED / INCOMPLETE as appropriate; missing tooling never becomes hardware PASS.
- System/vendor binaries may be supported later only through an explicit trust policy such as a validated Authenticode publisher/path rule, not implicit PATH lookup.

### Current external-engine release limitation
`tools/engine_manifest.txt` is configuration, not a cryptographic signature over itself. The repository intentionally leaves reviewed engine hashes empty, so optional third-party engines are disabled by default. A production release that enables external engines must additionally protect the engine + allowlist boundary, for example by installing under an ACL-protected location and/or validating a signed/embedded allowlist. Elevated execution from a user-writable portable directory containing a mutable engine and mutable manifest is not considered a production-trusted external-engine configuration.

## Cloud/profile privacy and provenance
- GUI audit and `--inventory-only` do not opt in to cloud network access. Cloud lookup is disabled by default at the API boundary so a missing local profile cannot silently transmit a Service Tag, model or vendor.
- Technician `--cache-tag` / `--pre-cache` is the explicit network opt-in path. Device identifiers supplied to that command may be sent to the configured OEM gateway and must be treated as sensitive data.
- OEM requests are bounded, percent-encoded, HTTPS-only, pinned to the configured LapSure OEM host and do not follow redirects. Responses and mutable cache files are size-bounded.
- A remote response must return the exact requested Service Tag. LapSure does not fill a missing remote identity from the request.
- HTTPS authenticates the transport endpoint; it is not authenticated profile provenance. Cloud responses and `profiles/cache` content therefore remain advisory and set no trusted factory truth in the current Beta.
- `LoadFactoryProfile()` uses reviewed static profile files only. Mutable cache content is excluded from the factory-comparison path until a signed/authenticated provenance mechanism is implemented.
- Future cloud factory truth requires both exact identity and authenticated provenance; neither a matching cache filename nor a matching unsigned JSON field is sufficient.

## Report and persistence boundary
- Report/history paths are treated as untrusted persisted input when reopened or deleted.
- Open/delete operations must canonicalize and remain inside the configured report/history root with an allowed artifact extension.
- LapSure does not modify Windows TrustedPublisher or other trust stores at runtime.

## Privilege model
The current beta executable requests administrator elevation because some diagnostic providers require privileged access. This is a known broad privilege boundary. Production hardening must continue to minimize privileged operations and should move toward a standard-user UI plus a narrowly scoped privileged helper if practical without weakening diagnostic evidence. Until that architecture is implemented, optional external engines remain fail-closed by default and release validation must treat broad elevation as an explicit limitation.

## Release signing
Runtime trust-store mutation is prohibited. Authenticode signing, certificate-chain validation, package hashes and release provenance belong to the build/release pipeline. A signing helper must not silently install its own certificate into TrustedPublisher on application startup.

## Reporting a vulnerability
Avoid posting sensitive device identifiers or exploit details in a public issue. Contact the repository owner privately through an appropriate GitHub contact channel, then provide a minimal reproducible description and affected version/commit.

## Release status
The project is beta and must not yet be treated as a hardened security product or forensic-certification tool.
