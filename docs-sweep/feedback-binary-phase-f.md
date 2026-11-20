# Binary docs — Phase F delta audit

**Verdict: yes with fixes.** The six pages are structurally sound, but two in-set protocol claims (Timer2 after reset; zero-timeout `0x70`) are stale against the repaired firmware, and several linked sets still describe pre-repair behavior.

Scope: only the statements changed by the Phase F sources. I read the six files in `docs/protocol/binary/` and checked changed claims against `#218 bpv4-51wait@e83945a`, `#202 bpv4-202@2d82dc0`, `#227 bpv4-oocd@96ccc14`, `#226 bpv4-wire@3bed559`, `#204 bpv4-spi@d1e4ef7` and core `bpv4-usb@d07430ca` — not the stale local `Firmware/`.

## In-set findings

### 1. [major] `0x0F` stops Timer2; the binary pages still say the timer keeps running

- `commands.md:214-216` — "On v4 this reset detaches the AUX output and clears the generator flag, but the timer itself still runs; send `0x13` before leaving if you want the module idle."
- `commands.md:287-290` — "`0x0F` does not stop the timer either; … so the target stops being driven while Timer2 may still run"
- `entering-and-framing.md:189-191` — "It is not a substitute for `0x13`: the PWM timer is left running, so send `0x13` first if the module must be idle."
- `edge-cases.md:118-122` — "the timer may still run… `0x0F` does not stop the PWM timer, and no code path stops it on this exit."
- `examples.md:154-155` — "Send `0x13` before leaving if you want the timer idle."

**Why wrong.** Core `d07430ca` `Firmware/core_system/base.c:201-202` does `T2CONbits.TON = OFF` in `bp_reset_board_state()`; the v4 terminal reset (`proc_menu.c:275-277`, `:430`) runs that function. The auxiliary set already has the correct wording: `capabilities/auxiliary/binary.md:84-87` — the reset "clears the generator flag, stops Timer2 and detaches the output.  It does not disable the OC5 channel or clear its period/duty setup; send `0x13` if the PWM module must be left fully idle."

**Exact fix.** Replace the four timer claims (and adjust the examples advice) with: "The v4 terminal reset clears the generator flag, stops Timer2 and detaches the AUX output source, but it does not disable the output-compare channel or clear its period/duty setup; send `0x13` if the PWM module must be fully cleared." Change `examples.md` "timer idle" to "PWM module cleared." Keep the v3 hardware-reset wording.

### 2. [major] Zero-timeout `0x70` is documented as if an edge could return `0x01`

- `commands.md:76-79` — "A stable pin gives `0x00` (nothing was awaited) and then the level."
- `commands.md:361-366` — "`0` checks the pin once and returns immediately. … a result byte (`0x01` when the requested edge arrived, `0x00` on timeout)"
- `edge-cases.md:199-203` — "a wait with a timeout of `0` and a pin that is already at rest… result byte `0x00` (timeout)"
- `examples.md:99-105` — "A timeout of `0` checks the pin once and returns immediately, which doubles as a level sample" … `host <- 0x00` "result: no edge"

**Why wrong.** `#218 e83945a` `Firmware/protocol_binary/binary_io.c:991-995` checks expiry before `CNIF`; the PR body says: "Valid zero-timeout requests always return `00 <current-level>`, even with an edge in the arm/read window." The auxiliary page is explicit: `capabilities/auxiliary/edge-wait.md:74-76` — "`0` does not wait: it always returns result `0x00` followed by the current level." The binary page only promises `0x00` for a stable pin and its general result row implies an edge could produce `0x01`.

**Exact fix.** In `commands.md` §Reading pins and §Wait for an edge, `edge-cases.md`, and `examples.md`, state: "A zero timeout always returns `0x00` (timeout) followed by the selected pin level, even if an edge is pending; the firmware evaluates expiry before the change-notification flag. Do not wait for `0x01` from a zero-timeout wait." Relabel the example result from "no edge" to "timeout."

### 3. [minor] `0xF0` is described as the "last feedback sample"; a start clears it

- `sub-protocols.md:290` — "`0xF0` | last feedback sample (ADC-node count, not a calibrated voltage) | 2 bytes, high byte first; no result byte"

**Why wrong.** `#218 e83945a` `Firmware/capability_smps/smps.c:62-63` sets `smps_state.voltage_reading = 0` while the ADC/ISR are disabled: "Do not expose a previous run's feedback before the first new conversion." A start therefore clears the stored sample. The row says "last feedback sample" without that qualification, and the linked SMPS page says the opposite (cross-set item C6).

**Exact fix.** Change the row to "last stored feedback sample; a start clears it, so before the first new conversion `0xF0` reads `0x00 0x00`." Add one sentence to the `0xFx` section: "A start clears the stored sample; a read after the new start is zero until the first conversion completes."

## Cross-set list (not part of the binary set)

