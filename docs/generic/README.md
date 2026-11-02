# Using the prompt — the generic (base) terminal layer

**Status:** draft.  The v3 column is not verified.

This folder documents the shared CLI that every mode leans into: the prompt,
command grammar, the base-layer controls, and the entry sequences into the
binary modes.  Per-mode behaviour lives in `docs/<mode>/`.  The full capability
matrix — which board supports what — is in `docs/SPEC.md`.

## Getting connected

- The Bus Pirate is a USB serial device: a v4 presents a USB CDC virtual COM
  port, a v3 uses its FTDI cable.  It appears as `COM<n>` (Windows) or
  `/dev/ttyUSB*` / `/dev/ttyACM*` (Linux/macOS).
- Default link settings are **115200 baud, 8 data bits, no parity, 1 stop bit
  (8N1)**, no flow control.  The firmware boots at 115200 and the `b` menu
  default is option 9 = 115200.
- Open the port and press Enter: the default prompt is `HiZ>`.  Commands end
  with Enter.
- Drivers, building and flashing are developer topics — see `docs/dev/`
  (stub).

## Glossary

- **Clutch** — the bus-engagement metaphor.  "Clutch disengaged" means the bus
  is parked: after a mode change the new mode is inert and its I/O is not driven
  until the supplies are started with `W`, which prints "Clutch engaged!!!".
  `w` (supplies off) parks the bus again.
- **VPU** — the pull-up supply.  The pull-up resistors tie the I/O lines to
  whatever voltage is present on the physical **Vpullup** pin (the pin marked
  `Vpu`, "input pull-up resistors 0-5 V", on the reference card).  On v4 the `e`
  command can select that source (external/none, on-board 3.3 V, on-board
  5.0 V); v3 has no `e`, so the Vpullup pin must be fed externally — see
  [commands.md](commands.md) §Wiring.
- **AUX** — the auxiliary I/O pin: `a` drives it low, `A` high, `@` reads it.
  On v4 it can be reassigned among `A0` / `CS` / `A1` / `A2` (`c`/`C`/`k`/`K`;
  the target pins are shown as `AUX` / `CS` / `AUX1` / `AUX2`).

This README is a **map, not the manual** — every detail lives in the files
below:

| File | Covers |
|---|---|
| [navigation.md](navigation.md) | prompt lifecycle, line editing, `i`, `?`, `m`, `b`, `o`; the v3/v4 delta table |
| [commands.md](commands.md) | wiring requirements, the complete reference tables (values, elements, macros, wrappers, bit ops, firmware-level), and leaving the CLI for binary/SUMP |
| [usage.md](usage.md) | worked terminal sessions (mode change, `W`/`P`, `e`/`v`, `$`, `#`, wrappers) |
| [edge-cases.md](edge-cases.md) | failure modes, warnings and known quirks |

## The five-minute version

- The prompt shows the current mode (`HiZ>`, `UART>`, …); commands end with
  Enter.
- `?` help · `i` version/status · `m` mode menu · `o` display format ·
  `b` baud.
- Elements: `W`/`w` supplies · `P`/`p` pull-ups · `a`/`A`/`@` AUX pin ·
  `l`/`L` bit order · `v` volts/pin states.
- Macros: `<n>` user, `(n)` mode.  Wrappers `[` `{` `]` `}`; reads `r`;
  bit ops `/ \ ^ - _ . !`.
- `#` reset · `$` bootloader · `s` BASIC.

**Pull-up note:** `P` only works if a voltage is already on the Vpullup pin —
select the on-board 3.3 V / 5.0 V source with `e2`/`e3` on v4, or feed the pin
externally on v3.  See [commands.md](commands.md) §Feeding the VPU pin.

A first session (abridged — the full transcript is in
[usage.md](usage.md)):

```
HiZ>m3
Set serial port speed: (bps) …          (answer six setup prompts; each defaults to 1)
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
UART>o1
Display format set
UART>v
Pinstates: …                            (rail + pin table)
UART>
```

## Availability (summary)

| Feature | v3 | v4 |
|---|---|---|
| command history `h` | 7 entries | 15 entries |
| user macros `<n>` | 3 slots × 24 B | 5 slots × 32 B |
| VPU pull-up source `e` | — | present |

Full delta table (buffers, `#`, help text): [navigation.md](navigation.md).

## Limits & out of scope

- Per-mode settings defaults, per-mode `r`/`[`/`]` semantics, per-mode pin
  tables and per-mode macros are **not** here — see `docs/<mode>/`.
- No pull-**down** command exists — the pull-up elements are on/off only and
  the output matrix offers open-drain or push-pull (a per-mode setup prompt),
  never a pull-down.
- No command-line editing beyond the keys in [navigation.md](navigation.md)
  (no word-delete, no tab completion).
- BASIC internals, the binary protocol, SUMP and self-test detail live on
  their own pages: [docs/basic/](../basic/README.md),
  [docs/binary/](../binary/README.md), [docs/sump/](../sump/README.md),
  [docs/self-test/](../self-test/README.md).
- The `o` help label and the `v` label-row control byte are firmware string
  defects, flagged but not fixed by this document.