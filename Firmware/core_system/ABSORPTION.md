# Option A - accepted core content inventory

Owner direction, 2026-11-04: the core starter carries the accepted core
improvements, not merely a combined-only union. Source history was enumerated
at `25d8cfd` with `--no-merges` over the seven original core files, then
content-compared with the validated intermediate union `fac021a`.
Ancestors of the original master base are already present and are not replayed.

New absorption commits preserve the source author identity and author date.
For a tight same-author logical group, the primary source supplies metadata
and the commit body names the other sources. Existing equivalent fixes are
recorded, not duplicated. Grouping/include-only churn is excluded.

| Accepted source / aliases | Core starter disposition |
| --- | --- |
| `a94ebc7` / `7fa7ba1` / `25923cf`, #208 | Already equivalent in `1f0e211`; no new duplicate. USB extern hardening is also already present. |
| `16459f2` / `6ed6cc2` / `c369d1f`, #216 | `4c31943`: Timer1 overflow counter/ISR, timestamp read/print/reset, public API and `t/T`. |
| `2531a14` / `c1dfb0f`, #211 | `0485e9e`: ADC repeat handling, retaining audited unsigned count arithmetic. |
| `f4fc2f8`, core side of #148 | `f206af9`: the shared probe owns its unit and raw ADC field; no duplicate outer unit. BASIC TIME token remains interpreter content. |
| `2a8ca99` | `f5609c4`: reset calls the accepted AUX reset owner, including its required root AUX helper/prototype. No generator-math import. |
| `d7aff15` | `16137a3`: accepted DIO setup binding plus the required flag-only helper/prototype. Central mode reset remains in place too. |
| `f1a7570` / `352b31b`, #117 | `74c3791`: core `O` speed toggle plus its two packed-message prerequisites. `8ec4b70` retains the two source-catalog entries for future regeneration. No wire timing engine imported. |
| `50eede3` / `afc07e4` | Width refusal is already stronger in `4ffe2a7` / `b7f027a`; `64a5590` carries the remaining accepted refusal-before-WRITE/READ-header behavior. |
| `6aaa1ac`, #225 core side | Subsumed by `5d8c09f`: real protocol/board/editor reset and no jump across parser-local initialization. Do not restore the weaker goto path. |
| `08890cd` | `d2b6e0a`: explicitly requested related SUMP reset/exit frontend (`0F/#` exits, `00` stays). No enhanced acquisition/packing/stream backend imported. |
| `e13e801` / `d88ffa4`, #237; `bf4009b`, #241 | `edc0199`: accepted uint16 decimal engine shared by byte/word output, exact voltage multiply/high-word conversion and v3 mask/shift reversal. Retains the audited v4 unsigned reversal and full uint32 output. |
| `b6d132b`, #241 | `80070e4`: public spaced-token wrapper. Noncore caller cleanup, especially SPI, is not imported. |
| `0653843`, #241 | `457598e`: private live protocol descriptors and current/index accessors. The existing BASIC clients receive only the required descriptor-access adaptation. |
| `6533da1`, core side of #217 | `0a8487a`: scripted mode API and default-input guards; interpreter MODE token/expression implementation remains with BASIC. |
| `af1bc3e` / `a6079c2`; `80cffbc` / `caaf1fd` | `d767276`: accepted core delay-helper API/inlined body, optional clock batching field, raw-mode bindings and `^` dispatch. The existing bitbang backend API already exists, so **bitbang.c/h are unchanged**; its engine pass has not started. |
| `665b73d`, decimal declaration merge fallout | No missing/undefined decimal helper remains; the public word declaration comes from base.h and the retained 32-bit helper has a matching definition. No duplicate declaration-only commit. |
| `254bd40`, direct-access merge fallout | All terminal and existing BASIC direct table accesses are already routed through the live API by `457598e`. No duplicate commit. |
| `c3185fa`, main allocation side of #232 | Already equivalent in `d0d8797`: even alignment for the shared sample/terminal allocation. Capture engines remain in SUMP. |
| `9492bbb`, shared core/USB audit portions | Millisecond promotion, matching CDC externs, SOF restoration and endpoint status are already covered by the USB part and the core audit. No programmer engine/debug import. |
| `a1999e9` + `2422e53` | Temporary ISP capture and its removal cancel in core content. Instrumentation is not imported. |
| `ebefa34`, `bedc643`, core portion of `15ae572` | Already in the USB part: bulk writer and immediate partial close. No duplicate. SUMP stream implementation remains its protocol content. |
| `c1597c1` | AUX generator math only; no scoped core file. Not required by the reset helper, so excluded rather than starting an AUX pass. |
| `037a59b`, `84c1503`, `e8e8add`, `a241155`, `4683410`, `7c3966d`, USB grouping | Source/include grouping only for these core paths; not replayed. Current `core_system/` and existing module include paths are retained. |

## Completeness and limits

The accepted timer, delay, formatter, voltage, reversal, flash/reset, live
descriptor, token and scripted-mode function bodies match the intermediate
combined after comments/whitespace are removed. The two default-input guards
use equivalent single-return braces/placement; native default-path tests prove
they do not consume, mutate or wait for input. Minimal AUX/DIO dependencies
and the raw ADC probe also match their accepted bodies. The source comparison
and author map are retained in the local core-system evidence directory.

`test_shared_format.py` retains 131072 formatter outputs and 1088 shared/default
parser cases; `test_protocol_access.py` retains 2200 live pointer checks.
`test_core_system.py` adds both target reversal algorithms, all ADC values,
AUX/DIO reset state and explicit scripted-default checks to the audit battery.

The hardware core battery adds ADC repeats/raw fields, in-place `O` state,
power/pullup preservation and SUMP reset/exit controls. The combined also
exercises its existing BASIC MODE token caller. Full enhanced SUMP captures
remain a combined CDC regression, not a claimed standalone backend feature.

Legacy PWM/servo output is hard-wired to AUX0, which is tied to the paused SPI
CLK fixture; no `g/S` signal is generated here. Its accepted reset is verified
by seeded production-source/MMIO state and the integrated board reset path.
No XM64 reflash/rewiring, SPI implementation change, bitbang engine audit,
other-family rebase or WORKLOG ledger edit is part of this absorption.
