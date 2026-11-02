# PC keyboard (KEYB) — wiring and electrical requirements

Part of the KEYB documentation set (`docs/keyboard/`).  This page owns the
wiring table and the supply/pull-up statement; the README carries the headline
only.

## Wiring

A PC/AT keyboard needs four connections: 5 V, ground, and the two-wire
clock/data interface.  The keyboard's two signals map to the Bus Pirate's
MOSI and CLK pins:

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
high between exchanges; with no pull-up the lines never go high and every read
fails (the data line is not seen low at a frame start — `*startbit error`; see
[edge-cases.md](edge-cases.md)).

KEYB fixes the Bus Pirate's output type to **open drain**
(`H=Hi-Z, L=GND`): the Bus Pirate only pulls a line low and otherwise releases
it to high impedance, so a pull-up supplies the high level.  There is no setup
prompt to change this — open drain is the only output type in this mode, which
is what the open-collector keyboard interface expects.

**Requires verification:** whether a given PC/AT keyboard contains its own
pull-ups.

If the keyboard does not provide them, add a pull-up one of these ways:

- Enable the Bus Pirate's own pull-ups: `e3` selects the onboard 5.0 V
  pull-up source, then `P` turns the pull-up resistors on (`e2` selects the
  3.3 V source instead).  The `e` and `P` commands are accepted in KEYB.
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
  command, then releases DATA to read the keyboard's acknowledge bit.

Because the Bus Pirate never drives a line high, there is no contention with the
keyboard.

## Voltage limit

The interface is 5 V; do not exceed **5.5 V** on any signal pin.

## Pin-type summary

| Property | Value |
|---|---|
| output type | open drain (`H=Hi-Z, L=GND`), fixed by the mode |
| high level | from the keyboard's own pull-ups to 5 V, or (if it has none) the Bus Pirate's `P` pull-ups or external 1–10 kΩ to 5 V |
| Bus Pirate pull-up needed | only if the keyboard lacks its own; off by default |
| lines the Bus Pirate drives | DATA (MOSI) and CLOCK (CLK), low only |