# I2C — entry & setup

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns how the
mode is entered, the driver and speed choices, the one-line settings form, and
the bring-up commands that power the bus. Wiring is in [wiring.md](wiring.md);
the numbers behind the speed labels are in [speeds.md](speeds.md); the binary
interface has its own entry path in [binary.md](binary.md).

## Entering I2C

`I2C` is mode number **4** on both boards (HiZ is 1). The default v4 menu lists
ten modes; the default v3 build omits `KEYB`, so `PIC` and `DIO` move up one
place, but `I2C` stays 4. Take the number from the `m` menu of the build in
front of you. The shared menu grammar is in
[docs/generic/navigation.md](../../generic/navigation.md).

Three forms enter the mode:

| Form | What happens |
|---|---|
| `m` [Enter], then `4` at `(1)>` | menu form; the setup prompts run |
| `m4` or `m 4` | direct form; the setup prompts run |
| `m4 <values>` | direct form with settings on one line; the header is printed |

Only `m` with no number on the same line (or `m0`, which counts as no number)
opens the menu.

The menu form and the direct form differ in what they print after the settings
are applied:

| Form | After the settings are applied |
|---|---|
| `m` then `4` | `Clutch disengaged!!!`, the finish-setup prompt, `Ready` |
| `m4` | `Ready` only |
| `m4 <values>` | the settings header, then `Ready` |

All three end at the `I2C>` prompt. The bus is not driven until the first bus
operation; `W` is what powers the target rails (see §Bring-up).

### The driver prompt

On v4, and on a v3 build that compiles the hardware engine in (not the default
v3 build), the first prompt chooses the driver:

| Prompt | Option | Text | Driver |
|---|---|---|---|
| `I2C mode:` | `1` | `Software` | bit-banged SDA/SCL |
| | `2` | `Hardware` | the on-chip I2C engine |

The default is `1` (`Software`). A default v3 build has no hardware engine
compiled in, so it skips this prompt and goes straight to the software speed
prompt.

**Choosing:** Software works on both boards and is what the binary `I2C1` path
always uses. Choose Hardware for the engine's clock timing, or for the on-board
EEPROM macros 3/4 (v4 only), which are refused under Software (see
[macros.md](macros.md)).

### The speed prompt

Both drivers use the prompt title `Set speed:` but offer different lists:

| Driver | Options |
|---|---|
| Software | `1. ~6kHz`, `2. ~60kHz`, `3. ~150kHz`, `4. Maximum speed` |
| Hardware | `1. 100kHz`, `2. 400kHz`, `3. 1MHz` |

The default answer is `1` in both menus, so pressing Enter selects `~6kHz`
(Software) or `100kHz` (Hardware). The labels are nominal; the actual SCL rate
and what each profile changes are in [speeds.md](speeds.md).

### Menu-form example

The default v4 menu is shown and the remaining lines are abridged. Both prompts
accept their defaults:

```
HiZ>m
1. HiZ
2. 1WIRE
3. UART
4. I2C
5. SPI
6. 2WIRE
7. 3WIRE
8. KEYB
9. PIC
10. DIO
x. exit(without change)

(1)>4
I2C mode:
 1. Software
 2. Hardware

(1)>1
Set speed:
 1. ~6kHz
 2. ~60kHz
 3. ~150kHz
 4. Maximum speed

(1)>1
Clutch disengaged!!!
To finish setup, start up the power supplies with command 'W'
Ready
I2C>
```

The blank line before each `(1)>` marker is printed by the prompt reader, not
typed.

## The one-line settings form

On v4 the form is `m4 <driver> <speed>`, where the driver is `1` (Software) or
`2` (Hardware) and the speed is the menu number: software 1–4, hardware 1–3.

| Line | Enters |
|---|---|
| `m4 1 1` | Software, `~6kHz` |
| `m4 1 4` | Software, `Maximum speed` |
| `m4 2 1` | Hardware, `100kHz` |
| `m4 2 3` | Hardware, `1MHz` |

Rules:

- **Hardware with speed 4 is rejected** — the engine has three rates. The
  firmware sets the speed to zero and reruns the driver and speed prompts. Use
  `1`–`3` with Hardware.
- A missing or out-of-range value also falls back to the prompts; nothing is
  reported as an error.
- On a default v3 build the driver digit does not exist: the first number is
  the speed. `m4 3` selects `~150kHz`. A v4-style line (`m4 2 1`) is read as
  speed 2 on v3 and the trailing digit is discarded.

### The settings header

```
I2C (mod spd)=( 0 0 )
```

The header is **what the firmware prints**, and its numbers are **zero-based**,
unlike the one-based menu numbers typed into the settings line:

| Field | Meaning | Printed | Typed |
|---|---|---|---|
| `mod` | driver | `0` Software, `1` Hardware | `1` Software, `2` Hardware |
| `spd` | speed | `0`–`3`, zero-based | `1`–`4`, menu number |

A v4 Software session at the first speed prints `( 0 0 )`; a v4 Hardware
session at `1MHz` prints `( 1 2 )`; a default v3 session always prints `0` for
`mod`.

The header appears in two places:

