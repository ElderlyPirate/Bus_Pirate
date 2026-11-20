# fixes — SPI Phase F delta repair (RA-3 / C4)

Fresh implementer. Docs worktree `C:\Users\jschl\dev\bpv4-docssweep`, branch
`docs/combined-sweep`, docs head `0ef82aaa`. Firmware verified in this
worktree's own `Firmware/` (combined `9b179cb5`); no other worktree was used.

Scope: only the CS direction/release statements named in
`feedback-spi-phase-f.md`. No unrelated SPI content was reopened.

Decision summary: **5/5 fixed** (0 partial, 0 not addressed, 0 audit-wrong).

Files changed:

- `docs/protocol/spi/binary.md`
- `docs/protocol/spi/edge-cases.md`
- `docs/protocol/spi/README.md`

No commit, no push.

Evidence sources (maintainer-only; not cited in user pages):

- `Firmware/protocol_spi/spi.c` — `spi_binary_set_cs()`,
  `SPI_BASE_COMMAND_CS_LOW` / `SPI_BASE_COMMAND_CS_HIGH`, `spi_read_write_io()`.
- `Firmware/protocol_binary/binary_io.c` — `bp_binary_io_peripherals_set()`
  (the shared `0x4x` group).

---

## 1 → fixed — "cannot recover" workaround removed

**File:** `docs/protocol/spi/binary.md` §Configure peripherals (`0x4x`).

**Removed quote:**

> `0x04`, `0x03` and `0x02` only write the CS latch; they do not change the pin
> direction. … A bare `0x02` writes the latch low but leaves the pin an input,
> so it cannot recover the connection.

**Replacement quote:**

> **CS direction interaction.** `0x4x` bit 0 still sets CS directly: set
> releases it (an input/Hi-Z in open drain, driven high in Normal); clear drives
> it low/asserted. `0x02`, `0x03` and a non-empty `0x04` set the direction
> themselves: `0x02` drives CS low and makes it an output, so it recovers a CS
> left as an input by `0x4x`; `0x03` releases CS (input/Hi-Z in open drain,
> driven high in Normal); `0x04` asserts through the same helper at the start
> and releases at the end. `0x6x`/`0x8x` re-run the module setup, which also
> leaves CS an output at its released level. A second `0x4x` is not needed for
> recovery.

**Evidence:** `spi.c` `spi_binary_set_cs()` — `high && high_impedance`:
`IODIR |= CS` + `IOLAT |= CS`; otherwise `IOLAT` low/high then
`IODIR &= ~CS`. So the low call always drives CS low as an output and
`0x02` recovers the `0x4x` input state. `spi_read_write_io()` calls the same
helper for `0x04` (and only when the transfer is non-empty).

## 2 → fixed — command rows and CS paragraph

**File:** `docs/protocol/spi/binary.md` §Commands.

**Before:**

> | `0x02` | action | CS latch low (asserts only while CS is an output) | `0x01` |
> | `0x03` | action | CS high (release) | `0x01` |
>
> … `0x02` writes the latch low; it drives CS low when the pin is already an
> output … It does not change the pin direction. …

**After:**

> | `0x02` | action | CS low (assert; enables the output) | `0x01` |
> | `0x03` | action | CS high (release: input/Hi-Z in open drain, driven high in Normal) | `0x01` |
>
> … `0x02` writes the CS latch low and makes CS an output, so it asserts.
> `0x03` writes the latch high and releases CS: an input (Hi-Z) in open drain,
> driven high in Normal. Both set the direction; …

**Evidence:** `spi.c` `SPI_BASE_COMMAND_CS_LOW` → `spi_binary_set_cs(LOW)`
(`IOLAT &= ~CS` and `IODIR &= ~CS`); `SPI_BASE_COMMAND_CS_HIGH` →
`spi_binary_set_cs(HIGH)` (open drain: input; Normal: latch high + output).

## 3 → fixed — auto-CS `0x04` after a `0x4x` release

