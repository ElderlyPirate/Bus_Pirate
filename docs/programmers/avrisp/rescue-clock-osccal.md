# AVRISP-MKII — rescue clock and AVR053 OSCCAL

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  This page
covers the two clock features of the ISP path: the **4 MHz rescue clock**
on `CS` and the **AVR053 OSCCAL calibration clock** on `MOSI`.

## The rescue clock

While the personality is active, the `CS` pin continuously outputs a
**4 MHz, 3.3 V push-pull square wave**.  This is the *rescue clock*: it
supplies a running clock to a target whose fuse settings select an
external clock or crystal that is absent or not oscillating.

### When it is needed

An AVR's serial-programming engine needs a valid MCU clock under the
selected fuses.  If the target is fused for an external clock/crystal and
that clock is not present, ISP entry fails — the target does not answer
the programming-enable echo — even though SCK is being driven.  The rescue
clock provides the missing clock.

### Wiring

- Wire **`CS` (pin 4) → the target's XTAL1** pin.  For a 64-pin
  ATmega64A that is package pin 24; XTAL2 is pin 23.
- Keep `/RESET` on `AUX0` and the data/SCK wires unchanged.
- **Do not connect `CS` to `/RESET` or to XTAL2**, and do not drive the
  rescue clock against another active oscillator output.
- `CS` is normally **left open for PDI** (the rescue clock is not used for
  PDI).

### Speed for a rescue trial

Use **`-B 8`** (8 µs SCK period = 125 kHz).  At 125 kHz a 4 MHz target
clock gives many MCU cycles per SCK half-period, comfortably inside the
datasheet clock requirement.  Remember `-B 125` is 125 µs = 8 kHz, not
125 kHz.

### Notes

- The rescue clock stops when the personality exits.  A successful
  program under an injected clock does **not** prove the target's own
  crystal runs afterwards.
- The clock runs continuously during the mode; it is **not** routed to the
  target unless you wire `CS` to XTAL1.

## AVR053 OSCCAL calibration

The host can ask the personality to calibrate an AVR's internal
oscillator using the **AVR053** procedure (the `CMD_OSCCAL` command,
`0x05`).

- The firmware drives a **32.787 kHz** calibration clock on **`MOSI`**
  (a hardware-derived clock, +0.06 % off 32768 Hz) and enables a pull-up
  on `MISO`.
- It then watches `MISO` and waits for a bounded number of toggles.  If
  the expected toggling does not complete within the command budget, it
  reports a timeout.
- **The rescue clock is paused during calibration** — the same clock
  generator is temporarily re-tasked to `MOSI` at the calibration
  frequency, then restored to the 4 MHz rescue output on `CS`.
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