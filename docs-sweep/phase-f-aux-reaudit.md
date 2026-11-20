# Phase F re-audit - `capabilities/auxiliary`

Re-audit of the six findings in `docs-sweep/phase-f-aux-audit.md` against the
current working tree (`docs/combined-sweep` @ `9dfc682` + 5 uncommitted files:
`pwm-servo.md`, `binary.md`, `edge-cases.md`, `edge-wait.md`, `wiring.md`).
Behaviour re-checked against core `d07430ca`, #218 `e83945a`, #202 `2d82dc0`
and #242 `b8c8296`.  No fresh full review.

## Verdict

**6 of 6 filled in full.**  0 partial, 0 not addressed, 0 regressed.  The set
is clean for the Phase F delta.

| # | Finding (short) | Verdict | Evidence in the current text |
|---|---|---|---|
| 1 | v4 reset now stops Timer2 | **filled in full** | `pwm-servo.md:129-130,133-134` "stops Timer2"; `:227` "a board reset (stops Timer2; the OC5 setup stays)"; `:229-236` "clears the generator flag, stops Timer2 and detaches the output.  It does not disable the OC5 channel ..."; `binary.md:84-88` same; `edge-cases.md:159-164` and `:348-351` same |
| 2 | I2C `0x09` is silent | **filled in full** | `binary.md:22` "`0x01`, then the sub-result (always two bytes)"; `:120-122` "Every reply is exactly **two bytes**"; `:124-136` two-byte table + "each answer `0x01 0x01`"; `:138-141` exchange `0x01 0x01`; `edge-cases.md:298-305` "exactly two bytes ... nothing is printed" |
| 3 | trailing spaces are ignored | **filled in full** | `pwm-servo.md:77-82` "A line with nothing but spaces after `g` is the bare form: when a generator runs it stops and returns"; `:138-140` "Trailing spaces are ignored: `g ` and `S ` behave exactly like `g` and `S`"; `edge-cases.md:85-92` entry inverted, "the trailing space is accepted" |
| 4 | zero-timeout `0x70` always `0x00` | **filled in full** | `edge-wait.md:74-76` "it always returns result `0x00` followed by the current level ... use a non-zero timeout to wait"; `edge-cases.md:245-249` "expiry is checked before the change flag ... no edge can be reported" |
| 5 | DIO `w` parks seven on v4 | **filled in full** | `wiring.md:148-150` "DIO parks its five signal pins and, on v4, `AUX1` and `AUX2` — seven in total; other modes may release fewer ... does not guarantee that every manually driven pin is released" |
| 6 | reflow the three-layer table | **filled in full** | `pwm-servo.md:217-227` "what it controls" moved to the intro; two-column table; longest row now 135 characters (was 239) |

## Per-item detail

### 1 - Timer2 on reset - filled in full

Current text (representative):

> A v4 reset (`#`, a mode change, the binary return, or the SUMP exit) clears
> the generator flag, stops Timer2 and detaches the output.  It does not
> disable the OC5 channel or clear its period/duty setup; send `0x13` if the
> module must be fully idle.  On an SMPS-enabled v4 build the reset also runs
> the SMPS stop path, which disables the OC5 channel, then stops Timer2.  On
> v3, `#` and the binary return are hardware resets; a mode change and the
> SUMP exit run the software reset, which also clears the flag, stops Timer2
> and detaches the output.

Matches the core: `bp_reset_board_state()` calls `smps_stop()` in the V4
block, then `T2CONbits.TON = OFF` (outside the V4 conditional) before
`bp_aux_pin_reset_state()`, which only clears `AUXPIN_RPOUT` and the generator
flag.  "Does not disable the OC5 channel" is correct for a non-SMPS build (the
reset never writes `OC5CON`/`PR2`/`OC5R`); the SMPS-build sentence correctly
carves out `smps_stop()` disabling it.  The `0x13` advice is retained for the
right reason.  All nine stale locations are fixed; no "Timer2 is not stopped" /
"may still run" text remains in the set.

### 2 - silent I2C `0x09` - filled in full

