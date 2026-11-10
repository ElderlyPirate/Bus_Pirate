# DIO

**Applies to:** v4 (documented); v3 is build-only (unverified).

DIO is direct pin control: the terminal sends the same pin values as the
binary bitbang layer to set each I/O pin's **direction** and **state** by
hand.  Use it for custom protocols and pin-level debugging when no packaged
mode fits.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | entering DIO, the mode number, the (empty) setup, `W` engagement |
| [wiring.md](wiring.md) | the pin header per board, which pins DIO drives, voltage limits, power, pull-ups |
| [pin-control.md](pin-control.md) | the write value format and full bit map, worked sessions |
| [read-back.md](read-back.md) | `r` and `v`, reading external signals, write-with-read |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- Enter with `m` then `10` on v4 (`m10`), or `m` then `9` on v3 (`m9`); the
  prompt becomes `DIO>`.  There is **no settings menu** — the mode starts
  immediately.
- `W` engages the on-board supply (`Clutch engaged!!!`); `w` releases it.
  `w` does **not** park the pins — use `#` for that.
- Type a value to drive the pins.  A value with **bit 7 clear**
  (`0x00`–`0x7F`) sets pin **direction**; a value with **bit 7 set**
  (`0x80`–`0xFF`) sets pin **state** (high/low) plus power and pull-up.
- The five controlled pins are `AUX`, `MOSI`, `CLK`, `MISO`, `CS`, mapped
  from bit 0 (`CS`) to bit 4 (`AUX`) — see
  [pin-control.md](pin-control.md).
- Read the pins with `r` (levels in the compact layout) or `v` (levels,
  directions and rails) — see [read-back.md](read-back.md).  DIO has no
  **mode** macros (`(n)`); shared user macros `<n>` still run.
- Safety: set a pin to input (direction bit = 1) before attaching an external
  driver; never drive a pin against an external source.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | present (default image; build-only) | present (default image) |
| mode number | 9 (unverified) | 10 |
| controlled pins | `AUX` `MOSI` `CLK` `MISO` `CS` | `AUX` `MOSI` `CLK` `MISO` `CS` |
| `AUX1` / `AUX2` pins | — | on the header but **not** in the DIO value |
| settings menu | none | none |
| hardware needed | a target or probe on the header pins | a target or probe on the header pins |

Full detail is in the topic files.

## Limits & out of scope

- No **mode** macros (`(n)`) and no bus wrappers (`[` `{` `]` `}`, `/` `\`
  `-` `_` `.` `^` `!`) — those are no-ops in DIO and report an error.  Shared
  user macros `<n>` are available.  See [edge-cases.md](edge-cases.md).
- No bus protocol, framing, or timing: DIO only sets a pin's direction and
  level; the host controls timing.
- No binary sub-protocol.  DIO shares the *value* bit map with the binary
  bitbang layer, but the binary command codes themselves do not apply in DIO.
- DIO outputs are push-pull only, so the on-board VPU source cannot be
  selected from DIO; feed VPU externally if the pull-ups are used (see
  [wiring.md](wiring.md)).
- The v4 `AUX1` / `AUX2` header pins cannot be driven by a DIO value.
- Shared prompt grammar (`m`, `W`/`w`, `P`/`p`, `v`, `r`, `#`, `$`, `(n)`,
  `<n>`) lives in `docs/generic/` and is not repeated here.

## Related docs

- `docs/generic/` — the shared prompt:
  [commands.md](../../generic/commands.md),
  [navigation.md](../../generic/navigation.md),
  [usage.md](../../generic/usage.md).
- `docs/SPEC.md` — the capability index.
- [docs/protocol/binary/](../binary/README.md) — the binary bitbang layer
  whose pin values DIO shares.
