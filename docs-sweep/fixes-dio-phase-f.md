# DIO documentation — Phase F repair (RA-7 delta audit)

**Function:** `dio` · **Phase:** F (post-RA-7) · **Tree:**
`docs/protocol/dio/` (9 pages) · **Docs head:** `dddcb02b` ·
**Verified against:** the worktree's final combined
`Firmware/` (`9b179cb5`), not a PR worktree.

Source read for verification: `Firmware/protocol_dio/dio.c`,
`Firmware/core_system/core.c`, `Firmware/core_system/proc_menu.c`,
`Firmware/core_system/base.c`, `Firmware/capability_aux/aux_pin.c`,
`Firmware/hardwarev4.h`.  Line refs are point-in-time evidence and are allowed
only in this local record.

**Status summary:** 7/7 in-set findings fixed (2 blockers, 3 majors, 2
minors); 0 partial, 0 rejected.  One cross-set item recorded for the main
session (`docs/dev/verification.md`), not edited here.

---

## 1. `park-and-reset.md` — safe-park contract was five-pin → `fixed`

The page now owns a **five-pin v3 / seven-pin v4** contract throughout.

Quotes replaced:

- “For the five signal pins, the mode's cleanup does five things, in this
  order:” → “The mode's cleanup parks seven pins on v4 and five on v3: the
  five signal pins on both boards, plus `AUX1` and `AUX2` on v4.”
- “`w` performs this park … It does **not** release `AUX1` or `AUX2`.” →
  “On v4 the park includes both `AUX1` and `AUX2`, regardless of which AUX
  target is selected.”
- “`w` releases only the five signal pins; `#` releases every I/O pin:” →
  “`w` releases every pin the park covers (five on v3, seven on v4); `#`
  releases every I/O pin and resets the mode settings too:”.
- The `w`/mode-change/`#` table row “`AUX1` / `AUX2` (v4) | **not changed**”
  is now `input | input | input`.
- The “what `w` resets / what it leaves” paragraph now names the parked seven
  pins and adds that the AUX selector, AUX output routing and `AUX1`
  open-drain setting are unchanged by `w`.
- The old “`AUX1` / `AUX2` exception (v4)” section is now “The v4 extra
  pins”, stating that `w` releases both regardless of the selected target.
- Post-`w` verification now checks five pins on v3 or all seven on v4.
- The v3/v4 table now has a row “`w` also parks `AUX1` / `AUX2` | — | yes
  (both, regardless of the selected target)”.

Code evidence:

- `Firmware/protocol_dio/dio.c` `dio_cleanup()`: `park_mask = AUX | MOSI |
  CLK | MISO | CS`, then `#ifdef BUSPIRATEV4 park_mask |= AUX1 | AUX2;`;
  `IODIR |= park_mask`; `bp_disable_pullup()`; v4
  `bp_disable_3v3_pullup()`; `IOLAT &= ~park_mask`; `numbits = 8`,
  `int16 = NO`.  The mask does not depend on `alternate_aux`.
- `Firmware/core_system/proc_menu.c` `switch_psu_off()` calls
  `bp_current_protocol()->cleanup()` before disabling the regulator;
  `Firmware/core_system/core.c` DIO table binds `.cleanup = dio_cleanup`.
- `dio_cleanup()` does not touch `alternate_aux`, `BP_AUX1_ODC`,
  `BP_AUX1_RPOUT` or `BP_AUX2_RPOUT`; those are mode settings.
- `Firmware/core_system/base.c` `bp_reset_board_state()` (used by `#` and a
  mode change) clears `BP_AUX1_ODC` and both RPOUTs; `clear_mode_configuration()`
  sets `alternate_aux = 0` (`AUX`/`A0`).

## 2. `wiring.md` — safety bullet and park row were five-pin → `fixed`

- Table row changed from “rails off, five signal pins parked” to “rails off,
  five signal pins parked (v3) / seven pins parked (v4)”.
- Safety bullet changed from “Park the five signal pins with `w`, or release
  every I/O pin with `#` … A driven `AUX1` / `AUX2` is not released by `w`” to
  “Park the five signal pins with `w` on v3, or all seven (the five signal pins
  plus `AUX1` / `AUX2`) on v4.  `#` releases every I/O pin and resets the
  selector; confirm with `v` before rewiring.”
- Added a v3/v4 row: “`w` park scope | five signal pins | five signal pins
  plus `AUX1` / `AUX2`”.

Code evidence: as in #1 (`dio.c` `dio_cleanup()`; `proc_menu.c`
`switch_psu_off()`).

## 3. `README.md` — front-page park and limits said five → `fixed`

- Five-minute wiring bullet now says `w` “parks the five signal pins on v3 and
  all seven pins — the five plus `AUX1` / `AUX2` — on v4”.
- Pre-rewiring bullet now says “after `w` the five signal pins (v3) or all
  seven pins (v4) are released”; the `#`-if-`AUX1`/`AUX2` advice is gone, and
  `#` is recommended only when the selector / bit-order / display reset is
  also needed.