Current text: "Every reply is exactly **two bytes**: the first byte is always
`0x01` and reports that the command byte was recognised, not that the
sub-command succeeded; the sub-result follows."  The sub-command table gives
`0x01` for `0x00`/`0x01`/`0x02`/`0x10`/`0x20`, the level for `0x03`, and `0x00`
for other.  Matches `bpv4-202@2d82dc0` (`case 9`: `REPORT_IO_SUCCESS()`, then
`user_serial_transmit_character(fr)`; the pads are written directly, the
terminal helpers are not called).  The text table, the text exchange and the
"read the text"/"terminal for a clean reply" advice are gone.  The
`edge-cases.md` entry is consistent.

### 3 - trailing spaces - filled in full

Current text: "Trailing spaces are ignored: `g ` and `S ` behave exactly like
`g` and `S` — a stop when a generator runs, the setup when none does."  Matches
`bpv4-51wait@e83945a`: both handlers skip spaces before the `cmdbuf[argument] ==
0x00` bare check, so a spaces-only line reaches the stop branch, prints
`PWM disabled` and returns; with nothing running it falls through to the
setup.  The `pwm-servo.md` "A `g` line with arguments ... stops it first"
paragraph no longer claims a trailing space is an argument, and the
`edge-cases.md` entry now says the space is accepted (the old "starts the
interactive setup / stops silently" claim is gone).

### 4 - zero-timeout `0x70` - filled in full

Current text: "`0` does not wait: it always returns result `0x00` followed by
the current level.  It is the documented way to read the selected pin with
this command; use a non-zero timeout to wait for an edge."  Matches
`bpv4-51wait@e83945a`: the loop computes `elapsed_ticks` and tests
`elapsed_ticks >= timeout_ticks` before `IFS1bits.CNIF`; with `timeout_ms == 0`
the first pass expires and `edge_seen` stays `NO`.  The "normally"/"`0x01` is
possible" caveats are removed in both pages.

### 5 - DIO `w` seven-pin park - filled in full

Current text: "It may park the pins of the active mode (DIO parks its five
signal pins and, on v4, `AUX1` and `AUX2` — seven in total; other modes may
release fewer) but it does not guarantee that every manually driven pin is
released."  Matches `bpv4-dio@b8c8296`: `park_mask = AUX | MOSI | CLK | MISO |
CS` plus `park_mask |= AUX1 | AUX2` under `BUSPIRATEV4`.  The general caveat is
now correct for the modes that do not extend their mask.

### 6 - table reflow - filled in full

The table is now `Layer | Changed by`, with the "what it controls" detail in
the intro paragraph.  Row lengths are 96, 85 and 135 characters (was
195/239/120).  The 135-character timer row is a normal table width for this set
(other tables run 120-160); no prose line overflows.  Accepted as fixed; no
further action needed.

## Presentation check (touched files)

| Check | Result |
|---|---|
| TAB characters | none (all 8 set files scanned) |
| Trailing whitespace | none (all 8 set files) |
| Prose ~80 columns | clean; the only >80 lines are table rows (set convention) |
| Table shape | new two-column table is well formed (2 pipes per row); sub-command table consistent |
| Code fences | balanced (`pwm-servo` 10, `binary` 6, `edge-wait` 4; even counts) |
| Relative links | 0 missing across the set (scripted check) |
| Anchors | in-set `#checking-the-target`, `#reset-and-persistence`, `#mode-and-timer-side-effects`, `#i2c-extended-aux-0x09` all exist; the external `#aux-0x09` and `#smps-owns-the-channel` headings are unchanged; no inbound link points at a renamed heading |
| Dates / bench / tested prose / pass counts | none in the set; only short `unverified` caveats (pre-existing, allowed) |
| Scope of edits | 5 files, all changes confined to the six findings; no cross-set page edited |

## Residual notes (not findings)

- The `timer registers` table row is still 135 characters.  It is a table row,
  not prose, and shorter than the 136-character row in the same page; nothing
  to fix for this delta.
- One uncommitted artefact outside the docs set:
  `Firmware/{bpv4.elf,bpv4.hex,bpv4.map}`, `Firmware/build/`,
  `Firmware/tests/__pycache__/` are untracked in the worktree.  They are not
  part of the doc set; the commit should stage only the five `.md` files and
  the local `docs-sweep/` records if the commit rule includes them.
