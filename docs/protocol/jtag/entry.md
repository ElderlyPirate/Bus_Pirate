# JTAG — entry and settings

Part of the JTAG documentation set.  This page covers how both JTAG paths
are reached, what each path starts with, the settings each accepts, and
every way out.  TAP mechanics are in [tap.md](tap.md); the byte tables are
in [openocd.md](openocd.md) and [xsvf.md](xsvf.md).

## Scope

- Entry from the terminal prompt into the binary layer, then into a JTAG
  path.
- The state each path starts in.
- OpenOCD-path settings: pin mode, features, TCK delay, link speed.
- The XSVF player's fixed setup and player commands.
- The exit and recovery matrix.  This page owns it; other pages link here.

Not covered: the host tools' own configuration.  That belongs to
[tools/openocd](../../tools/openocd/README.md) and
[tools/xsvf](../../tools/xsvf/README.md).

## No terminal mode

JTAG has no mode-menu entry and no terminal command grammar.  The default
v4 menu lists `HiZ`, `1WIRE`, `UART`, `I2C`, `SPI`, `2WIRE`, `3WIRE`,
`KEYB`, `PIC` and `DIO`; JTAG is not among them.  The v3 build is the same
without `KEYB`.

The only entry is the binary layer.  Once it is active, terminal commands
are not interpreted until the board returns.

## Entry from the terminal

1. Get to a mode prompt such as `HiZ>`.  If a settings prompt is waiting,
   press Enter until the prompt appears.
2. Send **20 consecutive `0x00`** bytes.  The layer answers `BBIO1`.
3. Send `0x06` for the OpenOCD path or `0x18` for the XSVF player.

The entry bytes are not printable, so a terminal that sends only text cannot
enter.  Use a host tool, a small script, or a terminal program that can send
raw bytes.

The count is consecutive: any other byte resets it.  A run of fewer than
twenty NULs produces no reply and no error.  Extra NULs are not an error
either: every `0x00` sent to `BBIO1` re-prints `BBIO1`, so a host can send
NULs until the banner appears.

`bp xsvf` writes **21** NULs — one leading, then the twenty.  The leading
NUL leaves most binary sub-modes with a `0x00` exit byte.  It does **not**
leave the XSVF player, which ignores `0x00`: if the board is stuck there,
press reset.  The OpenOCD path does accept `0x00` as its exit, so the same
write recovers from it; the host drains the extra banners.

A typical OpenOCD entry and exit, as bytes on the link:

```
host -> 0x00 x20          enter binary bitbang
host <- "BBIO1"
host -> 0x06              select OpenOCD path
host <- "OCD1"
host -> 0x00              leave OpenOCD path
host <- "BBIO1"
host -> 0x0F              leave binary layer
host <- 0x01              return-to-terminal ack
host <- "HiZ>"            (version banner omitted)
```

