# Bus Pirate — Function Specification (first draft)

**Status:** draft, 2026-11-01 — structure and taxonomy under discussion (see
`WORKLOG-DOCUMENTATION.md`).  This file is the one-page capability index; the
per-function documentation (`docs/<function>/`) expands each row.

**Legend —** `x` = present in that build · `-` = not in that build ·
`tbi` = to be implemented (planned, not in the firmware yet).
The BPv3 column means *present in the v3 build* — our bench has no v3 hardware,
so v3 is build-verified only.  "Docs" = current state in `Documentation/`
(✓ · partial · stale · stub · none).

Build/flash tooling is developer documentation (`docs/dev/`), not part of this
spec.  Client programs are mentioned with the function they serve.

## 1. Generic prompt layer (enablers)

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| Banner / `i` status | x | x | — | none |
| Help `?` | x | x | — | (self) |
| Mode menu `m` | x | x | — | per-mode |
| Display format `o` | x | x | HEX/DEC/BIN/RAW | none |
| Pin states / volts `v` | x | x | — | none |
| Elapsed timer `t`/`T` | x | x | — | none (not even help) |
| Frequency counter `f` | x | x | — | none |
| PWM / servo `g` / `S` | x | x | not in HiZ | none |
| AUX assignment `c`/`C` (`k`/`K` v4) | x | x | v3 two-way, v4 four-way | none |
| Bit order `l`/`L` | x | x | — | none |
| Pull-ups `P`/`p` | x | x | — | partial |
| VPU pull-up source `e` | - | x | External / 3.3 V / 5 V | none |
| Convert / reverse `=` `\|` | x | x | — | none |
| Reset `#` | x | x | v3: hardware; v4: simulated → HiZ | none |
| Self-test `~` | x | x | device diagnostic | stub |
| Bootloader jump `$` | x | x | enabler: replaces all functions | ✓ build doc |
| AUX pin `a`/`A`/`@` | x | x | — | mode docs |
| Power supplies `W`/`w` | x | x | — | mode docs |
| ADC probe `d`/`D` | x | x | `d:N` repeat | partial |
| Delays `&` / `%` | x | x | µs / ms | partial |
| User macros `<n>` | x | x | assign / run (slot counts differ) | none |
| Mode macros `(n)` | x | x | per mode | mode docs |
| Send string/value `"..."`, `0x`, `0b` | x | x | — | mode docs |
| Start/stop `[` `]` `{` `}` | x | x | mode-dependent | mode docs |
| Read `r` (+ `r:N`, `r;bits`) | x | x | — | mode docs |
| Bit ops `/ \ ^ - _ . !` | x | x | clock/data hi/lo/read | partial |
| Terminal baud `b` | x | x | 300…115200 + BRG | partial |
| History `h` | x | x | — | none |
| Bit inject `;` | x | x | into previous value | help only |

## 2. Modes & sub-functions

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| **HiZ** | x | x | default; self-test reachable | none |
| **1-WIRE** | x | x | DS2404, DS2431, … | ✓ `1-wire.md` |
| · standard / overdrive | x | x | `O` in-place toggle | ✓ |
| · ROM search macros 51/85/204/236/240 | x | x | — | ✓ |
| · device roster macros | x | x | address book | partial |
| **UART** | x | x | — | ✓ `uart.md` |
| · full duplex | x | x | — | ✓ |
| · half-duplex single-wire (HDUART) | x | x | open drain + pull-up; no self-echo | ✓ (new) |
| · bridge / live monitor / flow-control bridge | x | x | macros 1–3 | partial |
| · auto-baud detection | - | x | macro 4; v3 menu lists it (see note ⓐ) | none |
| · parity/framing/overrun reporting | x | x | — | ✓ |
| · custom BRG | x | x | — | ✓ |
| **I2C** | x | x | — | stale `i2c.md` |
| · software (bit-banged) | x | x | ~6k/~60k/~150k/max | stale |
| · hardware engine | - | x | 100k/400k/1M | none |
| · clock stretching | x | x | per-bit wait + timeout count | stale (doc denies v3) |
| · address scanner / sniffer macros | x | x | — | partial |
| · on-board EEPROM macros 3/4 | - | x | connect + write-enable | stale (missing) |
| **SPI** | x | x | — | partial `spi.md` |
| · sniffer (CS-low / all / live tweaks) | x | x | v4: hardware-CS note | partial |
| · binary mode | x | x | — | missing |
| **2WIRE (raw)** | x | x | ISO7816-3 ATR macros | ✓ `raw_2-wire.md` |
| **3WIRE (raw)** | x | x | no macros (by design) | ✓ `raw_3-wire.md` |
| **KEYB** | - | x | live input monitor | ✓ `pc_keyboard.md` |
| **PIC** | x | x | command/data `[`/`]`, devID macro; host tool `picprog` | partial |
| **DIO** | x | x | 100% pin control | stub |
| **HD44780 LCD** | off | off | optional build (not in default image) | `hd44780.md` |
| **SMPS** | off | off | optional build (not in default image) | none |

ⓐ v3 macro menu lists auto-baud, but the settings field and code path are
`#ifdef BUSPIRATEV4` — to verify in the per-function review.

## 3. Programmer personality — AVRISP-MKII

| Function | v3 | v4 | Notes | Docs |
|---|---|---|---|---|
| USB personality (STK500v2 / XPROG) | - | x | client: avrdude (`-c avrispmkII -P usb`) | ✓ `avrisp_mkii.md` |
| · ISP targets | - | x | — | ✓ |
| · PDI (Xmega) | - | x | benched (XM64) | ✓ |
| · TPI (ATtiny) | - | x | build-only (no ATtiny on bench) | ✓ |
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
| IR (infrared) mode | tbi | tbi | cannot be bench-tested here (no IR hardware) |
| SWD (ARM two-wire debug) | tbi | tbi | needs target hardware |

---
*First draft — corrections welcome (especially the auto-baud v3 entry and the
regular/extended SUMP split).*