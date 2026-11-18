# UART — wiring and electrical requirements

Part of the UART documentation set (`docs/protocol/uart/`).  This page owns
the wiring tables, the pull-up and supply rules, the VPU route, the `v` pin
names, and the voltage limit.  Entry and bring-up are in
[entry-and-setup.md](entry-and-setup.md); the half-duplex behaviour is in
[hduart.md](hduart.md).

## Pick your wiring

| Situation | Wires | Pull-up |
|---|---|---|
| Full-duplex UART device | **MOSI (TX) → target RX**, **MISO (RX) ← target TX**, **GND** | none in Normal output; **one on the TX line in open drain** |
| One-wire device, or a second Bus Pirate | **MOSI (shared data) + GND**; MISO unused | **required**, 1 kΩ–10 kΩ to the target's logic voltage (3.3 V typical) |
| Hardware handshaking (v3 only) | add **CS** and **CLK** | see §Flow-control signals |

A **non-self-powered target** also needs its supply: connect the target's
**VCC** to the Bus Pirate's **3.3 V** or **5.0 V** header pin and share
**GND**; `W` switches that rail on.  Vpullup is a separate reference and does
not power the target.

The Bus Pirate is **not** RS-232: the pins are logic-level, not ±13 V.  Do not
connect them to a real RS-232 port without a level converter.

## Full duplex

| Bus Pirate pin | Direction | UART signal | Notes |
|---|---|---|---|
| MOSI | → out | TX | the mode labels this pin `TxD` |
| MISO | ← in | RX | the mode labels this pin `RxD` |
| GND | ⏚ | ground | common ground with the target |

Connect the Bus Pirate **TX (MOSI)** to the target's **RX**, and the Bus
Pirate **RX (MISO)** to the target's **TX**.  Cross the two data lines; do
not tie them together in full duplex.

The three required signal wires are **TX, RX and GND**.  **Vpullup is not a
data wire** — it is only the pull-up reference when the output is open drain
(see §Feeding the pull-up voltage).

## Output type and pull-ups

At `Select output type:`:

| Option | High level | Pull-up | Typical use |
|---|---|---|---|
| `1. Open drain (H=Hi-Z, L=GND)` — the **default** | high impedance, set by the pull-up | **required on the TX line** | shared buses, 5 V buses, targets that already pull up their RX |
| `2. Normal (H=3.3V, L=GND)` | driven 3.3 V | none | 3.3 V point-to-point |

In open drain the Bus Pirate only pulls TX low; the high comes from a
pull-up.  Fit **one pull-up on MOSI** (the TX line) of **1 kΩ–10 kΩ** to the
target's logic voltage; the lower end suits higher speeds.  Many targets
already have a pull-up on their RX pin, but do not assume it.  The Bus
Pirate's own pull-ups (`P`, with a Vpullup source) can serve instead of a
discrete resistor.

MISO is an input driven by the target and does not need a Bus Pirate
pull-up.  If the target tri-states its TX, the line floats; fit a pull-up if
that matters.

Normal output cannot produce a 5 V high.  For a 5 V target use open drain
with a 5 V pull-up reference, or use a level converter.  Do not use Normal
output to drive a line that another device is already driving.

## Feeding the pull-up voltage (Vpullup)

The pull-up resistors connect the I/O lines to the **Vpullup** pin.  On v4
select the source with `e` before enabling the pull-ups:

| Command | Source |
|---|---|
| `e1` | External (or None) |
| `e2` | on-board 3.3 V |
| `e3` | on-board 5.0 V |

On **v3** there is no `e`; feed the `Vpu` pin from an external 0–5 V supply.

`P` connects the resistors and prints `Pull-up resistors ON`; with no voltage
on Vpullup it still enables them and adds
`Warning: no voltage on Vpullup pin`.  `p` disables them.

The pull-up path only makes sense in open drain.  In Normal output, `P` still
switches the resistors on but prints `WARNING: pins not open drain (HiZ)`,
and `e` is refused with the same warning.  `e` is also refused with
`Warning: already a voltage on Vpullup pin` when the pin is already powered;
remove the external feed first, then select the source.

**Set Vpullup to the target's logic supply.**  A 5 V pull-up on a 3.3 V
target can damage it: use `e2` for a 3.3 V target and `e3` only for a 5 V
target, or feed `Vpu` externally at the target's voltage.

Vpullup is separate from the target supply.  `W` enables the on-board 3.3 V
and 5.0 V rails on their header pins to power the target; Vpullup only sets
the pull-up reference and does not power the target.

### Bring-up order with the on-board pull-ups (v4)

