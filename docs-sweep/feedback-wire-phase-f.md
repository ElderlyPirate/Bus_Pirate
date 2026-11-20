# Wire — Phase F delta audit (2-Wire ATR / RA-2 / C5)

Read for this audit: `PHASE-F-DOCS.md` (wire row + C5), `WORKLOG-DOCUMENTATION.md`
§2F, the standing auditor prompt, the current `docs/protocol/wire/` set, and the
final firmware in the docs worktree — `Firmware/protocol_wire/raw2wire.c` at
docs head `728b3ffc` / combined `9b179cb5`.  No files under `docs/` were edited.

**Firmware truth used:** `trigger_atr()` drives CS **low**, holds ~255 µs,
advances one clock tick, then drives CS **high**; `raw2wire_run_macro()` then
calls `read_atr_header()`, so the four values are sampled with reset released
and CS high.  `(2)` calls only `read_atr_header()` and does not touch CS.

## Verdict

**No** — the 2-Wire ATR pages still document the pre-#226 order (reset asserted
while sampling, CS left low) and tell the reader the now-valid ATR is unusable.

## Findings

### 1. [major] `2wire/macros.md:53-64` — the ATR edge order and final CS state are stale

> The reset is on **CS**, not on the bus lines.  The banner's three words name
> the middle of the sequence; the macro's full edge order on CS is:
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

**Why it is wrong.** The final firmware does: CS low (assert reset) → hold
~255 µs → one clock tick → CS high (release reset) → `read_atr_header()`.  CS is
high while the four values are sampled and stays high after `(1)`.  The quoted
banner is a fixed firmware caption (`RESET HIGH, CLOCK TICK, RESET LOW`) and is
now in the old order; the docs must not present it as the sequence.  The
"release step" pointer is also obsolete.

**Exact fix.** Replace the four-step list and the two following sentences with:

> The reset is on **CS**, not on the bus lines.  The macro's edge order on CS
> is: drive CS low (assert reset) and hold about 255 µs; advance one clock tick
> on SCL (CLK); drive CS high (release reset).  It then reads the four values
> with CS high and **leaves CS high** after it finishes.  The printed banner
> `RESET HIGH, CLOCK TICK, RESET LOW` is a fixed firmware caption and does not
> describe the current edge order; use the three steps above.

Also change the Limits bullet at `:159-160`:

> `(1)` leaves CS low; release it (or leave the mode) before using the card
> for a different purpose.

to:

> `(1)` leaves CS high (reset released); no release step is needed.

### 2. [major] `2wire/macros.md:66-69,158` — the "not a valid ATR" warning is obsolete

> **The four values are not a valid ATR.**  `(1)` samples them while the card's
> reset is asserted, so a card cannot answer; this is a known firmware
> limitation.  Do not use the decoded protocol, read-type or data-unit fields as
> card data.

and `:158`:

> - The `(1)` output is not a valid ATR (see the warning above).

**Why it is wrong.** Reset is released before sampling, so a card can answer;
the four values are the card's reply (or idle/floating data if no card answers).
Telling the reader not to use the decoded fields makes the fixed headline
feature look broken.

**Exact fix.** Delete the warning and the Limit bullet.  Replace with a short
factual note:

> `(1)` releases reset before sampling, so the four values are the card's reply
> when a card answers; with no card, no power, or no pull-up they are idle or
> floating data.  The decode still prints `Protocol: unknown` for a standard
> `TS` byte (`0x3B`/`0x3F`) because the firmware reads high nibble `3` — the
> existing standard-`TS` caveat already covers it.

Do not replace this with "verified/tested" prose; the smart-card path is
source-derived and has no qualified fixture (see Unknowns).

### 3. [major] `2wire/smartcard.md:28,57-65,98-99` — the same "reset asserted / not usable" claim in three places

Quotes:

- `:28` — ``| C2 | RST | CS | driven by the ATR macro; left low after it |``
- `:57-58` — "The macro pulses the card reset (reset on CS), reads four values
  with reset still asserted, then decodes them."
- `:63-65` — "**The four values from `(1)` are not a valid ATR.**  The macro
  samples them while reset is asserted, so a card cannot answer; this is a
  known firmware limitation.  Do not use the decoded fields as card data."
- `:98-99` — "`(1)` is not a usable ATR source: it samples the four values with
  reset asserted.  Do not use the decoded fields as card data."

**Why it is wrong.** Every one describes the pre-#226 order.  The reset is
released high before sampling; CS is high afterward; the four values can be the
card's ATR.

**Exact fix.**  `:28` → "driven low then released high by the ATR macro; left
high after it."  `:57-58` → "The macro pulses the card reset (reset on CS)
low, releases it high, then reads four values and decodes them."  Delete the
`:63-65` warning and replace the `:98-99` Limit with:

> `(1)` reads the ATR after releasing reset; the four values are meaningful
> when the card answers (otherwise they are idle/floating data).  The parse
> still reads exactly four values at the default 8-bit width.

