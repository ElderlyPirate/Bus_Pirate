# PC-AT keyboard (KEYB) — wiring and electrical requirements

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
the connector pinouts, the Bus Pirate connections, the pull-up requirement and
the supply.  The frame and timing are in [protocol.md](protocol.md).

**In the default builds, KEYB is v4 only.**  A v3 build has no keyboard mode;
the module can be compiled in, but the wiring here is **not verified on v3**.

## Connector pinouts

A PS/2 keyboard uses the 6-pin mini-DIN connector:

| Pin | Signal |
|---|---|
| 1 | DATA |
| 2 | not connected |
| 3 | GND |
| 4 | +5 V |
| 5 | CLOCK |
| 6 | not connected |

A PC-AT keyboard uses the older 5-pin DIN:

| Pin | Signal |
|---|---|
| 1 | CLOCK |
| 2 | DATA |
| 3 | not connected |
| 4 | GND |
| 5 | +5 V |

Pin numbers are the standard connector numbering.  Check the keyboard's own
label or datasheet before making a cable; some keyboards and adapters differ.

## Bus Pirate connections

| Bus Pirate pin | Direction | Keyboard signal | Notes |
|---|---|---|---|
| `MOSI` | both | DATA | keyboard drives on reads; the Bus Pirate pulls low on writes |
| `CLK` | both | CLOCK | the keyboard clocks frames; the Bus Pirate holds it low to inhibit or request |
| `5.0V` | out | VCC (+5 V) | switched by `W` |
| `GND` | — | GND | common ground |

Use the same ground for the Bus Pirate, the keyboard and any external 5 V
supply.  A missing common ground produces parity or stop-bit errors, or
timeouts, that look like a bad keyboard.

## Pull-up — required

The interface is **open drain**: the keyboard and the Bus Pirate only ever
pull a line low; a pull-up supplies the high level and the 1 bits.  **A
pull-up must be present on both DATA and CLOCK.**  Neither line reaches a high
level without one.

The pull-up can come from:

| Source | How | Notes |
|---|---|---|
| the keyboard itself | keyboard's internal resistors | keyboard-dependent; check before relying on it |
| the Bus Pirate | `W`, then `e3`, then `P` (v4) | the 5 V rail is the pull-up source |
| external resistors | 1–10 kΩ from DATA and CLOCK to 5 V | the safe choice if the keyboard has none |

The Bus Pirate's pull-ups are **off by default**.  If the keyboard provides its
own, leave them off.

To use the Bus Pirate's pull-ups, keep this order:

1. `W` — switches on the 5 V rail that feeds Vpullup.
2. `e3` — selects the on-board 5.0 V pull-up source (`e2` selects 3.3 V).
3. `P` — connects the pull-up resistors.

```
KEYB>W
POWER SUPPLIES ON
Clutch engaged!!!
KEYB>e3
5V on-board pullup voltage enabled
KEYB>P
Pull-up resistors ON
KEYB>
```

`P` before the rail or before `e3` prints the warning:

```
KEYB>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
```

The resistors are switched on; the warning means the Vpullup pin had no
voltage at that moment.  It is an ordering warning, not a failed pull-up.
Run `W` and `e3`, then `P` again, or select `e1`, connect the external feed,
then `P` (see Supply).

A missing or weak pull-up looks like this:

| Condition | Likely symptom |
|---|---|
| no pull-up, line floats low | `TIMEOUT`, or a silent monitor |
| no pull-up, line floats high | `NONE` |
| weak pull-up, high level late | `*parity error` or `*stopbit error` |

With the keyboard driving the start bit low, a missing pull-up alone does not
cause `*startbit error`; a floating DATA line can.

## Supply

The keyboard runs from **5 V**.  On the Bus Pirate the rail is switched by
`W` (`POWER SUPPLIES ON`); feed the keyboard's VCC from the `5.0V` pin.  The
keyboard must be powered before any read, write or the monitor.

**Never connect an external supply to the `5.0V` pin.**  That pin is an
output, and back-feeding it can damage the regulator.  With an external 5 V
supply, power the keyboard from that supply and share ground with the Bus
Pirate.  Then choose a pull-up path:

| Path | How |
|---|---|
| external resistors | 1–10 kΩ from DATA and CLOCK to the external 5 V |
| external Vpullup | select `e1`, feed `Vpullup`/`VPU` from the external supply (0–5 V), then `P` |
| Bus Pirate pull-ups | `W`, `e3`, `P`; the on-board 5 V rail feeds only Vpullup, not the keyboard |

`e3` routes only the Bus Pirate's own 5 V rail; do not run it while `Vpullup`
is fed externally.  `e1` refuses while Vpullup already has voltage, so select
it before connecting an external feed.  `W` is still recommended with an
external supply: it engages the CLOCK-low idle inhibit.  An externally powered
keyboard can clock without it.

A keyboard's current draw from the 5 V rail is keyboard-dependent.  A
keyboard that over-loads the rail can make `W` refuse or make the rail sag
during a frame.

`w` switches the rail off but leaves the interface as it was: CLOCK stays
driven low (it was set low by `W`), and the pull-up switch (`P`) stays set; an
on-board VPU source (`e3`) also stays selected.  The Bus Pirate's pull-ups then
have no reference, so no line idles high from them; turn them off with `p`
(and `e1`) before rewiring.

## Direction and levels

Both interface lines are bidirectional in use:

| Direction | Who drives | What happens |
|---|---|---|
| read | keyboard | CLOCK and DATA; the Bus Pirate samples DATA on the low phase |
| write | Bus Pirate, then keyboard | pulls CLOCK low, drives DATA, releases it; the keyboard clocks the frame and ACK slot |

Because the Bus Pirate never drives a line high, it cannot contend with the
keyboard: both sides only pull low.  The header I/O is **5 V class**: 5.5 V is
the maximum on DATA or CLOCK.  The keyboard's open-drain 5 V pull-up is within
that.  A 5 V push-pull source does not belong on this bus: it can contend with
the Bus Pirate's low drive, so use a level shifter or an open-drain buffer.
Do not apply an external voltage to a line that is already driven.

After `W`, `v` shows `CLK` as an output (the idle inhibit) and `MOSI` as an
input; a write drives `MOSI` only for the data bits.

## Pin-type summary

| Property | Value |
|---|---|
| output type | open drain (`H=Hi-Z, L=GND`), fixed by the mode |
| idle state | DATA high through a pull-up; CLOCK driven low between exchanges |
| Bus Pirate pull-up | only if the keyboard lacks its own; off by default |
| lines the Bus Pirate drives | DATA (MOSI) and CLOCK (CLK), low only |
| supply | 5 V on `5.0V`, switched by `W` |
| ground | common with the Bus Pirate |

## v3

The default v3 build has no KEYB mode.  In a custom build the same four
connections and the same 5 V/pull-up rules apply.
