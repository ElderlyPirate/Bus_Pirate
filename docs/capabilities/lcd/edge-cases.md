# LCD — edge cases and failure modes

Part of the LCD documentation set (`docs/capabilities/lcd/`).  Each entry gives
the trigger, the observable symptom, the behaviour and the recovery.  The
adapter and the on-display result are `unverified`; the firmware's own replies
are quoted exactly.

## Failure table

| Trigger | Symptom | Behaviour | Recovery |
|---|---|---|---|
| `m9` on a stock image | enters `PIC` on v4, `DIO` on v3; no LCD | the mode is not compiled in | read your own `m` list; build an image with LCD ([entry-and-setup.md](entry-and-setup.md#the-menu-number-is-build-dependent)) |
| out-of-range direct form (`m12`) | `Nonexistent protocol!` | no mode change | use a number the menu lists |
| a write before `W` (clean entry) | `WRITE: ...` then `Syntax error at char N`; display unchanged | the SPI port is disabled on a clean entry, so the write fails | send `W`, then write ([details](#writes-before-w)) |
| a macro before `W` | the macro's word (`RESET`, `CLEAR`, ...) then a syntax error | the macro's writes fail the same way | send `W`, then the macro |
| `W` and a rail reads low | `VREG too low, is there a short?` then `POWER SUPPLIES OFF`; no adapter line | the SPI setup and the adapter reminder do not run | remove the short or excess load; `W` again ([details](#w-supply-failure)) |
| no adapter, or miswired SPI/power | `W` still succeeds and prints the adapter line; the display stays blank | no acknowledgement is checked, so a missing adapter is silent | check supply, MOSI/CLK/CS, ground and contrast ([wiring.md](wiring.md)) |
| `r` | `READ: ERROR: command has no effect here`, `0x00`, then `Syntax error at char 1` | the read handler is the no-effect one | none; the display cannot be read back |
| `(5)` | `Unknown macro, try ? or (0) for help` | there is no macro 5 | use the macros `(0)` lists |
| `(4:0)` | `Syntax error at char 3` | the colon form is not parsed | use `(4) 0` or `(4)0` |
| `(2)` with a bad or missing argument | after `RESET`, the line-mode menu and the `(2)>` prompt | a missing, `0`, or outside `1`-`2` argument is treated as not given; default `2` | answer `1` or `2`, or press Enter |
| text before `(1)`/`(2)` | blank or stray characters | the controller has not been put into 4-bit mode with a known line mode | initialise after power-up ([details](#text-before-initialisation)) |
| a two-line glass initialised as `(2) 1` | the second row stays blank | the function set selects one-line mode | re-run `(2) 2` |
| a 4-line glass with a second controller/enable | the extra lines may not show | the mode has no 4-line-specific init and the adapter provides one EN; a single-controller 4-line glass may work with `(2) 2` and `(4)` addresses (`unverified`) | use a 2-line glass, or handle the extra controller externally |
| `(4)` with an address above 127 | the cursor lands at the wrapped address | only the low seven bits reach the controller | use `0`-`127` |
| `(6)` or `(7)` with `0` or no argument | 80 characters are written | `0` means 80, not "none" | give an explicit count |
| `(6)` or `(7)` with a large count | characters continue into off-screen addresses | the count is bytes, not a row width | position with `(4)` first |
| `L` set | bytes are mirrored; text is garbled | the dispatcher reverses the value before the shift | return to `l` ([spi-link.md](spi-link.md#shared-settings-that-still-affect-a-write)) |
| `;N` other than 8 | the `WRITE:` echo uses the selected width (and a `;N` suffix unless 16); with MSB the shifted byte is unchanged | the width affects the echo, not the 8-bit shift; with `L` it also sets the reversal width | use `;8` for a plain echo |
| `/`, `\`, `-`, `_`, `.`, `^`, `!` | the shared prefix, then `ERROR: command has no effect here`, then a syntax error | the LCD handlers for these are null | use the register-select keys and a write |
| open-drain carry-over from a binary SPI session | weak highs; the adapter may not latch reliably | the open-drain bits persist and the LCD setup does not clear them | power-cycle; or select normal SPI outputs and send `W` ([details](#open-drain-carry-over)) |
| SMPS feedback active (all-features build) | `ADC busy: SMPS feedback active; command rejected.` then `Syntax error at char 1`; `W` takes no supply action | `W` refuses while SMPS owns the converter | stop SMPS first ([SMPS exit](../smps/exit.md)) |
| `w`, then a write | the same error as before `W` | the cleanup disabled the SPI port and released the pads | send `W` again |
| leaving the mode | the display clears or keeps its content | the board reset cuts the adapter's rail; a separate display feed stays on | re-`W`; `(2)` if the display lost power ([details](#leaving-and-power)) |

## The stock-image trap

`LCD` is not in the default v3 or v4 image, and the menu number is positional.
On the default v4 image, entry 9 is `PIC`; on the default v3 image, entry 9 is
`DIO`.  Typing `m9` expecting LCD therefore enters a working mode with the
wrong pin behaviour.  Check the `m` list, or the mode list in the `i` banner;
if `LCD` is absent, the image does not have it.

## Writes before `W`

`W` is what configures the SPI hardware.  On a clean entry, before `W` the
write handler still echoes the value, then reports the failure through the
dispatcher:

```
LCD>0x41
WRITE: 0x41
Syntax error at char 4
LCD>
```

The char number is the parse position, so it changes with the line length.  A
quoted string behaves the same way (`WRITE: "Hi"` then a syntax error); each
character is attempted, and one syntax-error line follows.  Send `W` and retry.

## `W` supply failure

`W` enables the regulator and checks both rails before running the mode's SPI
setup.  If either rail reads low:

```
LCD>W
VREG too low, is there a short?
POWER SUPPLIES OFF
LCD>
```

The SPI setup does not run, so the adapter reminder is **not** printed.  Check
for a short or excess load on the rails; a Bus Pirate-powered adapter or
display is on the same supply and can cause the low reading too.  Retry `W`
after fixing it.

## Display blank or garbled

- **Trigger:** the display was never initialised, was initialised for the
  wrong number of lines, or its supply/contrast is wrong.
- **Symptom:** nothing, or stray characters.
- **Behaviour:** `W` only powers the link; it does not initialise the
  controller.  The controller needs the 4-bit wake-up before it accepts
  nibbles as intended.
- **Recovery:** run `(2)` and answer the line-mode prompt to match the glass;
  confirm the display's VDD/VSS, the common ground and the V0 contrast.  If
  the display is powered but blank, adjust contrast before suspecting the data
  path ([wiring.md](wiring.md#display-power-contrast-and-backlight)).

## Text before initialisation

The controller powers up in 8-bit mode.  The adapter is wired for 4-bit, so
text sent before `(1)`/`(2)` is split into nibbles that the controller does not
interpret as a complete character.  The result is undefined and usually blank
or stray characters (`unverified`).  Always send `(2)` after the display has
powered up, including after `w` or a mode change cut its supply.

## Open-drain carry-over

- **Trigger:** an earlier **binary** SPI session configured open-drain SPI
  outputs, and the terminal was entered again without a power cycle.  The
  terminal SPI mode clears its own open-drain setting when it is left; the
  binary path does not run that cleanup.
- **Symptom:** the adapter may not see clean logic highs; `i` reports
  `Open drain outputs`, which is also the mode's normal status text.
- **Behaviour:** the LCD setup does not write the per-pin open-drain bits, and
  neither the mode change nor the v4 terminal reset clears them.  A high is
  then a released pin, not a driven high.  The `i` line alone cannot
  distinguish the two cases.
- **Recovery:** power-cycle the Bus Pirate (on v3, `#` reboots the CPU and also
  clears it).  Alternatively, enter the SPI mode, select normal outputs, send
  `W` once, then leave for LCD.

## Leaving and power

`w`, a mode change and `#` all switch the regulated rails off.  A display
whose VDD comes from the adapter's Bus Pirate rail therefore loses power and
clears; a display with its own VDD feed keeps its content.  A mode change and
`#` also run the board reset, which releases the pins.  On re-entry send `W`,
then `(2)` if the display lost power
([entry-and-setup.md](entry-and-setup.md#leaving-the-mode)).

## v3 / v4

The failure modes and the mode messages are the same on both builds.  The
default menu numbers differ, and the v3 hardware path is **unverified on
v3**.
