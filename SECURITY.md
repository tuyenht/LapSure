# Security Policy

LapSure executes low-level diagnostic workflows and may invoke optional external diagnostic engines. Reports can contain model, serial/service-tag and hardware-identifying information and should be handled accordingly.

## External engine policy
- No silent runtime downloads.
- Reviewed binaries must be pinned by SHA-256 in `tools/engine_manifest.txt`.
- A hash mismatch blocks execution.
- Missing providers remain NOT TESTED/WARNING.

## Reporting a vulnerability
Avoid posting sensitive device identifiers or exploit details in a public issue. Contact the repository owner privately through an appropriate GitHub contact channel, then provide a minimal reproducible description and affected version/commit.

## Release status
The project is beta and must not yet be treated as a hardened security product or forensic-certification tool.