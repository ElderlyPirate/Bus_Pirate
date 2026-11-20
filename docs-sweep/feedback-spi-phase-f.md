# feedback — SPI Phase F delta audit (RA-3 / C4)

**Scope:** only the CS direction/release statements touched by the #204 RA-3
repair (`0x02` asserts and sets output; `0x03` releases input/drives high;
auto-CS uses the same helper). Verified against the final tree in this
worktree: `Firmware/protocol_spi/spi.c` (`spi_binary_set_cs()`,
`SPI_BASE_COMMAND_CS_LOW/HIGH`, `spi_read_write_io()`) and
`Firmware/protocol_binary/binary_io.c` (`bp_binary_io_peripherals_set()`).
Docs head `0ef82aaa`. Not reopening unrelated SPI content.

## Verdict

**No.** The RA-3 behaviour is not reflected. The main binary reference still
teaches the old "latch-only / cannot recover" workaround, and the same
precondition is repeated in the command table, the auto-CS rows, the edge
cases and the README.

## Findings

### 1. [severity: major] The explicit "cannot recover" workaround is still in the binary reference

**File:** `docs/protocol/spi/binary.md:190-197`.

> **CS direction interaction.** Setting bit 0 releases CS as an **input** in
> open-drain output. `0x04`, `0x03` and `0x02` only write the CS latch; they do
> not change the pin direction. After a `0x4x` that released CS this way, send
> another `0x4x` with bit 0 clear and the other bits at the states you want (for
> example `0x44` for pull-ups on, or `0x4C` for supply and pull-ups on), or send
> `0x6x`/`0x8x` to re-run the module setup. Both restore CS as an output. A bare
> `0x02` writes the latch low but leaves the pin an input, so it cannot recover
> the connection.

**Why it is wrong.** In the final tree `spi_binary_set_cs(LOW)` writes
`IOLAT &= ~CS` **and** `IODIR &= ~CS`, so `0x02` drives CS low as an output
regardless of the previous direction. `spi_binary_set_cs(HIGH)` is an input
only when `high_impedance` is on; with Normal output it writes a high output.
`spi_read_write_io()` calls the same helper for `0x04`, so a non-empty
`0x04` also asserts CS and recovers an input CS. The "only write the CS latch"
and "cannot recover" sentences are exactly the pre-RA-3 behaviour.

**Exact fix.** Replace the paragraph with:

> **CS direction interaction.** `0x4x` bit 0 still sets CS directly: set
> releases it (an input/Hi-Z in open drain, driven high in Normal); clear drives
> it low/asserted. `0x02`, `0x03` and a non-empty `0x04` set the direction
> themselves: `0x02` drives CS low and makes it an output, so it recovers a CS
> left as an input by `0x4x`; `0x03` releases CS (input/Hi-Z in open drain,
> driven high in Normal); `0x04` asserts through the same helper at the start
> and releases at the end. `0x6x`/`0x8x` re-run the module setup, which also
> leaves CS an output at its released level. A second `0x4x` is not needed for
> recovery.

### 2. [severity: major] The command table and the CS-command paragraph still say `0x02` does not set the direction

**File:** `docs/protocol/spi/binary.md:90-91` and `105-110`.

> | `0x02` | action | CS latch low (asserts only while CS is an output) | `0x01` |
> | `0x03` | action | CS high (release) | `0x01` |

> `0x01` is success/ack; `0x00` is failure. The CS commands write the CS latch.
> `0x02` writes the latch low; it drives CS low when the pin is already an
> output, which is the state after entry and after `0x6x`/`0x8x`. It does not
> change the pin direction. `0x03` writes the latch high (Hi-Z in open drain,
> driven high in Normal output when the pin is an output). The pin-direction
> interaction is in §Configure peripherals (`0x4x`).

**Why it is wrong.** `0x02` calls `spi_binary_set_cs(LOW)`, which sets
`IODIR &= ~CS` (output) and `IOLAT &= ~CS` (low). It does not need a prior
output state. `0x03` calls `spi_binary_set_cs(HIGH)`: open drain sets
`IODIR |= CS` (input/Hi-Z); Normal output sets `IOLAT |= CS` and
`IODIR &= ~CS` (driven high). "Does not change the pin direction" is false for
both commands.

**Exact fix.** Replace the two rows with:

> | `0x02` | action | CS low (assert; enables the output) | `0x01` |
> | `0x03` | action | CS high (release: input/Hi-Z in open drain, driven high in Normal) | `0x01` |

and replace the paragraph with:

> `0x01` is success/ack; `0x00` is failure. `0x02` writes the CS latch low and
> makes CS an output, so it asserts. `0x03` writes the latch high and releases
> CS: an input (Hi-Z) in open drain, driven high in Normal. Both set the
> direction; the `0x4x` bit-0 interaction is in §Configure peripherals
> (`0x4x`).

### 3. [severity: major] Auto-CS `0x04` is said to be ignored after a `0x4x` released CS

**Files:** `docs/protocol/spi/binary.md:121-122`, `binary.md:446`,
`docs/protocol/spi/edge-cases.md:113`.

> `0x04` writes CS low at the start of the transfer and high (released; Hi-Z in
> open drain) at the end; CS is an output from entry.

> | `0x4x` released CS as input | transfer ignored; CS never goes low | `0x4x` bit 0 clear, or `0x6x`/`0x8x` |

> | `0x04` after a `0x4x` released CS | transfer ignored; CS never goes low | `0x4x` bit 0 clear, or `0x6x`/`0x8x` |