**Files and quotes.**

`binary.md` §Write-then-read:

> `0x04` uses the same CS helper: it drives CS low and makes it an output at
> the start, then releases it (input/Hi-Z in open drain, driven high in Normal)
> at the end.

`binary.md` edge table:

> | `0x4x` released CS as input | CS high/Hi-Z, not asserted | `0x02` or a non-empty `0x04` asserts; `0x6x`/`0x8x` restore the released output |

`edge-cases.md` binary table:

> | `0x04` after a `0x4x` released CS | not a failure: `0x04` drives CS low and makes it an output for the transfer, then releases it | no recovery needed |

**Evidence:** `spi.c` `spi_read_write_io()` — after the zero-length early
return, `if (engage_cs) spi_binary_set_cs(LOW)` before the payload/read and
`spi_binary_set_cs(HIGH)` after it. The low call sets `IODIR &= ~CS`, so the
transfer asserts regardless of a preceding `0x4x` release.

## 4 → fixed — remaining "while CS is an output" preconditions

**Files and quotes.**

`binary.md` §Bulk transfer:

> … use `0x02` (it asserts and enables the output; see §Configure peripherals
> for the `0x4x` bit-0 level).

`binary.md` §Extended AVR command:

> Assert CS with `0x02` before the read (it drives low as an output) and
> release it with `0x03` afterwards (input/Hi-Z in open drain, driven high in
> Normal).

`binary.md` and `edge-cases.md` `0x06`-without-CS rows:

> first assert CS with `0x02` (it enables the output)

`edge-cases.md` `0x1x`-without-CS row:

> if the target needs CS, assert it with `0x02` (it enables the output)

`README.md` binary summary:

> `0x02` asserts CS and enables the output; `0x03` releases it (input/Hi-Z in
> open drain, driven high in Normal).

**Evidence:** same `spi_binary_set_cs()` path as finding 2; the old phrase
described a latch-only precondition that no longer exists.

## 5 → fixed — `0x03` release row

**Files and quotes.**

`binary.md` edge table:

> | `0x4x` bit 0 set (open drain) | CS released as an input | expected; `0x02` asserts, `0x03` releases |

`edge-cases.md` binary table:

> | `0x03` after a `0x4x` released CS | CS released (input in open drain, driven high in Normal) | expected; use `0x02` to assert |

**Evidence:** `spi_binary_set_cs(HIGH)` in open drain sets `IODIR |= CS`
(input/Hi-Z); in Normal output it sets `IOLAT |= CS` and `IODIR &= ~CS`
(driven high). That is the normal release, not a latch failure.

---

## Residual scan for pre-fix wording

Searched the whole SPI set after the edits for the pre-fix model:

- `while CS is an output`, `output only` (as a precondition), `already an
  output`, `does not change the pin direction`, `writes the latch only`,
  `cannot recover`, `0x4x trap`, `0x4x bit 0 clear` as recovery.
- Remaining hits are the approved post-fix wording ("writes the CS latch low
  and makes CS an output"), the unrelated "open-drain output only" pull-up
  cell in `wiring.md`, and the shared `0x4x` level table.
- `binary.md` reference "binary framing and CS-direction failures" in Related
  docs still describes the page correctly.

No pre-fix latch-only claim remains.

## Presentation

- No TAB characters in the set.
- Changed table rows stay pipe-aligned; no new lists.
- Changed prose is within the set's ~80-column style.
- No dates, bench prose, pass counts or tested/verified wording added.
- No new code references or symbol names in user pages; links and anchors are
  unchanged and still resolve (the `Configure peripherals (0x4x)` heading
  exists).

## For the re-auditor

- Confirm the `0x04` "non-empty" qualifier is preserved (zero/zero returns
  before any CS change).
- Confirm `0x02` is presented as the recovery path and no page sends the
  reader to a second `0x4x`.
- Re-check the two edited edge tables and the README binary bullet against
  `spi_binary_set_cs()` in the final tree.
