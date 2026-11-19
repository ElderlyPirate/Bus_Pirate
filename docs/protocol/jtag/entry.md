# JTAG — entry and settings

Part of the JTAG documentation set.  This page covers how both JTAG paths
are reached, the settings each accepts, and how to leave them.  The TAP
mechanics are in [tap.md](tap.md); the complete byte tables are in
[openocd.md](openocd.md) and [xsvf.md](xsvf.md).

## Scope

- Entry from the terminal prompt into the binary layer, then into a JTAG
  path.
- The OpenOCD path settings: pin mode, features, TCK delay and link speed.
- The XSVF player's fixed settings and player commands.
- Exit rules for both paths.
- Not the host-tool configuration: that belongs to
  [tools/openocd](../../tools/openocd/README.md) and
  [tools/xsvf](../../tools/xsvf/README.md).

## No terminal mode

JTAG has no mode-menu entry and no terminal command grammar.  The default
v4 menu lists the terminal modes — `HiZ`, `1WIRE`, `UART`, `I2C`, `SPI`,
`2WIRE`, `3WIRE`, `KEYB`, `PIC`, `DIO` — and JTAG is not among them.  The
v3 build is the same without `KEYB`.

The only entry is the binary layer:

1. From the terminal prompt, send **20 consecutive `0x00` (NUL) bytes**.
   The bytes must be consecutive; any other byte resets the count.
2. The layer answers `BBIO1` (five bytes, no line ending).
3. Send `0x06` for the OpenOCD path or `0x18` for the XSVF player.

The entry bytes are not printable, so a terminal that sends only text
cannot enter.  Use a host tool, a small script, or a terminal program that
can send raw bytes.  The binary layer takes over the link; terminal
commands are not interpreted until you return.

## What the link shows

The identifiers are the firmware's own strings.  A typical OpenOCD entry
and exit, as bytes on the link:

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
hardware reset instead of the v4 terminal reset.  The XSVF path uses `0x0F`
where OpenOCD uses `0x00`; see the exit table below.

## OpenOCD path settings

`0x06` starts the `OCD1` command loop.  Settings are per-session; there is
no settings menu and no read-back.

| Setting | Request | Default | Effect |
|---|---|---|---|
| Pin mode | `0x01` + mode | `0x00` Hi-Z | `0x00` inputs; `0x01` push-pull; `0x02` open-drain |
| Mode LED | `0x02 0x01` + action | on at binary entry | action `0x00` off, non-zero on |
| VREG | `0x02 0x02` + action | off | action `0x00` off, non-zero on; drives the `3.3V` and `5.0V` rails |
| TRST | `0x02 0x04` + action | low, input | action is the pin level, not "assert" |
| SRST | `0x02 0x08` + action | low, input | action is the pin level, not "assert" |
| Pull-ups | `0x02 0x10` + action | off | ties the lines to Vpullup |
| TCK delay | `0x08` + 2 bytes | `1` | larger = slower TCK; no reply |
| Link speed | `0x07` + selector + `0xAA 0x55` | normal at boot | v3 UART rate; v4 no-op |

`0x06` entry resets the pin mode, VREG, pull-ups, SRST (`AUX`) level and
TCK delay, but not the mode LED, the link rate or TRST (`AUX1` on v4, `PGD`
on v3).

The pin mode byte drives the five outputs (TDI, TMS, TCK, SRST, TRST) low
before it applies the direction; TDO is always an input.  A mode change
therefore drives SRST and TRST low.  If they are wired as active-low
resets, release them with the matching feature after the pin mode:

| Line | Assert (usual active-low) | Release |
|---|---|---|
| TRST | `0x02 0x04 0x00` | `0x02 0x04 0x01` |
| SRST | `0x02 0x08 0x00` | `0x02 0x08 0x01` |

The feature order matters: a later pin mode re-parks both lines low.

The action byte is a level, not an assert flag.  TRST and SRST are commonly
active-low, so `0x00` asserts and `0x01` releases.  Check the target's
schematic; the firmware does not invert either line.  The reset features
only drive after the pin mode has made the pins outputs.

Feature `0x10` switches the on-board pull-up resistors, which tie the JTAG
lines to Vpullup.  Vpullup must already have a voltage: feed it externally,
because the v4 on-board source is off in this path and the terminal `e`
command is not reachable.  The terminal `P` command controls the same
resistors.  See [wiring.md](wiring.md#pull-ups).

Feature `0x02` switches the same on-board regulator that the XSVF entry
turns on.  It drives the `3.3V` and `5.0V` rails.  Leave it off when the
target has its own supply, and never back-drive it.  The XSVF player
switches it on automatically; OpenOCD switches it on only if the host sends
the feature (for example with `buspirate vreg 1`).

`0x08` takes a 16-bit big-endian delay, default `1`, and has no reply.
Larger values slow TCK; `0` is accepted.  The value is a loop count, not a
frequency.  See [clock.md](clock.md).

`0x07` takes a selector and then `0xAA 0x55`.  `0x01` selects fast; any
other value selects normal.  The reply is `0x07` plus the echoed selector,
not a success code.  On v3 the rate changes before the confirm bytes; on
v4 the CDC rate is host-controlled and the command is a no-op.  The rate
is not reset by `0x06`; it stays at the last `0x07` setting until changed
or a board reset.  See [clock.md](clock.md).

## XSVF player settings

The player has no settings menu and no per-session settings.  Entry fixes
the electrical and stream setup:

| Item | Value |
|---|---|
| Pins | TDI, TCK and TMS 3.3 V push-pull outputs; TDO input; no open-drain mode |
| SRST / TRST | not driven by the player |
| Pull-ups | not needed and not changed by entry |
| VREG | on at entry; stays on after the player returns |
| TCK | no host control |
| Stream | XSVF 5.01; chunks ≤ 4096 bytes; longest shift ≤ 400 bits |

Player commands:

| Request | Action | Reply |
|---|---|---|
| `0x01` | reset the chain, then Run-Test/Idle | — |
| `0x02` | probe the chain | count byte, then IDCODE bytes |
| `0x03` | play a stream | data handshake, then a result byte |
| `0x0F` | reset the chain and leave the player | none; back to `BBIO1` |
| other | ignored, including `0x00` | — |

## Leaving the paths

| Path | First step | Result |
|---|---|---|
| OpenOCD | `0x00` | `BBIO1`; pins inputs, VREG and pull-ups off |
| XSVF | `0x0F` | `BBIO1` silently; pins driven, VREG on |

From `BBIO1`, send `0x0F`: it replies `0x01` and runs the terminal reset
(v3: a hardware reset), which switches VREG off.  The XSVF player ignores
`0x00`.  `0x0F` is a player command only while the player loop is idle;
during a run it is consumed as data or as a busy byte.  The reset button
leaves either path.  A terminal `#` once the prompt is back runs the same
board reset.

## Related

- [README.md](README.md) — overview and index.
- [wiring.md](wiring.md) — pins, power, levels and pull-ups.
- [tap.md](tap.md) — TAP states, shifts, IDCODE and BYPASS.
- [clock.md](clock.md) — TCK delay and link speed.
- [openocd.md](openocd.md) — the `0x06` command set.
- [xsvf.md](xsvf.md) — the `0x18` player.
- [edge-cases.md](edge-cases.md) — failure modes and recovery.
