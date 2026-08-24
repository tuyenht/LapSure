# Security Policy

LapSure executes low-level diagnostic workflows and may invoke optional external diagnostic engines. Reports can contain model, serial/service-tag and hardware-identifying information and should be handled accordingly.

## External engine policy
- No silent runtime downloads.
- Diagnostic engines are executed only from reviewed bundled paths under the LapSure application directory.
- Reviewed binaries must be pinned by SHA-256 in `tools/engine_manifest.txt`.
- PATH-discovered `smartctl.exe`, `nvidia-smi.exe`, or similarly named binaries are not trusted merely because they exist.
- A missing allowlist entry, empty/unconfigured hash, missing file, or hash mismatch blocks that external engine from execution.
- Missing providers remain NOT TESTED / UNSUPPORTED / INCOMPLETE as appropriate; missing tooling never becomes hardware PASS.
- System/vendor binaries may be supported later only through an explicit trust policy (for example, validated Authenticode publisher/path rules), not implicit PATH lookup.

## Report and persistence boundary
- Report/history paths are treated as untrusted persisted input when reopened or deleted.
- Open/delete operations must canonicalize and remain inside the configured report/history root with an allowed artifact extension.
- LapSure does not modify Windows TrustedPublisher or other trust stores at runtime.

## Privilege model
The current beta executable requests administrator elevation because some diagnostic providers require privileged access. This is a known broad privilege boundary. Production hardening must continue to minimize privileged operations and should move toward a standard-user UI plus a narrowly scoped privileged helper if practical without weakening diagnostic evidence.

## Reporting a vulnerability
Avoid posting sensitive device identifiers or exploit details in a public issue. Contact the repository owner privately through an appropriate GitHub contact channel, then provide a minimal reproducible description and affected version/commit.

## Release status
The project is beta and must not yet be treated as a hardened security product or forensic-certification tool.