- when the mode is entered with values on the line: `m4 1 1` prints
  `I2C (mod spd)=( 0 0 )` then `Ready`;
- in the `i` status block, which reprints it with the current values.

Entering through the prompts does **not** print the header. Typing the header
as the arguments to `m4` fails, because its labels, brackets and `=` are not
numbers; the prompts then run. Typing it at the `I2C>` prompt is a syntax
error.

## Output type — not asked

I2C has **no `Select output type:` prompt**, unlike SPI, UART and the raw
modes. Its two lines are always driven open drain, and the mode forces that
state itself. There is no push-pull option in the terminal and no `0x8x` group
in the binary layer.

The consequence is unconditional: the high level of both lines must come from a
pull-up. See [wiring.md](wiring.md).

## Bring-up: supplies, VPU and pull-ups

| Step | Command | Effect |
|---|---|---|
| select the VPU source (v4) | `e2` / `e3` | on-board 3.3 V or 5 V source on |
| power the rails | `W` | `POWER SUPPLIES ON`, then `Clutch engaged!!!` |
| enable the pull-ups | `P` | `Pull-up resistors ON` |
| release the pull-ups | `p` | `Pull-up resistors OFF` |
| park and remove the target rails | `w` | `Clutch disengaged!!!`, then `POWER SUPPLIES OFF` |

Recommended order is `e2`/`e3` first, then `W`, then `P`. `W` and `e` are
independent and may be issued in either order; only `P` must follow the VPU
source. A bus with an external pull-up to the target rail needs neither `e` nor
`P`.

`w` removes the target rails only: it leaves `P` and the VPU source as they
were. Use `p` and, on v4, `e1` to switch them off before rewiring.

```
I2C>W
POWER SUPPLIES ON
Clutch engaged!!!
I2C>e2
3.3V on-board pullup voltage enabled
I2C>P
Pull-up resistors ON
I2C>
```

Doing `P` first, with nothing on Vpullup, is allowed but warns because the
pull-ups have no reference:

```
I2C>P
Pull-up resistors ON
Warning: no voltage on Vpullup pin
I2C>e2
3.3V on-board pullup voltage enabled
I2C>
```

The resistor switch is on in both cases; without a voltage on **Vpullup** the
bus simply has no idle high. The exact strings and the `e` refusal cases are in
[docs/generic/commands.md](../../generic/commands.md#feeding-the-vpullup-pin).

Notes:

- `W` puts the Bus Pirate's 3.3 V and 5.0 V rails on their header pins. Leave
  those pins disconnected from a self-powered target.
- `W` is not needed to configure the driver: the software path is set up at mode
  entry, and the hardware engine initialises on its first start. `W` is the step
  that powers the target; skip it only for a self-powered target.
- The success check reads the Bus Pirate's own rails, not the target's VCC.
- A mode change, `#` or reset turns the rails off, turns the pull-ups off,
  clears the on-board VPU source to External (or None) on v4, and re-asserts the
  on-board EEPROM write protect. Re-issue `e`/`W`/`P` after re-entering.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `m4 2 4` is typed | the driver and speed prompts run | the engine has no fourth rate; use `1`–`3` |
| A value is missing or out of range | the prompts run, nothing is reported | answer the prompts, or type valid menu numbers |
| The header is typed after `m4` | the prompts run | type menu numbers; at the `I2C>` prompt the header is a syntax error |
| The printed `mod` is `1` after `2` was typed | the header is zero-based | `0` in the header is Software (typed `1`); `1` is Hardware (typed `2`) |
| The printed `spd` is one lower than typed | the header is zero-based | type the menu number; read the index |
| `m4` on a default v3 build | only the speed prompt appears | there is no hardware engine in the default v3 build |
| A v4 settings line is used on v3 | the first number is the speed; trailing digits are dropped | use `m4 <speed>` on v3 |
| `e` is issued with a voltage already on Vpullup | `Warning: already a voltage on Vpullup pin` plus a syntax error | remove the external source first |
| `P` is issued with no VPU voltage | `Pull-up resistors ON` then `Warning: no voltage on Vpullup pin` | select `e2`/`e3` on v4, or feed Vpu externally |
| `e` is issued on v3 | syntax error | v3 has no `e`; feed Vpullup externally |
| An output-type prompt is expected | it never appears | I2C is fixed open drain; the pull-up is mandatory |

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| mode number 4 | yes | yes |
| driver prompt | not in the default build | always asked |
| software speed menu | same four entries | same four entries |
| hardware speed menu | only in an optional build | same three entries |
| one-line settings form | speed only | driver + speed |
| settings header `mod` field | always `0` | `0`/`1` |
| output-type prompt | not asked | not asked |
| on-board VPU source (`e`) | no | yes |

The v3 column is **not verified**.

## Related docs

- [speeds.md](speeds.md) — the speed profiles, labels and binary operands.
- [wiring.md](wiring.md) — pins, pull-ups, Vpullup and the voltage limit.
- [bus-operations.md](bus-operations.md) — start, stop, read and write.
- [macros.md](macros.md) — the `(0)` list and macros 1–4.
- [binary.md](binary.md) — the `I2C1` entry and its own defaults.
- [edge-cases.md](edge-cases.md) — setup and bring-up failures.
