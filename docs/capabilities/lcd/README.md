# LCD (HD44780)

**Status:** optional build — **not in the default v3 or v4 images**.  The mode
is documented from the firmware; the adapter and the on-display result are
`unverified`.

The LCD mode drives an **HD44780** character display through the
**74HCT595** shift-register adapter.  The Bus Pirate shifts commands and text
into the shift register over its SPI pins; the adapter presents them to the
display's 4-bit interface.  The firmware names the mode **LCD** (its prompt is
`LCD>`); the display controller is the HD44780, and the adapter is the
74HCT595.  This README is a map, not the manual — the details live in the
files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | the build path, entering the mode, the menu number, defaults, status output, `W`/`w` |
| [wiring.md](wiring.md) | the adapter, Bus Pirate pins, supply, contrast, pull-up statement |
| [commands.md](commands.md) | the LCD-specific commands, the `(0)` macro set, controller instruction bytes |
| [usage.md](usage.md) | worked terminal sessions and custom characters |
| [edge-cases.md](edge-cases.md) | failure modes and known quirks |

## The five-minute version

- **Not in the standard images.**  The mode appears only in a custom build
  with the LCD feature enabled; enabling it is a firmware configuration
  choice, not a runtime setting ([entry-and-setup.md](entry-and-setup.md),
  [dev/building.md](../../dev/building.md)).
- Enter with `m` and answer the menu, or `m<number>`; there are **no setup
  prompts** — the mode always enters high-impedance.  In a build with every
  other optional protocol present, it is entry **9** of the mode menu.
- Use the **Bus Pirate LCD adapter** (a separate 74HCT595-based board, not a
  stock Bus Pirate accessory) on the SPI pins (MOSI, CLK, CS) plus VCC and
  GND; **no pull-up is required** (unverified).  See [wiring.md](wiring.md).
- `W` powers the supplies and prints `This mode requires an adapter`.
- `{` / `[` select the command register; `}` / `]` select the data register;
  a value (`0x41`, `"Hello"`) is written to whichever register is selected.
- `(0)` lists the macros: `(1)` reset, `(2)` init, `(3)` clear, `(4)` cursor,
  `(6)` test numbers, `(7)` test characters.

## Availability (summary)

| | v3 | v4 |
|---|---|---|
| build | optional — not in the default image | optional — not in the default image |
| hardware | Bus Pirate LCD adapter (74HCT595) + HD44780 display | same |
| pull-up | none required (unverified) | none required (unverified) |

The full wiring and the build caveat are in [wiring.md](wiring.md) and
[entry-and-setup.md](entry-and-setup.md).

## Limits & out of scope

- **Using this requires building and flashing custom firmware;** a stock v3
  or v4 unit does not have the mode, and no image with LCD is distributed.
- Reading the display is **not implemented** — `r` reports no effect and the
  dispatcher then prints a syntax error.
- There is no macro `(5)`; the historical "write test text" macro is gone.
- The mode is documented from the firmware only; the adapter, its pin-out and
  the on-display result are `unverified`.
- Shared prompt grammar (`W`, `r`, `(n)`, the wrapper keys, value entry) lives
  in [the generic layer](../../generic/README.md) and is not repeated here.

## Related docs

- [the generic layer](../../generic/README.md) — prompt grammar, `W`/`w`,
  `P`/`p`, `r`, reset and the idle state.
- [dev/building.md](../../dev/building.md) — build and flash the custom image.
- [dev/code-map.md](../../dev/code-map.md) — maintainer map of the module.
- [SPEC.md](../../SPEC.md) — function and capability index.
