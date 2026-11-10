# LCD — edge cases and failure modes

Part of the LCD documentation set (`docs/capabilities/lcd/`).  Each entry gives
the trigger, the observable symptom, the behaviour, and the recovery.
Everything here is code-derived and `unverified` where the adapter or the
on-display result is involved.

## Failure table

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| the mode is not in the menu | no `LCD` entry in `m`; `m9` replies `Nonexistent protocol!` | the mode is not compiled in, so it cannot be selected | build an image with the LCD feature enabled ([entry-and-setup.md](entry-and-setup.md#build-it-in)), or use a different mode |
| writes or text before `W` | nothing appears on the display | the supplies are off and the bus is parked (`Clutch disengaged!!!`); the SPI pins are not driven | send `W`, then write |
| `W` sent and the supply fails | `VREG too low, is there a short?` then `POWER SUPPLIES OFF` | the SPI setup and the `This mode requires an adapter` line do not run; the supply is switched off | check for a short or excess load, then send `W` again ([details](#w-supply-failure)) |
| no adapter, or SPI pins / power miswired | `W` still prints `This mode requires an adapter`; the display stays blank | no acknowledgement is checked, so a missing adapter is silent | check the adapter's power and the MOSI / CLK / CS wiring ([wiring.md](wiring.md)) |
| `w` sent, then the pins must be safe | the display loses power if the adapter is BP-powered; MOSI, CLK and CS may stay driven | `w` does not run the board reset, so the pin directions are unchanged | use `m` (mode change) or `#` for a full Hi-Z state |
| `(5)` entered | `Unknown macro, try ? or (0) for help` | the firmware has no macro 5; only `0`–`4`, `6`, `7` exist | use `(1)`–`(4)`, `(6)`, or `(7)` as listed by `(0)` |
| `r` sent | `READ: ERROR: command has no effect here`, `0x00`, then `Syntax error at char 1` | the read handler is the no-effect one and sets the command error | none — the display content cannot be read back; keep a copy of what you wrote |
| `(2)` with no argument or a bad one | after `RESET`, `Display lines:` / `1. 1` / `2. Multiple` and the `(2)>` prompt | an argument outside `1`–`2` is treated as "not given"; the default is `2` (Multiple) | answer `1` or `2`, or press Enter for Multiple |
| display blank or garbled after init | nothing, or stray characters, appear | the display was not initialised, was initialised for the wrong number of lines, or its supply / contrast is wrong | run `(2)` and match the line mode; check the supply and the display's V0 contrast ([details](#display-blank-or-garbled)) |
| leaving and re-entering | the display is cleared or keeps its content, depending on its supply; the mode returns in HiZ | leaving runs the board reset, which switches the regulated VCC off; the Bus Pirate stores no display state | send `W`, then `(2)` to re-initialise if needed ([details](#leaving-and-power)) |

## `W` supply failure

`W` first enables the regulator and samples both rails.  Only when both read
good does it print `POWER SUPPLIES ON` and run the mode's SPI setup.  If a
rail reads low:

```
LCD>W
VREG too low, is there a short?
POWER SUPPLIES OFF
LCD>
```

The regulated supply is switched off again, the SPI setup does not run, and
the `This mode requires an adapter` line is **not** printed.  That is why a
failed `W` shows no adapter reminder even though the mode has one.  Remove the
short or excess load before retrying; a BP-powered adapter or display on the
same rail can also cause the low reading, so check it as well.

## Pin labels look like I2C

- **Trigger:** `i` or `v` while in LCD mode (both print the pin/voltage
  table).
- **Symptom:** the mode's pin-state row prints four signal labels borrowed
  from the I2C mode rather than LCD/SPI names: `-`, `-`, `SCL`, `SDA` on v4,
  and the same four in the other order, `SCL`, `SDA`, `-`, `-`, on v3.
- **Behaviour:** the LCD mode prints I2C's pin-state string, so the labels do
  not describe the LCD wiring; neither revision names MOSI, CLK, MISO, or CS.
  This is the same row `i` shows inside its status block
  ([entry-and-setup.md](entry-and-setup.md#status-output-i-and-v)).
- **Recovery:** informational only — read the rail columns for the supplies
  and treat the four pin labels as not meaningful in this mode.  The active
  signals are the SPI pins (see [wiring.md](wiring.md)).
- **Unverified:** the exact rendering (including alignment) of this row is not
  confirmed.

## Display blank or garbled

- **Trigger:** the display was not initialised, was initialised for the wrong
  number of lines, or the adapter supply / display contrast is wrong.
- **Symptom:** nothing, or stray characters, appear.
- **Behaviour:** the display needs the 4-bit wake-up sequence before it
  accepts commands; `(1)` performs only the wake-up, `(2)` performs wake-up
  and configuration, and `(3)` clears.
- **Recovery:** run `(2)` and answer the line-mode prompt to match the
  display; confirm the adapter supply and the display's V0 contrast on the
  display board itself (a property of the display, not the Bus Pirate).

## Leaving and power

- **Trigger:** `w`, a mode change (`m`), or a reset (`#`) after the display
  has content.
- **Symptom:** the display either clears or keeps its content.
- **Behaviour:** all three paths switch the regulated VCC off.  A display
  powered from the Bus Pirate VCC rail therefore loses power and clears; a
  display with its own supply keeps its content.  A mode change and `#` also
  run the board reset, which makes the bus lines inputs.
- **Recovery:** on re-entry send `W`, then `(2)` to power and re-initialise.
  The full table is in
  [entry-and-setup.md](entry-and-setup.md#leaving-the-mode).

## v3 / v4

The failure modes above are the same on both builds; the v3 build is **not
verified**.
