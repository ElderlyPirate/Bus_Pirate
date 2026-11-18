# SPI — macros

Part of the SPI documentation set (`docs/protocol/spi/`). This page owns the
mode macro list and the in-place clock tweaks. The sniffer itself is described
in [sniffer.md](sniffer.md); the macro grammar `(n)` is shared and is owned by
[docs/generic/macros.md](../../generic/macros.md).

## The `(0)` list

`(0)` prints the macro menu:

```
 0.Macro menu
 1.Sniff CS low
 2.Sniff all traffic
10.Set clock idle low
11.Set clock idle high
12.Set edge idle to active
13.Set edge active to idle
14.Sample phase on middle
15.Sample phase on end
```

The firmware uses a single leading space before `0`–`9` and none before
`10`–`15`; the block above is the exact list.

| Macro | Description |
|---|---|
| `(0)` | macro menu |
| `(1)` | sniff while CS is low |
| `(2)` | sniff all traffic |
| `(10)` | set clock polarity idle low |
| `(11)` | set clock polarity idle high |
| `(12)` | set clock edge idle to active |
| `(13)` | set clock edge active to idle |
| `(14)` | set input sample phase to middle |
| `(15)` | set input sample phase to end |
| any other | `Unknown macro, try ? or (0) for help` |

The list jumps from `2` straight to `10`; there are **no macros 3–9**. Using
one of those values, or any other undefined number, prints the unknown-macro
message.

## Sniff macros

`(1)` and `(2)` start the bus sniffer — see [sniffer.md](sniffer.md). Both
configure the four pins as inputs themselves; the terminal `W` step is not
required for sniffing. `(0)` lists the macros.

## Clock tweak macros — 10–15

Macros 10–15 change one SPI setting and re-apply it **without leaving the
mode**. Each sets the corresponding bit and then prints the settings header:

| Macro | Setting changed | Header field |
|---|---|---|
| `(10)` | clock polarity → idle low | `ckp` 0 |
| `(11)` | clock polarity → idle high | `ckp` 1 |
| `(12)` | clock edge → idle to active | `ske` 0 |
| `(13)` | clock edge → active to idle | `ske` 1 |
| `(14)` | input sample phase → middle | `smp` 0 |
| `(15)` | input sample phase → end | `smp` 1 |

The header shows the current settings (see
[entry-and-setup.md](entry-and-setup.md) for the field meanings):

```
SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )
```

For example, `(11)` on a bus set to speed 4 changes only `ckp`:

```
SPI>(11)
SPI (spd ckp ske smp csl hiz)=( 4 1 1 0 1 0 )
SPI>
```

The tweak applies to the whole SPI interface, and later bus operations use it
until the next tweak or a re-entry. The stored setting survives `w`/`W`; a
re-entry resets it to the setup answer. The tweaks also work before `W` — the
value is stored and applied when the bus is set up.

There is no macro for CS polarity, output type or speed. Re-enter the mode to
change CS polarity. Speed and output type can also be set in binary mode with
`0x6x` and `0x8x` — see [binary.md](binary.md).

Whether an in-place change in the middle of a byte transfer is safe for a
device that is already mid-transaction is **not verified**; change the clock
settings between transactions, not inside one.

The `i` status block also reprints the settings header, so it can confirm the
current values without running a tweak.

## Edge cases

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `(3)` … `(9)` | `Unknown macro, try ? or (0) for help` | those macros do not exist; only 0–2 and 10–15 are defined |
| A clock tweak was run mid-transaction | the header prints with the new value; the device may misread the current byte | change clock settings between transactions |
| A macro was expected to change CS polarity or output type | no such macro exists | re-enter the mode and answer the setup prompts |
| A tweak was run, then `w`/`W` | the tweak is still in effect | expected; re-enter the mode for the setup values |
| A tweak was run before `W` | the header prints; the bus is not engaged yet | run `W`; the stored setting is applied then |

## v3 / v4

The macro list, the sniff macros and the clock tweaks 10–15 are the same in
both builds. The v3 behaviour is **not verified**.

## Related docs

- [entry-and-setup.md](entry-and-setup.md) — setup values and the header.
- [sniffer.md](sniffer.md) — the `(1)`/`(2)` capture modes.
- [bus-operations.md](bus-operations.md) — the byte operations the tweaks
  affect.
- [binary.md](binary.md) — binary configuration of the same settings.
- [edge-cases.md](edge-cases.md) — macro and settings failures.
