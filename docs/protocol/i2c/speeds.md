# I2C — speed profiles

Part of the I2C documentation set (`docs/protocol/i2c/`). This page owns the
speed table, the two drivers' rates, and the mapping between the terminal menu,
the settings header and the binary operand. Entry and the prompts are in
[entry-and-setup.md](entry-and-setup.md); the binary command is in
[binary.md](binary.md); the bus limits that decide whether a rate is usable are
in [wiring.md](wiring.md).

## The two tables

The speed prompt is the same in both drivers (`Set speed:`), but the lists and
their meanings differ.

### Software driver — four bit-bang profiles

| Menu | Label | Settings `spd` | Binary `0x6x` | Bit timing |
|---:|---|---:|---|---|
| 1 | `~6kHz` | 0 | `0x60` | longest phase delays |
| 2 | `~60kHz` | 1 | `0x61` | medium phase delays |
| 3 | `~150kHz` | 2 | `0x62` | short phase delays |
| 4 | `Maximum speed` | 3 | `0x63` | no deliberate phase delay |

The four profiles are fixed delay sets used by the bit-banged driver. Profile 4
removes the phase delays; the rate is then limited by the instruction path and
the bus, so "maximum" is a mode, not a number.

### Hardware engine — three rates

| Menu | Label | Settings `spd` | Binary | Engine note |
|---:|---|---:|---|---|
| 1 | `100kHz` | 0 | — | slew limiting off |
| 2 | `400kHz` | 1 | — | slew limiting on |
| 3 | `1MHz` | 2 | — | slew limiting on; rise time dominates |

The engine has **no binary selection**: the `0x6x` group always configures the
software driver, and the binary `I2C1` sub-protocol never uses the hardware
engine. The engine rates are available only from the terminal mode.

## What the labels mean

- **Software labels are intended rates, not guarantees.** Each profile sets the
  SCL high and low phases to a fixed delay; the label is the resulting order of
  magnitude at the default build. Instruction overhead adds to the fixed
  delays, so the actual SCL is at or below the label. Exact rates on a given
  build are **unverified**.
- **Hardware labels are nominal.** The engine divides the peripheral clock and
  then synchronises to the actual SCL edge. Clock synchronisation plus the bus
  rise time make the real SCL **lower** than the selected label, increasingly so
  at the fast settings. The exact rate on a given board and bus is
  **unverified**; measure it at the target when the margin matters.
- The `Maximum speed` profile has no defined rate; the limit depends on the
  firmware path and the bus, not on a value in the table.
- The delay profile also sets the hold/settle time between a data change and the
  clock edge. A faster profile shortens that margin along with the clock.

## Which profile applies when

| Situation | Profile in force |
|---|---|
| A terminal session entered through the prompts | the speed answered at `Set speed:` |
| A terminal session entered with `m4 …` | the speed given on the line |
| After `W` | the session speed is re-applied |
| After a mode change or reset | the mode returns to HiZ; re-entering asks again |
| Binary `I2C1` entry | profile 3 (`Maximum speed`), always software |
| Binary `0x6x` | the low two bits select profile 0–3 immediately |
| On-board EEPROM macros 3/4 | the session's **hardware** rate (macros require Hardware) |

`0x6x` masks its operand to the low two bits, so `0x64` selects profile 0,
`0x65` profile 1, and so on; every operand selects some profile.

The `i` status block prints the stored speed as the zero-based `spd` field of
the settings header. Binary changes are not part of the terminal session: a
return to the terminal resets the board to HiZ, so re-entering I2C takes a fresh
speed answer.

## Choosing a speed

1. Start with the default (`~6kHz` software, `100kHz` hardware) and confirm the
   target answers its address.
2. Raise the profile until the target becomes unreliable, then step back one.
3. Treat the bus rise time as the real limit at the fast settings. The v4
   on-board SCL pull-up is 10 kΩ, which gives a slow rise on a loaded bus; an
   external 1.5–2.2 kΩ on SCL is a starting point for fast modes. The
   trade-off and the current the target must sink are in [wiring.md](wiring.md).
4. The scanner follows the selected profile. The sniffer is passive and does not
   use the profile for its own sampling.
5. A target's datasheet limit is the authority. Many 3.3 V parts specify a
   maximum clock and a maximum rise time; both must hold at once.

## v3 / v4

| Item | v3 | v4 |
|---|---|---|
| software profiles 1–4 | yes | yes |
| hardware rates 1–3 | not in this tree | yes |
| binary `0x6x` profiles | yes, software only | yes, software only |
| `spd` in the header | zero-based | zero-based |

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — the prompts, defaults and
  one-line form.
- [binary.md](binary.md) — `0x6x` and the binary entry defaults.
- [wiring.md](wiring.md) — rise time, pull-ups and the voltage limit.
- [clock-stretching.md](clock-stretching.md) — what happens when a slave
  cannot follow the clock.
- [edge-cases.md](edge-cases.md#entry-and-settings) — speed and setup failures.
