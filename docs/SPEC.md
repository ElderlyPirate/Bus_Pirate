# Bus Pirate — Function Specification

**Status:** current.  This file is the capability index for the Bus Pirate v3/v4
community firmware: each mode, function and sub-function, its board status, and
a link to the documentation set that covers it.  The tree is grouped by cluster
— `generic/` (the prompt), `protocol/`, `tools/`, `capabilities/`,
`programmers/`, and the maintainer sets [core/](core/README.md) (firmware
internals) and [dev/](dev/README.md) (build, flash and tooling).

**Legend —** `x` = present in that build · `-` = not in that build ·
`off` = optional feature, not in the default build · `tbi` = planned, not in
the firmware yet.
The BPv3 column means *present in the v3 build*; the v3 build is documented
but not verified on v3 hardware.
"Docs" is the documentation set for the row: the set's README, or the named
page where a set splits a topic.

Build/flash tooling is developer documentation ([dev/](dev/README.md)), not
part of this spec.  Client programs are mentioned with the function they
serve.  Prompt mechanics (navigation, formatting, value entry, macros,
wrappers, reads, reset, bootloader) are *usage* documentation — "how to use
the prompt" ([generic/](generic/README.md)) — not capability rows; only the
elements/features they control are listed here.

## 1. Device elements & capabilities (controlled from the prompt)

| Capability | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Pull-up resistors | x | x | `P` / `p` on/off; needs a VPU voltage | ✓ [generic/](generic/README.md) |
| VPU / pull-up source | - | x | `e`: External / 3.3 V / 5 V | ✓ [generic/](generic/README.md) |
| Power supplies | x | x | `W` / `w` on/off (target VCC) | ✓ [generic/](generic/README.md) |
| AUX pin control | x | x | `a` / `A` / `@` low / high / read | ✓ [capabilities/auxiliary/](capabilities/auxiliary/README.md) |
| AUX assignment | x | x | `c` / `C`; v4 adds `k` / `K` (four targets) | ✓ [capabilities/auxiliary/](capabilities/auxiliary/README.md) |
| Bit order | x | x | `l` / `L` MSB-first / LSB-first | ✓ [generic/](generic/README.md) |
| ADC probe | x | x | `d` / `D` one-shot / continuous; `d:N` repeats | ✓ [capabilities/adc/](capabilities/adc/README.md) |
| Frequency counter | x | x | `f` on the AUX pin | ✓ [capabilities/auxiliary/](capabilities/auxiliary/README.md) |
| PWM / servo output | x | x | `g` / `S` on the AUX pin (not in HiZ) | ✓ [capabilities/auxiliary/](capabilities/auxiliary/README.md) |
| Elapsed timer | x | x | `t` / `T` read / reset (µs) | ✓ [generic/](generic/README.md) |
| Self-test | x | x | `~` (HiZ only) | ✓ [capabilities/selftest/](capabilities/selftest/README.md) |

