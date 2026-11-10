# Command reference — base layer

Part of the generic-layer documentation set (`docs/generic/`).  This page is
the single command-table authority for the shared CLI;
[navigation.md](navigation.md) explains the same navigation controls in prose.

## Wiring

The prompt is a serial interface; the *commands* have wiring implications.
Stated for every element command:

| Command | Wiring / electrical requirement |
|---|---|
| `W` / `w` | `W` enables the on-board VREG; target VCC is sourced on the VCC pin.  Aborts with `VREG too low, is there a short?` if the rails do not come up.  `w` disables VREG and re-parks the bus (`Clutch disengaged!!!`).  Both are refused in HiZ. |
| `P` / `p` | `P` ties the I/O lines to VPU through the pull-up resistors.  A voltage must already be present on the physical **Vpullup** pin or the firmware prints `Warning: no voltage on Vpullup pin`.  `p` releases the pull-ups.  Both are refused in HiZ. |
| `e` | (v4) selects the VPU pull-up source: 1 External/none, 2 on-board 3.3 V, 3 on-board 5.0 V.  Refused in HiZ, in a mode whose output type is not open drain, and when a voltage is already on VPU.  There is no `e` on v3. |
| `a` / `A` / `@` | drive / read the currently selected AUX pin (see `c`/`C`/`k`/`K`). |
| `f`, `g`, `S` | use the AUX pin as input (frequency) or output (PWM/servo).  `g`/`S` are refused in HiZ. |
| `d`, `D` | sample the dedicated ADC/probe pin (see [ADC capability](../capabilities/adc/README.md)). |
| `v` | samples the 5 V, 3.3 V, VPU and ADC rails and reads the pin states. |

### Feeding the VPU pin

The pull-up resistors connect the I/O lines to whatever voltage is on the
**Vpullup** pin — the pin marked `Vpu` ("input pull-up resistors 0-5 V")
on the reference card; the firmware calls it "Vpullup".  The per-board
route is:

- **v4** — the pin can be fed by an on-board source.  `e1` selects
  **External (or None)**, `e2` routes the on-board **3.3 V** rail to VPU, and
  `e3` routes the on-board **5.0 V** rail.  `e` is only accepted in a non-HiZ
  mode whose output type is open drain, with no voltage already on VPU; enter
  a mode (e.g. `m3`), answer its output-type prompt with open drain, then issue
  `e2`/`e3`.  From the `HiZ>` prompt the command is refused.
- **v3** — there is no `e` command, and the v3 firmware does not switch any
  on-board source onto VPU.  The Vpullup pin must be fed from an external
  0-5 V supply.  (v3 routing is not verified.)

Pull-up / supply requirements are never implied — the table above is the
authority for the base-layer commands; per-mode docs own the bus-specific
requirements (e.g. I2C needs pull-ups).

## Command reference

Base-layer commands are handled before the active mode sees them; mode-specific
effects are called out in the tables.

### Navigation, display and info

| Input | Action | v3/v4 |
| --- | --- | --- |
| `i` | version + chip info; `+status` when not HiZ | v3 prints the banner at boot; v4 does not |
| `?` | help screen (23 lines) | help text differs (see [navigation.md](navigation.md)) |
| `m` / `m<N>` | mode menu / direct change | — |
| `h` | command history list + recall prompt `(x. exit)` | 7 vs 15 entries |
| `b` / `b<N>` | terminal baud rate | v4 prints one line (host-controlled); v3 opens the rate menu |
| `o` / `o<N>` | display format HEX/DEC/BIN/RAW | — |
| `v` | pin states + rail voltages | pin set differs (10 vs 12 columns) |
| `t` / `T` | elapsed timer: read (µs) / reset | — |
| `~` | self-test | HiZ only |

### Value entry grammar

- `:` repeats the preceding command N times — for example `r:10` reads ten
  times, `0x55:3` writes `0x55` three times, `@:4` samples the AUX pin four
  times.
- `;` sets the read/write width in bits for the value — for example `0x55;2`
  sends `0x55` as a 2-bit value, and `r;16` reads 16 bits.

| Input | Action |
| --- | --- |
| `123` | decimal value, sent to the active mode |
| `0x1F` | hex value (typed `0x`/`0X`) |
| `0b101` | binary value (typed `0b`/`0B`) |
| `"abc"` | send the literal bytes of the string |
| `:N` | repeat the preceding command N times (`r:10`, `0x55:3`, `@:4`, `d:3`, `^:4`, `!:4`) |
| `;N` | read/write width in bits for this value (`0x55;2`, `r;16`) |
| `=X` | convert X → hex/decimal/binary line |
| `\|X` | bit-reverse X (within current width) then convert |

