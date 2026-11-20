# SUMP logic analyzer

**Status:** current.  Applies to the v4 image; the v3 feature is present in the
build but not verified on v3 hardware.

The Bus Pirate can act as a small SUMP-compatible logic analyzer: it samples its
signal pins into a memory buffer and streams the capture to a SUMP/OLS host
program such as sigrok or PulseView.  The analyzer is not on the mode menu.  It
is entered by the byte sequence a SUMP client sends when it opens the port, and
it has no visible prompt until it is left again.

This README is a map, not the manual; the details live in the files below.

## Scope

This set covers the analyzer as the firmware presents it, driven by a host
program over the serial port.  It does not cover a client's own user interface
or file formats, and it does not cover the binary layer (`BBIO1`), which is a
separate set.

| File | Covers |
|---|---|
| [entry.md](entry.md) | the hidden entry sequence, the 20-NUL `BBIO1` boundary, wiring, what entry changes, the exit bytes and the `HiZ>` return |
| [client.md](client.md) | host programs, `1ALS`, `enh`, the capture workflow, sample order, options that are ignored |
| [commands.md](commands.md) | the byte-level command reference, long-command framing, metadata, parser recovery, configuration lifetime |
| [rates.md](rates.md) | divider quantisation, the five sampling engines, jitter, capture duration, timer and counter rollover |
| [depth.md](depth.md) | capture depth tiers, the channel trade-off, capacity per board, ring arithmetic |
| [trigger.md](trigger.md) | trigger masks, edge semantics, trigger position, pre-trigger capture |
| [readout.md](readout.md) | reply length and framing, newest-first order, padding, back-pressure, aborts |
| [examples.md](examples.md) | worked byte-level captures and an exit/recovery session |
| [edge-cases.md](edge-cases.md) | failure modes: trigger, symptom, behaviour and recovery |

## Names used in this set

| Form used here | Meaning |
|---|---|
| `SUMP` | the protocol and this documentation set |
| `1ALS` | the four-byte identification reply; the published `SLA1` written in reverse |
| `BPv4` / `BPv3` | device name in the metadata response |
| `enh` | ancillary version string; the marker the enhanced profiles match |
| `HiZ>` | terminal prompt after leaving the analyzer |
| `BBIO1` | binary bitbang layer, entered by twenty NULs |
| `AUX` `AUX1` `AUX2` `CS` `MISO` `CLK` `MOSI` | header pin names used throughout |
| `SLA0` / `SLA1` | published SUMP signatures; not sent by this device |

The terminal's AUX target tokens are a different form of the same pin names:
`A0` is the pin printed `AUX`, `A1` is `AUX1`, `A2` is `AUX2` (the last two on
v4).  See
[generic commands.md](../../generic/commands.md#aux-targets-and-pin-names).

## The five-minute version

- **Entry is a byte sequence, not a menu item.**  A SUMP client sends **five to
  nineteen `0x00` (NUL) bytes immediately followed by one raw `0x02` (STX)**
  when it opens the port.  The device answers `1ALS` and stays in SUMP.
- **Twenty NULs are the boundary.**  The twentieth NUL already enters the
  binary layer (`BBIO1`), so five to nineteen NULs is the whole SUMP entry
  range.  The `0x02` that follows twenty NULs selects the binary I2C mode
  instead.  This is the one mistake that sends a host to the wrong protocol;
  the recovery is in [entry.md](entry.md#recovery).
- **The client owns the port.**  Choose the SUMP/OLS profile in sigrok or
  PulseView and select the Bus Pirate's serial port; close any terminal on that
  port first.
- **Configure, run, read.**  The host sets a divider (rate), a read/delay count
  (depth and trigger position) and a trigger mask, then sends `0x01`.  The
  reply is exactly the read count in bytes, one byte per sample,
  **newest-first**.
- **Wiring:** connect the probe pins to the signals and share ground.  **No
  pull-up and no supply are required**; every probe pin is an input.  The
  interface is 5 V safe (5.5 V maximum).
- **Leave with `0x0F`, `#` or `$`.**  The board stops the analyzer and returns
  to the terminal in `HiZ` with the version banner and the `HiZ>` prompt.  `$`
  is an exit here; it does not enter the bootloader.

## First capture

Install [sigrok or PulseView](https://sigrok.org/wiki/Downloads) and close any
terminal on the port.  Choose the SUMP/OLS-compatible profile and the Bus
Pirate port, set 1 MHz and 4096 samples, leave the trigger off, and run with a
known square wave on one probe pin (common ground).  The exact menu names vary
by release; the byte-level version is in [examples.md](examples.md).

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | present in the build | present, default image |
| entry, exit, command set | present in the build, not verified | present |
| probes | 5 | 8 advertised, 7 wired |
| sample memory (metadata `0x21`) | 4096 bytes | 4096 bytes |
| timer depths | 4096 / 8192 / 16384 / 32768 samples | same |
| fast capture | 4096 samples at 2/4 MHz; 2048 at 8/16 MHz | 2048 samples at every fast rate |
| trigger mask bits | 0–4 | 0–5 and 7; bit 6 has no pin |

The v3 feature is present in the build and **not verified on v3 hardware**.
Fast capacities, 4 MHz jitter and readout timing are the v3-specific caveats.

## Limits & out of scope

- No RLE compression, no demux/filter, no channel-group selection, no external
  clock, no inverted clock, no trigger values or staged triggers, no XON/XOFF
  flow control and no continuous sampling.  Those options are accepted or
  ignored; see [client.md](client.md#options-that-do-nothing).
- Above about 1.06 MHz the rate is one of four fixed engines (2, 4, 8 or
  16 MHz), the trigger marks the start of the window and the capture is capped
  at the fast buffer.  Use a timer rate (1 MHz or slower) for a trigger
  position and for the deep packed captures.
- The capture, not the readout, sets the timing: the device finishes sampling
  before it sends a byte.  The reply length is the read count even when the
  capture is shorter; the difference is padding.
- Building and flashing are developer topics:
  [docs/dev](../../dev/README.md).
- No SUMP client ships with the firmware; [First capture](#first-capture) names
  the client and the first settings.

## Related docs

- [generic/commands.md](../../generic/commands.md#leaving-the-cli--binary-and-sump-entry)
  covers the terminal side of the NUL/STX entry and the twenty-NUL boundary.
- [protocol/binary/](../binary/README.md) — `BBIO1`, the layer twenty NULs
  enter.
- [generic/usage.md](../../generic/usage.md#v-in-hiz) — the header pin table
  and the `v` pin-state display.
- [docs/dev](../../dev/README.md) — build, flash and maintainer verification
  records.
- [SPEC.md](../../SPEC.md) — the capability matrix.
