# UART — half-duplex single wire (HDUART)

Part of the UART documentation set (`docs/protocol/uart/`).  A half-duplex,
single-wire, open-drain serial link.  Wiring is owned by
[wiring.md](wiring.md); this page owns the behaviour.

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
(`2` = half; omit it or use `0` for full) — see
[entry-and-setup.md](entry-and-setup.md).  The settings header shows the
choice in its last (`dup`) field (`1` = half).

Selecting half-duplex **forces open-drain output** regardless of the
output-type answer, because a shared single-wire bus is open-drain by
definition.

## Wiring, and the pull-up is required

The full pin table is in
[wiring.md §Half-duplex single wire](wiring.md#half-duplex-single-wire-hduart).
Briefly: **MOSI** is the shared data line, **GND** is ground, and **MISO is
unused** — the receiver is mapped internally to MOSI, so do not jumper the
two signal pins.

**A pull-up to 3.3 V is required — stated explicitly.**  Both ends must be
single-wire / open-drain capable.  A discrete **1 kΩ – 10 kΩ** pull-up is
suitable; the Bus Pirate's own pull-ups (`P` with a VPU source) can serve in
place of the discrete resistor.

With no pull-up there is no idle-high, and the line reads as continuously
low — see [edge-cases.md](edge-cases.md).

## No self-echo — what is sent vs what comes back

On a shared wire the Bus Pirate hears its own transmission.  This engine
**discards that self-echo**:

- Bytes the Bus Pirate transmits are shown once, as your own `WRITE:`
  echo, and are **not** shown again as `READ:`.
- A read with nothing from the peer prints `FAILED, NO DATA`, not the byte
  the Bus Pirate just sent.
- Bytes the peer sends are read normally (`r`, `[`/`{`, macros).

The discard is precise: after the Bus Pirate finishes a byte it drops only
what its receiver heard **during that byte**; anything the peer sent before
the byte was queued stays in the receive buffer.  There is **no collision
detection** and no pre-transmit idle check, so a byte the peer sends while
the Bus Pirate is mid-transmission is not kept.

## Speeds

The speed menu and its entries are the same as full duplex — see
[entry-and-setup.md](entry-and-setup.md).

**115200 needs a matched peer, not a firmware change:** a 16 MHz peer's
divider truncates to ≈ 125 kbaud against the Bus Pirate's preset
(≈ 114.3 kbaud), an ≈ 9 % mismatch that produces `-f` framing errors on a
shared wire.  Use a peer whose baud generator hits 115200 exactly.

## Behaviour of this engine

- The receive path keeps a **128-byte software receive ring** filled from an
  interrupt, so short bursts survive a stalled main loop.
- Each byte carries its **parity/framing status** in the ring, so those flags
  are still reported correctly when the byte is read.
- Oversaturation is **cleared and counted** internally (a hardware overrun or
  a full ring) rather than stopping the port.
- The **self-echo is suppressed** as described above.
- The transparent bridge, live monitor and binary protocol work unchanged.

## Bridge and monitor use

The macros work in half-duplex exactly as in full duplex: `(1)` transparent
bridge, `(2)` live monitor, `(3)` flow-control bridge.  Two caveats:

- The **flow-control bridge `(3)`** follows the v3/v4 rules in
  [bridge.md](bridge.md); on v4 it carries no flow-control signals.
- The **binary `ART1` protocol stays full duplex** — it does not use the
  shared-wire mode.

## Limitations

- **No collision detection.**  The Bus Pirate does not check that the wire
  is idle before transmitting, and does not arbitrate with the peer.
- **Cross-traffic during transmit is lost** (see "No self-echo" above).
- **Auto-baud samples the hardware MISO pin** (the normal full-duplex RX
  input).  In half-duplex the receive path is remapped onto the shared MOSI
  line, so MISO is no longer connected to the traffic; the entry-time
  auto-baud option and the `(4)` macro therefore do not work in HDUART —
  both are meant for full duplex.
- **115200 needs a matched peer** (see "Speeds").

## v3 / v4

| | v3 | v4 |
|---|---|---|
| half-duplex single wire | present | present |
| pull-up required | yes | yes |
| speed menu | same | same |