`0x01` is a raw byte, not the text "1".  On v3 the final `0x0F` performs a
hardware reset instead of the v4 terminal reset.  The XSVF path uses
`0x0F` where OpenOCD uses `0x00`; see [Leaving](#leaving).

## What each path starts with

| Item | OpenOCD path (`0x06`) | XSVF player (`0x18`) |
|---|---|---|
| TDI, TMS, TCK | inputs (Hi-Z) | outputs, driven low |
| TDO | input | input |
| SRST (`AUX`) | input, latch low | as the binary entry left it |
| TRST | not changed by entry | not changed by entry |
| VREG | off | **on** |
| on-board pull-ups | off | as the binary entry left it |
| TCK delay | `1` | no setting |
| link rate | unchanged | unchanged |
| mode LED | on | on |
| TAP state kept | none | none |

The OpenOCD entry reset runs twice: once for the 20 NULs and again before
`OCD1` is printed.  A board that a terminal left with the supplies on loses
them at binary entry.  The XSVF selection does not repeat that reset; it
configures TDI, TDO, TCK and TMS only and switches VREG on.

SRST is one of the five binary-layer pins, so entry makes it an input with
its latch low.  TRST is not one of them: it keeps its state until a pin mode
takes it over (a terminal AUX1 write can also drive it on v4).  See
[wiring.md](wiring.md#reset-lines).

## OpenOCD path settings

`0x06` starts the `OCD1` command loop.  Settings are per session; there is
no settings menu and no read-back.

| Setting | Request | Default | Effect |
|---|---|---|---|
| Pin mode | `0x01` + mode | `0x00` Hi-Z | `0x00` inputs; `0x01` push-pull; `0x02` open-drain |
| Mode LED | `0x02 0x01` + level | on at binary entry | `0x00` off, any non-zero on |
| VREG | `0x02 0x02` + level | off | `0x00` off, any non-zero on |
| TRST | `0x02 0x04` + level | not changed by `0x06` | the action is the pin level |
| SRST | `0x02 0x08` + level | input, latch low | the action is the pin level |
| Pull-ups | `0x02 0x10` + level | off | ties the lines to Vpullup |
| TCK delay | `0x08` + 2 bytes | `1` | larger is slower; no reply |
| Link speed | `0x07` + selector + `0xAA 0x55` | rate unchanged | v3 UART rate; v4 rate unchanged |

`0x06` does not reset the mode LED, the link rate or TRST.  A fresh entry
from `BBIO1` returns the pin mode to Hi-Z, VREG and the pull-ups to off, and
the TCK delay to `1`.

### Pin mode and the reset lines

The pin-mode byte writes the five output latches low before it applies the
direction.  Selecting either JTAG mode therefore drives SRST and TRST low,
and a later pin-mode change does it again.  If either line is wired to an
active-low reset, release it **after** the pin mode:

| Line | Assert (usual active-low) | Release |
|---|---|---|
| TRST | `0x02 0x04 0x00` | `0x02 0x04 0x01` |
| SRST | `0x02 0x08 0x00` | `0x02 0x08 0x01` |

### Action bytes

The action byte is a level, not an "assert" flag, and the two reset lines
take only its low bit:

| Feature | Action rule |
|---|---|
| `0x04` TRST, `0x08` SRST | the low bit is the level; use exactly `0x00` or `0x01` |
| `0x01` LED, `0x02` VREG, `0x10` pull-ups | `0x00` off; any non-zero on |

TRST and SRST are commonly active-low, so `0x00` asserts and `0x01`
releases.  The firmware does not invert either line; check the target's
schematic.  The reset features change a latch until a pin mode makes the
pin an output.

Feature `0x10` switches the on-board pull-up resistors.  They tie the lines
to Vpullup, so a voltage must already be on that pin; feed it externally.
The terminal `P` command is only a pre-entry check: binary entry switches
the resistors off and terminal commands are not reachable here.  See
[wiring.md](wiring.md#pull-ups).

Feature `0x02` switches the same regulator that the XSVF entry turns on.  It
drives the `3.3V` and `5.0V` rails.  Leave it off when the target has its
own supply.

`0x08` and `0x07` are covered in [clock.md](clock.md).  The full command
set, including the TAP shift, is in [openocd.md](openocd.md).

## XSVF player settings

The player has no settings menu and no per-session settings.  Entry fixes
the electrical and stream setup:

| Item | Value |
|---|---|
| Pins | TDI, TCK and TMS 3.3 V push-pull outputs; TDO input |
| Open-drain mode | none |
| SRST / TRST | not driven |
| Pull-ups | not needed; entry does not change them |
| VREG | on at entry; stays on after the player returns |
| TCK | no host control |
| Stream | XSVF 5.01; chunks ≤ 4096 bytes; longest shift ≤ 400 bits |

Player commands:

| Request | Action | Reply |
|---|---|---|
| `0x01` | reset the chain, then Run-Test/Idle | — |
| `0x02` | probe the chain | count byte, then IDCODE bytes |
| `0x03` | play a stream | data handshake, then a result byte |
| `0x0F` | reset the chain and leave the player | none |
| other | ignored, including `0x00` | — |

On a v3 build, `0x18` is answered with a single `0x00` and the binary layer
stays active.  The full player protocol is in [xsvf.md](xsvf.md).

## Leaving

Every route out, in one place.  "Pins" means the five binary-layer pins:
`AUX`, `CS`, `MISO`, `CLK` and `MOSI`.

| From | Send | Reply | Effect | Not changed |
|---|---|---|---|---|
| `OCD1`, command loop | `0x00` | `BBIO1` | leaves; pins inputs; VREG/pull-ups off | TRST level |
| `OCD1`, middle of `0x05` | — | — | `0x00` and `0x0F` are payload, not commands | mode stays |
| `BBIO1` | `0x0F` | `0x01` | v4: terminal reset, banner, `HiZ>`; v3: hardware reset | — |
| XSVF player, command loop | `0x0F` | none | TAP reset; `BBIO1` silently | pins driven; VREG on |
| XSVF player, mid-run | — | — | `0x0F` is data or a busy byte | run aborts |
| XSVF player, to terminal | `0x0F`, then `0x0F` | none, then `0x01` | terminal; VREG off | — |
| any path, any time | reset button | — | terminal; pins inputs; VREG off | — |

Notes:

- A bare `0x00` does **not** release TRST.  Send `0x01 0x00` (Hi-Z) first,
  or complete the `0x0F` return to the terminal, which releases it.
- `0x0F` inside `OCD1` is not a command: it is answered with `0x00` and the
  mode stays.  Send `0x00` first, then `0x0F`.
- A mid-shift stop leaves the firmware waiting for payload.  Finish the
  payload or press reset; `0x00` and `0x0F` are consumed as data.
- The player ignores `0x00`, so the NUL entry sequence cannot recover a
  board that is inside it.  Press reset.
- After a reset, the terminal can power the rails again with `m` (any mode)
  then `W`; `W` is refused in `HiZ>`.

### Leftover reset levels

The XSVF player does not drive SRST or TRST, so a level set earlier can
hold the target.  Release it at the terminal:

| Leftover | Symptom | Terminal sequence |
|---|---|---|
| SRST (`AUX`) low | target held in system reset | `#`, then `c`, then `A` |
| TRST (`AUX1`) low | test logic in reset; all-zero reads | `#`, then `k`, then `A` |

`#` resets the board.  `c` makes `a`/`A` control `AUX` (`A0`); `k` makes
them control `AUX1` (`A1`); `A` drives the selected pin high.  A board reset
releases both pins regardless.  On v3 TRST is the `PGD` pin on the
programming header, not a main-header pin.

## Related

- [README.md](README.md) — overview and index.
- [wiring.md](wiring.md) — pins, power, levels and pull-ups.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK delay and link speed.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
