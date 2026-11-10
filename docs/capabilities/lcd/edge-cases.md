# LCD — edge cases and failure modes

Part of the LCD documentation set (`docs/lcd/`).  Each entry gives the
trigger, the observable symptom, the behaviour, and the recovery.  Everything
here is code-derived and **unverified** — the adapter is not available for
testing.

## The mode is not in the menu

- **Trigger:** a default v3 or v4 image (LCD is an optional build).
- **Symptom:** `m` lists the modes but there is no `LCD` entry; `m9` replies
  `Nonexistent protocol!`.
- **Behaviour:** the mode is not compiled in, so it cannot be selected.
- **Recovery:** build an image with the LCD feature enabled (developer task,
  `docs/dev/`), or use a different mode.

## `W` not sent before writing

- **Trigger:** writing bytes or text right after entering the mode, before
  `W`.
- **Symptom:** nothing appears on the display.
- **Behaviour:** the bus is parked (`Clutch disengaged!!!`) and the supplies
  are off, so writes do not reach the shift register.
- **Recovery:** send `W` (supplies + SPI setup, prints
  `This mode requires an adapter`), then write.
- **Requires verification:** the exact behaviour of a write attempted before
  `W` (when the SPI interface is not yet configured) is not confirmed.

## Adapter not connected or miswired

- **Trigger:** no 74HCT595 adapter, or the SPI pins / power wrongly wired.
- **Symptom:** `W` still prints `This mode requires an adapter` (that line is
  unconditional), but the display stays blank and no text appears.
- **Behaviour:** the firmware shifts bytes out with no acknowledgement and
  never reads the display, so it cannot tell whether an adapter is present.
  A missing adapter is silent.
- **Recovery:** check the adapter's power and the MOSI / CLK / CS wiring (see
  [wiring.md](wiring.md)).  Because no ACK is checked, the bus reports success
  either way — the only feedback is the display itself.

## `(5)` is rejected

- **Trigger:** entering the historical test-text macro `(5)`.
- **Symptom:** `Unknown macro, try ? or (0) for help`.
- **Behaviour:** the firmware has no macro 5; only `0`–`4`, `6`, `7` exist.
- **Recovery:** use `(1)`–`(4)`, `(6)`, or `(7)` as listed by `(0)`.

## `r` reports no effect

- **Trigger:** sending `r` to read the display.
- **Symptom:** `READ: ERROR: command has no effect here`, followed by a read
  value of `0x00` on the next line (the error string is the base-layer one).
- **Behaviour:** reading is not implemented; the display's read line and busy
  flag are never used, and the mode's read callback does nothing but report the
  error.
- **Recovery:** none — the display content cannot be read back.  Keep a copy
  of what you wrote.

## `(2)` re-asks for the line mode

- **Trigger:** `(2)` with no argument, or with an argument other than `1`/`2`.
- **Symptom:** after `RESET`, the firmware prints
  `Display lines: / 1. 1 / 2. Multiple` and prompts `(2)>`.
- **Behaviour:** an argument outside `1`–`2` is treated as "not given" and the
  prompt is shown; the default is `2` (Multiple).
- **Recovery:** answer `1` or `2`, or press Enter for Multiple.

## `v` pin labels look like I2C

- **Trigger:** `v` while in LCD mode.
- **Symptom:** the mode's pin-state row prints four signal labels borrowed
  from the I2C mode rather than LCD/SPI names: `-`, `-`, `SCL`, `SDA` on v4,
  and the same four in the other order, `SCL`, `SDA`, `-`, `-`, on v3.
- **Behaviour:** the LCD mode prints I2C's pin-state string, so the labels do
  not describe the LCD wiring; neither revision names MOSI, CLK, MISO, or CS.
- **Recovery:** informational only — read the rail columns for the supplies
  and treat the four pin labels as not meaningful in this mode.  The active
  signals are the SPI pins (see [wiring.md](wiring.md)).
- **Requires verification:** the exact rendering (including alignment) of this
  row is not confirmed.

## Display blank or garbled after init

- **Trigger:** the display was not initialised, was initialised for the wrong
  number of lines, or the adapter supply is wrong.
- **Symptom:** nothing, or stray characters, appear.
- **Behaviour:** the display needs the 4-bit wake-up sequence before it
  accepts commands; `(1)` performs only the wake-up, `(2)` performs wake-up and
  configuration, and `(3)` clears.
- **Recovery:** run `(2)` and answer the line-mode prompt to match the display;
  confirm the adapter supply and contrast voltage on the display itself (a
  property of the display board, not the Bus Pirate).

## Leaving and re-entering

- **Trigger:** changing mode (`m`) and returning to LCD.
- **Symptom:** the display keeps its previous content; the mode returns in
  HiZ.
- **Behaviour:** leaving releases the SPI interface; nothing about the display
  is stored by the Bus Pirate, and the mode re-enters parked.
- **Recovery:** send `W`, then `(2)` to re-initialise if needed.

## v3 / v4

The failure modes above are the same on both builds; the v3 build is **not
verified**.