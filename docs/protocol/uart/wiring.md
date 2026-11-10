# UART — wiring and electrical requirements

Part of the UART documentation set (`docs/uart/`).  This page owns the full
wiring tables and the pull-up statement; the README carries the headline
only.  The half-duplex variant is described in [hduart.md](hduart.md).

## Pick your wiring

- **Normal UART device?** full duplex — **three wires**: MOSI (TX), MISO
  (RX) and GND.
- **One-wire device or a second Bus Pirate?** HDUART — **one wire on MOSI
  plus GND**, and a **pull-up resistor to 3.3 V is required**.
- **Need hardware handshaking?** the flow-control bridge — CTS/RTS on CS and
  CLK (**v3 only**).

## Full duplex

| Bus Pirate pin | Direction | UART signal | Notes |
|---|---|---|---|
| MOSI | → out | TX (transmit) | the mode labels this pin `TxD` |
| MISO | ← in | RX (receive) | the mode labels this pin `RxD` |
| GND | ⏚ | ground | common ground with the target |

The three required wires are **TX, RX and GND**.  **VPU (`Vpu`) is not a
data wire** — it is only the pull-up reference when the output is open drain
(see [Feeding the pull-up voltage](#feeding-the-pull-up-voltage-vpu) below).

Connect the Bus Pirate **TX (MOSI)** to the target's **RX**, and the Bus
Pirate **RX (MISO)** to the target's **TX**.  Cross the two data lines; do
not tie them together in full duplex.

The Bus Pirate is **not** RS-232: it works at logic levels, not ±13 V.  Do
not connect it to a real RS-232 port without a level converter.

## Output type and pull-ups

At the `Select output type:` prompt:

| Option | Effect | When you need it |
|---|---|---|
| `2. Normal` | push-pull: the Bus Pirate drives the line both ways at 3.3 V | when the target needs a driven high, or no pull-up is available |
| `1. Open drain` | the Bus Pirate only pulls the line low; the high comes from a pull-up | when the bus is shared / open-drain; **a pull-up is then required** (the Bus Pirate's own, via `P` + a VPU source, can serve) |

Value guidance for a discrete pull-up: **1 kΩ – 10 kΩ** to the interface
voltage.  The lower end suits higher speeds.

### Feeding the pull-up voltage (VPU)

`P` connects the Bus Pirate's pull-up resistors between the I/O lines and
the physical **Vpullup** pin, which must already be at the pull-up voltage:

- **v4** — select the on-board source with `e`:
  - `e1` External (or None),
  - `e2` on-board **3.3 V**,
  - `e3` on-board **5.0 V**.
- **v3** — there is no `e`; feed the Vpullup pin from an external 0–5 V
  supply.

If nothing is on VPU, `P` still enables the pull-ups but prints
`Warning: no voltage on Vpullup pin`.  The shared `P`/`e` commands and the
VPU routing are documented in `docs/generic/` — see
[commands.md](../../generic/commands.md).

The target's own supply (VCC) is separate from VPU: `W` enables the Bus
Pirate's programmable supply to power the target, while VPU only sets the
pull-up reference.

## Half-duplex single wire (HDUART)

**One data wire, open-drain, pull-up required.**  Select `2. Half (single
wire)` at the duplex prompt (see [entry-and-setup.md](entry-and-setup.md)),
then wire:

| Bus Pirate pin | Direction | Signal | Notes |
|---|---|---|---|
| MOSI | ↔ | the single shared data line | TX drives it, RX listens to the same pin |
| GND | ⏚ | ground | common ground with the target |
| MISO | — | not used | the RX input is mapped onto MOSI in this mode |

The bus is **open drain by definition** (the mode forces it, whatever the
output-type answer was): idle is defined solely by an external pull-up.

**A pull-up resistor to 3.3 V is required — stated explicitly.**  The bus
needs **both** the Bus Pirate's two signal lines tied into one node **and**
a discrete pull-up: a **1 kΩ to 3.3 V** is suitable, as is any value in the
**1 kΩ – 10 kΩ** range.  The Bus Pirate's own pull-ups (`P`, with a VPU
source) can serve in place of the discrete resistor.

Both ends must be single-wire / open-drain capable — another Bus Pirate, or
a target running its USART with a wired-AND/open-drain TX.  In this mode
the Bus Pirate does not echo its own bytes back (see [hduart.md](hduart.md)).

## Flow-control signals (flow-control bridge, macro 3)

The flow-control bridge (`(3)`) is a **v3** feature.  It relays two handshake
lines between the target and the host's serial port:

| Bus Pirate pin | Direction | Signal | Notes |
|---|---|---|---|
| CS | ← in | CTS | target ready-to-send is passed to the host serial chip |
| CLK | → out | RTS | the host's request-to-send is mirrored out to the target |

On **v4** those two pins are not wired to the host serial path, so `(3)` has
no flow control and behaves exactly like `(1)` — see [bridge.md](bridge.md).

## Voltage limit

The UART interface is 5 V safe; do not exceed **5.5 V** on the data line.
The Bus Pirate's own pull-up source is at most the on-board 5.0 V rail.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| TX / RX pins | MOSI / MISO | MOSI / MISO |
| output-type prompt | asked | asked |
| HDUART single wire | present | present |
| HDUART pull-up required | yes | yes |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| flow-control pins (macro 3) | CS = CTS, CLK = RTS | not wired |

The v3 column is **not verified**.