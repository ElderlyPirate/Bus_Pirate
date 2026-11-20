# PIC — wiring and electrical requirements

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the wiring tables, the pull-up and supply requirements, the LVPP/HVPP
question and the HVPP adapter.  The README carries the headline only.

## Signal names

One canonical spelling per signal; the firmware and board labels are mapped
once.

| This page | Firmware / board | Signal |
|---|---|---|
| **PGC (CLK)** | `v` prints `PGC`; Bus Pirate pin `CLK` | program clock |
| **PGD (MOSI)** | `v` prints `PGD`; Bus Pirate pin `MOSI` | program data |
| **VPP/MCLR** | target pin `MCLR` / `VPP` | programming-mode control |
| **VDD / VSS** | Bus Pirate `3.3V` / `5.0V` and `GND` | target supply and ground |
| **VPU** | header `Vpu`; warnings say `Vpullup pin`; `v` prints `VPU` | pull-up reference |

## Wiring

| Bus Pirate pin | Target signal | Notes |
|---|---|---|
| CLK | **PGC** | program clock, driven by the Bus Pirate |
| MOSI | **PGD** | program data; driven out and released for a read |
| GND | **VSS** | common ground, required |
| 3.3V or 5.0V | **VDD** | optional target power; `W` enables the rail; leave open for a self-powered target |
| VPU (`Vpu`) | — | pull-up reference; not a target wire |