1. Wire the target with its supply off.
2. Enter UART and answer `Select output type:` with open drain.  `e` is
   refused in Normal output.
3. Run `e2` for a 3.3 V target or `e3` for a 5 V target.
4. Run `W` to start the rails and configure the UART port.  `W` is required
   even when the target is self-powered.
5. Run `P` to connect the pull-up resistors.
6. Check `v`; the TX line is `O` and its high level comes from Vpullup.

For an external Vpullup feed, select `e1` before connecting it.  On v3 the
same order applies without step 3: wire, feed `Vpu` externally, `W`, then
`P`.

## Half-duplex single wire (HDUART)

**One data wire, open drain, pull-up required.**  Select
`2. Half (single wire)` at the duplex prompt, then wire:

| Bus Pirate pin | Direction | Signal | Notes |
|---|---|---|---|
| MOSI | ↔ | the single shared data line | TX drives it, RX listens to it |
| GND | ⏚ | ground | common ground with the target |
| MISO | — | not used | the receiver is mapped to MOSI; do not jumper MISO to MOSI |

The bus is **open drain by definition**: the mode stores the output type as
open drain whatever the output-type answer was.  Idle is defined solely by an
external pull-up.

A pull-up to the target's logic voltage (3.3 V typical) is required.  Wire
**MOSI, GND and the pull-up only**; MISO is unused.  A value in the
**1 kΩ–10 kΩ** range is suitable; the lower end suits higher speeds.  The
Bus Pirate's own pull-ups (`P`, with a Vpullup source) can serve in place of
the discrete resistor.

Both ends must be single-wire / open-drain capable — another Bus Pirate, or
a target running its USART TX as open drain / wired-AND.  In this mode the
Bus Pirate does not echo its own bytes back; see [hduart.md](hduart.md).

Half duplex also requires **idle-high** polarity (`Idle 1`).  With `Idle 0`
selected, the duplex prompt accepts only full duplex; see
[entry-and-setup.md](entry-and-setup.md).

## Flow-control signals (flow-control bridge, macro 3)

The flow-control bridge (`(3)`) is a **v3** feature.  It relays two
handshake lines between the target and the host's serial port; the firmware
names them `CTS` and `RTS`:

| Bus Pirate pin | Direction | Firmware name | Notes |
|---|---|---|---|
| CS | ← in | CTS | target side, passed to the host serial chip |
| CLK | → out | RTS | host side, mirrored out to the target |

This relays the **host serial handshake**, not flow control on the target
UART itself.  On **v4** those pins are not wired to the host serial path, so
`(3)` sets no flow control and behaves exactly like `(1)` — see
[bridge.md](bridge.md).

## Pin names in `v`

The `v` table names the UART pins in its last four signal columns.  The
order differs between boards, and the UART labels are `TxD`/`RxD`:

| Build | Last four columns (left to right) | UART row |
|---|---|---|
| v4 | `CS` `MISO` `CLK` `MOSI` | `-` `RxD` `-` `TxD` |
| v3 | `CLK` `MOSI` `CS` `MISO` | `-` `TxD` `-` `RxD` |

So **`TxD` is MOSI** and **`RxD` is MISO** in both boards.  The row is
static: in **half duplex** the receiver actually listens on MOSI, and the
`RxD` label on the MISO column is not used.  In the v3 flow-control bridge,
CS and CLK carry the handshake lines although the UART row still shows them
as `-`.

The firmware separates columns with tab characters, so raw terminal spacing
depends on the terminal's tab stops; the mapping above is the reliable
content.  Before `W`, the UART pins are inputs; after a successful `W`, TX is
an output.

## Voltage limit

The data pins are **3.3 V outputs** in Normal output; in open drain the high
is the pull-up voltage.  Do not exceed **5.5 V** on a data line.  The limit is
board-level and is not enforced by the firmware; the exact board ratings are
**unverified**.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| TX / RX pins | MOSI / MISO | MOSI / MISO |
| output-type prompt | asked | asked |
| default output | open drain | open drain |
| HDUART single wire | present | present |
| HDUART pull-up required | yes | yes |
| HDUART idle-high required | yes | yes |
| Vpullup source selection | none — feed externally | `e1` / `e2` / `e3` |
| flow-control pins (macro 3) | CS = CTS, CLK = RTS | not wired |
| `v` last-four order | `CLK` `MOSI` `CS` `MISO` | `CS` `MISO` `CLK` `MOSI` |

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — settings, defaults, `W`.
- [hduart.md](hduart.md) — half-duplex selection and behaviour.
- [bridge.md](bridge.md) — the bridge and the v3 flow-control relay.
- [generic/commands.md](../../generic/commands.md) — `P`/`p`, `e`, `v`.
