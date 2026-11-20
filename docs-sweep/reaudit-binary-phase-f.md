# Binary docs — Phase F re-audit

**4 of 4 filled in full** (3 in-set findings + the presentation nit). The binary set is clean; the 7 recorded cross-set items remain open for the main session.

Scope: only my Phase F findings, checked against the current working tree of `docs/protocol/binary/`. The repair is uncommitted but present; the local `Firmware/` was not used. The cross-set pages were not edited by this set, so they are listed separately and are not counted in the 4.

## Per-item result

| # | Item | Status | Current evidence | Remainder |
|---:|---|---|---|---|
| 1 | Timer2 / PWM after reset | **Filled in full** | `commands.md:215-218`: "The reset also clears the generator flag and stops Timer2, but it does not disable the output-compare channel or clear its period/duty setup; send `0x13` if the PWM module must be left fully idle." `commands.md:288-291`: "The v4 terminal reset clears the generator flag, stops Timer2 and detaches the AUX output source…" `entering-and-framing.md:190-192`: "It clears the generator flag and stops Timer2, but it does not disable the output-compare channel…" `edge-cases.md:120-124` and `:80-81` agree; `examples.md:155-156` now says "Send `0x13` if the PWM module must be left fully idle." | None. The `0x40` recovery also now says `0x0F` "resets the pins, the supply and the pull-ups and stops a PWM timer." |
| 2 | Zero-timeout `0x70` | **Filled in full** | `commands.md:76-80`: "It always returns `0x00` (timeout) and then the current level, even if an edge is pending: expiry is evaluated before the change-notification flag." `commands.md:369-371`: "A **zero timeout always returns `0x00`** (timeout)… Do not wait for `0x01` from a zero-timeout wait." `edge-cases.md:198-205` heading "Wait-for-edge with a zero timeout always reports a timeout"; `examples.md:99-106` says "always reports `0x00`, even if an edge is pending" and labels the result "timeout". | None. The general `0x70` result row still correctly says `0x01` when an edge arrives for non-zero timeouts. |
| 3 | `0xF0` start clears the sample | **Filled in full** | `sub-protocols.md:290`: "last stored feedback sample; a start clears it". `sub-protocols.md:294-296`: "A start clears the stored sample, so a read after the new start is `0x00 0x00` until the first conversion completes; a stop leaves the last sample unchanged." | None in this set. The wrong "not cleared / stale previous sample" text on the linked SMPS pages is cross-set C6. |
| P1 | Presentation wrap | **Filled in full** | `sub-protocols.md:300-303`: "The regulation loop, the range where the firmware cutoff can act and the electrical caveats are in [the SMPS binary page](...); whether the add-on actually reaches a requested target is unknown." The changed line is now within 80 columns; the remaining wide lines are the pre-existing link lines. | None. |

## Cross-set items (not counted; main session)

These are outside `docs/protocol/binary/` and the repair record did not edit them. They remain open.

| # | Sev. | Current page / claim | Status |
|---:|---|---|---|
| C1 | major | `protocol/i2c/binary.md:232-235` still says `0x09` is "not always two bytes" and that drive/release print a terminal text line; `:265-266` still says "consume the `AUX INPUT/HI-Z` text." | Open; `#202`/`#218` make `0x09` exactly `0x01` + sub-result, silent. |
| C2 | major | `protocol/jtag/xsvf.md:100-102` still says an `XCOMMENT` stall "keeps reading `0xFF` and does not end until the board is reset"; also `:180-181`, `jtag/edge-cases.md:74`, `tools/xsvf/limits-and-errors.md:74-76`, `tools/xsvf/player.md:155-159`, `tools/xsvf/README.md:98`. | Open; `#227` `micro.c:1542-1548` exits the comment loop and returns `0x01`; `0x0F` is reachable. |
| C3 | minor | `protocol/jtag/openocd.md:120-121` still says the v3 zero-count path is unverified; also `jtag/edge-cases.md:42`, `tools/openocd/commands.md:196-198`, `tools/openocd/limits-and-errors.md:25`. | Open; `#227` `openocd.c:224-249` guards v3 zero count; it is a no-op with the 3-byte echo and no payload/TDO. |
| C4 | major | `protocol/spi/binary.md:105-110` still says `0x02` "does not change the pin direction"; `:190-197` still says a bare `0x02` "cannot recover the connection"; `spi/edge-cases.md:114`. | Open; `#204` `spi_binary_set_cs()` makes `0x02` assert CS as an output and `0x03` release it (input in open drain, driven high in normal). |
| C5 | major | `protocol/wire/2wire/macros.md:61-68` still says the ATR values are "not a valid ATR" because reset is asserted; `:158-159` still says `(1)` leaves CS low; `smartcard.md:63-65`, `:98`; `edge-cases.md:32,34`. | Open; `#226` `raw2wire.c:155-163` releases CS before `read_atr_header()` and leaves CS high. |
| C6 | major | `capabilities/smps/binary.md:196-198` still says the stored sample "is not cleared" and a post-start read "can still return the previous sample"; also `:241-242`, `smps/edge-cases.md:194-200`, `smps/README.md:77-78`, `:133-134`. | Open; `#218` `smps.c:62-63` clears `voltage_reading` at start. |
| C7 | minor | `core/system.md:534-537` reset order still has no Timer2 stop and no V4 `AUX1` ODC clear. | Open; core `d07430ca` `base.c:186-202` adds both. |

## Presentation check (touched files)

- **TAB characters:** none.
- **Captures/tables:** every `host ->`/`host <-` comment is in column 41; all changed tables have consistent columns.
- **~80 columns:** the changed `0xFx` paragraph is reflowed and no changed prose line exceeds 80. The remaining wide lines are pre-existing link lines: `commands.md:26` (100), `edge-cases.md:28` (89), `entering-and-framing.md:33`/`:229` (89), `examples.md:180` (81), `README.md:109` (89), `sub-protocols.md:94` (81).
- **Links/anchors:** all relative links resolve; the anchor check found no missing target for `#reply-shapes`, `#reading-pins`, `#wait-for-an-edge-0x70`, `#pwm-0x12-0x13`, `#self-test-0x10-0x11`, `#leaving`, `#0x0f-inside-a-mode-does-not-return-to-the-terminal`, `#adc-is-busy--the-smps-feedback-owns-the-converter`, `#entry-bytes`, `#the-aux-pin`, `#same-byte-different-layer-or-mode`, `#0x1x--bulk-byte-transfer` and `#0xfx--smps-raw-wire-v4-only`.
- **No bench/date/pass-count prose:** no run dates, pass counts, capture provenance, evidence sections or log names. The v3 "unverified / hardware not verified" caveat remains the allowed short caveat.

---

**4 of 4 filled in full.** In-set binary set clean; 7 cross-set items remain for the main session.