Keep the parse-only distinction already at `:84` ("`(2)` does not touch CS") —
that is correct and should stay.

### 4. [major] `2wire/smartcard.md:69-84` — "Releasing the card reset" is an obsolete procedure

> ## Releasing the card reset
>
> `(1)` ends with CS **low**, so the card's reset is still asserted.  To release
> it without leaving the mode:
>
> 1. `C` selects the CS pin as the AUX target; it prints
>    `a/A/@ controls CS pin`.
> 2. `A` drives that pin high; it prints `AUX HIGH` ...
>
> Alternatively, leave and re-enter the mode: the mode change makes CS an input,
> so it floats unless a pull-up is fitted.  `C` then `A` is the deterministic
> release.  `(2)` does not touch CS; use it when you drive the reset yourself.

**Why it is wrong.** `(1)` leaves CS **high**; the card reset is already
released.  The section sends the reader through an unnecessary `C`/`A` step and
reinforces the false "still in reset" premise.

**Exact fix.** Replace the section with a conditional note:

> ## Card reset after the macro
>
> `(1)` releases the card reset (CS high) before it samples and leaves CS high;
> no release step is needed.  `(2)` does not touch CS.  If you assert CS low
> yourself (`C` then `a`, or binary `0x04`), release it with `C` then `A` (or
> `0x05`), or leave and re-enter the mode, which makes CS an input.

This keeps the genuine manual-CS advice while removing the stale premise.

### 5. [major] `2wire/edge-cases.md:32,34` — the ATR failure and CS rows are stale

> `(1)` expected to return usable card data | four values print and decode, but
> they are sampled with reset asserted | a card cannot answer while reset is
> asserted, so the four values are **not a valid ATR**; this is a known
> firmware limitation.  Do not use the decoded fields as card data.  `(2)`
> parses only; it does not fix the reset order

and:

> `(1)` finished, then the card expected to run | CS is left low, so the card's
> reset is still asserted | select CS (`C`) and drive it high (`A`); leaving
> the mode makes CS an input, which floats unless a pull-up is fitted

**Why it is wrong.** The first row is the old limitation; the no-answer case is
already covered by the next row (`:33`).  The second row's premise is false:
`(1)` leaves CS high and needs no release.  `(2)` does not drive reset, but it
does not "fix the reset order" because there is no broken order left.

**Exact fix.** Delete the `:32` row (the `:33` no-answer row remains the right
failure case).  Rewrite `:34` as:

> `(1)` finished, then the card expected to run | no release step is needed |
> `(1)` leaves CS high (reset released).  `(2)` leaves CS unchanged; if you
> asserted CS low yourself (`C` then `a`), release it with `C` then `A`, or
> leave and re-enter the mode (CS becomes an input).

## Cross-set list

Checked for the same stale wording; **none carries a stale ATR/CS claim**, so no
cross-set edits are required:

- `docs/protocol/wire/README.md:17` — "smart-card ATR" in the mode selector; no
  reset/CS claim.
- `docs/SPEC.md:66` — "ISO 7816-3 ATR macros"; no reset/CS claim.
- `docs/dev/verification.md:36` — "smart-card ATR trigger/parse"; no reset/CS
  claim, and it does not repeat the old limitation.
- `docs/protocol/wire/3wire/bus-operations.md:200` — "as in 2-Wire's ISO
  7816-3 ATR"; generic reference only.
- `docs/protocol/wire/binary.md:239` — "no binary command for the 2-Wire ATR
  macros"; correct.
- `docs/protocol/binary/` — the raw-wire `0x04`/`0x05` CS text is generic and
  does not describe the ATR's final CS state.

Same-set pages checked that mention ATR/CS and need **no change**:
`2wire/README.md:42` ("reset the card, parse the reply" is accurate),
`2wire/binary.md:29,39,45` (CS low/high and "0x8x/0x6x set CS high" are
consistent), `2wire/wiring.md:16,90` ("CS as the card's reset line" is
accurate), and `2wire/bus-operations.md` / `2wire/entry-and-setup.md` (no ATR
claim).

## Presentation on the changed passages

- No TAB characters in the 2-Wire set.
- Changed prose lines are within ~80 columns; the long lines are table rows
  (`edge-cases.md`, `smartcard.md` contact table), which are excluded from the
  prose rule.
- Tables are aligned and readable; the macros edge order is a numbered list,
  appropriate for a sequence.
- No dates, bench narratives, pass counts, or tested/verified claims in the
  changed passages.  The "known firmware limitation" phrase is the stale content
  being removed, not a presentation defect.

## Unknowns

- No qualified smart-card/ATR fixture is recorded (Phase F is build/native only).
  The corrected order and CS state are from the final firmware source; the actual
  card response and ATR timing remain unverified.  Do not turn the corrected
  wording into a "verified/tested" claim.
- The printed banner `RESET HIGH, CLOCK TICK, RESET LOW` is a firmware string
  that now reads in the old order.  This audit fixes the docs to explain the
  actual order; a firmware caption cleanup would be a code-owner decision, not a
  docs edit.