- Limits bullet now says the v4 extra pins “are not in the value, but `w`
  releases them along with the five signal pins (seven pins on v4; five on
  v3)”, with `#` / a mode change for the selector reset.
- Added an availability row: “`w` park scope | five signal pins | five signal
  pins plus `AUX1` / `AUX2`”.

Code evidence: as in #1.

## 4. `entry-and-setup.md` — `w` table, paragraph and v3/v4 row said five → `fixed`

- Table row changed to “rails off **and five pins (v3) / seven pins (v4)
  parked**”.
- Paragraph changed to “`w` runs the mode's safe park, which releases the five
  signal pins (and, on v4, `AUX1` / `AUX2`) before the rails go off”; the
  “`AUX1` / `AUX2` exception” wording is removed.
- v3/v4 row changed from “`W` / `w` | same | same” to “`W` same; `w` parks
  five pins | `W` same; `w` parks seven pins”.

Code evidence: as in #1.

## 5. `edge-cases.md` — obsolete “extra pin stays driven” case → `fixed`

- Removed the symptom-index row “an extra pin stays driven | `w` after `k` /
  `K` then `A` | [`w` leaves `AUX1` / `AUX2` driven]…”.
- The “Parking and leaving” table row now says “`w` parks the five signal pins
  (v3) or seven pins (v4) and resets the width”.
- The menu-cancel recovery no longer sends the reader to `#` for `AUX1` /
  `AUX2`; it recommends `#` / a mode change only for the selector, bit-order
  or display reset.
- The dedicated “`w` leaves `AUX1` / `AUX2` driven (v4)” section was replaced
  with “`w` releases the v4 extra pins”: `w` sets both `AUX1` and `AUX2` to
  input and clears their latches, regardless of the selected AUX target;
  selecting a target does not release a previously driven one, but `w` (or
  `@` on that target) does.

Code evidence: as in #1 (`dio.c` `dio_cleanup()` unconditional AUX1/AUX2
mask under `BUSPIRATEV4`).

## 6. `aux-pin.md` — v3/v4 table said `w` does not release an AUX1/AUX2 target → `fixed`

Row changed:

- “| `w` releases a driven `AUX1` / `AUX2` target | — | no |” → “| `w`
  releases a driven `AUX1` / `AUX2` target | — | yes (both, regardless of the
  selected target) |”.

The surrounding statement that selecting `AUX2` does not release a previously
driven `AUX1` remains true and unchanged.

Code evidence: as in #1.

## 7. `pin-control.md` — value reference said five → `fixed`

Row changed:

- “`W` / `w` | supply on / park and off | `w` parks the five signal pins …” →
  “`w` parks the five signal pins (v3) or seven pins including `AUX1` /
  `AUX2` (v4); see [park-and-reset.md](park-and-reset.md)”.

Code evidence: as in #1.

---

## Cross-set item (recorded, not edited)

**`docs/dev/verification.md:42`** — the DIO safe-park row is stale:

> “| [dio](../protocol/dio/README.md) | source + v4 history | read-back (`r`
> bits 4–0); the safe-park contract covers the five signal pins, not
> `AUX1`/`AUX2`; v3 unverified |”

Suggested replacement for the owning `dev` pass:

> “| [dio](../protocol/dio/README.md) | source + v4 history | read-back (`r`
> bits 4–0); the safe-park contract covers the five signal pins on v3 and
> seven pins including `AUX1`/`AUX2` on v4; v3 unverified |”

The audit's other cross-set checks were confirmed and need no edit:
`docs/capabilities/auxiliary/wiring.md` already states the seven-pin v4 park;
`docs/capabilities/auxiliary/{aux-pin,edge-cases}.md` carry no DIO-specific
five-pin claim.

## Files changed

- `docs/protocol/dio/park-and-reset.md`
- `docs/protocol/dio/wiring.md`
- `docs/protocol/dio/README.md`
- `docs/protocol/dio/entry-and-setup.md`
- `docs/protocol/dio/edge-cases.md`
- `docs/protocol/dio/aux-pin.md`
- `docs/protocol/dio/pin-control.md`

## Verification performed

- Read every changed passage back; no TAB characters in the DIO set.
- Checked all relative links in the DIO set resolve on disk (no broken
  links).
- Grepped for the stale wording (“`w` leaves … driven”, “not released by
  `w`”, “five signal pins only”, “`AUX1` / `AUX2` exception”): none remain.
- Behaviour claims checked against the final combined tree's
  `Firmware/protocol_dio/dio.c` (`dio_cleanup()`), `core.c` (DIO `.cleanup`),
  `proc_menu.c` (`switch_psu_off()`), `base.c` (board reset), and
  `capability_aux/aux_pin.c` (AUX-target path).
- Not run: no firmware build and no hardware capture; the pages contain no
  bench/date/pass-count claims.
