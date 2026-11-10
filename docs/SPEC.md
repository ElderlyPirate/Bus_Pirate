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
| Pull-up resistors | x | x | `P` / `p` on/off; needs a VPU voltage | ✓ [generic/](generic/README.md) |
| VPU / pull-up source | - | x | `e`: External / 3.3 V / 5 V | ✓ [generic/](generic/README.md) |
| Power supplies | x | x | `W` / `w`; programmable voltages | ✓ [generic/](generic/README.md) |
| AUX pin control | x | x | `a` / `A` / `@` low / high / read | ✓ [generic/](generic/README.md) |
| AUX assignment | x | x | `c`/`C`; v4 adds `k`/`K` (four targets) | ✓ [generic/](generic/README.md) |
| Bit order | x | x | `l` / `L` LSB-first / MSB-first | ✓ [generic/](generic/README.md) |
| ADC probe | x | x | `d` / `D` one-shot / continuous; `d:N` repeats | ✓ [generic/](generic/README.md) |
| Frequency counter | x | x | `f` on the AUX pin | ✓ [generic/](generic/README.md) |
| PWM / servo output | x | x | `g` / `S` on the AUX pin (not in HiZ) | ✓ [generic/](generic/README.md) |
| Elapsed timer | x | x | `t` / `T` read / reset (µs) | ✓ [generic/](generic/README.md) |
| Self-test | x | x | `~` (HiZ only) | ✓ [self-test/](capabilities/selftest/README.md) |

## 2. Modes & sub-functions

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| **HiZ** | x | x | default; self-test reachable | ✓ [generic/](generic/README.md#idle-mode-hiz) |
| **1-WIRE** | x | x | DS2404, DS2431, … | ✓ [1wire/](protocol/wire/1wire/README.md) |
| · standard / overdrive | x | x | `O` in-place toggle | ✓ |
| · ROM search macros 51/85/204/236/240 | x | x | — | ✓ |
| · device roster macros | x | x | address book | ✓ |
| **UART** | x | x | — | ✓ [uart/](protocol/uart/README.md) |
| · full duplex | x | x | — | ✓ |
| · half-duplex single-wire (HDUART) | x | x | open drain + pull-up; no self-echo | ✓ |
| · bridge / live monitor / flow-control bridge | x | x | macros 1–3 | ✓ |
| · auto-baud detection | x | x | macro 4 handled on both builds; settings field + bps form v4-only | ✓ |
| · parity/framing/overrun reporting | x | x | — | ✓ |
| · custom BRG | x | x | — | ✓ |
| **I2C** | x | x | — | ✓ [i2c/](protocol/i2c/README.md) |
| · software (bit-banged) | x | x | ~6k/~60k/~150k/max | ✓ |
| · hardware engine | - | x | 100k/400k/1M | ✓ |
| · clock stretching | x | x | per-bit wait + timeout count | ✓ |
| · address scanner / sniffer macros | x | x | — | ✓ |
| · on-board EEPROM macros 3/4 | - | x | connect + write-enable | ✓ |
| **SPI** | x | x | — | ✓ [spi/](protocol/spi/README.md) |
| · sniffer (CS-low / all / live tweaks) | x | x | v4: CN CS capture (bounded) | ✓ |
| · binary mode | x | x | — | ✓ |
| **2WIRE (raw)** | x | x | ISO7816-3 ATR macros | ✓ [2wire/](protocol/wire/2wire/README.md) |
| **3WIRE (raw)** | x | x | no macros (by design) | ✓ [3wire/](protocol/wire/3wire/README.md) |
| **KEYB** | - | x | live input monitor | ✓ [keyboard/](protocol/keyboard/README.md) |
| **PIC** | x | x | command/data `[`/`]`, devID macro; host tool `picprog` | ✓ [pic/](programmers/pic/README.md) |
| **DIO** | x | x | 100% pin control | ✓ [protocol/dio/](protocol/dio/README.md) |
| **HD44780 LCD** | off | off | optional build (not in default image) | ✓ [lcd/](capabilities/lcd/README.md) |
| **SMPS** | off | off | optional **v4** build; legacy external board; binary commands `0xF0`–`0xF2` | ✓ [smps/](capabilities/smps/README.md) |

ⓐ The v3 macro menu lists auto-baud; the macro itself is handled on both
builds — only the settings field behind the entry-time option and the bps
custom-baud form are v4-only (confirmed in the UART round).

## 3. Programmer personality — AVRISP-MKII

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| USB personality (STK500v2 / XPROG) | - | x | client: avrdude (`-c avrispmkII -P usb`) | ✓ [avrisp/](programmers/avrisp/README.md) |
| · ISP targets | - | x | — | ✓ |
| · PDI (Xmega) | - | x | — | ✓ |
| · TPI (ATtiny) | - | x | not verified | ✓ |
| serial route (framed STK500v2 over CDC) | - | x | ISP only; avrdude `-c stk500v2 -P COMx` | ✓ |
| rescue clock / AVR053 OSCCAL | - | x | — | ✓ |

## 4. Logic analyzer — SUMP

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| SUMP protocol | x | x | client: sigrok / PulseView | ✓ [sump/](protocol/sump/README.md) |
| · regular capture | x | x | — | ✓ |
| · extended (enhanced LA, #32) | x | x | 2k–32k samples, 1/2/4/8/16 MHz engines, trigger position | ✓ |
| entry sequence | x | x | hidden: ≥5× `0x00` then LEFT ARROW | ✓ |

## 5. Script engine — BASIC

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| BASIC interpreter (`s`) | x | x | statements incl. `MODE` / bus access | ✓ [basic/](tools/basic/README.md) |

## 6. Binary bitbang layer (BBIO1)

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Core (pin I/O, PWM, ADC, freq, wait-for-edge `0x70`, reset) | x | x | — | ✓ [binary/](protocol/binary/README.md) |
| SPI / I2C / UART / 1-Wire / RAW sub-protocols | x | x | per-protocol libraries as clients | ✓ |
| PIC (`0x07`) | x | x | — | ✓ |
| OpenOCD JTAG (`0x06`) | x | x | client: OpenOCD | ✓ [jtag/](protocol/jtag/README.md) |
| XSVF (`0x18`) | - | x | — | ✓ |
| AVRISP (`0x08` / `0x09`) | - | x | see §3 | ✓ |
| Self-test (`0x10` / `0x11`) | x | x | — | ✓ [self-test/](capabilities/selftest/README.md) |

## 7. Planned — tbi

| Function | v3 | v4 | Notes |
|---|---|---|---|
| IR (infrared) mode | tbi | tbi | no IR hardware available |
| SWD (ARM two-wire debug) | tbi | tbi | needs target hardware |

---
*First draft — corrections welcome.*