# Phase F fixes — `capabilities/auxiliary`

**Scope:** repair of the six findings in
`docs-sweep/phase-f-aux-audit.md` only.  The rest of the set was not
re-reviewed and is unchanged.

**Verified against the PR heads** (the local combined `Firmware/` is the old
`c57e223` and was not used as authority):

- core `C:\Users\jschl\dev\bpv4-usb` @ `d07430ca`
- #218 `C:\Users\jschl\dev\bpv4-51wait` @ `e83945a`
- #202 `C:\Users\jschl\dev\bpv4-202` @ `2d82dc0`
- #242 `C:\Users\jschl\dev\bpv4-dio` @ `b8c8296`

**Docs head:** `docs/combined-sweep` @ `9dfc682`.  **No commit.**

**Result:** 6/6 fixed, 0 rejected, 0 partial.

**Files changed**

- `docs/capabilities/auxiliary/pwm-servo.md`
- `docs/capabilities/auxiliary/binary.md`
- `docs/capabilities/auxiliary/edge-cases.md`
- `docs/capabilities/auxiliary/edge-wait.md`
- `docs/capabilities/auxiliary/wiring.md`

No cross-set page was edited.

---

## 1 — v4 reset now stops Timer2 — **fixed**

**Files:** `pwm-servo.md` (changing table, three-layer table, prose);
`binary.md` (`0x0F` paragraph); `edge-cases.md` (mode-change entry and the
`0x12` → `0x0F` entry).

**Old claims removed:** "Timer2 is not stopped", "does not stop Timer2",
"Timer2 still runs", and the timer row that omitted a board reset.

**New text (representative):**

> A v4 reset (`#`, a mode change, the binary return, or the SUMP exit) clears
> the generator flag, stops Timer2 and detaches the output.  It does not
> disable the OC5 channel or clear its period/duty setup; send `0x13` if the
> module must be fully idle.  On an SMPS-enabled v4 build the reset also runs
> the SMPS stop path, which disables the OC5 channel, then stops Timer2.  On
> v3, `#` and the binary return are hardware resets; a mode change and the
> SUMP exit run the software reset, which also clears the flag, stops Timer2
> and detaches the output.

The three layers are stated separately: **generator flag** (what makes `f`
refuse and a bare `g` / `S` stop), **output path** (whether the timer reaches
`AUX`), and **timer registers** (Timer2 plus the OC5 compare/period setup).

**Evidence:** `bpv4-usb@d07430ca`, `Firmware/core_system/base.c`,
`bp_reset_board_state()`: after the V4 block calls `smps_stop()`, the
unconditional line `T2CONbits.TON = OFF;` runs before
`bp_aux_pin_reset_state()`.  It is outside the `BUSPIRATEV4` conditional, so
the v3 software reset paths stop Timer2 as well.  `bp_aux_pin_reset_state()`
in `bpv4-51wait@e83945a`, `Firmware/capability_aux/aux_pin.c`, only does
`AUXPIN_RPOUT = OFF; state.mode = AUX_MODE_IO;` — it does not touch
`OC5CON`, `PR2`, `OC5R` or `OC5RS`.  That is why the OC5 advice remains but
the reason changed.

---

## 2 — I2C `0x09` is silent — **fixed**

**Files:** `binary.md` (command-map row and the whole
`I2C extended AUX (0x09)` section); `edge-cases.md` (`0x09` entry).

**Old claims removed:** "optional text for drive/release",
`AUX LOW` / `AUX HIGH` / `AUX INPUT/HI-Z` in the reply table, the text
exchange, and the "read the text"/"terminal for a clean reply" advice.

**New text (representative):**

> Every reply is exactly **two bytes**: the first byte is always `0x01` and
> reports that the command byte was recognised, not that the sub-command
> succeeded; the sub-result follows.

`0x00` / `0x01` / `0x02` / `0x10` / `0x20` all return `0x01 0x01`; `0x03`
returns `0x01` plus the pin level; an unknown sub-command returns
`0x01 0x00`.

**Evidence:** `bpv4-202@2d82dc0`, `Firmware/protocol_i2c/i2c.c`, `case 9`:
`REPORT_IO_SUCCESS()` sends the leading `0x01`, then the handler reads the
sub-command, sets `fr`, and ends with
`user_serial_transmit_character(fr);`.  The source comment is explicit:
"Do not call the terminal helpers here: their messages would corrupt the
reply."  `0x03` sets `fr = bp_aux_pin_read()`; `0x10`/`0x20` leave `fr = 1`;
the default sets `fr = 0`.

---

## 3 — trailing spaces are ignored by `g` / `S` — **fixed**

**Files:** `pwm-servo.md` (the "any `g` line" paragraph and the bare-command
paragraph); `edge-cases.md` (the old trailing-space entry).

**Old claim removed:** "a trailing space is read as an argument, so `g ` and
`S ` take the setup path, stopping a running output first (`g ` prints
`PWM disabled`; `S ` stops silently)".

**New text (representative):**

> Trailing spaces are ignored: `g ` and `S ` behave exactly like `g` and `S` —
> a stop when a generator runs, the setup when none does.

The edge-case entry now says the trailing space is accepted and the result is
the same as the bare command.

**Evidence:** `bpv4-51wait@e83945a`, `Firmware/capability_aux/aux_pin.c`,
`bp_pwm_setup()` and `bp_servo_setup()` both look ahead with
`argument = (cmdstart + 1) & CMDLENMSK; while (cmdbuf[argument] == ' ')
argument = (argument + 1) & CMDLENMSK;` before the bare check
`cmdbuf[argument] == 0x00`.  A line with only spaces therefore reaches the
stop branch and returns after `BPMSG1028` (`PWM disabled`).

