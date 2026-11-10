# PIC — wiring and electrical requirements

Part of the PIC documentation set (`docs/programmers/pic/`).  This page owns
the full wiring table, the pull-up statement, and the programming-voltage
question; the README carries the headline only.

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
| CS | → | **VPP/MCLR** | programming-mode control; high-voltage parts only (see HVPP) |
| GND | ⏚ | VSS / ground | common ground with the target |
| 3.3V / 5.0V | → | VDD / VCC | target power (optional; `W` to enable) |
| VPU (`Vpu`) | in | pull-up voltage reference | the voltage the pull-ups tie PGC/PGD to; not a wire to the PIC |

Do not connect the Bus Pirate supply rail to the target VCC when the target is
self-powered — that back-feeds one supply into the other.

The mode's own pin table (`v`) prints the two signal names **PGC** for the
clock and **PGD** for the data in the clock/data columns, and leaves the
CS/MISO columns as `-`.  This set writes them as **PGC (CLK)** and
**PGD (MOSI)**.

PGD is bidirectional: it carries the programming data out and the device's
reply back in on a read.

## The pull-up is required — stated explicitly

**A pull-up on PGC and PGD is required.**  PIC mode always drives its pins
open-drain (high = released to the pull-up, low = driven to ground), so with
no pull-up there is no idle-high and the clocked data is meaningless.  With
no target and the pull-up fitted, both lines idle high, so `(1)` reads all
ones (see [macros.md](macros.md)); without the pull-up the read value is
undefined.

Value guidance: **2 kΩ – 10 kΩ** from each line to the logic supply.  The
Bus Pirate's own on-board pull-ups can serve when a voltage is on VPU:
on v4 select `e2` (3.3 V) or `e3` (5.0 V) and then `P`; on v3 the Vpullup
pin must be fed from an external 0–5 V supply.  Fit the VPU voltage to the
target's own VCC level before pulling up.

There is no output-type choice in this mode (unlike SPI/I2C/2WIRE); the
open-drain behaviour is fixed.

## The programming voltage (VPP/MCLR)

Two entry styles exist:

- **LVPP (low-voltage ICSP)** — no high voltage.  **Tie MCLR to the target's
  VDD (or drive it high)** so the part is not held in reset; the Bus Pirate
  does not drive MCLR.  Use the target's programming specification for the
  exact level, because some parts differ.
- **HVPP (high-voltage ICSP)** — many PIC families need **VPP**, typically
  6–13 V, on VPP/MCLR to enter programming mode.

### HVPP — the high-voltage path

The Bus Pirate cannot generate VPP, and **no Bus Pirate pin is
VPP-tolerant**.  VPP lives only on the target side of the switching circuit;
the Bus Pirate drives the switch's control inputs, never the high voltage.

| Line | Adapter use | Control from |
|---|---|---|
| CS | VPP/MCLR (switched) | terminal (assign CS to AUX, then `A`/`a`) or `PIC1` `0x18`–`0x1F` |
| MISO | VPP_CTL (pull-up power enable) | `PIC1` `0x18`–`0x1F` only |
| AUX | VPPEN (programming power enable) | terminal `A`/`a` or `PIC1` `0x18`–`0x1F` |
| VPU | VTARGET (pull-up voltage) | the target's supply |

- The **legacy high-voltage adapter is v3 pinout only**; it is not compatible
  with v4.
- On **v4**, use an external VPP supply plus a switching circuit, and drive
  its control inputs as in the table.
- **Terminal mode** can drive CS (by assigning it to AUX) and AUX, but not
  MISO/VPP_CTL.  The complete adapter therefore needs the `PIC1` binary
  protocol, which has explicit AUX/MISO/CS switches (`0x18`–`0x1F`).
- Take VPP sequencing and timing from the target's programming specification.

## Voltage limit and grounds

The interface is 5 V safe: **do not exceed 5.5 V** on PGC, PGD, CS, AUX,
MISO or the VPU pin.  The low level is driven to ground.  Always share a
common ground with the target, and power the target (its own supply, or `W`)
before reading the device.

## v3 / v4

| | v3 | v4 |
|---|---|---|
| PIC terminal mode | present (build-only) | present (default image) |
| PGC / PGD pins | CLK / MOSI | CLK / MOSI |
| pull-up required | yes | yes |
| VPU source selection | none — feed externally | `e1` / `e2` / `e3` |
| `v` signal row (CLK/MOSI columns) | `PGC PGD - -` | `- - PGC PGD` |
| on-board pull-up value | documented 2 kΩ (v3a+) | unverified |

The two builds use the same ICSP lines and the same open-drain behaviour; the
pins simply sit in different columns of the `v` row.  v3 wiring and the v3
VPU route are **not verified**.
