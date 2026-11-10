# SPI — macros

Part of the SPI documentation set (`docs/spi/`). This page owns the mode macro
list and the in-place clock tweaks. The sniffer itself is described in
[sniffer.md](sniffer.md).

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

| Macro | Description | Where |
|---|---|---|
| `(0)` | macro menu | both |
| `(1)` | sniff while CS is low | both |
| `(2)` | sniff all traffic | both |
| `(10)` | set clock polarity idle low | both |
| `(11)` | set clock polarity idle high | both |
| `(12)` | set clock edge idle to active | both |
| `(13)` | set clock edge active to idle | both |
| `(14)` | set input sample phase to middle | both |
| `(15)` | set input sample phase to end | both |
| any other | `Unknown macro, try ? or (0) for help` | both |

The macro menu jumps from `2` straight to `10`; there are **no macros 3–9**.
Using one of those values prints the unknown-macro message. An older published
SPI page listed a struck-through "macro 3" as a temporarily removed
CS-high sniffer; that macro does not exist in the current firmware. Only the
two sniff macros `(1)` and `(2)` are present.

## Sniff macros

`(1)` and `(2)` start the bus sniffer — see [sniffer.md](sniffer.md). `(0)`
lists the macros.

## Clock tweak macros — 10–15

Macros 10–15 change one SPI setting and re-apply it **without leaving the
mode**. Each sets the corresponding bit and then prints the settings header:

| Macro | Setting changed |
|---|---|
| `(10)` | clock polarity → idle low |
| `(11)` | clock polarity → idle high |
| `(12)` | clock edge → idle to active |
| `(13)` | clock edge → active to idle |
| `(14)` | input sample phase → middle |
| `(15)` | input sample phase → end |

The header shows the current settings (see
[entry-and-setup.md](entry-and-setup.md) for the field meanings):

```
SPI (spd ckp ske smp csl hiz)=( 4 0 1 0 1 0 )
```

These macros were added so the clock can be adjusted mid-session; the change
applies to the whole SPI interface, and later bus operations use it until the
next tweak or a re-entry. Whether the in-place change is safe for a device that
is already mid-transaction is **not verified** — change the clock settings
between transactions.

## v3 / v4

The macro list, the sniff macros and the clock tweaks 10–15 are the same in
both builds. The v3 column is **not verified**.