---

## 4 — zero-timeout `0x70` always returns `0x00` + level — **fixed**

**Files:** `edge-wait.md` (timeout contract); `edge-cases.md` (zero-timeout
entry).

**Old claim removed:** "normally returns result `0x00` ... A change that
lands between arming and the first sample can still be reported as an edge, so
`0x01` is possible".

**New text (representative):**

> `0` does not wait: it always returns result `0x00` followed by the current
> level.  It is the documented way to read the selected pin with this command;
> use a non-zero timeout to wait for an edge.

**Evidence:** `bpv4-51wait@e83945a`, `Firmware/protocol_binary/binary_io.c`,
`handle_wait_command()`: `timeout_ticks = timeout_ms * 2000`; the loop tests
`if (elapsed_ticks >= timeout_ticks)` **before** `if (IFS1bits.CNIF)`, with
the comment "Expiry wins over a pending edge, including the zero-timeout arm
window".  With `timeout_ms == 0` the first pass expires, `edge_seen` stays
`NO`, and the reply byte is `BP_BINARY_IO_RESULT_FAILURE` (`0x00`).

---

## 5 — DIO `w` parks seven pins on v4 — **fixed**

**File:** `wiring.md` ("Leaving the board").

**Old text:** "DIO parks its five signal pins, for example) but it does not
guarantee that a manually driven `AUX1` / `AUX2` is released."

**New text:**

> It may park the pins of the active mode (DIO parks its five signal pins and,
> on v4, `AUX1` and `AUX2` — seven in total; other modes may release fewer)
> but it does not guarantee that every manually driven pin is released.

**Evidence:** `bpv4-dio@b8c8296`, `Firmware/protocol_dio/dio.c`,
`dio_cleanup()`: `park_mask = AUX | MOSI | CLK | MISO | CS;` and, under
`BUSPIRATEV4`, `park_mask |= AUX1 | AUX2;`.  Five pins on v3, seven on v4.

---

## 6 — reflow the three-layer table — **fixed**

**File:** `pwm-servo.md` ("Mode and timer side effects").

The table is now two columns (`Layer` | `Changed by`) after moving the
"what it controls" detail into the intro sentence.  Content is preserved; the
timer row also gains the board-reset entry from finding 1.

**Old cell lengths:** 195 and 239 characters.  **New row lengths:** 96, 85
and 135 characters.  No code evidence is involved; this is presentation only.

---

## Cross-set list (for the main session — not edited here)

1. **`docs/protocol/i2c/binary.md` — silent `0x09`.**
   - `:72` "`0x01`, optional text for drive/release, then the sub-result".
   - `:127` "consume the `AUX INPUT/HI-Z` text".
   - `:234-241` "It is **not always two bytes**: the drive/release
     sub-commands print a terminal text line".
   - `:266` "`AUX INPUT/HI-Z` text".
   - The worked exchange at `:304` still shows the text.
2. **`docs/protocol/binary/commands.md` — reset/Timer2 wording.**
   - `:216` "detaches the AUX output and clears the generator flag, but the
     timer itself still runs; send `0x13` ...".
   - `:291` "the target stops being driven while Timer2 may still run".
   The `0x0F`/`0x12` reset sections need the same stop-Timer2 correction as
   finding 1.
3. **`docs/capabilities/smps/` — stored sample and ODC preload.**
   - `binary.md:84-86` and `:118-119` still say a start does not clear the
     stored sample and there is no freshness flag.
   - `feedback-loop.md:82-84` "A start does not clear the stored sample".
   - `README.md:133` "`0xF0` can return the previous run's sample after a
     start".
   - `edge-cases.md:200` and the `0xF0`/ODC entries; `wiring.md:74` still
     says the SMPS start does not clear the open-drain bit.
4. **`docs/capabilities/lcd/` — ODC persistence.**
   - `edge-cases.md:99-107` the open-drain carry-over workaround.
   - `README.md:55-57` "still says `Open drain outputs`".
   - `entry-and-setup.md:142-144` the same persistence claim.
   - `spi-link.md:114-125` "the bits persist ... Power-cycle ... select
     normal outputs".
5. **`docs/protocol/dio/` — five-pin park.**
   - `park-and-reset.md:34`, `:52`, `:59` ("`AUX1` / `AUX2` ... **not
     changed**"), `:133`.
   - `edge-cases.md:27`, `:86`, `:89-95` ("`w` leaves `AUX1` / `AUX2`
     driven (v4)").
   - `README.md:56`, `:112`; `aux-pin.md:143` ("`w` releases a driven
     `AUX1` / `AUX2` target | - | no"); `pin-control.md:174`;
     `entry-and-setup.md:124`, `:133-136`; `wiring.md:140-142`.
   The DIO set needs the seven-pin (v4) / five-pin (v3) contract.

---

## Verification performed

- Re-read the four PR worktrees at the listed heads for every changed claim.
- Confirmed the local combined `Firmware/` lacks these fixes and was not used
  as authority.
- No TAB characters and no trailing whitespace in the changed set.
- All relative links in the set resolve on disk (scripted check: 0 missing).
- Code fences balanced; changed table cells shortened from the audited
  195/239 characters.
- No cross-set page was modified; the list above is the handoff.

**Not run:** hardware confirmation.  The four behaviour findings are
code-verified at the PR heads; nothing in this repair adds a new bench claim.
