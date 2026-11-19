# LCD (HD44780)

**Status:** optional build; off in both default images.  The published
adapter revisions are documented in [wiring.md](wiring.md); the board you have
and the on-display result are `unverified`.

The **LCD** mode drives an **HD44780** character display through the
**Bus Pirate LCD adapter**, a separate board built around a **74HCT595** or
**74HC595** shift register.  The Bus Pirate shifts one byte at a time over
its **SPI** pins; the adapter's outputs drive the display's 4-bit interface.
The mode's prompt is `LCD>`.  This README is a map, not the manual; the
details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | the optional build and build status, the build-dependent menu number, entry, defaults, `i`/`v`, `W`/`w`, leaving |
| [wiring.md](wiring.md) | the adapter revisions, the Bus Pirate pins, the eight shift-register outputs, display power, contrast, backlight, the pull-up statement |
| [spi-link.md](spi-link.md) | how the mode uses the SPI hardware, the latch sequence, and how it differs from the SPI mode |
| [display.md](display.md) | the HD44780 path: init modes, display sizes and addresses, CGRAM, instruction bytes |
| [commands.md](commands.md) | register select, write forms, `r`, the `(0)` macro set, shared keys |
| [usage.md](usage.md) | worked terminal sessions |
| [edge-cases.md](edge-cases.md) | concrete failure modes and recovery |

## Names used in this set

| Form | Meaning |
|---|---|
| `RW` (pin table) / `R/W` (prose) | the display's read/write line; always written low |
| `AUX0` (v4b adapter jumper label) | the Bus Pirate `AUX` pin; drive it with `a` / `A` |
| `Vpullup` (warning) / `VPU` (`v` table) / `Vpu` (v4 select prompt) | the pull-up supply pin; three firmware forms |
| `3.3V` / `5.0V` (captures) | firmware strings; prose writes `3.3 V` / `5.0 V` |
| `Multiple` | the two-line init choice; `1` is one-line |
| `unverified` / `not verified` | the same state: no board confirmation in this set |

## The five-minute version

- **Not in the stock images.**  The mode appears only in a build that enables
  the LCD adapter; on v4 the all-features build includes it and builds with
  SPI enabled.  Enabling it is a firmware configuration choice, not a runtime
  setting ([entry-and-setup.md](entry-and-setup.md),
  [dev/building.md](../../dev/building.md)).
- Enter with `m` and answer the menu, or `m<N>`.  There are **no setup
  prompts**.  In the all-features v4 build `LCD` is entry 9; in a build that
  includes it, the number is positional, so read your own `m` list.  On a
  stock image `m9` is `PIC` on v4 and `DIO` on v3, not LCD; if `LCD` is absent
  from the `m` list, the image does not have it.
- `W` switches the regulated supplies on, configures the SPI link and prints
  `This mode requires an adapter`; then `(2) 2` initialises a two-line display.
- `}` selects the data register; `"Hello"` writes text.  `{` selects the
  command register; `0x01` sends a controller instruction.
- `(0)` lists the mode macros: reset, init, clear, cursor, and two test
  patterns.  There is no macro 5.
- Wiring: adapter on **MOSI**, **CLK** and **CS**, plus a **3.3 V or 5.0 V**
  supply rail and **GND**; **MISO is unused**.  **No pull-up is required** in
  the normal case; a binary SPI session that left the outputs open drain is
  the exception ([edge-cases.md](edge-cases.md#open-drain-carry-over)).  `i`
  still says `Open drain outputs`; that is the mode's flag, not the pad state.
  Contrast is set on the adapter or display, not by the Bus Pirate
  ([wiring.md](wiring.md)).

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | optional; off in the default image | optional; off in the default image, on in the all-features build |
| hardware | Bus Pirate LCD adapter + HD44780 display | same |
| terminal `LCD` mode | present only in a build that enables it; unverified on v3 | present only in a build that enables it; unverified |
| binary path | none | none |
| pull-up | none required | none required |
| supply | `W` switches the on-board regulated rails; the display VDD normally comes from the adapter's selected rail | same |

The full wiring, the build path and the address map are in
[wiring.md](wiring.md), [entry-and-setup.md](entry-and-setup.md) and
[display.md](display.md).

## Limits & out of scope

- **Using this requires building and flashing custom firmware.**  A stock v3
  or v4 unit has no `LCD` entry, and no image with LCD is distributed.
- **Reading the display is not implemented.**  `r` reports no effect and the
  dispatcher follows it with a syntax-error line.
- **No firmware backlight or contrast setting.**  Contrast is the adapter's
  V0 pot; the v3a also has a manual Backlight pot.  The v4b can be jumpered
  for fixed on or AUX0 control.  The shift register's eight outputs are all
  assigned to the display interface and the activity LED.
- **No speed or timing setting.**  The SPI shift clock is fixed; the display
  is always driven in 4-bit mode with fixed delays and no busy-flag polling.
- **No 4-line-specific init.**  `(2)` offers one-line and two-line modes; a
  single-controller 4-line glass may work with the two-line init and explicit
  `(4)` addresses, but that is `unverified`.
- The board revision you have and the on-display result are `unverified`;
  confirm them on the board and the display.  The published revisions'
  connector and supply details are in [wiring.md](wiring.md).
- Shared prompt grammar (`W`/`w`, `r`, `(n)`, `<n>`, `o`, `P`/`p`, the wrapper
  keys) belongs to [docs/generic/](../../generic/README.md) and is not repeated
  here.

## Related docs

| Page | Why |
|---|---|
| [docs/generic/README.md](../../generic/README.md) | prompt grammar, mode entry, `i`/`v`, reset |
| [docs/generic/usage.md](../../generic/usage.md) | the `v` table layout, mode change, reset |
| [docs/protocol/spi/README.md](../../protocol/spi/README.md) | the SPI mode that shares the same pins |
| [docs/protocol/binary/sub-protocols.md](../../protocol/binary/sub-protocols.md) | where the binary layer has no LCD path |
| [docs/dev/building.md](../../dev/building.md) | select the features and build the custom image |
| [docs/dev/flashing.md](../../dev/flashing.md) | write the built image to the Bus Pirate |
| [docs/dev/code-map.md](../../dev/code-map.md) | maintainer map of the module |
| [docs/SPEC.md](../../SPEC.md) | function and capability index |