`X` must be 0…65535.  `;` accepts widths 1…16; a width >16 is a syntax
error.
`=X`/`|X` print `0x05 = 5 = 0b00000101` for `=5` and `0xA0 = 160 = 0b10100000`
for `|5`.

### Repetition / width grammar details

- `:N` is read only when the character immediately after the command is `:`.
  A bare command repeats once.
- `;N` is read only immediately after the value/repeat; it sets the width for
  subsequent reads and writes on that line.
- `r` takes an optional display letter: `rx` hex, `rd` decimal, `rb` binary,
  `rw` raw (the `w` here is the raw format, not supplies-off).  The letter is
  part of the command, so a one-shot display override works and combines with
  the suffixes — `rx:4` is four hex reads, `rd;16` one 16-bit decimal read.
  The override applies to that command only; the format set with `o` is
  unchanged.  In a mode where the read has no handler (HiZ), the read reports
  `ERROR: command has no effect here`.

### Elements

| Input | Action | Notes |
| --- | --- | --- |
| `W` / `w` | power supplies on / off | `W` prints `POWER SUPPLIES ON` + `Clutch engaged!!!`; refused in HiZ (`Command not used in this mode`) |
| `P` / `p` | pull-up resistors on / off | `P` warns `Warning: no voltage on Vpullup pin` if VPU is unpowered; warns `WARNING: pins not open drain (HiZ)` in push-pull mode; refused in HiZ |
| `e` | (v4) VPU pull-up source menu | `e1` External/none, `e2` 3.3 V, `e3` 5.0 V; refused in HiZ / non-open-drain / VPU already powered |
| `a` / `A` / `@` | AUX pin low / high / read | `@` prints `AUX INPUT/HI-Z, READ:` then `0` or `1`; `@:N` samples N times |
| `c` / `C` (`k`/`K` v4) | AUX assignment: targets `A0` / `CS` / `A1` / `A2` | prints `a/A/@ controls <pin> pin`; the pins show as `AUX` / `CS` / `AUX1` / `AUX2` (see below) |
| `l` / `L` | bit order MSB-first / LSB-first | `l` → `MSB set: MOST sig bit first`; `L` → `LSB set: LEAST sig bit first` |
| `d` / `D` | ADC one-shot / continuous voltmeter | `d:N` takes N readings; `D` exits on any key.  Scale and wiring: [ADC capability](../capabilities/adc/README.md) |
| `f` | frequency counter (AUX pin) | errors if PWM is currently active |
| `g` / `S` | PWM / servo generator (AUX pin) | refused in HiZ |
| `t` / `T` | elapsed timer read (µs) / reset | no help entry; read is not reset by reading |
| `~` | self-test | HiZ only; prompts `Disconnect any devices` / `Space to continue` |
| `&:N` / `%:N` | busy delay N µs / N ms | prints `DELAY <N>us` / `DELAY <N>ms` |

`l`/`L` values are reversed from intuition but match the help line
`l/L  Bitorder (msb/LSB)`: lowercase `l` selects MSB-first.

#### AUX targets and pin names

`c`/`C` (and `k`/`K` on v4) choose which pin the `a`/`A`/`@` commands act
on.  The firmware names the same target two ways: the `?` help lists the
assignment tokens `A0`/`CS`/`A1`/`A2`, while the confirmation message and the
`v` table use the pin name.  Both name the same pin:

| `?`-help token | Select with | Confirmation message | Pin name in the `v` table |
|---|---|---|---|
| `A0` (`aux` on v3) | `c` | `a/A/@ controls AUX pin` | `AUX` |
| `CS` | `C` | `a/A/@ controls CS pin` | `CS` |
| `A1` (v4) | `k` | `a/A/@ controls AUX1 pin` | `AUX1` |
| `A2` (v4) | `K` | `a/A/@ controls AUX2 pin` | `AUX2` |

After a board reset (`#`, `m`/`m1`, the binary return or the SUMP exit) the
default target is `A0` — the pin shown as `AUX`.  v3 offers only the first
two targets (`A0`/`AUX` and `CS`), and its help prints the pair as
`aux/CS`.  This set uses the pin-name form (`AUX`, `CS`, `AUX1`, `AUX2`)
throughout.

### Macros

| Input | Action |
| --- | --- |
| `<0>` | list user macros (slots 1..N) |
| `<n>` | run user macro n |
| `<n=body>` | assign macro n (body terminated by `>`) |
| `(0)` | list the current mode's macros |
| `(n)` | run mode macro n |