| # | Sev. | Page and current quote | Why / exact fix |
|---:|---|---|---|
| C1 | major | `docs/protocol/i2c/binary.md:232-235` — "It is **not always two bytes**: the drive/release sub-commands print a terminal text line on the same wire between the two." See also `:243-245` "A host must consume that text before reading the sub-result." | `#202 2d82dc0` / `#218 e83945a` `Firmware/i2c.c:1075-1123` sends only `REPORT_IO_SUCCESS()` then the sub-result — exactly two bytes, no text. Remove the text table and the "consume" advice; state "`0x01`, then the sub-result (always two bytes, silent)" and update the example at `:302-305`. |
| C2 | major | `docs/protocol/jtag/xsvf.md:100-102` — "except when the stall falls inside an `XCOMMENT` string: that loop keeps reading `0xFF` and does not end until the board is reset." Also `:180-181`; `jtag/edge-cases.md:74`; `tools/xsvf/limits-and-errors.md:74-76`; `tools/xsvf/player.md:155-159`; `tools/xsvf/README.md:98`. | `#227 96ccc14` `Firmware/tool_xsvf/micro.c:1542-1548` exits the comment loop on `xsvf_stream_stalled`; `xsvfRun` returns `XSVF_ERROR_UNKNOWN` (`0x01`) at the next command boundary, and `0x0F` is reachable. Remove the reset-only exception; state the stall unwinds with `0x01` and the `0x0F` exit works. |
| C3 | minor | `docs/protocol/jtag/openocd.md:120-121` — "On v4 a **zero count** consumes no payload and returns no TDO byte. On v3 the zero-count path is unverified." Also `jtag/edge-cases.md:42`; `tools/openocd/commands.md:196-198`; `tools/openocd/limits-and-errors.md:25`. | `#227 96ccc14` `Firmware/tool_openocd/openocd.c:224-249` guards the v3 fast shift with `if (j)`; zero count is a no-op with the 3-byte echo and no payload/TDO. Change all four to "zero count is a guarded no-op on both boards; v3 is no longer unverified/unsupported." |
| C4 | major | `docs/protocol/spi/binary.md:105-110` — "`0x02` writes the latch low; it drives CS low when the pin is already an output… It does not change the pin direction." Also `:190-197` "A bare `0x02` writes the latch low but leaves the pin an input, so it cannot recover the connection"; `spi/edge-cases.md:114`. | `#204 d1e4ef7` `Firmware/protocol_spi/spi.c:1162-1177` `spi_binary_set_cs()` sets the direction: `0x02` asserts CS and makes it an output; `0x03` releases it (input in open drain, driven high in normal); auto-CS uses the same helper. Rewrite the rows/section and remove the "cannot recover" workaround and the `0x03` edge-case row. |
| C5 | major | `docs/protocol/wire/2wire/macros.md:61-68` — "The read happens after the final CS low, so the card's reset is asserted while the four values are sampled… **The four values are not a valid ATR.**" Also `:158-159` "`(1)` leaves CS low"; `smartcard.md:63-65` and `:98`; `edge-cases.md:32,34`. | `#226 3bed559` `Firmware/protocol_wire/raw2wire.c:155-163` drives CS low, delays, clocks, then drives CS high **before** `read_atr_header()` samples the four values; the macro leaves CS high. Remove the invalid-ATR warning; state reset is released before sampling and CS is high afterward. |
| C6 | major | `docs/capabilities/smps/binary.md:196-198` — "The stored sample is not cleared, so a `0xF0` read immediately after the new start can still return the previous sample"; also `:241-242`; `smps/edge-cases.md:194-200`; `smps/README.md:77-78` and `:133-134`. | `#218 e83945a` `Firmware/capability_smps/smps.c:62-63` clears `voltage_reading` at start. A start does **not** expose the previous run's sample; the first `0xF0` after a start is zero until a new conversion completes. Replace "not cleared / previous sample / stale" with the clear-at-start rule; keep "stop leaves the last sample unchanged." |
| C7 | minor | `docs/core/system.md:534-537` — "Release the bus pins (MOSI, CLK, MISO, CS) and `AUX` (`BP_AUX0`): direction input." / "v4: release AUX1/AUX2; on SMPS builds run `smps_stop()`; detach the AUX1/AUX2 peripheral outputs and clear their latches." The ordered reset list has no Timer2 stop and no V4 `AUX1` ODC clear. | Core `d07430ca` `Firmware/core_system/base.c:186-202` clears `BP_AUX1_ODC` and does `T2CONbits.TON = OFF`. Add both steps to the reset order and to the reset-path table. This is an omission, not a direct contradiction. |

## Presentation check (changed passages only)

- **TAB characters:** none in the six files.
- **Captures/tables:** changed tables render; every `host ->`/`host <-` comment remains in column 41.
- **Wraps:** one changed prose line is 96 columns — `sub-protocols.md:297` "act and the electrical caveats are in [the SMPS binary page](…);" — wrap it. The other wide lines (`commands.md:26`, `README.md:109`, etc.) are pre-existing link lines accepted in round 1; I did not reopen them.
- **No bench/date/pass-count prose:** no run dates, pass counts, capture provenance, evidence sections or log names. The v3 "unverified / hardware not verified" caveat remains the allowed short caveat.

## Counts

- **3 in-set findings:** 2 major, 1 minor.
- **1 presentation nit:** the changed 96-column line above.
- **7 cross-set items:** C1–C7, to be handled by the main session.