## 2. Modes & sub-functions

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| **HiZ** | x | x | default; self-test reachable | ✓ [generic/](generic/README.md#idle-mode-hiz) |
| **1WIRE** | x | x | Dallas/Maxim 1-Wire; DS2404, DS2431, … | ✓ [protocol/wire/1wire/](protocol/wire/1wire/README.md) |
| · standard / overdrive | x | x | `O` in-place toggle | ✓ |
| · ROM search macros 51/85/204/236/240 | x | x | — | ✓ |
| · device roster macros | x | x | address book | ✓ |
| **UART** | x | x | — | ✓ [protocol/uart/](protocol/uart/README.md) |
| · full duplex | x | x | — | ✓ |
| · half-duplex single-wire (HDUART) | x | x | open drain + pull-up; no self-echo | ✓ |
| · bridge / live monitor / flow-control bridge | x | x | macros 1–3 | ✓ |
| · auto-baud detection | x | x | macro 4 handled on both builds; settings field + bps form v4-only ⓐ | ✓ |
| · parity/framing reporting | x | x | overruns counted, not printed | ✓ |
| · custom BRG | x | x | — | ✓ |
| **I2C** | x | x | — | ✓ [protocol/i2c/](protocol/i2c/README.md) |
| · software (bit-banged) | x | x | ~6k/~60k/~150k/max | ✓ |
| · hardware engine | - | x | 100k/400k/1M | ✓ |
| · clock stretching | x | x | per-bit wait + timeout count | ✓ |
| · address scanner / sniffer macros | x | x | — | ✓ |
| · on-board EEPROM macros 3/4 | - | x | connect + write-enable | ✓ |
| **SPI** | x | x | — | ✓ [protocol/spi/](protocol/spi/README.md) |
| · sniffer (CS-low / all / live tweaks) | x | x | v4: CN CS capture (bounded) | ✓ |
| · binary mode | x | x | — | ✓ |
| **2WIRE (raw)** | x | x | ISO7816-3 ATR macros | ✓ [protocol/wire/2wire/](protocol/wire/2wire/README.md) |
| **3WIRE (raw)** | x | x | no macros (by design) | ✓ [protocol/wire/3wire/](protocol/wire/3wire/README.md) |
| **KEYB** | - | x | live input monitor | ✓ [protocol/keyboard/](protocol/keyboard/README.md) |
| **PIC** | x | x | command/data `[`/`]`, devID macro; host tool `picprog` | ✓ [programmers/pic/](programmers/pic/README.md) |
| **DIO** | x | x | 100% pin control | ✓ [protocol/dio/](protocol/dio/README.md) |
| **HD44780 LCD** | off | off | optional build | ✓ [capabilities/lcd/](capabilities/lcd/README.md) |
| **SMPS** | - | off | optional **v4** build; legacy external board; binary `0xF0`–`0xFF` | ✓ [capabilities/smps/](capabilities/smps/README.md) |

**JTAG has no terminal mode.**  Both JTAG paths — the OpenOCD adapter and the
XSVF player — are entered from the binary layer; see §6.

ⓐ The v3 macro menu lists auto-baud; the macro itself is handled on both
builds — only the settings field behind the entry-time option and the bps
custom-baud form are v4-only.

## 3. Programmer personality — AVRISP-MKII

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| USB personality (STK500v2 / XPROG) | - | x | client: avrdude (`-c avrispmkII -P usb`) | ✓ [programmers/avrisp/](programmers/avrisp/README.md) |
| · ISP targets | - | x | — | ✓ |
| · PDI (Xmega) | - | x | — | ✓ |
| · TPI (ATtiny) | - | x | not verified | ✓ |
| serial route (framed STK500v2 over CDC) | - | x | ISP only; avrdude `-c stk500v2 -P COMx` | ✓ |
| rescue clock / AVR053 OSCCAL | - | x | — | ✓ |

## 4. Logic analyzer — SUMP

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| SUMP protocol | x | x | client: sigrok / PulseView | ✓ [protocol/sump/](protocol/sump/README.md) |
| · regular capture | x | x | — | ✓ |
| · extended (enhanced LA, #32) | x | x | 2k–32k samples, 1/2/4/8/16 MHz engines, trigger position; v3 unverified | ✓ |
| entry sequence | x | x | hidden: 5–19× `0x00` then raw STX (`0x02`); 20 NULs enter binary instead | ✓ |

## 5. Script engine — BASIC

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| BASIC interpreter (`s`) | x | x | statements incl. `MODE` / bus access | ✓ [tools/basic/](tools/basic/README.md) |

## 6. Binary bitbang layer (BBIO1)

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Core (pin I/O, return to terminal, reset) | x | x | — | ✓ [protocol/binary/](protocol/binary/README.md) |
| PWM, frequency, wait-for-edge (`0x12`/`0x13`/`0x16`/`0x70`) | x | x | units and limits | ✓ [capabilities/auxiliary/](capabilities/auxiliary/README.md) |
| ADC (`0x14` / `0x15`) | x | x | one sample / stream | ✓ [capabilities/adc/](capabilities/adc/README.md) |
| SPI / I2C / UART / 1W01 / RAW sub-protocols | x | x | per-protocol libraries | ✓ [protocol/binary/](protocol/binary/sub-protocols.md) |
| PIC (`0x07`) | x | x | — | ✓ [programmers/pic/](programmers/pic/README.md) |
| OpenOCD JTAG (`0x06`) | x | x | client: OpenOCD | ✓ [protocol/jtag/](protocol/jtag/README.md) · [tools/openocd/](tools/openocd/README.md) |
| XSVF (`0x18`) | - | x | — | ✓ [tools/xsvf/](tools/xsvf/README.md) |
| AVRISP (`0x08` / `0x09`) | - | x | see §3 | ✓ [programmers/avrisp/](programmers/avrisp/README.md) |
| Self-test (`0x10` / `0x11`) | x | x | — | ✓ [capabilities/selftest/](capabilities/selftest/binary.md) |

## 7. Planned — tbi

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| IR (infrared) mode | tbi | tbi | no IR hardware available | — (no set yet) |
| SWD (ARM two-wire debug) | tbi | tbi | needs target hardware | — (no set yet) |
