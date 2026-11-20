# Wire — Phase F repair record (2-Wire ATR / RA-2 / C5)

Fresh implementer, Phase F.  Read `PHASE-F-DOCS.md` (wire row + C5) and
`docs-sweep/feedback-wire-phase-f.md`, then verified every changed claim
against the final firmware in this worktree:
`Firmware/protocol_wire/raw2wire.c` (combined `9b179cb5`).

**Firmware truth used:** `trigger_atr()` drives CS **low**, holds ~255 µs,
advances one clock tick, then drives CS **high**; `raw2wire_run_macro()` then
falls through to `read_atr_header()`, so the four values are sampled with the
reset released and CS high.  `(2)` calls only `read_atr_header()` and does not
touch CS.  The printed banner string `RESET HIGH, CLOCK TICK, RESET LOW` is a
fixed firmware caption and no longer describes the edge order.

## Result

| Finding | Decision | File |
|---|---|---|
| 1 — ATR edge order / final CS state | **fixed** | `docs/protocol/wire/2wire/macros.md` |
| 2 — obsolete "not a valid ATR" warning | **fixed** | `docs/protocol/wire/2wire/macros.md` |
| 3 — same stale claim in three places | **fixed** | `docs/protocol/wire/2wire/smartcard.md` |
| 4 — obsolete "Releasing the card reset" procedure | **fixed** | `docs/protocol/wire/2wire/smartcard.md` |
| 5 — stale ATR failure / CS rows | **fixed** | `docs/protocol/wire/2wire/edge-cases.md` |

**5 fixed / 0 rejected / 0 partial.**  The cross-set sweep found no other
2-Wire ATR/CS claim that needed an edit (see §Cross-set sweep).

## Detail

### 1 → fixed — ATR edge order and final CS state

**File:** `docs/protocol/wire/2wire/macros.md` (§Macro `(1)` — ISO 7816-3 ATR).

**Removed:**

> The banner's three words name the middle of the sequence; the macro's full
> edge order on CS is:
>
> 1. drive CS low and hold about 255 µs,
> 2. drive CS high — the banner's `RESET HIGH`,
> 3. advance one clock tick on SCL (CLK) — the banner's `CLOCK TICK`,
> 4. drive CS low — the banner's `RESET LOW`,
>
> then read the reply.  The read happens after the final CS low, so the card's
> reset is asserted while the four values are sampled.  The macro **leaves CS
> low** after it finishes.  See [smartcard.md](smartcard.md) for the contact
> mapping and the release step.

**Now:**

> The reset is on **CS**, not on the bus lines.  The macro's edge order on CS
> is: drive CS low (assert reset) and hold about 255 µs; advance one clock tick
> on SCL (CLK); drive CS high (release reset).  It then reads the four values
> with CS high and **leaves CS high** after it finishes.  The printed banner
> `RESET HIGH, CLOCK TICK, RESET LOW` is a fixed firmware caption and does not
> describe the current edge order; use the three steps above.

The Limits bullet was changed from "`(1)` leaves CS low; release it …" to:

> `(1)` leaves CS high (reset released); no release step is needed.

**Evidence:** `raw2wire.c` `trigger_atr()` (CS low → delay → clock tick → CS
high) and `raw2wire_run_macro()` case `RAW2WIRE_MACRO_ISO_7816_3_ATR`
(trigger, then intentional pass-through to `read_atr_header()`).

### 2 → fixed — "not a valid ATR" warning removed

**File:** `docs/protocol/wire/2wire/macros.md` (after the edge-order paragraph
and in §Limits).

**Removed warning:**

> **The four values are not a valid ATR.**  `(1)` samples them while the
> card's reset is asserted, so a card cannot answer; this is a known firmware
> limitation.  Do not use the decoded protocol, read-type or data-unit fields
> as card data.

**Removed Limit:** "The `(1)` output is not a valid ATR (see the warning
above)."

**Replacement note:**

> `(1)` releases reset before sampling, so the four values are the card's reply
> when a card answers; with no card, no power, or no pull-up they are idle or
> floating data.  The decode still prints `Protocol: unknown` for a standard
> `TS` byte (`0x3B`/`0x3F`) because the firmware reads high nibble `3` — the
> existing standard-`TS` caveat already covers it.

No "verified/tested" prose was added; the note stays source-derived.

**Evidence:** `raw2wire.c` `trigger_atr()` releases CS (reset) before
`read_atr_header()` runs; `read_atr_header()` samples four values.  Standard
`TS` `0x3B`/`0x3F` still hits the `default: unknown` branch of the protocol
nibble switch.

### 3 → fixed — same stale claim in three places

**File:** `docs/protocol/wire/2wire/smartcard.md`.

Contact table C2 note, before → after:

