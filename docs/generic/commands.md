# Command reference — base layer

Part of the generic-layer documentation set (`docs/generic/`).  This page is
the command-table authority for the shared layer: wiring, element commands,
the AUX target mapping, wrappers and bit ops, firmware-level commands, and the
byte sequences that leave the CLI.  Prompt mechanics are in
[navigation.md](navigation.md); value and format grammar is in
[values.md](values.md); macros are in [macros.md](macros.md).

## Wiring

The prompt is a serial interface; the commands act on the header pins.  The
pull-up and supply requirements are stated per command — never assume them.

| Command | Wiring / electrical requirement |
|---|---|
| `W` / `w` | `W` puts the on-board 3.3 V and 5.0 V rails on their header pins; `w` removes them.  Keep the target ground common; do not back-drive the rails.  Both are refused in HiZ. |
| `P` / `p` | pull-ups on / off.  They tie the I/O lines to **Vpullup**; put 0–5 V on that pin.  `P` warns and still enables them if it is missing.  Refused in HiZ. |
| `e` | (v4) selects what feeds Vpullup: external/none, on-board 3.3 V or 5.0 V.  Needs a non-HiZ open-drain mode and no voltage on the pin.  No `e` on v3. |
| `a` / `A` / `@` | drive the selected AUX target low / high, or read it as an input.  A push-pull drive needs no supply or pull-up; do not fight an external driver. |
| `f` | measures frequency on the AUX pin (input).  Do not drive the pin from the Bus Pirate at the same time. |
| `g` / `S` | PWM / servo output on the AUX pin.  Refused in HiZ. |
| `d` / `D` | sample the dedicated ADC/probe pin.  See [ADC](../capabilities/adc/README.md). |
| `v` | samples the 5 V, 3.3 V, VPU and ADC rails and reads the pin states.  No external wiring required. |

