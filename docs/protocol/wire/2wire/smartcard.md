# 2-Wire — smart cards (ISO 7816-3)

Part of the [2-Wire documentation set](README.md) in the
[wire protocol set](../README.md).  This page is the use context for the ATR
macros: what a contact smart card is, how to wire one to the Bus Pirate, and
how the ATR flow runs.  The macro mechanics are in [macros.md](macros.md).

## Why 2-Wire for a smart card

A contact smart card is a two-wire-plus-reset device: the Bus Pirate supplies
power and ground, drives the card's **reset** and **clock**, and exchanges data
on the card's **I/O** contact.  2-Wire provides the primitives needed — a
clocked data line, discrete CS/clock/data control, and the ISO 7816-3 **ATR**
macros that reset the card and decode its Answer-To-Reset.

This is the mode's client/use context, not a full ISO 7816-3 stack: 2-Wire
only resets the card and reads the ATR.  It does not implement the T=0/T=1
transport, commands (APDU), or file access.

## Contact wiring

A card's contacts map to the Bus Pirate as follows.  **CS is the card's reset
line** in this mode, not a chip-select:

| Card contact | Signal | Bus Pirate pin | Notes |
|---|---|---|---|
| C1 | VCC | supply rail (`W`) or external VCC | card power; use the card's rated voltage |
| C2 | RST | CS | driven low then released high by the ATR macro; left high after it |
| C3 | CLK | SCL (CLK) | card clock |
| C5 | GND | GND | common ground |
| C7 | I/O | SDA (MOSI) | bidirectional data, open drain on the card |
| C4, C6, C8 | RFU / VPP / I/O | — | not used by this mode |

The card's I/O contact is **open drain**, so 2-Wire must run in **open-drain**
output with **one pull-up (2 kΩ–10 kΩ) on each of SDA and SCL**.  The Bus
Pirate's on-board pull-ups are switched as one group and cover both lines;
there is no per-line control.  An external build therefore needs two
resistors, one per line.  The pull-up reference must be supplied on VPU.  The
card needs its I/O pull-up; the clock line needs the other.  See
[wiring.md](wiring.md).

The clock rate comes from the mode's speed profile.  The default `~6kHz` is
slow but safe for bringing a card up; choose a faster profile for a higher
clock.  The profile is approximate — see
[entry-and-setup.md](entry-and-setup.md#clock-speeds).

## The ATR flow

1. Enter 2-Wire (`m6`), choose **open drain**; wire the card per the table.
2. `W` puts the on-board rails on their header pins; wire the card's VCC to
   the rail it needs, or power the card externally and share ground.
3. On v4, put a voltage on VPU first (`e2` 3.3 V / `e3` 5 V); on v3 feed VPU
   externally.  **Feed VPU before `P`**, or `P` only warns
   `Warning: no voltage on Vpullup pin`.
4. `P` enables the on-board pull-ups so the open-drain lines have an idle
   high.
5. Run `(1)`.  The macro pulses the card reset (reset on CS) low, releases it
   high, then reads four values and decodes them.  The banner, the four
   printed values and the decoded lines are shown in [macros.md](macros.md)
   §Macro `(1)`; the firmware's field mapping is in §The parse of the same
   page.

## Card reset after the macro

`(1)` releases the card reset (CS high) before it samples and leaves CS high;
no release step is needed.  `(2)` does not touch CS.  If you assert CS low
yourself (`C` then `a`, or binary `0x04`), release it with `C` then `A` (or
`0x05`), or leave and re-enter the mode, which makes CS an input.

## What the ATR is

The Answer-To-Reset is the byte string a card sends after its reset is
released.  It tells the reader which transfer protocol the card supports and
the timing it needs.  A reader normally uses it to configure the transport
before sending any command; here it is exposed raw, decoded only to the level
the firmware decodes.

## Limits and caveats

- The mode **does not sense a card** and prints no error when none answers; the
  four values are simply whatever the lines read.
- `(1)` reads the ATR after releasing reset; the four values are meaningful
  when the card answers (otherwise they are idle/floating data).  The parse
  still reads exactly four values at the default 8-bit width.
- The protocol field's `unknown` result for a standard ATR `TS` byte is
  documented once in [macros.md](macros.md) §Limits.
- No voltage, timing, or safety check is made against the card's requirements;
  confirm the card's VCC range before powering it.
- The mode applies **one output type to both lines**: the card's I/O needs open
  drain (with a pull-up), while a card may expect the reader to drive its
  clock.  In open drain the clock high is a pull-up, not a push-pull drive.
  If a card does not answer, the output type and the clock rate are the first
  two settings to review.
- No transport-level ISO 7816-3 support (APDU, T=0/T=1) exists in this mode.