**Why it is wrong.** `spi_read_write_io()` calls `spi_binary_set_cs(LOW)`
before a non-empty transfer and `spi_binary_set_cs(HIGH)` afterwards. The low
call writes `IODIR &= ~CS`, so it makes CS an output and drives it low; the
transfer is not ignored and `0x04` is a valid recovery from the `0x4x`
input state. The `0x04` paragraph also never says it uses the same helper.

**Exact fix.** Replace the `0x04` sentence with:

> `0x04` uses the same CS helper: it drives CS low and makes it an output at
> the start, then releases it (input/Hi-Z in open drain, driven high in
> Normal) at the end.

Replace the `binary.md` edge row with:

> | `0x4x` released CS as input | CS high/Hi-Z, not asserted | `0x02` or a non-empty `0x04` asserts; `0x6x`/`0x8x` restore the released output |

Replace the `edge-cases.md` row with:

> | `0x04` after a `0x4x` released CS | not a failure: `0x04` drives CS low and makes it an output for the transfer, then releases it | no recovery needed |

### 4. [severity: minor] Remaining "while CS is an output" / "output only" preconditions

**Files:** `docs/protocol/spi/binary.md:142-143`, `binary.md:308-310`,
`binary.md:442`; `docs/protocol/spi/edge-cases.md:115`, `edge-cases.md:124`;
`docs/protocol/spi/README.md:47-49`.

> if the target needs CS asserted, use `0x02` while CS is an output (see
> §Configure peripherals for the `0x4x` trap).

> Assert CS with `0x02` (output only) before the read and release it with
> `0x03` afterwards; both act only while CS is an output (see §Configure
> peripherals).

> | `0x06` without CS | the AVR sees no chip select; the read is meaningless | first assert CS with `0x02`, while CS is an output |

> | `0x06` without CS | the AVR sees no chip select; the read is meaningless | first assert CS with `0x02`, while CS is an output |

> | `0x1x` without CS | bytes clocked with CS in its current state | if the target needs CS, assert it with `0x02` while CS is an output |

> `0x02`/`0x03`
>   write the CS latch low/high (assert/release while CS is an output),

**Why it is wrong.** `0x02` makes CS an output itself; "while CS is an output"
is no longer a precondition, and the `0x4x` "trap" is recoverable with `0x02`.
The phrases keep the old latch-only model alive even after the main paragraph
is fixed.

**Exact fix.** Remove the precondition and use the post-fix wording everywhere:

- `binary.md:142-143`: "if the target needs CS asserted, use `0x02` (it asserts
  and enables the output; see §Configure peripherals for the `0x4x` bit-0
  level)."
- `binary.md:308-310`: "Assert CS with `0x02` before the read (it drives low as
  an output) and release it with `0x03` afterwards (input/Hi-Z in open drain,
  driven high in Normal)."
- `binary.md:442`, `edge-cases.md:115`: "first assert CS with `0x02` (it
  enables the output)".
- `edge-cases.md:124`: "if the target needs CS, assert it with `0x02` (it
  enables the output)".
- `README.md:47-49`: "`0x02` asserts CS and enables the output; `0x03` releases
  it (input/Hi-Z in open drain, driven high in Normal)."

### 5. [severity: minor] The `0x03` edge row describes a normal release as a latch-only failure

**Files:** `docs/protocol/spi/binary.md:447`,
`docs/protocol/spi/edge-cases.md:114`.

> | `0x4x` bit 0 set (open drain) | CS stays an input; `0x03` writes the latch only | `0x4x` bit 0 clear, or `0x6x`/`0x8x` |

> | `0x03` after a `0x4x` released CS | latch high, pin stays an input | `0x4x` bit 0 clear, or `0x6x`/`0x8x` |

**Why it is wrong.** `0x03` is the release command. In open drain it correctly
leaves CS an input (Hi-Z); in Normal output it drives CS high as an output.
"Writes the latch only" is false (it sets the direction too), and treating the
release as a fault sends the reader to the old `0x4x`/`0x6x`/`0x8x` workaround.
The only real issue is a user who wanted an assertion; `0x02` is the answer.

**Exact fix.** Replace the rows with:

> | `0x4x` bit 0 set (open drain) | CS released as an input | expected; `0x02` asserts, `0x03` releases |

> | `0x03` after a `0x4x` released CS | CS released (input in open drain, driven high in Normal) | expected; use `0x02` to assert |

## Cross-set pages

Checked the pages outside the SPI set that describe the shared `0x4x` group or
the binary layer:

- `docs/protocol/binary/sub-protocols.md:171-173` — the shared `0x4x` bit-0
  description is still correct (set releases: input/Hi-Z in open drain, driven
  high in Normal; clear drives low/asserted) and its "check the mode page"
  pointer remains valid. **No stale `0x02`/`0x03` claim; no edit needed.**
- `docs/protocol/binary/edge-cases.md:268` and
  `docs/protocol/binary/sub-protocols.md:313` — "AUX and CS latched high" is
  the BBIO1 state-byte effect, not the SPI `0x02` command. **No edit needed.**
- `docs/protocol/binary/README.md`, `commands.md`, `entering-and-framing.md` —
  no SPI CS-direction claim.
- `docs/SPEC.md` SPI row — no CS-direction claim.

No cross-set page repeats the pre-fix wording; the defect is confined to
`docs/protocol/spi/binary.md`, `docs/protocol/spi/edge-cases.md` and the README
summary.

## Presentation (changed passages)

- No TAB characters in the set.
- The changed command/edge table rows remain pipe-aligned; no new option list
  or prose list was added.
- No dates, bench prose, pass counts or tested/verified claims were introduced;
  the only "verified" forms are the existing v3 "not verified" caveats.
- The changed prose lines stay within the set's usual ~80-column style.

**Finding count: 5** (3 major, 2 minor). **Verdict: No — RA-3 not reflected.**
