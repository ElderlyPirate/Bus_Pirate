# PC keyboard (KEYB) — wiring and electrical requirements

Part of the KEYB documentation set (`docs/protocol/keyboard/`).  This page owns
the wiring table and the supply/pull-up statement; the README carries the
headline only.

**KEYB is v4 only.**  A v3 build has no keyboard mode and no equivalent, so
this page does not apply to v3 hardware.

## Wiring

A PC/AT keyboard needs four connections: 5 V, ground, and the clock/data
interface.  The keyboard's two signals map to the Bus Pirate's MOSI and CLK
pins:

| Bus Pirate pin | Direction | Keyboard signal | Description |
|---|---|---|---|
| MOSI | ↔ | DATA | keyboard data; the Bus Pirate also drives it when sending |
| CLK | ↔ | CLOCK | normally clocked by the keyboard; the Bus Pirate can pull it low |
| 5.0V | → | +5 V (VCC) | keyboard supply, switched on by `W` |
| GND | ⏚ | GND | common ground |

Naming (signals): the firmware's pin table calls the two interface pins **MOSI**
and **CLK**; the keyboard's own connector calls the same wires **DATA** and
**CLOCK**.  This set uses the Bus Pirate labels and states the mapping once
here.

Naming (supply): the firmware's pin table labels the switched 5 V output pin
**5.0V**; this set also writes it as the **5 V rail**, and the keyboard
connector calls it **VCC** (`+5 V`).  All refer to the same switched supply.

A standard PC/AT keyboard uses the 6-pin mini-DIN connector: 1 DATA, 3 GND,
4 +5 V, 5 CLOCK (2 and 6 unused).  Check your own keyboard's pinout before
wiring.

## The pull-up

**A PC/AT keyboard may or may not carry its own DATA/CLOCK pull-ups.**  In the
classic AT interface the pull-ups are often supplied on the host side, so this
set does not assume the keyboard provides them.  Each open-drain line must idle
high between exchanges: a pull-up provides the high level and the 1 bits, while
the keyboard and the Bus Pirate only ever pull a line low.

What a missing or weak pull-up looks like:

- **No pull-up at all** — the open-drain lines have no solid high level.  The
  clock may never reach its high phase (a read reports `TIMEOUT`, monitor
  silent), or a frame may arrive corrupt (`*parity error` / `*stopbit error`);
  with nothing driving the lines, a read can also report `NONE`.  Fit the
  pull-up before diagnosing further.
- **A weak pull-up** — the line reaches high late or not at all.  The 1 bits
  and the stop bit read as 0, so a read reports `*parity error` or
  `*stopbit error`.

A missing pull-up does **not** by itself produce `*startbit error` — the start
bit is a low, which a line without a pull-up supplies anyway.  See the
diagnosis table in [edge-cases.md](edge-cases.md).

**Requires verification:** whether a given PC/AT keyboard contains its own
pull-ups.

KEYB fixes the Bus Pirate's output type to **open drain**
(`H=Hi-Z, L=GND`): the Bus Pirate only pulls a line low and otherwise releases
it to high impedance, so a pull-up supplies the high level.  There is no setup
prompt to change this — open drain is the only output type in this mode, which
is what the open-collector keyboard interface expects.

If the keyboard does not provide the pull-ups, add them one of these ways:

- Enable the Bus Pirate's own pull-ups, in this order:

  1. `W` — switches on the 5 V rail, which is the pull-up source.
  2. `e3` — selects the onboard 5.0 V pull-up source (`e2` selects 3.3 V).
  3. `P` — turns the pull-up resistors on.

  `P` before the rail is up prints the warning:

  ```
  KEYB>P
  Pull-up resistors ON
  Warning: no voltage on Vpullup pin
  ```

  The warning means the pull-up source had no voltage at that moment — usually
  `P` was sent before `W`, or before the on-board source was selected with
  `e3`.  It is an ordering warning, not a failed pull-up: run `W`, select the
  source with `e3`, then `P` again.
- Or fit external 1–10 kΩ resistors from DATA and CLOCK to 5 V.

The Bus Pirate's pull-ups are **off by default**; leave them off when the
keyboard already pulls the lines up.

## Supply

The keyboard runs from **5 V**.  On the Bus Pirate the 5 V rail is switched by
`W` (`POWER SUPPLIES ON`); feed the keyboard's VCC from the Bus Pirate's 5.0V
pin, or from an external 5 V supply that shares ground with the Bus Pirate.
The keyboard must be powered before any read or the monitor.

**Requires verification:** the keyboard's current draw from the Bus Pirate's
5 V rail.

## Direction and pin type

Both interface lines are bidirectional in use:

- **Reading** — the keyboard drives CLOCK and DATA; the Bus Pirate watches the
  CLOCK edges and samples DATA.
- **Writing** — the Bus Pirate pulls CLOCK low and drives DATA to send a host
  command, then releases DATA for the stop bit and reads the keyboard's
  acknowledge bit.

Because the Bus Pirate never drives a line high, there is no contention with the
keyboard.

## Voltage limit

The interface is 5 V.  The Bus Pirate's I/O pins are 5 V safe; do **not exceed
5.5 V** on DATA or CLOCK — that figure is the input ceiling, not a working
level.

## Pin-type summary

| Property | Value |
|---|---|
| output type | open drain (`H=Hi-Z, L=GND`), fixed by the mode |
| high level | the keyboard's own pull-ups, the Bus Pirate's `P` pull-ups, or external 1–10 kΩ |
| Bus Pirate pull-up needed | only if the keyboard lacks its own; off by default |
| lines the Bus Pirate drives | DATA (MOSI) and CLOCK (CLK), low only |
