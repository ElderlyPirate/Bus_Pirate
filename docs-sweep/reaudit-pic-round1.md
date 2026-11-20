# PIC docs — round 1 re-audit

**Scope:** current `docs/programmers/pic/**` against the 17 findings in
`docs-sweep/feedback-pic-round1.md`.  This is not a fresh review; it only
checks the recorded items, the five named major areas, and presentation.

**Verdict: 16 of 17 items filled in full.** The one partial is finding 8: the
PIC page is fixed, but the recorded `docs/generic/values.md` cross-set half is
still pending (the main session owns that edit). No new major defect.

## Per-item table

| # | Severity | Status | Current evidence | Remainder |
|---|---|---|---|---|
| 1 | blocker | **Filled in full** | `config-words.md:30-31`: "The table below is the six-bit map used by the common PIC16F midrange parts; other six-bit families can differ."  The table at `34-44` now runs `0x00`, `0x02`, `0x03`, `0x04`, `0x05`, `0x06`, `0x08`, `0x09`, `0x0B`; the wrong `0x0A`/`0x0C`/`0x0E` and the `0x18` claim are gone. | none |
| 2 | major | **Filled in full** | `README.md:16-19`: "apply the programming-mode condition (VPP for HVPP, MCLR at VDD for LVPP), then run `(1)`."  Quick-start step 4 repeats it (`README.md:43`); `sessions.md:10-12` and `macros.md:40-42` repeat the precondition. | none |
| 3 | major | **Filled in full** | `wiring.md:94-100`: bring-up is `e2`/`e3` → `W` → `P`, plus "`e` may be issued before or after `W` ... Keep `P` last."  `README.md:42`, `sessions.md:23-28` and `edge-cases.md:27` use the same order. | none |
| 4 | major | **Filled in full** | `wiring.md:140-163`: control inputs are split from programming outputs; `CS` is "the adapter's MCLR control input"; the output table says the VPP output is "switched high voltage; **never** to a Bus Pirate pin"; then "The adapter's VPP output goes only to the target's MCLR. **No Bus Pirate pin is VPP-tolerant**". | none |
| 5 | major | **Filled in full** | `picprog.md:17-31`: "The route" gives `BBIO1` → `0x05` (`RAW1`) → `0x4x` → `0xA0` → `0xAx`, chiefly `0xA7`; "does not use the terminal PIC mode or `PIC1`"; "a build without terminal `PIC` still answers this path"; source link at `127`. | none |
| 6 | major | **Filled in full** | `picprog.md:55-60`: "The tool ships a small part table.  That table is the authority for which parts it supports and which need a high-voltage adapter; this set does not re-verify it."  The old result table and parser diagnosis are gone. | none |
| 7 | major | **Filled in full** | `README.md:16-20` starts with the start sequence; `24-34` is the file map; `36-45` is the six-row quick start; `68-85` is the naming table after the start.  The file is 112 lines, under the ~120 target. | none |
| 8 | major | **Partial** | PIC side: `bus-operations.md:38-45` now says "The mode shifts **LSB-first** on the wire.  The generic bit-order setting is a separate layer ... `L` reverses ... the wire order becomes MSB-first ... the label describes the generic transformation, not the wire order in this mode."  Generic side is unchanged: `generic/values.md:178-186` still says "`L` ... least-significant bit first" with no PIC note. | Apply the recorded cross-set sentence to `docs/generic/values.md`; the PIC side needs no further edit. |
| 9 | minor | **Filled in full** | `wiring.md:42-70`: inline v4 2x6 header table (pins 1–12) and v3 2x5 cable order (`GND`, `3.3V`, `5.0V`, `ADC`, `VPU`, `AUX`, `CLK`, `MOSI`, `CS`, `MISO`); the AVRISP-only pointer is gone. | none |
| 10 | minor | **Filled in full** | `README.md:99`: "the terminal can drive CS after selecting it with `C` (the command family is called AUX; the pin is CS)". | none |
| 11 | minor | **Filled in full** | `wiring.md:178-190`: v4 and v3 rows are separate aligned code blocks; "PGC is always the CLK column and PGD the MOSI column". | none |
| 12 | minor | **Filled in full** | `binary.md:114-121`: `0xA4` and `0xA5` are split by engine (416/424), each `0xA5` row says "**no result byte**". | none |
| 13 | minor | **Filled in full** | `README.md:3-4` keeps the one status line; `wiring.md:203-204` keeps one v3 procedural note.  The per-page v3 closers in entry/bus/macros/config/binary/picprog/edge are gone.  Remaining "not verified" hits are topic-specific (binary paths, raw-wire script, VPP levels, lock-out). | none |
| 14 | minor | **Filled in full** | `README.md:62` says "`e1` external/none"; `README.md:84-85` maps `picprog`/PiratePICprog and the banner `(Bus) Pirate Pic Programmer`; `wiring.md:200` says "v3a and later".  `picprog.md:51` uses the two-m banner. | none |
| 15 | minor | **Filled in full** | `README.md:38-45` is a six-step quick-start table; `README.md:93-100` is a six-row limits table plus one shared-grammar sentence. | none |
| 16 | minor | **Filled in full** | `picprog.md:87`: "The tool prints its own progress; an example:".  `config-words.md:53`: "The full terminal sequence is in sessions.md."  The duplicate transcript is gone. | none |
| 17 | minor | **Filled in full** | `bus-operations.md:129-130`: "A 14-bit payload is 16 clocks including the leading and trailing framing zeros, about 1.6 ms, plus the command delay." | none |