CS and MISO are not target signals.  They are adapter control inputs when an
HVPP adapter is used; see [the HVPP switch](#the-hvpp-switch).

- **Do not connect a Bus Pirate supply rail to a self-powered target.**  That
  back-feeds one supply into the other.  Leave the rail pin open.  If you use
  external pull-ups, skip `W`; if you use the on-board pull-up feed, `W` is
  still needed to power that feed, but keep the rail pin itself unconnected.
- **Common ground is required.**  A missing ground gives floating reads and
  a failed entry.
- The physical header order is board-level.  The v4 table and the v3 cable
  order are below.

### v4 header

Shrouded 2x6 header, counted from the key (key = pin 1):

| Pin | Signal |
|---|---|
| 1 | MOSI |
| 2 | CLK |
| 3 | MISO |
| 4 | CS |
| 5 | AUX |
| 6 | AUX1 |
| 7 | AUX2 |
| 8 | ADC |
| 9 | Vpu |
| 10 | +3V3 |
| 11 | +5V |
| 12 | GND |

The terminal displays `+3V3` and `+5V` as `3.3V` and `5.0V`.

### v3 header

The classic 2x5 header, cable order from pin 1:

`GND`, `3.3V`, `5.0V`, `ADC`, `VPU`, `AUX`, `CLK`, `MOSI`, `CS`, `MISO`.

Check the board silkscreen and the key before using the order: a reversed
cable or a different board revision will not match.

## The pull-up is required

**A pull-up on PGC and PGD is required.**  PIC mode always drives both lines
open-drain: a low is driven to ground, a high is the line released, and only
the pull-up makes it high.  With no pull-up the high phases float and the
clocked data is meaningless; a read has no defined idle level.

Value guidance: **2 kΩ – 10 kΩ** from each line to the target's logic supply.
The board's own pull-ups can serve when VPU is fed: on v4 select `e2` (3.3 V)
or `e3` (5.0 V), then `W` if the feed uses the rail, then `P`; on v3 feed the
VPU pin from an external 0–5 V supply and enable `P`.  Match the pull-up
voltage to the target's VCC.

The mode has no output-type choice (unlike SPI, I2C and 2WIRE); open drain is
fixed.  The v4 board pull-ups are shared with the other modes; their values
differ by pin (see [the I2C wiring page](../../protocol/i2c/wiring.md)).  An
external 2–10 kΩ pair is the predictable choice.

### Bring-up order

| Step | Command | Effect |
|---|---|---|
| 1 | `e2` / `e3` (v4) | `3.3V` / `5V on-board pullup voltage enabled`; on v3 feed VPU externally |
| 2 | `W` | `POWER SUPPLIES ON`; the `3.3V`/`5.0V` rails come up |
| 3 | `P` | `Pull-up resistors ON` (warns if VPU is dead) |
| 4 | `w` (later) | `Clutch disengaged!!!`, `POWER SUPPLIES OFF` |

`e` may be issued before or after `W`; it only needs a non-HiZ open-drain
mode, and it refuses if the VPU pin already has voltage.  Keep `P` last.
`W` is needed only when the target is powered from the Bus Pirate or when the
on-board pull-up feed is used.  A self-powered target with external pull-ups
needs neither `W` nor `e`; an external resistor pair needs no command.

Programming mode is separate: apply VPP for HVPP, or tie MCLR to the target's
VDD for LVPP, before `(1)` or any programming sequence.  The terminal bus
operations do not drive MCLR.

## Supply and voltage limits

The interface is **5 V safe**; do not exceed **5.5 V** on PGC, PGD, CS, AUX,
MISO or VPU.  The low level is driven to ground.  Target VDD may be 3.3 V or
5.0 V; match the pull-up feed to it.

## The programming voltage: LVPP and HVPP

**LVPP** is low-voltage programming (MCLR at the target's VDD); **HVPP** is
high-voltage programming (a VPP pulse on MCLR).  **VPP** is the programming
voltage itself, never a signal on a Bus Pirate header pin.

| Entry | MCLR / VPP | Bus Pirate role | Typical |
|---|---|---|---|
| **LVPP** | held at the target's VDD | clock and data only; no high voltage | parts whose specification allows low-voltage ICSP |
| **HVPP** | a VPP pulse, typically 6–13 V | clock, data and the adapter control inputs; never VPP itself | many PIC10/12/16/18 parts |

- The terminal PIC bus operations do not drive MCLR.  For LVPP, tie MCLR to
  the target's VDD so the part is not held in reset; use the part's
  programming specification for the exact level.  For HVPP, the VPP switch is
  controlled separately (see below).
- For HVPP, VPP must be switched on the target side.  **No Bus Pirate pin is
  VPP-tolerant**; VPP must never reach a header pin.
- Exact VPP levels and timing are part-specific and **unverified** here; take
  them from the programming specification.

### The HVPP switch (the HVPP adapter)

An HVPP part needs VPP on MCLR.  The Bus Pirate has no VPP generator and no
VPP-tolerant pin.  The published Dangerous Prototypes adapter is a 2x5 board
that stacks on the v3 header: it generates 13 V and switches it to the
target's MCLR.  It does not fit the v4 2x6 header; for v4 you need a harness
that maps the signals or your own switch.  The adapter is not part of this
firmware; its schematic is in [the adapter repository][adapter].

The adapter's control inputs and reference connections are on the Bus Pirate
header; CS, AUX and MISO are control inputs, not target signals:

| Bus Pirate pin | Adapter input | Role |
|---|---|---|
| CS | MCLR | the adapter's MCLR control input |
| AUX | VPPEN | the adapter's VPP enable input |
| MISO | MISO | the adapter's target-VDD switch input |
| CLK | PGC | programming clock to the target |
| MOSI | PGD | programming data to the target |
| VPU | VTARGET | target-voltage sense / pull-up reference; input to the adapter, not a supply |
| GND | GND | common ground |

The adapter's programming outputs go to the target, not to the Bus Pirate:

| Adapter output | Target signal | Rule |
|---|---|---|
| VPP | MCLR / VPP | switched high voltage; **never** to a Bus Pirate pin |
| PGC | PGC | from CLK |
| PGD | PGD | from MOSI |
| VTARGET | VDD | sense connection to the target's own supply; not a source |

The adapter's VPP output goes only to the target's MCLR.  **No Bus Pirate pin
is VPP-tolerant**; VPP must never reach a header pin.

- The adapter schematic defines the active level of each control input and
  the VPP sequencing.  Do not assume active-high.
- In the terminal, `c` selects an AUX target and `A`/`a` drive or release it.
  `CS` is selectable with `C`; `MISO` is not drivable from the terminal or
  raw wire, so the complete adapter needs `PIC1` (or a custom host that sends
  the `PIC1` control bytes).  `picprog` uses raw wire and cannot drive that
  switch.
- `PIC1` `0x18`–`0x1F` sets all three control inputs from the byte (AUX bit 2,
  MISO bit 1, CS bit 0); it is an absolute state, not a per-line toggle.

## The `v` signal row

The mode's row is the last four columns of the `v` table.  The firmware
separates columns with tabs; these are the space-aligned rows.

```
v4:  CS    MISO  CLK   MOSI
     -     -     PGC   PGD
```

```
v3:  CLK   MOSI  CS    MISO
     PGC   PGD   -     -
```

PGC is always the CLK column and PGD the MOSI column; the row order differs:
v4 prints CS, MISO, CLK, MOSI; v3 prints CLK, MOSI, CS, MISO.  The full `v`
row layout is owned by [the generic usage page](../../generic/usage.md).

## v3 / v4

| | v3 | v4 |
|---|---|---|
| PIC terminal mode | present | present |
| PGC / PGD pins | CLK / MOSI | CLK / MOSI |
| pull-up required | yes | yes |
| VPU source | external feed only | `e1` external/none, `e2` 3.3 V, `e3` 5.0 V |
| on-board pull-up value | documented 2 kΩ on v3a and later; external guidance is 2–10 kΩ | shared with other modes; see the I2C page |
| HVPP adapter | 2x5 board for the v3 header | no v4-header version; use a harness |

The two builds use the same signals and the same open-drain behaviour.  v3
wiring and the v3 VPU route are not verified.

[adapter]: https://github.com/DangerousPrototypes/Buspirate_Adapter_Pic_Programmer