User macro count/width: v4 5 slots, v3 3 slots.  The whole `<n=…>` command
must fit the slot, so the usable body is about **22 bytes (v3) / 30 bytes
(v4)**; a longer line or an out-of-range slot is a syntax error.  Running
`<n>` replays the stored body as if typed.  `(n)` in a mode with no macros
(HiZ, 3WIRE, DIO) prints `ERROR: command has no effect here` plus a syntax
error.

**Note —** assignment syntax: `<n=` + body + `>` —
the trailing `>` terminates the body.  `<1=0x55>` stores `0x55`; `<0>` then
lists `1. <0x55>`; `<1>` replays the stored text as if typed.  The help line
`<x= >` is the terse form.

### Transaction wrappers and bit ops

The base layer routes these to the active mode.  The exact bus effect is
per-mode; the grammar is shared.

| Input | Meaning (grammar) |
| --- | --- |
| `[` | start (begin a bus transaction) |
| `{` | start with read |
| `]` | stop |
| `}` | stop-from-read / finish a read |
| `r` | read a value (`r:N`, `r;bits`, optional `x`/`d`/`b`/`w` display letter) |
| `/` | clock high (`CLOCK, 1`) |
| `\` | clock low (`CLOCK, 0`) |
| `^` | clock tick(s) (`^:N`) |
| `-` | data high (`DATA OUTPUT, 1`) |
| `_` | data low (`DATA OUTPUT, 0`) |
| `.` | read data pin state (`DATA STATE:` + `0`/`1`) |
| `!` | read bit(s) (`READ BIT: <bits> *pin is now HiZ`) |

`r` prints `READ: <value>`.  Writes via a value print `WRITE: <value>`.  In HiZ
these actions have no effect, so a write/read prints `ERROR: command has no
effect here` and a syntax error.

A short session showing `[ / - _ ^ \ ]` in 2WIRE mode is in
[usage.md](usage.md#wrappers-and-bit-ops-2wire).

### Firmware-level

| Input | Action |
| --- | --- |
| `#` | reset: v3 real reboot; v4 full terminal reset to HiZ + banner (see [usage.md](usage.md#reset-and-the-idle-state)) |
| `$` | confirm then jump to bootloader |
| `s` | enter the BASIC interpreter (prompt becomes `HiZ(BASIC)>`) |
| `O` | 1-Wire standard/overdrive toggle, in place (1-Wire only) |

`s` sets the BASIC flag; the interpreter launches on the next command line and
prints `Ready`.  Full behaviour: [../tools/basic/](../tools/basic/README.md).
`O` is dispatched by the base layer but is owned by
[../protocol/wire/1wire/](../protocol/wire/1wire/README.md); outside 1-Wire it
prints `Command not used in this mode`.

### Commands in HiZ

HiZ is the idle mode (see [README.md](README.md#idle-mode-hiz)).  The elements
that need a bus - `W`/`w`, `P`/`p`, `g`/`S` - and `O` (needs 1-Wire) print
`Command not used in this mode` and take no action.  `e` prints the same line
and, because it raises an error, is followed by `Syntax error at char 1`.
`~` runs only in HiZ; elsewhere it prints `Self-test in HiZ mode only`.  The
bus operations (`[` `{` `]` `}` `r` `/` `\` `^` `-` `_` `.` `!`, values and
`(n)`) print `ERROR: command has no effect here` and set a syntax error.  The
AUX pin is the one exception: `a`/`A` drive it and `@` reads it in HiZ.

## Leaving the CLI — binary / SUMP entry

Two terminal byte sequences leave the CLI instead of acting as commands:

1. **Binary bitbang:** 20× `0x00` (NUL bytes) → enters the binary bitbang
   layer and prints `BBIO1`.  Send `0x0F` to return to the terminal; on v4 the
   return runs the terminal reset and prints the banner, on v3 it is a real
   hardware reset.  Page: [../protocol/binary/README.md](../protocol/binary/README.md)
   — see [../protocol/binary/commands.md](../protocol/binary/commands.md) for
   the byte map.
2. **SUMP logic analyzer:** ≥ 5× `0x00` followed by a Left-arrow byte
   (`0x02`) → enters the SUMP logic-analyzer mode.  Leaving SUMP returns to
   HiZ with the banner.  Page: [../protocol/sump/README.md](../protocol/sump/README.md).

Neither is a "command"; both are documented here only so the generic page is
complete.
