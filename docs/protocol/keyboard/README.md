# PC-AT keyboard (KEYB)

**Status:** v4 default image; the default v3 build excludes it.

KEYB is the firmware's PC-AT / PS/2 keyboard mode.  It drives a keyboard's
clock/data interface, prints the raw scancodes it receives, and can send host
commands to the keyboard.  It does not translate scancodes into characters or
quoted strings into typed text.

## First key in five minutes

1. **Wire** DATA to `MOSI`, CLOCK to `CLK`, keyboard VCC to `5.0V` and GND to
   `GND`.  Each line needs a pull-up: the keyboard's own, or external 1–10 kΩ
   to 5 V.
2. **Enter** `m` then `8`, or `m8`.  KEYB has no setup prompts.  On a default
   v3 build `m8` selects PIC; KEYB is not in that image.
3. **Engage** with `W`: the 5 V rail comes on and CLOCK is driven low (the
   idle inhibit).  A keyboard powered from `5.0V` cannot answer before this;
   an externally powered keyboard can clock without it.
4. **Pull-ups** if needed: on v4, `e3` selects the on-board 5 V source and
   `P` connects the resistors.  On v3 there is no `e`; fit external resistors.
5. **Read** with `(1)` for the live monitor, `r` for one scancode or `r:N`
   for N.  The monitor's exit key is on the terminal, not the keyboard.

`(0)` lists the two macros.  `w` powers down; `m` leaves the mode.

This README is a map, not the manual — the details live in the files below.

| File | Covers |
|---|---|
| [entry-and-setup.md](entry-and-setup.md) | mode 8, no setup prompts, defaults, `W`, `i` / `v`, v3 availability |
| [wiring.md](wiring.md) | DATA/CLOCK, connector pinouts, pull-ups, 5 V supply, open drain |
| [protocol.md](protocol.md) | the 11/12-slot frame, odd parity, ACK, clock ownership, bit order, read window |
| [usage.md](usage.md) | the live monitor `(1)`, `r`, `r:N`, host writes, the `(0)` list, limits |
| [scancodes.md](scancodes.md) | make/break, the Set 2 roster and code index, Set 1, display settings |
| [edge-cases.md](edge-cases.md) | silent monitor, fail words, presence test, power/pull-up faults |

## Names used in this set

| Form | Meaning |
|---|---|
| `KEYB` / `KEYB>` | the firmware mode name and its prompt |
| keyboard | the PC-AT or PS/2 keyboard being read |
| PC-AT / PS/2 | the same clock/data interface; PS/2 is the 6-pin mini-DIN version |
| `MOSI` / `DATA` | firmware pin name / keyboard connector name for the data wire |
| `CLK` / `CLOCK` | firmware pin name / keyboard connector name for the clock wire |
| `5.0V` / `VCC` / `+5 V` | switched 5 V rail / keyboard supply pin |
| `Vpullup` / `VPU` | pull-up reference pin (`Vpullup` in messages, `VPU` in the `v` table) |

`PC-AT` names the interface; `PS/2` is the common 6-pin mini-DIN form of the
same interface.  The `e` menu writes the on-board 5 V source as
`Onboard 5.0v`, and its confirmation message writes `5V`; both are the 5 V rail
above.  The `e` command exists on v4 only.

## Availability (summary)

| Item | v3 | v4 |
|---|---|---|
| build | optional (not in the default image) | present in the default image |
| menu slot | 8 when enabled | 8 (`[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]`) |
| hardware | same keyboard wiring | a PC-AT or PS/2 keyboard on MOSI/CLK + 5 V + GND |
| live monitor `(1)` | available in a custom build | yes |
| host writes | available in a custom build; responses vary | yes |
| binary sub-protocol | none | none |

The default v3 image omits `KEYB` (PIC is 8 and DIO is 9).  The module can be
compiled into a v3 build, but that build is **unverified on v3**.

## Limits & out of scope

[usage.md §Limits](usage.md#limits) owns the limit list: raw bytes only, no
key-to-character decoding, no make/break interpretation, no monitor buffer,
and no binary (BBIO) path.

## Related docs

- [generic/](../../generic/README.md) — the shared prompt: `r`, `:N`, `;N`,
  `o`, `l`/`L`, `W`/`w`, `P`/`p`, `e`, `(n)`, `#`, `$`.
- [generic/navigation.md](../../generic/navigation.md) — the mode menu and `i`.
- [generic/values.md](../../generic/values.md) — width, display format, bit
  order.
- [binary/sub-protocols.md](../binary/sub-protocols.md) — the mode has no
  binary entry.
- [SPEC.md](../../SPEC.md) — the KEYB row.
