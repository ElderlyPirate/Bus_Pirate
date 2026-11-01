# Developer documentation

**Status:** stub — developer writing lives here (build, flash, bootloaders,
packaging, tooling).  Not part of the function spec.

- Source today: `Documentation/building-and-flashing-firmware.md`
  (migrate + fix: stale paths, v7.1 sample output, missing CLI build flow).
- Planned: `README.md` (build + flash), bootloader notes, tooling.
- See also [code-map.md](code-map.md) — maintainer code map for the user docs
  (file + symbol, no line numbers).
- SPEC: intentionally **not** listed in `docs/SPEC.md` (not device functions).

## Documentation verification

- Generic layer: verified against the v4 build `e4b2239` (bench runs,
  2026-11-01); v3 behaviour is derived from source only (no v3 hardware).
  Per-function caveats are stated inline in the user docs.
- Evidence logs from the bench live in the development workspace (outside this
  repository).  Summary records belong here, not in the user docs — a
  per-topic `verification.md` in the user set was removed for exactly that
  reason (it reads like a changelog and adds no user value).