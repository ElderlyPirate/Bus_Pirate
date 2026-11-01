# Bus Pirate — Function Specification (first draft)

**Status:** draft — structure and taxonomy under discussion.  This file is
the one-page capability index; the per-function documentation
(`docs/<function>/`) expands each row.

**Legend —** `x` = present in that build · `-` = not in that build ·
`tbi` = to be implemented (planned, not in the firmware yet).
The BPv3 column means *present in the v3 build* — v3 is build-verified only.
"Docs" = state of the per-function documentation
— the built set under `docs/<function>/` where it exists, otherwise the legacy
`Documentation/` file (✓ · partial · stale · stub · none).

Build/flash tooling is developer documentation (`docs/dev/`), not part of this
spec.  Client programs are mentioned with the function they serve.
Prompt mechanics (navigation, formatting, value entry, macros, wrappers,
reads, reset, bootloader) are *usage* documentation — "how to use the prompt"
(`docs/generic/`) — not capability rows; only the elements/features they
control are listed here.

## 1. Device elements & capabilities (controlled from the prompt)

| Capability | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Pull-up resistors | x | x | `P` / `p` on/off; needs a VPU voltage | partial |
| VPU / pull-up source | - | x | `e`: External / 3.3 V / 5 V | none |
| Power supplies | x | x | `W` / `w`; programmable voltages | mode docs |
| AUX pin control | x | x | `a` / `A` / `@` low / high / read | mode docs |
| AUX assignment | x | x | `c`/`C`; v4 adds `k`/`K` (four targets) | none |
| Bit order | x | x | `l` / `L` LSB-first / MSB-first | none |
| ADC probe | x | x | `d` / `D` one-shot / continuous; `d:N` repeats | partial |
| Frequency counter | x | x | `f` on the AUX pin | none |
| PWM / servo output | x | x | `g` / `S` on the AUX pin (not in HiZ) | none |
| Elapsed timer | x | x | `t` / `T` read / reset (µs) | none |
| Self-test | x | x | `~` (HiZ only) | stub |

## 2. Modes & sub-functions

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| **HiZ** | x | x | default; self-test reachable | none |
| **1-WIRE** | x | x | DS2404, DS2431, … | ✓ [1wire/](1wire/README.md) |
| · standard / overdrive | x | x | `O` in-place toggle | ✓ |
| · ROM search macros 51/85/204/236/240 | x | x | — | ✓ |
| · device roster macros | x | x | address book | ✓ |
| **UART** | x | x | — | ✓ [uart/](uart/README.md) |
| · full duplex | x | x | — | ✓ |
| · half-duplex single-wire (HDUART) | x | x | open drain + pull-up; no self-echo | ✓ |
| · bridge / live monitor / flow-control bridge | x | x | macros 1–3 | ✓ |
| · auto-baud detection | x | x | macro 4 handled on both builds; settings field + bps form v4-only | ✓ |
| · parity/framing/overrun reporting | x | x | — | ✓ |
| · custom BRG | x | x | — | ✓ |
| **I2C** | x | x | — | ✓ [i2c/](i2c/README.md) |
| · software (bit-banged) | x | x | ~6k/~60k/~150k/max | ✓ |
| · hardware engine | - | x | 100k/400k/1M | ✓ |
| · clock stretching | x | x | per-bit wait + timeout count | ✓ |
| · address scanner / sniffer macros | x | x | — | ✓ |
| · on-board EEPROM macros 3/4 | - | x | connect + write-enable | ✓ |
| **SPI** | x | x | — | partial `spi.md` |
| · sniffer (CS-low / all / live tweaks) | x | x | v4: hardware-CS note | partial |
| · binary mode | x | x | — | missing |
| **2WIRE (raw)** | x | x | ISO7816-3 ATR macros | ✓ `raw_2-wire.md` |
| **3WIRE (raw)** | x | x | no macros (by design) | ✓ `raw_3-wire.md` |
| **KEYB** | - | x | live input monitor | ✓ `pc_keyboard.md` |
| **PIC** | x | x | command/data `[`/`]`, devID macro; host tool `picprog` | partial |
| **DIO** | x | x | 100% pin control | stub |
| **HD44780 LCD** | off | off | optional build (not in default image) | `hd44780.md` |
| **SMPS** | off | off | optional **v4** build; legacy external board; binary commands `0xF0`–`0xF2` | none |

ⓐ The v3 macro menu lists auto-baud; the macro itself is handled on both
builds — only the settings field behind the entry-time option and the bps
custom-baud form are v4-only (confirmed in the UART round).

## 3. Programmer personality — AVRISP-MKII

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| USB personality (STK500v2 / XPROG) | - | x | client: avrdude (`-c avrispmkII -P usb`) | ✓ `avrisp_mkii.md` |
| · ISP targets | - | x | — | ✓ |
| · PDI (Xmega) | - | x | — | ✓ |
| · TPI (ATtiny) | - | x | not verified | ✓ |
| serial route (framed STK500v2 over CDC) | - | x | ISP only; avrdude `-c stk500v2 -P COMx` | ✓ |
| rescue clock / AVR053 OSCCAL | - | x | — | ✓ |

## 4. Logic analyzer — SUMP

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| SUMP protocol | x | x | client: sigrok / PulseView | **none** |
| · regular capture | x | x | — | none |
| · extended (enhanced LA, #32) | x | x | 2k–32k samples, 1/2/4/8/16 MHz engines, trigger position | none |
| entry sequence | x | x | hidden: ≥5× `0x00` then LEFT ARROW | none |

## 5. Script engine — BASIC

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| BASIC interpreter (`s`) | x | x | statements incl. `MODE` / bus access | ✓ `basic.md` |

## 6. Binary bitbang layer (BBIO1)

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Core (pin I/O, PWM, ADC, freq, wait-for-edge `0x70`, reset) | x | x | — | partial `bitbang.md` |
| SPI / I2C / UART / 1-Wire / RAW sub-protocols | x | x | per-protocol libraries as clients | missing sections |
| PIC (`0x07`) | x | x | — | missing |
| OpenOCD JTAG (`0x06`) | x | x | client: OpenOCD | partial |
| XSVF (`0x18`) | - | x | — | missing |
| AVRISP (`0x08` / `0x09`) | - | x | see §3 | ✓ |
| Self-test (`0x10` / `0x11`) | x | x | — | stub |

## 7. Planned — tbi

| Function | v3 | v4 | Notes |
|---|---|---|---|
| IR (infrared) mode | tbi | tbi | no IR hardware available |
| SWD (ARM two-wire debug) | tbi | tbi | needs target hardware |

---
*First draft — corrections welcome (especially the regular/extended SUMP
split).*