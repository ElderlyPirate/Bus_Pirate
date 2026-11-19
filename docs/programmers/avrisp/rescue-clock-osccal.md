# AVRISP-MKII — rescue clock and AVR053 OSCCAL

Part of the AVRISP-MKII documentation set.  This page covers the two clock
features of the ISP path: the **4 MHz rescue clock** on `CS` and the
**AVR053 OSCCAL calibration clock** on `MOSI`.

## The rescue clock

While the personality is active, the `CS` pin outputs a **4 MHz, 3.3 V
push-pull square wave** for the whole session — from mode entry to mode end.
It is paused only while a calibration command runs (see AVR053 below).  This
is the *rescue clock*: it supplies a running clock to a target whose fuse
settings select an external clock or crystal that is absent or not
oscillating.

### When it is needed

An AVR's serial-programming engine needs a valid MCU clock under the
selected fuses.  If the target is fused for an external clock/crystal and
that clock is not present, ISP entry fails — the target does not answer
the programming-enable echo — even though SCK is being driven.  The rescue
clock provides the missing clock.

### Wiring

- Wire **`CS` (pin 4) → the target's `XTAL1`** pin.  For a 64-pin
  ATmega64A that is package pin 24 (check the part's datasheet); `XTAL2` is
  pin 23.
- Keep `RESET` on `AUX` (5) and the data/`SCK` wires unchanged **for the
  rescue trial**.
- **Do not connect `CS` to `RESET` or to `XTAL2`**, and do not drive the
  rescue clock against another active oscillator output.
- Leave `CS` open for PDI and for any session that does not need the clock
  (the output is present for the session; it is only *connected* if you wire
  it).

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
oscillator using the **AVR053** procedure (the `OSCCAL` command,
`0x05`).

### AVR053 wiring

Calibration is a separate session, not the normal ISP wiring: it reuses the
two ISP data pins for a different job — `MOSI` carries the calibration clock
into the target's `MOSI` pin and `MISO` senses the target's `MISO` output —
and the AVR053 reference wiring leaves `SCK` off.  The personality disables
the SPI clock output and makes `CLK` an input for the command, so a still
connected `SCK` is a wiring-convention difference, not a fault.

| Programmer signal | Bus Pirate pin | Target signal |
|---|---|---|
| 32.787 kHz calibration clock | MOSI (1) | target **MOSI** (the AVR053 C-clock input) |
| calibration sense | MISO (3) | target **MISO** (the calibration OK output) |
| RESET | AUX (5) | RESET |
| SCK | CLK (2) | optional; the AVR053 reference wiring leaves it off (the command does not drive `CLK`) |
| ground | GND (12) | GND |

- **The clock does not go to `XTAL1`.**  The target runs the AVR053 firmware
  and takes its C-clock from the `MOSI` input; `XTAL1` is only used by the
  4 MHz rescue clock above.
- `MOSI` and `MISO` stay on the target's ISP data pins — they are
  repurposed, not removed.  `SCK` is the only wire the reference wiring
  leaves off; `CLK` is not driven during the command.
- The firmware drives the **32.787 kHz** clock on **`MOSI`** (a
  hardware-derived clock, +0.06 % off 32768 Hz) and enables a pull-up on
  `MISO`; the previous pull-up state is restored afterwards.
- The command **releases `RESET`** before it starts counting: the line is
  tristated, so the target's own reset network must hold it high.  A normal
  AVR's internal reset pull-up is enough; if the target holds `RESET` low
  (reset pin disabled, or an external pull-down), calibration cannot start.
- **The rescue clock is paused during calibration** — the same clock
  generator is temporarily re-tasked to `MOSI` at the calibration
  frequency, then restored to the 4 MHz rescue output on `CS`.

### Order of operations

Calibration needs the AVR053 firmware already running on the target, so the
sequence crosses two sessions.  Rewiring happens **between** sessions; do
not move wires while the personality is active (see [entry.md](entry.md)).

1. Wire for ISP ([wiring.md](wiring.md)) and program the AVR053 calibration
   firmware (the target-side firmware from the AVR053 application note) with
   the normal ISP flow.
2. End the ISP session: about five seconds of idle ends the personality and
   the binary layer prints `BBIO1`; send `0x0F` if you want the terminal.
3. **Optional — reference wiring only.**  With the mode inactive, remove the
   `SCK` wire to match the AVR053 reference wiring; leave `MOSI`, `MISO`,
   `RESET` and ground in place.  Leaving `SCK` connected does not affect the
   command: the personality stops driving `CLK` and makes it an input.
4. Re-enter (`0x08` / `0x09`) and have the host send the calibration command
   (`0x05`).
5. End the mode, restore the ISP wiring and program the application.

### What the command does

- It watches `MISO` for eight level changes within about **32 ms nominal**
  (1024 calibration-clock cycles plus a 25-cycle margin at 32.787 kHz).  If
  the expected toggling does not complete in that window, it reports a
  timeout.  The window is short: a target whose calibration firmware is
  slow to start will report a timeout even though the wiring is right.
- The personality does **not** provide or install the AVR053 firmware and
  does not write the OSCCAL fuse directly.  It only performs the clock/sense
  part of the procedure that the calibration firmware cooperates with.
- The rescue clock is restored on `CS` when the command ends, so a second
  calibration command can be sent in the same session; each run gets a
  fresh 32 ms window.
- `OSCCAL` is reachable on the USB route and, over the STK500v2 command set,
  on the serial route.  The worked exchange is below.

### Worked calibration exchange

Send the command outside an ISP programming session; it takes over the ISP
pins and does not restore an active session.

| Direction | Bytes | Meaning |
|---|---|---|
| host → | `05` | `OSCCAL` |
| host ← | `05 00` | success: eight level changes were seen inside the window |
| host ← | `05 80` | timeout: the 32 ms window expired; no value is returned |
| host ← | `05 C0` | rejected: extra bytes after `05`; no target pin was touched |

The reply never carries an OSCCAL value, and the personality never writes
the OSCCAL fuse or register.  The target firmware performs the calibration
and stores its own result.  On the serial route the same payload is wrapped
in an AVR068 frame.

A current `avrdude` sends the same command with `-O` / `--osccal`:

```
avrdude -c avrispmkII -P usb -p m328p -O
```

Older builds may not have `-O`; send `05` with a custom host instead.  The
target must already be running the AVR053 calibration firmware.  That
firmware is the target-side part of the [AVR053 application
note](https://ww1.microchip.com/downloads/en/Appnotes/Atmel-2555-Internal-RC-Oscillator-Calibration-for-tinyAVR-and-megaAVR-Devices_ApplicationNote_AVR053.pdf);
it is the operator's own and is not installed by this personality.

## Limits

- The rescue clock is a fixed 4 MHz square wave; the frequency is not
  host-selectable.
- OSCCAL is a bounded pass/fail operation, not a loop the host can tune.
- Neither feature changes fuses.  Changing clock fuses is a separate,
  deliberate target operation with its own failure modes.
- AVR053 calibration is **not verified** on a physical target for this
  port.

## Related docs

- [entry.md](entry.md) — entry and the mode's lifetime (the clock's
  lifetime is the same).
- [wiring.md](wiring.md) — the ISP table and the `CS` note.
- [isp.md](isp.md) — the ISP flow and the `-B` speed ladder.
- [edge-cases.md](edge-cases.md) — clock and calibration failures.