## Finding 8 detail — the one partial

PIC page, resolved:

```text
docs/programmers/pic/bus-operations.md:38-45
- The mode shifts **LSB-first** on the wire.  The generic bit-order setting
  is a separate layer: the default `l` (MSB-first, no reversal) leaves the
  value unchanged, so the mode's native LSB-first order goes out.  `L`
  reverses the value within the display width before the mode sees it, so the
  wire order becomes MSB-first.  Leave the generic setting at `l` (no
  reversal) unless the host sequence expects the reversal; the label
  describes the generic transformation, not the wire order in this mode.
```

Generic page, still unchanged:

```text
docs/generic/values.md:178-186
| `L` | `LSB set: LEAST sig bit first` | least-significant bit first |
...
With LSB-first the byte on the bus is the bit reverse of the number shown for a
write, and a read is reversed before it is shown.
```

The PIC half now reads naturally and matches the recorded evidence
(`proc_menu.c` applies the generic reversal before `protocol->send()`;
`pic.c` `write_bits()` shifts LSB-first).  The generic half is the recorded
cross-set edit; it is not a PIC-page regression.

## Recorded handoffs

- **CO-1 (code-owner candidate).** The partial-apply trap is documented in
  `entry-and-setup.md:90-93` ("`m 9 2` runs 4b/16b at 0 ms, then the terminal
  reports a syntax error") and in `edge-cases.md:52`/`60-71` with the full
  capture.  No doc change was made; the firmware behaviour remains open.
- **Cross-set 1 — generic `L`.** Still pending in `docs/generic/values.md`;
  the PIC side is complete.  This is the main session's required edit.
- **Cross-set 2 — optional root `README.md` `picprog` link.** Closed: the
  root README line 53 links `picprog` to
  `docs/programmers/pic/picprog.md`.

## Presentation check

| Check | Result |
|---|---|
| TAB characters | **None** in the 10 PIC files. |
| Aligned captures | `binary.md:78-97` comments all start at column 24; `wiring.md:178-186` v4/v3 rows align; the entry/sessions/macros prompt captures are single-column transcripts. |
| Menus / option lists as tables | README quick start and limits are tables; wiring has bring-up/header/adapter tables; binary has command and engine tables; `picprog.md:69-75` is an option table; edge cases are tables.  The exact `Commandmode?`/`Delay?` transcript in `entry-and-setup.md:31-41` is a capture, and the same options are tabulated at `21-25`. |
| ~80-column wraps | No non-table/non-code prose line is over 85 characters.  Three lines are 81–82 (`bus-operations.md:7`, `wiring.md:79`, `wiring.md:206`); they are within the ~80 target. |
| Bench prose | None.  No dates, pass counts, bench/rig/battery narratives, capture provenance, evidence sections or internal log names. |
| Tested/verified-result prose | None.  The remaining hits are explicit caveats: `binary.md:10` ("Neither path is verified on hardware"), `wiring.md:128` (VPP levels unverified), `config-words.md:94` (lock-out recovery unverified), `edge-cases.md:105-110` (binary/raw-wire paths unverified).  The `picprog.md:85-104` block is introduced as "an example", not a result claim. |

## New problems

1. **[minor] `wiring.md:150`/`160` — VTARGET is described both as an input from VPU and as an output to VDD.**
   The input table says "VPU | VTARGET | target-supply / pull-up reference";
   the output table says "VTARGET | VDD | target supply / pull-up reference".
   `wiring.md:28` still says VPU is "pull-up reference; not a target wire".
   This is a new ambiguity from the control/output split: is VTARGET a
   sense/reference, or can it power the target VDD?  If it can power VDD, the
   "do not back-feed a self-powered target" warning needs to reach this table.

2. **[minor] `README.md:16-19` — "select the VPU source" lacks the v3 caveat.**
   The one-line start says "enter `m9` (v4) or `m8` (v3), select the VPU source";
   v3 has no `e` command and must feed VPU externally.  The quick-start table
   at line 42 does say "v3 feeds VPU externally", but the start sentence can
   send a v3 reader looking for a missing command.

**Verdict: 16 of 17 items filled in full.** The only open finding is the
cross-set generic `L` sentence (finding 8); the two new items above are minor.
