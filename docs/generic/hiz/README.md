# HiZ (idle mode)

**Status:** draft — v4 is the documented target; v3 is build-only and
unverified.

HiZ is the Bus Pirate's idle mode: every header I/O pin is high-impedance,
nothing is driven, and no supply or pull-up is on. It is the default mode after
power-up or reset and the entry point of the terminal — the prompt reads
`HiZ>`. This folder covers only what is HiZ-specific; the shared prompt grammar
and the general command tables live in `docs/generic/`.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [overview.md](overview.md) | what HiZ is, the pin-state model, safety, entering and leaving |
| [commands.md](commands.md) | what works in HiZ, what is refused, what is HiZ-only |
| [usage.md](usage.md) | worked sessions: the idle prompt, `v`, self-test, refused commands |
| [edge-cases.md](edge-cases.md) | failure modes: trigger → symptom → recovery |

## The five-minute version

- On boot and after a reset the default mode is HiZ; the prompt is `HiZ>`.
- In HiZ the header I/O pins (MOSI, CLK, MISO, CS) and the AUX pins are
  high-impedance (held as inputs) — nothing is driven, no supply and no
  pull-up is on. The selected AUX target is the one exception: `A`/`a` can
  still drive it (see [overview.md](overview.md)). That is the safe state to
  leave the board in.
- The general commands work here: `i ? m b o v t T f a A @ c C k K l L d D & %
  = | s # $`, user macros `<n>`, and the help screen. The full grammar is in
  `docs/generic/`.
- Refused in HiZ (all print `Command not used in this mode`): `W`/`w`
  supplies, `P`/`p` pull-ups, `g`/`S` PWM/servo, the `O` 1-Wire toggle, and
  (v4) `e`, the VPU pull-up source.
- HiZ-only: the self-test `~`. In any other mode it prints
  `Self-test in HiZ mode only`. See `docs/self-test/`.
- To leave, enter a mode with `m`/`m<N>`; to come back, enter HiZ with `m1`
  (its mode change resets pin state, pull-ups and supplies) or `#` (returns
  to HiZ without that board-state reset — see [overview.md](overview.md)).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (build-only) | present |
| default mode on boot / reset | yes | yes |
| self-test `~` | present | present |
| VPU pull-up source `e` | — (no such command) | present; refused in HiZ |
| AUX assignment `c`/`C` (`k`/`K`) | `AUX` / `CS` | `AUX` / `CS` / `AUX1` / `AUX2` — [mapping](overview.md#aux-targets-and-pin-names) |

The v3 column is unverified (v3 is build-only); the full v3/v4 delta is in
[overview.md](overview.md#v3--v4-differences).

## Limits & out of scope

- The shared prompt grammar, help screen, mode menu, line editing, value
  entry, wrappers and macros are owned by `docs/generic/` and are not repeated
  here. This set documents only what changes in HiZ.
- The self-test itself (its checks, jumpers and output) is owned by
  `docs/self-test/`; this set only covers entering it from HiZ.
- The binary bitbang layer and the SUMP logic analyzer have their own entry
  and reset behaviour — see `docs/binary/` and `docs/sump/`.
- There is no bus protocol in HiZ: no clocking, no data transfer, no bus
  transactions. Bus work belongs to `docs/<mode>/`.

## Related docs

- [../generic/README.md](../README.md) — the shared terminal prompt
  layer and the base command tables.
- [../self-test/README.md](../../capabilities/selftest/README.md) — the `~` self-test.
- [../SPEC.md](../../SPEC.md) — the capability index (`v3 | v4`).
- [../binary/README.md](../../protocol/binary/README.md) — the binary bitbang layer.
- Mode sets (`docs/1wire/`, `docs/uart/`, `docs/i2c/`, `docs/spi/`, …) —
  the bus modes you switch into from HiZ.