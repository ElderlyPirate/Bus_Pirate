# UART — half-duplex single wire (HDUART)

Part of the UART documentation set (`docs/protocol/uart/`).  This set calls
the firmware's `Half (single wire)` duplex option **HDUART**.  Wiring is owned
by [wiring.md](wiring.md); this page owns the behaviour.

## What it is

HDUART uses **one shared data wire plus ground**.  The Bus Pirate transmits
and receives on the same pin; the bus is **open drain**, so idle is defined
by an external pull-up.  It is the usual single-wire open-drain arrangement
used for "one wire" serial links.

## How it is selected

At the end of the normal UART setup chain the `Duplex:` prompt offers:

| Option | Text |
|---|---|
| `1` | `Full (TX+RX) *default` |
| `2` | `Half (single wire)` |

Choose `2`.  In the one-line settings form the last field is the duplex
(`2` = half; omit it for full).  The settings header shows the choice in its
last (`dup`) field (`1` = half) — see
[entry-and-setup.md](entry-and-setup.md).

Selecting half duplex **stores the output type as open drain** regardless of
the output-type answer, because a shared single-wire bus is open-drain by
definition.  The settings header's `hiz` field and the `e`/`P` pull-up
controls use that effective type, so `hiz` shows `1`.

Half duplex also requires **idle-high** receive polarity.  With `Idle 0`
selected, the `Duplex:` prompt accepts only `1. Full`; typing `2` prints
`Invalid choice, try again` and re-asks.  A one-line entry that combines
idle 0 with half duplex falls back to the full prompt chain.

## Wiring, and the pull-up is required

The full pin table is in
[wiring.md §Half-duplex single wire](wiring.md#half-duplex-single-wire-hduart).
Briefly: **MOSI** is the shared data line, **GND** is ground, and **MISO is
unused** — the receiver is mapped internally to MOSI, so do not jumper the
two signal pins.

A pull-up to the target's logic voltage (3.3 V typical) is required.  Wire
**MOSI, GND and the pull-up only**.  A value in the **1 kΩ–10 kΩ** range is
suitable.  The Bus Pirate's own pull-ups (`P`, with a Vpullup source) can
serve in place of the discrete resistor.

Both ends must be single-wire / open-drain capable — another Bus Pirate, or
a target running its USART TX as open drain / wired-AND.  With no pull-up
there is no idle-high, and the line reads as continuously low — see
[edge-cases.md](edge-cases.md).

The `v` pin row is static and still labels MISO as `RxD`; in HDUART the
receiver is on MOSI, so that label is not used (see
[wiring.md §Pin names in `v`](wiring.md#pin-names-in-v)).

## No self-echo — what is sent vs what comes back

On a shared wire the Bus Pirate hears its own transmission.  This engine
**discards that self-echo**:

- A word the Bus Pirate transmits from the terminal is shown once, as your
  own `WRITE:` echo, and is **not** shown again as `READ:`.
- A read with nothing from the peer prints the no-data token, not the word
  the Bus Pirate just sent: `READ: FAILED, NO DATA0x00` (the trailing zero is
  the value the read returns; see [terminal-usage.md](terminal-usage.md)).
- Words the peer sends are read normally (`r`, `[`/`{`, macros).

The discard is precise about the window:

| Moment | What happens |
|---|---|
| before the byte | the receiver drains and **keeps** completed peer words already waiting |
| during the byte | what its receiver hears is discarded |
| after the stop bit | it drains again and discards the turnaround echo |

So a word the peer sends **before** the byte is kept; a word it sends
**during** the byte or the immediate turnaround is not.  There is **no
collision detection** and no pre-transmit idle check.

Entering with `m3`, running `W`, and using a line with its pull-up looks like
this.  The single line needs a pull-up to the target's logic voltage (enable
`P` after `W`, or fit one externally); with no peer, the read reports no data:

```
UART>m3 5 1 1 1 1 2
UART (spd brg dbp sb rxp hiz dup)=( 4 416 0 0 0 1 1 )
Ready
UART>W
POWER SUPPLIES ON
Clutch engaged!!!
UART>0x55
WRITE: 0x55
UART>r
READ: FAILED, NO DATA0x00
UART>
```

The `WRITE:` echo appears once; the read does not show the sent byte.

## Speeds

The speed menu and its entries are the same as full duplex — see
[entry-and-setup.md](entry-and-setup.md).  MIDI 31250 is reachable by the
custom/raw route or binary preset `0x69`.

**115200 needs a matched peer, not a firmware change.**  A 16 MHz peer's
divider truncates to about 125 kbaud against the Bus Pirate's preset of
about 114.3 kbaud, an about 9 % mismatch that produces `-f` framing errors on
a shared wire.  Use a peer whose baud generator hits 115200 exactly.

## Auto-baud

Both auto-baud paths follow the **active receive input** — MISO in full
duplex, the shared MOSI line in half duplex:

- the v4 entry-time option `11` at the speed prompt (runs at `W`);
- the `(4)` macro (see [macros.md](macros.md)).

In HDUART the measurement therefore works, provided the peer transmits
during the measurement window.  The entry-time path keeps the same
five-second window, `** Early Exit!` message and 115200 fallback as in full
duplex; the `(4)` macro only reports.

## Bridge, monitor and binary

The macros work in half duplex exactly as in full duplex: `(1)` transparent
bridge, `(2)` live monitor, `(3)` flow-control bridge.  Two caveats:

- The **flow-control bridge `(3)`** follows the v3/v4 rules in
  [bridge.md](bridge.md); on v4 it carries no flow-control signals.
- The bridge and monitor forward only the **low eight data bits**; the ninth
  data bit and the error flags are not encoded into the stream.

The **binary `ART1` protocol stays full duplex** — it does not use the
shared-wire mode, and entering it sets the port back to separate TX/RX pins.

## Behaviour of this engine

Receive buffering and overruns are the same as full duplex, including the
ninth-bit and flag storage.  See
[terminal-usage.md](terminal-usage.md#receive-buffer-and-overruns)
§Receive buffer and overruns and [errors.md](errors.md#overruns) §Overruns.
The only difference is the **self-echo suppression** described above.

## Limitations

- **No collision detection.**  The Bus Pirate does not check that the wire is
  idle before transmitting, and does not arbitrate with the peer.
- **Cross-traffic during transmit is lost** (see §No self-echo).
- **115200 needs a matched peer** (see §Speeds).
- **The `v` pin row is misleading**: it always shows the full-duplex
  `RxD`/MISO label, although the receiver is on MOSI.
- **The binary layer is full duplex only.**

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| half-duplex single wire | yes | yes |
| pull-up required | yes | yes |
| output forced open drain | yes | yes |
| idle-high required | yes | yes |
| speed menu | same | same |
| entry-time auto-baud | — | option `11` |
| auto-baud macro `(4)` | yes | yes |
| no self-echo | yes | yes |
| bridge / monitor | yes | yes |
| binary stays full duplex | yes | yes |

## Related docs

- [wiring.md](wiring.md) — the pin table and the pull-up.
- [entry-and-setup.md](entry-and-setup.md) — duplex selection and the
  one-line form.
- [macros.md](macros.md) — auto-baud and the monitor.
- [edge-cases.md](edge-cases.md) — HDUART failure modes.
