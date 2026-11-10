# PIC — wiring and electrical requirements

Part of the PIC documentation set (`docs/pic/`).  This page owns the full
wiring table, the pull-up statement, and the programming-voltage question;
the README carries the headline only.

## Signal names used here

One canonical spelling per signal, stated once; the firmware's own labels are
mapped on the right.

| This page | Firmware label / pin | Signal |
|---|---|---|
| **PGC (CLK)** | `v` prints `PGC` | program clock |
| **PGD (MOSI)** | `v` prints `PGD` | program data |
| **VPP/MCLR** | — | target programming-mode control |
| **VPU** | pin `Vpu`; warnings name the `Vpullup` pin | pull-up voltage reference |

## Wiring

| Bus Pirate pin | Direction | PIC signal | Notes |
|---|---|---|---|
| CLK | → | **PGC** | program clock, driven by the Bus Pirate |
| MOSI | ↔ | **PGD** | program data; the mode drives it out and reads it back |
| CS | → | **VPP/MCLR** | programming-mode control, high-voltage parts only; the host drives it, not terminal mode (see below) |
| GND | ⏚ | VSS / ground | common ground with the target |
| 3.3V / 5.0V | → | VDD / VCC | target power (optional; `W` to enable) |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie PGC/PGD to; not a wire to the PIC |

The mode's own pin table (`v`) prints the two signal names **PGC** for the
clock and **PGD** for the data, and leaves the CS/MISO columns as `-`.  This
set writes them as **PGC (CLK)** and **PGD (MOSI)**.

PGD is bidirectional: it carries the programming data out and the device's
reply back in on a read.

## The pull-up is required — stated explicitly

**A pull-up on PGC and PGD is required.**  PIC mode always drives its pins
open-drain (high = released to the pull-up, low = driven to ground), so with
no pull-up there is no idle-high and the clocked data is meaningless.

Value guidance: **2 kΩ – 10 kΩ** from each line to the logic supply.  The
Bus Pirate's own on-board pull-ups can serve when a voltage is on VPU:
on v4 select `e2` (3.3 V) or `e3` (5.0 V) and then `P`; on v3 the Vpullup
pin must be fed from an external 0–5 V supply.  Fit the VPU voltage to the
target's own VCC level before pulling up.

There is no output-type choice in this mode (unlike SPI/I2C/2WIRE); the
open-drain behaviour is fixed.

## The programming voltage (VPP/MCLR)

Some PIC families need a **high voltage** — typically 6–13 V — on VPP/MCLR
to enter programming mode (high-voltage ICSP, HVPP).  Others support
**low-voltage** ICSP (LVPP) and need no high voltage.

- The Bus Pirate's own supplies are the 3.3 V and 5.0 V rails (`W`); it
  cannot generate 6–13 V.  Its outputs are 5 V safe — do not exceed
  **5.5 V** on PGC, PGD or the VPU pin.
- For high-voltage parts, apply VPP from an **external 13 V supply** with a
  switching circuit, or use the Bus Pirate high-voltage programming adapter
  (a legacy accessory).  PIC **terminal mode does not drive the CS line**;
  VPP sequencing is a host or adapter function.  The CS pin can still be
  driven from the terminal through the generic AUX facility (`c`/`C`/`k`/`K`,
  then `A`/`a`), and the `PIC1` binary protocol has explicit CS switches
  (`0x18`–`0x1F`).

### High-voltage adapter signals (legacy accessory)

The documented high-voltage adapter uses three further Bus Pirate lines:

| Bus Pirate pin | Direction | Adapter signal | Notes |
|---|---|---|---|
| CS | → | VPP/MCLR | programming-mode control |
| MISO | → | VPP_CTL | pull-up power enable |
| AUX | → | VPPEN | programming power enable |
| VPU | ← | VTARGET | pull-up voltage |

The legacy adapter is documented as **not compatible with the v4 pinout**.

## Voltage limit and grounds

The interface is 5 V safe; do not exceed **5.5 V** on PGC/PGD.  The low level
is driven to ground.  Always share a common ground with the target, and power
the target (its own supply, or `W`) before reading the device.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| PGC / PGD pins | CLK / MOSI | CLK / MOSI |
| pull-up required | yes | yes |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| `v` pin-row order | `PGC PGD - -` | `- - PGC PGD` |
| on-board pull-up value | documented 2 kΩ (v3a+) | unverified |

The two builds use the same ICSP lines and the same open-drain behaviour; the
pins simply sit in different columns of the `v` pin row.  v3 wiring and the
v3 VPU route are **not verified**.