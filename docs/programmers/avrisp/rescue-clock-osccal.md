# AVRISP-MKII — rescue clock and AVR053 OSCCAL

Part of the AVRISP-MKII documentation set (`docs/programmers/avrisp/`).  This
page covers the two clock features of the ISP path: the **4 MHz rescue clock**
on `CS` and the **AVR053 OSCCAL calibration clock** on `MOSI`.

## The rescue clock

While the personality is active, the `CS` pin continuously outputs a
**4 MHz, 3.3 V push-pull square wave**.  It starts when the mode is entered
(on either transport) and stops when the mode ends.  This is the *rescue
clock*: it supplies a running clock to a target whose fuse settings select
an external clock or crystal that is absent or not oscillating.

### When it is needed

An AVR's serial-programming engine needs a valid MCU clock under the
selected fuses.  If the target is fused for an external clock/crystal and
that clock is not present, ISP entry fails — the target does not answer
the programming-enable echo — even though SCK is being driven.  The rescue
clock provides the missing clock.

### Wiring

- Wire **`CS` (pin 4) → the target's XTAL1** pin.  For a 64-pin
  ATmega64A that is package pin 24; XTAL2 is pin 23.
- Keep `/RESET` on `AUX` (5) and the data/SCK wires unchanged.
- **Do not connect `CS` to `/RESET` or to XTAL2**, and do not drive the
  rescue clock against another active oscillator output.
- Leave `CS` open for PDI and for any session that does not need the clock
  (the output is always present; it is only *connected* if you wire it).

### Speed for a rescue trial

Use **`-B 8`** (8 µs SCK period = 125 kHz).  At 125 kHz a 4 MHz target
clock gives many MCU cycles per SCK half-period, comfortably inside the
datasheet clock requirement.  Remember `-B 125` is 125 µs = 8 kHz, not
125 kHz.

### Notes

- A successful program under an injected clock does **not** prove the
  target's own crystal runs afterwards.

## AVR053 OSCCAL calibration

The host can ask the personality to calibrate an AVR's internal
oscillator using the **AVR053** procedure (the `CMD_OSCCAL` command,
`0x05`).

### AVR053 wiring

Calibration is **not** an in-circuit ISP operation: the ISP data wires are
removed and the two clock/sense wires move to the pins the AVR053
firmware uses.

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| 32.787 kHz calibration clock | MOSI (1) | XTAL1 (the target's clock input) |
| calibration sense | MISO (3) | the target's calibration output, driven by the AVR053 firmware |
| /RESET | AUX (5) | /RESET — released by the command |
| SCK | CLK (2) | **not connected** — remove the ISP clock wire |
| ground | GND (12) | GND |
| all other ISP wires | — | remove (the target's ISP data pins are not used) |

- The firmware drives the **32.787 kHz** clock on **`MOSI`** (a
  hardware-derived clock, +0.06 % off 32768 Hz) and enables a pull-up on
  `MISO`; the previous pull-up state is restored afterwards.
- The command **releases `/RESET`** before it starts counting: the line is
  tristated, so the target's own reset pull-up must hold it high.  A target
  with no reset pull-up will not calibrate.
- **The rescue clock is paused during calibration** — the same clock
  generator is temporarily re-tasked to `MOSI` at the calibration
  frequency, then restored to the 4 MHz rescue output on `CS`.

### What the command does

- It watches `MISO` for eight level changes within about **31 ms** (1024
  cycles of the 32.787 kHz clock, plus a small margin).  If the expected
  toggling does not complete in that window, it reports a timeout.  The
  window is short: a target whose calibration firmware is slow to start
  will report a timeout even though the wiring is right.
- **AVR053 requires the target to already be running the AVR053
  calibration firmware.**  This personality does not install that
  firmware and does not write the OSCCAL fuse directly.  It only performs
  the clock/sense part of the procedure that the calibration firmware
  cooperates with.
- `CMD_OSCCAL` is reachable on the USB route and, over the STK500v2
  command set, on the serial route.

## Limits

- The rescue clock is a fixed 4 MHz square wave; the frequency is not
  host-selectable.
- OSCCAL is a bounded pass/fail operation, not a loop the host can tune.
- Neither feature changes fuses.  Changing clock fuses is a separate,
  deliberate target operation with its own failure modes.

## Related docs

- [entry.md](entry.md) — entry and the mode's lifetime (the clock's
  lifetime is the same).
- [wiring.md](wiring.md) — the ISP table and the `CS` note.
- [isp.md](isp.md) — the ISP flow and the `-B` speed ladder.
- [edge-cases.md](edge-cases.md) — clock and calibration failures.