> `| C2 | RST | CS | driven by the ATR macro; left low after it |`
>
> `| C2 | RST | CS | driven low then released high by the ATR macro; left high after it |`

ATR flow step 5, before → after:

> "The macro pulses the card reset (reset on CS), reads four values with reset
> still asserted, then decodes them."
>
> "The macro pulses the card reset (reset on CS) low, releases it high, then
> reads four values and decodes them."

The warning block "**The four values from `(1)` are not a valid ATR.** …" was
deleted.  The orphaned follow-up "The parse applies the ATR field decoding to
those four values." was also removed; the flow above now carries the fact.

The Limit was replaced with:

> `(1)` reads the ATR after releasing reset; the four values are meaningful
> when the card answers (otherwise they are idle/floating data).  The parse
> still reads exactly four values at the default 8-bit width.

The now-duplicate following Limit ("The parse reads exactly four values, and
assumes the default 8-bit width.") was removed; the replacement covers it.

**Evidence:** same `trigger_atr()` / `raw2wire_run_macro()` / `read_atr_header()`
path.  `(2)` (`RAW2WIRE_MACRO_ISO_7816_3_ATR_PARSE`) calls only
`read_atr_header()` and does not touch CS.

### 4 → fixed — obsolete "Releasing the card reset" procedure

**File:** `docs/protocol/wire/2wire/smartcard.md`.

**Removed section:** `## Releasing the card reset` — the `C` then `A` release
procedure premised on "`(1)` ends with CS **low**".

**Replacement section:**

> ## Card reset after the macro
>
> `(1)` releases the card reset (CS high) before it samples and leaves CS high;
> no release step is needed.  `(2)` does not touch CS.  If you assert CS low
> yourself (`C` then `a`, or binary `0x04`), release it with `C` then `A` (or
> `0x05`), or leave and re-enter the mode, which makes CS an input.

This keeps the genuine manual-CS advice but removes the false premise.  The
parse-only distinction is unchanged.

**Evidence:** `trigger_atr()` ends with `bitbang_set_cs(HIGH)`;
`read_atr_header()` never drives CS.  `(2)` does not call `trigger_atr()`.

### 5 → fixed — stale ATR failure and CS rows

**File:** `docs/protocol/wire/2wire/edge-cases.md` (§ATR and smart card).

**Deleted row** (old limitation; the no-answer row below it remains the right
failure case):

> `| (1)` expected to return usable card data | four values print and decode,
> but they are sampled with reset asserted | … not a valid ATR … `(2)` parses
> only; it does not fix the reset order |`

**Rewritten row:**

> `| (1)` finished, then the card expected to run | no release step is needed |
> `(1)` leaves CS high (reset released).  `(2)` leaves CS unchanged; if you
> asserted CS low yourself (`C` then `a`), release it with `C` then `A`, or
> leave and re-enter the mode (CS becomes an input). |`

**Evidence:** `raw2wire.c` `trigger_atr()` (`bitbang_set_cs(HIGH)` last) and
`raw2wire_run_macro()`.

## Cross-set sweep (finding 5 continuation)

Searched the whole `docs/protocol/wire/` set for
`not a valid ATR|leaves CS low|reset asserted|still asserted|sampled with
reset|samples them while|reset still asserted|left low after`.  After the
edits: **no matches**.

Pages checked that mention ATR/CS but need no change (confirmed accurate):

- `2wire/README.md` — "reset the card, parse the reply".
- `2wire/binary.md` — `0x04`/`0x05` CS low/high and `0x8x`/`0x6x` forcing CS
  high; no ATR final-state claim.
- `2wire/wiring.md` — "CS as the card's reset line".
- `2wire/bus-operations.md`, `2wire/entry-and-setup.md` — no ATR claim.
- `wire/README.md`, `wire/binary.md`, `3wire/bus-operations.md`,
  `docs/SPEC.md`, `docs/dev/verification.md` — no stale ATR/CS claim (matches
  the audit's cross-set list).

## Parse-only macro

Unchanged and still distinct:

> `(2)` runs only the parse step: it reads four values and decodes them,
> without driving the reset.

`(2)` still reads four values at the current width, prints the same reply
header, and decodes the same fields; it leaves CS untouched.

## Presentation checks

- 0 TAB characters in `docs/protocol/wire/`.
- Changed prose lines are within ~80 columns; table rows are excluded.
- All markdown links resolve; checked anchors used by the 2-Wire set
  (`entry-and-setup.md#clock-speeds`, `../binary.md#worked-exchange`,
  `../README.md#power-pull-ups-and-vpu`, generic `usage.md#wrappers-and-bit-ops-2wire`).
- No code references, dates, bench narratives, pass counts, or
  tested/verified claims in the changed passages.