The exact warning and failure messages, in order, are in
[edge-cases.md](edge-cases.md#power-pull-ups-and-vpu).

### Feeding the Vpullup pin

The pull-up resistors connect the I/O lines to whatever voltage is on the
**Vpullup** pin — the pin marked `Vpu` ("input pull-up resistors 0-5 V")
on the reference card; the firmware calls it "Vpullup".  The route is:

- **v4** — the pin can be fed by an on-board source.  `e1` selects
  **External (or None)**, `e2` routes the on-board **3.3 V** rail, and `e3`
  routes the on-board **5.0 V** rail.  Enter a non-HiZ mode whose setup answers
  `Select output type:` with open drain (most bus modes default to it; DIO has
  no such prompt), then issue `e2`/`e3`, then `P`.  From `HiZ>` the command is
  refused.
- **v3** — there is no `e` command, and the v3 firmware does not switch any
  on-board source onto Vpullup.  Feed the pin from an external 0–5 V supply.
  This routing is not verified on v3.

Per-mode bus requirements (I2C pull-ups, SPI chip select, and so on) are owned
by the mode's page.

## Command reference

Base-layer commands are handled before the active mode sees them; the mode's
own effect is noted where it matters.  The `v3/v4` column marks a board
difference.

### Navigation, display and info

| Input | Action | v3/v4 |
|---|---|---|
| `i` | version and chip info; adds the status block when not HiZ | v3 also prints the banner at startup |
| `?` | the help screen | rows differ; see [navigation.md](navigation.md) |
| `m` / `m<N>` | mode menu / direct mode change | — |
| `h` | command history list and recall prompt | 7 vs 15 entries |
| `b` / `b<N>` | terminal baud rate | v4 prints one line; v3 opens the rate menu ([navigation.md](navigation.md)) |
| `o` / `o<N>` | display format HEX/DEC/BIN/RAW | — |
| `v` | pin states and rail voltages | 10 vs 12 columns |
| `t` / `T` | elapsed timer read (µs) / reset | — |
| `~` | self-test | HiZ only |

### Value entry, repeat and width

| Input | Action |
|---|---|
| `123`, `0x1F`, `0b101`, `"abc"` | send a decimal, hex, binary or string value |
| `:N` | repeat the preceding command N times |
| `;N` | set the read/write width in bits |
| `=X` / `\|X` | convert X / bit-reverse X and convert |
| `r` | read, with optional `x` / `d` / `b` / `w` display letter |

The full grammar, examples and limits are in [values.md](values.md).

### Elements

| Input | Action | Notes |
|---|---|---|
| `W` / `w` | power supplies on / off | `W` prints `POWER SUPPLIES ON` then `Clutch engaged!!!`; refused in HiZ |
| `P` / `p` | pull-up resistors on / off | see the wiring table |
| `e` | (v4) VPU pull-up source menu | `1) External (or None)`, `2) Onboard 3.3v`, `3) Onboard 5.0v` |
| `a` / `A` / `@` | selected AUX target low / high / input-and-read | `@` prints `AUX INPUT/HI-Z, READ: ` then `0` or `1` |
| `c` / `C` (`k` / `K`) | AUX assignment: `A0`, `CS`; v4 adds `A1`, `A2` | confirmation line names the pin; see below |
| `l` / `L` | bit order MSB-first / LSB-first | lowercase `l` is MSB-first |
| `d` / `D` | ADC one-shot / continuous voltmeter | `d:N` repeats; scale in the ADC page |
| `f` | frequency counter on AUX | refuses while a generator is active |
| `g` / `S` | PWM / servo generator on AUX | refused in HiZ; details in the AUX page |
| `t` / `T` | elapsed timer read / reset | plain decimal microseconds, no unit; `T` is silent |
| `~` | self-test | HiZ only; prompts before running |
| `&:N` / `%:N` | busy delay N µs / N ms | prints `DELAY <N>us` / `DELAY <N>ms` |

`t` reads a 31-bit microsecond count, so it wraps after about 35.8 minutes.
`&` and `%` take their count only as `:N`, not as a bare number; a bare `&` or
`%` is a 1 µs or 1 ms delay.

### `e` — the VPU source menu

On v4, when the command is allowed (non-HiZ, open drain, no voltage on VPU),
a bare `e` prints `Select Vpu (Pullup) Source:` and the prompt `(1)>`:

| Answer | Selection | Message |
|---|---|---|
| 1 | External (or None) | ` on-board pullup voltage disabled` |
| 2 | Onboard 3.3v | `3.3V on-board pullup voltage enabled` |
| 3 | Onboard 5.0v | `5V on-board pullup voltage enabled` |

`e1` / `e2` / `e3` select directly.  The command refuses with
`Command not used in this mode`, `WARNING: pins not open drain (HiZ)` or
`Warning: already a voltage on Vpullup pin`, and adds a syntax error in each
case.  `(HiZ)` in that warning names the high-impedance high state of an
open-drain output; it does not mean the board is in the HiZ mode.

### AUX targets and pin names

`c` / `C` (and `k` / `K` on v4) choose which pin the `a` / `A` / `@` commands
act on.  The firmware names the same target two ways: the `?` help lists the
assignment tokens `A0` / `CS` / `A1` / `A2`, while the confirmation message and
the `v` table use the pin name.  Both name the same pin:

| `?`-help token | Select with | Confirmation message | Pin name in the `v` table |
|---|---|---|---|
| `A0` (`aux` on v3) | `c` | `a/A/@ controls AUX pin` | `AUX` |
| `CS` | `C` | `a/A/@ controls CS pin` | `CS` |
| `A1` (v4) | `k` | `a/A/@ controls AUX1 pin` | `AUX1` |
| `A2` (v4) | `K` | `a/A/@ controls AUX2 pin` | `AUX2` |

After a board reset (`#`, `m` / `m1`, a mode change, the binary return or
the SUMP exit) the default target is `A0` — the pin shown as `AUX`.  v3
offers only `A0` / `AUX` and `CS`, and its help prints the pair as `aux/CS`.
This set uses the pin-name form (`AUX`, `CS`, `AUX1`, `AUX2`) throughout.

The drive and read messages use the literal string `AUX` even when the selected
target is `CS`, `AUX1` or `AUX2`: `a` prints `AUX LOW`, `A` prints `AUX HIGH`,
and `@` prints `AUX INPUT/HI-Z, READ: …`.  The confirmation line from
`c` / `C` / `k` / `K` is the record of the real target.

### Transaction wrappers and bit ops

The base layer routes these to the active mode.  The grammar is shared; the bus
effect and the reply wording belong to the active mode.

| Input | Meaning (grammar) | Typical reply |
|---|---|---|
| `[` | start a bus transaction | mode-specific start message |
| `{` | start with read; SPI and 3WIRE then read back on writes | mode-specific start message |
| `]` | stop; closes either start | mode-specific stop message |
| `}` | stop from read; runs the same stop as `]` | mode-specific stop message |
| `r` | read a value | `READ: <values>` |
| `/` | clock high | `CLOCK, 1` |
| `\` | clock low | `CLOCK, 0` |
| `^` | clock tick(s) | `CLOCK TICKS: <n>` |
| `-` | data high | `DATA OUTPUT, 1` |
| `_` | data low | `DATA OUTPUT, 0` |
| `.` | read the data pin state | `DATA STATE: 0` or `1` |
| `!` | read bit(s) | `READ BIT: <bits> *pin is now HiZ` |

The wrappers are not strict pairs: `]` and `}` both stop either start, and in
this firmware `}` is the same stop as `]` in every mode.  `{` differs from `[`
only in SPI and 3WIRE; those pages own how long the read-back setting lasts.
The base layer does not track whether a transaction is open, so in a bus mode
`r` and `r:N` also work outside a start/stop; the mode decides what an unframed
read means.

`r` prints the `READ: ` prefix once and then each value; a value line prints
`WRITE: ` once and then each value.  A non-standard width appends `;N` to each
printed value or read (see [values.md](values.md)).  The `^` count also uses
the current display format and width, but it never carries a `;N` suffix.

Not every mode implements every wrapper.  If the active mode has no handler,
the command reports `ERROR: command has no effect here` and then a syntax
error.  That is also what every bus operation does in HiZ.

A short 2WIRE session is in
[usage.md](usage.md#wrappers-and-bit-ops-2wire).

### Macros

| Input | Action |
|---|---|
| `<0>` | list the user-macro slots |
| `<n>` | copy user macro n onto the prompt line (press Enter to run it) |
| `<n=body>` | store a user macro; the first `>` ends the body |
| `(0)` | list the active mode's macros |
| `(n)` | run mode macro n |

User macros live in RAM and are recalled, not run, by `<n>`.  Mode macros are
owned by the active mode; HiZ, 3WIRE and DIO have none.  Counts, limits and the
recall example are in [macros.md](macros.md).

### Firmware-level

| Input | Action |
|---|---|
| `#` | reset.  v4: full terminal reset to HiZ, then the banner.  v3: hardware reset. |
| `$` | confirm, then reset into the bootloader. |
| `s` | enter BASIC.  No acknowledgement; the prompt gains `(BASIC)`.  See [BASIC](../tools/basic/README.md). |
| `O` | 1-Wire standard/overdrive toggle, in place.  Outside `1WIRE` it prints `Command not used in this mode`. |

`#` prints `RESET` first; on v4 it also discards the rest of the line, restores
HEX and prints `HiZ>`.  Inside BASIC it is a syntax error; type `EXIT` first.
`$` prints `Are you sure? `, reads one byte (`y` / `Y` accepts, anything else
cancels), then prints `BOOTLOADER` and resets.  The full `$` procedure,
including the v3 loader limit, is in [usage.md](usage.md).

### Commands in HiZ

HiZ is the idle mode ([README.md](README.md#idle-mode-hiz)).  The elements that
need a bus mode — `W` / `w`, `P` / `p`, `g` / `S` — and `O` (which needs
1-Wire) print `Command not used in this mode` and take no action.  v4 `e` also
prints that line, and because it flags an error it is followed by
`Syntax error at char 1`.

`~` runs only in HiZ; elsewhere it prints `Self-test in HiZ mode only`.

The bus operations (`[` `{` `]` `}` `r` `/` `\` `^` `-` `_` `.` `!`, a value
and `(n)`) print `ERROR: command has no effect here` and flag a syntax error.
Some start with their normal prefix, so you may see
`READ: ERROR: command has no effect here` or
`WRITE: 0x05 ERROR: command has no effect here`; a read also prints a zero
placeholder (for example `0x00` in HEX), and the syntax-error line follows.

The AUX pin is the exception: `a` / `A` drive it and `@` reads it in HiZ.
`c` / `C` (v4 `k` / `K`) choose the target, and `&` / `%` run their delays.
`i`, `?`, `m`, `h`, `b`, `o`, `v`, `t` / `T`, `d` / `D`, `f`, `l` / `L`,
`=` / `|`, `<n>`, `s`, `#` and `$` also work from HiZ (with their own limits).

## Leaving the CLI — binary and SUMP entry

Two byte sequences leave the CLI instead of acting as commands:

| Sequence | Enters | Returns |
|---|---|---|
| 20 consecutive `0x00` (NUL) bytes | binary bitbang, which replies `BBIO1` | v4: send `0x0F`, which runs the terminal reset and prints the banner + `HiZ>`.  v3: `0x0F` performs a hardware reset. |
| 5–19 consecutive `0x00` bytes, then a raw `0x02` (STX) | SUMP logic analyzer | send `0x0F`, `#` or `$` to return to HiZ with the banner (`$` is an exit here, not a bootloader jump) |

The NUL runs must be consecutive: any byte other than NUL or the raw STX resets
the counter, and the count does not survive Enter.  A raw `0x02` without enough
NULs is the left-arrow control byte, not an entry request.

Twenty NULs enter the binary layer, so the SUMP window ends at nineteen.  The
binary and SUMP pages own the rest:
[binary](../protocol/binary/README.md) ·
[SUMP](../protocol/sump/README.md).

## Related docs

- [navigation.md](navigation.md) — prompt, mode menu, `i`, `?`, `b`.
- [values.md](values.md) — value grammar, width, display.
- [macros.md](macros.md) — `<n>` and `(n)`.
- [usage.md](usage.md) — worked sessions.
- [edge-cases.md](edge-cases.md) — refusals and traps.
- [SPEC.md](../SPEC.md) — the capability matrix.
