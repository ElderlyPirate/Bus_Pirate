# Final SPEC / README / TEMPLATE reconciliation

Worktree `C:\Users\jschl\dev\bpv4-docssweep`, branch `docs/combined-sweep`,
tip `24543c1`.  Scope: the three top-level documents only.  No commit, no
push; no module set edited.

## Checks run

- Every SPEC row checked against the accepted set READMEs and the firmware
  mode registry (`core.c` mode order) and default/all-features configuration.
- All relative links in the three files resolve: 58 checked, 0 missing; the
  two `<topic>.md` skeleton placeholders are intentional.
- No `Documentation/`, `docs/<function>/`, draft, date or bench prose in the
  three files; no TABs; CRLF preserved; table column counts consistent.

## Issues found -> fix applied

| # | Issue | Fix | File |
|---|---|---|---|
| 1 | §2 had no mode numbers | added a Mode (v4 / v3) column: HiZ 1, 1WIRE 2, UART 3, I2C 4, SPI 5, 2WIRE 6, 3WIRE 7, KEYB 8/-, PIC 9/8, DIO 10/9; LCD build-dependent; SMPS binary-only; custom-build footnote | `docs/SPEC.md` |
| 2 | KEYB v3 was `-`; the module is an optional v3 build, not absent | v3 `off`; note now says raw scancode read/write + live monitor | `docs/SPEC.md` |
| 3 | DIO "100% pin control" did not match the set | five-pin direction/level + AUX-target access | `docs/SPEC.md` |
| 4 | UART bridge note hid the v3-only flow control | note says macros 1-3 and `(3)` = `(1)` on v4 | `docs/SPEC.md` |
| 5 | UART "custom BRG" named only the v3 form | "custom baud", raw BRG (v3) / bps (v4) | `docs/SPEC.md` |
| 6 | UART auto-baud note and orphan footnote mixed entry-time option and custom baud | note says macro 4 both builds, entry-time option 11 v4-only; footnote removed | `docs/SPEC.md` |
| 7 | I2C clock-stretch note cited a "timeout count" the user cannot see | per-bit wait + bounded timeout | `docs/SPEC.md` |
| 8 | I2C hardware rates looked exact | marked nominal | `docs/SPEC.md` |
| 9 | I2C on-board EEPROM macros had no caveat | v4 only; not verified | `docs/SPEC.md` |
| 10 | PIC devID scope was not stated | devID 6b/14b only | `docs/SPEC.md` |
| 11 | SUMP row used the retired "extended (enhanced LA, #32)" label and a 2k start | "enhanced profiles (`enh`)"; timer depths 4k-32k | `docs/SPEC.md` |
| 12 | SUMP sample memory was missing and the old v3 2048 assumption could persist | 4096-byte sample memory on both boards | `docs/SPEC.md` |
| 13 | AVRISP USB row implied all v4 builds | default v4 image; the all-features build omits it | `docs/SPEC.md` |
| 14 | AVRISP PDI/OSCCAL caveats missing | PDI not verified for every XMEGA part; calibration not verified | `docs/SPEC.md` |
| 15 | XSVF row linked the host tools but not the firmware protocol | added `protocol/jtag/xsvf.md` | `docs/SPEC.md` |
| 16 | Power-supply row said only "target VCC" | `W`/`w` switch the 3.3 V and 5.0 V rails | `docs/SPEC.md` |
| 17 | `@` was described as a read, not a release-and-read | drive low / high / release and read | `docs/SPEC.md` |
| 18 | Bit order could be read as wire order | MSB-first / LSB-first within the width | `docs/SPEC.md` |
| 19 | Elapsed timer omitted its wrap limit | 31-bit us count | `docs/SPEC.md` |
| 20 | Root README had no "what works" summary | added a short section listing the functions, shared prompt capabilities, optional builds and the v3/v4 support statement | `README.md` |
| 21 | TEMPLATE README target of ~120 lines did not match the accepted sets (13 of 25 READMEs exceed it; the generic README is 168) | target <= ~150 lines; generic base layer may run to ~170 | `docs/TEMPLATE.md` |
| 22 | TEMPLATE path wording implied the generic set was `docs/<cluster>/<set>` | states `docs/generic/` explicitly; assets/refcard are not sets | `docs/TEMPLATE.md` |
| 23 | "Verification" could be read as a required user page | renamed to "Verification basis"; states a user set has no verification page | `docs/TEMPLATE.md` |
| 24 | TEMPLATE topic example "extended" is no longer a set term | replaced with `sniffer` | `docs/TEMPLATE.md` |
| 25 | TEMPLATE assumed every function has a settings menu | "the settings menu with its defaults, if any" | `docs/TEMPLATE.md` |
| 26 | SPEC legend mixed the source-documented basis with the v3 build-only status | "present in the v3 build; v3 is build-only - documented from source, not verified on v3 hardware" | `docs/SPEC.md` |

## Notes

- SMPS stays in §2 as a binary-only row so the existing section layout is
  preserved; the Mode cell and the note both say it has no menu mode.
- The SPEC keeps `protocol/wire/README.md` unlinked (the three child sets are
  linked directly); the tree-alignment record already calls this intentional.
- The uncommitted module deltas in the worktree were not touched.

**Reconciled / remaining questions:** reconciled; no unresolved SPEC row.
One judgment call for the owner: the root README now carries a short
"What works" summary alongside the SPEC link; remove that section if the
entry point should stay purely navigational.
