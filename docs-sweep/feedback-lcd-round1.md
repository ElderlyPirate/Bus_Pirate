# LCD set — fresh-reader feedback (round 1)

Reader: old Bus Pirate v3/v4 owner with the optional HD44780 adapter. I read the
LCD set in reader order, then the linked `generic/` and `protocol/spi/` pages,
then `TEMPLATE.md` and `SPEC.md` for context. I did not read the audit artifacts
in `docs-sweep/`.

## Verdict

**yes with fixes** — a clear, well-organized set with an unusually honest
unverified-hardware stance, but a fresh reader cannot currently get from the
README to a working build (the all-features path contradicts the build page),
and the wiring page omits the display's own power path and the v3a board.

## Task log

### 1. First impression from the README

**Where I looked:** `README.md` top, five-minute version, availability table,
limits. Then `entry-and-setup.md` and `dev/building.md`.

**How fast:** ~3 minutes.

**Success:** mostly. I knew it was an optional build, not in the stock images,
that I would have to build custom firmware, that `W` then `(2) 2` is the bring-up,
that MISO is unused, and that contrast is on the display.

**What was unclear:** the README says "other builds number it differently" but
not that a stock v4's entry 9 is PIC and a stock v3's entry 9 is DIO. It also
presents the all-features build as the short route, while `dev/building.md`
says that combination is expected to fail at link time. I had to open two other
pages to find out that the starting point may not exist.

### 2. Enter and set it up

**Where I looked:** `entry-and-setup.md` (menu table, entry captures, defaults,
`i`/`v`, `W`/`w`), then `commands.md` and `generic/navigation.md`.

**How fast:** ~5 minutes.

**Success:** yes. The flow is followable: `m9` in the all-features build, no
setup prompts, `W`, `(2) 2`, then text. The `W` and `w` captures match the shared
layer. The v3/v4 `v` column orders are stated.

**What was unclear:** the `i` block says `Open drain outputs` while the README
says no pull-up is required; the entry page's one-line explanation took a second
read. The defaults table has a typo in the `(2)` default. The direct form takes
no setup answers, but nothing says what `m9 2` would do.

### 3. Wire the adapter

**Where I looked:** `wiring.md` (chain, Bus Pirate pins, 595 outputs, supply,
contrast, backlight, confirm table), the `v`/`i` section of
`entry-and-setup.md`, and the repo's adapter images.

**How fast:** ~8 minutes.

**Success:** partial. The signal chain, the 595 bit assignment, the CS/latch
distinction, the no-pull-up reasoning, and the "what to confirm" table are
clear. I can tell which statements are firmware-side and which are board-side.

**What was unclear:** the display's own VDD/VSS connection and supply voltage
are never stated. The page powers "the adapter"; it does not say how the glass
is powered. The adapter named in the docs is v2 (plus v4); the board in the
repo's own photo is v3a. No image is embedded. I would still need the adapter
schematic and the display datasheet to actually wire it.

### 4. Use the display

**Where I looked:** `display.md` (init, geometry/DDRAM, CGRAM, instruction
bytes), `commands.md` (write forms, `r`, macros), `usage.md` (sessions, custom
glyph).

**How fast:** ~8 minutes.

**Success:** yes for the model. The 4-bit path, the 2-line DDRAM map, the raw
`(4)` address and its 7-bit wrap, the 20x4 address example, the CGRAM layout,
and the `(6)`/`(7)` byte-count rule are all usable. The `r` no-effect capture
and the "no macro 5" note are good.

**What was unclear:** the worked sessions show only terminal echoes. I cannot
tell what the glass should show after `"Hello"` then `"Bus Pirate"`, and the
custom glyph has no expected shape. The `(2)` argument rule differs across
three pages (`0` is included in `commands.md`, omitted in `display.md` and
`edge-cases.md`).

### 5. Hit the edge cases

**Where I looked:** `edge-cases.md` (failure table and prose sections), plus the
`W`/`w` and leaving sections of `entry-and-setup.md`.

**How fast:** ~5 minutes.

**Success:** yes. The stock `m9` trap, pre-`W` writes, `W` supply failure,
blank/garbled display, open-drain carry-over, and leaving power are all covered
with symptom and recovery.

**What was unclear:** the power-loss behaviour is attributed to the "adapter
supply" in `entry-and-setup.md` and to the "display supply" in `edge-cases.md`.
With a Bus Pirate-powered adapter and an externally powered display, those two
statements disagree. The open-drain status line is indistinguishable from the
normal status line, so I would power-cycle rather than diagnose.

### 6. Rendering check

**Where I looked:** all eight LCD files; scanned for tabs, prose width, capture
alignment, menu/option lists, percentages, dates, tested/verified-result prose,
and naming.

**How fast:** ~6 minutes.

**Success:** mostly. No TAB characters. No prose line over 80 columns. The `v`
capture is correctly aligned on a 9-character grid, and the page says the device
sends tabs and the transcript uses fixed spacing. The `(2)` and `(0)` lists are
readable code captures. No dates, no bench/rig/battery narrative, no "captured
on" provenance, no positive "tested/verified" claims. The 25-row failure table
is dense but works as a lookup table.

**What was unclear:** `about 95% used` / `about 30% used` in a user page; the
typo in the defaults table's `(2)` row; `RW` vs `R/W`; `Vpullup`/`VPU` without
the `Vpu` form; `3.3 V`/`5 V` vs `3.3V`/`5.0V`; `unverified` vs `not verified`.

### 7. README navigation test

**Where I looked:** `README.md` file table, five-minute version, related docs.

**How fast:** ~4 minutes.

**Success:** yes. Entry, wiring and commands are one click and are also in the
five-minute version. Display and edge cases are one click from the file table,
but are not signposted in the quickstart. There are no dead ends.

**Doubling back:** display ↔ commands ↔ usage is a useful loop, not an annoying
one. The flashing path is two hops: README → `dev/building.md` → `dev/flashing.md`,
even though the README row says "build and flash."

## Findings

### 1. [severity: blocker] The documented reference build contradicts the build page

**Location:** `docs/capabilities/lcd/entry-and-setup.md:23-24`:

> On v4, the documented all-features build includes the LCD adapter and is the
> shortest route.

`docs/dev/building.md:153-157`:

> The all-features v4 list also enables the HD44780 adapter.  Its SPI output
> helper has no external definition when SPI support is also enabled (the
> `spi.c` implementation is `static`), so that combination is expected to fail
> at link time; `unverified` — see [the LCD set](../capabilities/lcd/README.md).

**Problem:** the LCD set's reference is the all-features v4 build (entry 9, about
30% flash), but the build page says that exact combination is expected to fail
at link time because LCD and SPI support are both enabled. The link says "see
the LCD set," but the LCD set never mentions the conflict. The own-set v4 path —
"enable the LCD module in the v4 configuration" — starts from the default v4
config, which also has SPI support on, so it may hit the same failure. The v3
own-set path may hit it too. A fresh reader cannot tell whether the reference
image exists, which recipe works, or why "shortest route" is safe. The "about
95% used" numbers describe a build that may not link.

**Suggested fix:** resolve the conflict at the source, then make the LCD set own
the answer. If the link failure is real, put a short "build status" box at the
top of `entry-and-setup.md` and a one-line pointer in the README that states the
exact working configuration (which switches to enable/disable) or says the mode
is not currently buildable with SPI support. If the build page is stale, fix it.
Until a build is confirmed, drop the 30%/95% numbers or move them to
`dev/building.md`.

### 2. [severity: major] The display's power path is missing, and the power-loss behaviour is attributed to the wrong supply

**Location:** `docs/capabilities/lcd/wiring.md:102-108`:

> **Power the adapter.**  The adapter needs a logic supply and a common ground.
> `W` switches the Bus Pirate's regulated rails on and checks both the 5.0 V and
> 3.3 V rails, but it does not know which pin the adapter uses.  If the adapter
> is fed from a Bus Pirate supply pin, `W` powers it and `w` / a mode change cuts
> it; if it has its own feed, `W` does not switch that feed and the ground must
> still be common.  Confirm the board's supply input and any voltage selector
> (`unverified`).

`docs/capabilities/lcd/entry-and-setup.md:215-218`:

> | Adapter supply | On leaving the mode |
> |---|---|
> | a Bus Pirate 3.3V / 5.0V rail | power is cut; the display clears |
> | its own supply | the display keeps its last content |

`docs/capabilities/lcd/usage.md:241-242`:

> If the adapter is powered from a Bus Pirate supply rail, the display clears;
> an externally powered display keeps its content.

`docs/capabilities/lcd/edge-cases.md:117-119`:

> A display powered from a Bus Pirate supply rail therefore loses power and
> clears; a display with its own supply keeps its content.

**Problem:** every page talks about powering "the adapter," but the display is a
separate load with its own VDD/VSS. No page states how the display's VDD/VSS are
connected, what supply voltage the glass needs, or that the adapter's logic
supply and the display's supply can be different. Many HD44780 glasses are
5 V-only; a reader could power the adapter at 3.3 V and get a blank display.
The leaving behaviour depends on the **display's** supply, not the adapter's:
a BP-powered adapter with an externally powered display keeps its content, but
the `entry-and-setup.md` and `usage.md` sentences say it clears. `edge-cases.md`
has the correct model. The `W` requirement with an external feed is also not
explicit; `protocol/spi/README.md` says it plainly for SPI ("required even when
the target is self-powered"), and the LCD page should say the same.

**Suggested fix:** add a "Display power" subsection to `wiring.md` with a small
table: adapter logic supply, display VDD/VSS, V0 contrast, backlight. State the
typical display VDD (often 5 V, check the datasheet), that the adapter's display
header may supply it, and that a separate display feed is possible. Use "display
supply" in the `w` and leaving tables. Add a "display VDD/VSS and voltage" row to
"What to confirm on the board." Add one sentence: "W is still required with an
external adapter/display feed; it runs the SPI setup and checks the Bus Pirate
rails."

### 3. [severity: major] Adapter identification and media: the docs name v2/v4, the repo's own board is v3a, and no image is embedded

**Location:** `docs/capabilities/lcd/wiring.md:20-23`:

> | Bus Pirate LCD adapter v2 (the v3-era board) | <http://dangerousprototypes.com/docs/Bus_Pirate_LCD_adapter_v2> |
> | Bus Pirate v4 LCD adapter | <http://dangerousprototypes.com/docs/Bus_Pirate_v4_LCD_adapter> |

`docs/capabilities/lcd/README.md:65-66`:

> **No backlight or contrast control.**  The shift register's eight outputs
> are all assigned to the display interface and the activity LED.

**Assets:** `docs/assets/art/BP-LCD-v3a-600.jpg` is a board silkscreened "Bus
Pirate LCD Adapter v3a" with **Contrast** and **Backlight** trimpots and a
**Power** jumper. `docs/assets/images/BP-LCDv2-250px.jpg` shows the v2 board,
the adapter, the ribbon and a 16x2 glass.

**Problem:** a user with the v3a board sees a revision the docs never name, and
the docs only link external pages for the board's own pinout. The repo already
contains two useful photos and the v3a design zip, but the wiring page embeds
neither. The README's "No backlight or contrast control" reads as "the board has
none"; the v3a has manual pots. The intended meaning is "the firmware has no
control," which the wiring page says correctly.

**Suggested fix:** add the v3a to the adapter table (as a revision of the v3-era
board or a separate design, whichever is true), embed
`BP-LCD-v3a-600.jpg` and `BP-LCDv2-250px.jpg` in `wiring.md`, and link
`Bus Pirate Lcd Adapter v3a.zip`. Use the photo to label the Power, Contrast and
Backlight controls. Change the README bullet to "**No firmware backlight or
contrast control.** Manual presets on the adapter are board-side."

### 4. [severity: major] The README never states the stock `m9` trap, and "other builds number it differently" is wrong for stock builds

**Location:** `docs/capabilities/lcd/README.md:30-32`:

> Enter with `m` and answer the menu, or `m<N>`.  There are **no setup
> prompts**.  In the all-features v4 build `LCD` is entry 9; other builds
> number it differently, so read your own `m` list.

**Problem:** a stock build does not number LCD "differently"; it has no LCD
entry. On a stock v4, `m9` enters PIC, which then asks setup questions; on a
stock v3, `m9` enters DIO, whose pin writes work without `W`. The explicit
warning is only in `edge-cases.md` ("The stock-image trap"). A reader who skims
the five-minute version and types `m9` gets a working mode with the wrong pin
behaviour, exactly the trap the set describes.

**Suggested fix:** add one sentence to the five-minute version: "On a stock
image `m9` is PIC on v4 and DIO on v3, not LCD; if `LCD` is absent from the `m`
list, the image does not have it." Change "other builds number it differently"
to "in a build that includes it, the number is positional; read your own `m`
list."

### 5. [severity: major] The worked sessions never say what the glass should show

**Location:** `docs/capabilities/lcd/usage.md:74-79`:

> `W` selected the data register, so text can be sent directly:
>
> ```
> LCD>"Hello"
> WRITE: "Hello"
> LCD>
> ```

`docs/capabilities/lcd/usage.md:82-83`:

> Home the cursor, turn the display on with the cursor off, and write again.
> `{` moves RS low for the command; `}` moves it high for data:

`docs/capabilities/lcd/usage.md:176-177`:

> With the display powered (`W`) and initialised (`(2)`), define glyph 0, then
> print it.  The eight row bytes are the glyph bitmap, one byte per row.

**Problem:** the whole point of an LCD capability is what appears on the glass,
but every session reports only the terminal echo. A reader cannot tell whether
"Hello" then "Bus Pirate" should produce one visible row, two rows, or an
overwrite; the custom glyph has no expected shape. The set says the on-glass
result is unverified, which is honest, but the intended result can still be
described without claiming it was tested.

**Suggested fix:** add one line per session: "The glass shows `Hello` on row 1";
"`Bus Pirate` overwrites it from the home position"; and an ASCII bitmap for the
glyph (the eight row bytes are a 5×7 face; show the rows as `01110`, `10001`,
…). For the geometry discussion, add one tiny "what you should see" table for
16x2, 20x2 and 20x4.

### 6. [severity: minor] `i` says "Open drain outputs" while the README says no pull-up; the entry-page explanation is incomplete

**Location:** `docs/capabilities/lcd/entry-and-setup.md:133-135`:

> Before `W`, the same lines read `POWER SUPPLIES OFF`.  The `Open drain outputs`
> text is the mode's output-type flag, not a description of the SPI pads: the
> LCD setup does not select open drain ([spi-link.md](spi-link.md#output-type)).

`docs/capabilities/lcd/spi-link.md:114-118`:

> The LCD mode sets the shared output-type flag to open drain, so `i` reports
> `Open drain outputs (H=Hi-Z, L=GND)`.  Its own setup does not write the per-pin
> open-drain bits, and on a clean entry they are clear, so the SPI pins are driven
> normally.  The status text is a mode flag, not proof of the pad state.

**Problem:** the entry page says "the LCD setup does not select open drain"
without saying the mode **sets the status flag** but not the pad bits. A reader
sees `Open drain outputs` and `Pull-up resistors OFF` and may add pull-ups,
contradicting the README's "No pull-up is required." The spi-link page has the
correct explanation; the entry page should not read as if the status line is
simply wrong.

**Suggested fix:** use the spi-link wording on the entry page. Add one line to
the README's no-pull-up bullet: "`i` still says `Open drain outputs`; that is the
mode's flag, not the pad state."

### 7. [severity: minor] Resource percentages and meta reference-build prose in a user page

**Location:** `docs/capabilities/lcd/entry-and-setup.md:20`:

> | program flash | about 95% used with LCD added to the v3 default set | about 30% used in the all-features build |

`docs/capabilities/lcd/entry-and-setup.md:33-34`:

> In the all-features v4 build used as this set's reference, LCD is entry **9**.

**Problem:** the flash percentages are maintainer measurements in a user page;
they are unverifiable and describe the build that finding 1 says may not link.
"used as this set's reference" is docs-about-docs, not something a user needs.

**Suggested fix:** replace the percentages with qualitative guidance ("the v3
default set is nearly full; the all-features v4 build leaves room") and say "In
the all-features v4 build, LCD is entry 9." Move the numbers to
`dev/building.md` if they are needed.

### 8. [severity: minor] The `(2)` argument rule differs across three pages

**Location:** `docs/capabilities/lcd/display.md:37`:

> Without an argument, or with an argument outside `1`-`2`, the macro prompts:

`docs/capabilities/lcd/commands.md:127`:

> If the argument is missing, `0`, or outside `1`-`2`, the firmware prints the
> line-mode menu and prompts.

`docs/capabilities/lcd/edge-cases.md:21`:

> an argument outside `1`-`2` is treated as not given; default `2`

**Problem:** only `commands.md` says `(2) 0` opens the prompt. A user who reads
the display page or the failure table first will not expect that, and may think
`(2) 0` is an error.

**Suggested fix:** use the `commands.md` wording in all three places, or state
the rule once in `commands.md` and link to it from the other two.

### 9. [severity: minor] Typo in the defaults table

**Location:** `docs/capabilities/lcd/entry-and-setup.md:105`:

> | display lines | chosen by `(2)`; default `2. Multiple` |

**Problem:** the backticks are mismatched and the parenthesis is missing; it
reads as "default 2. Multiple."

**Suggested fix:**

```
| display lines | chosen by `(2)`; default `2` (Multiple) |
```

### 10. [severity: minor] Naming drift without a mapping

**Location:** `docs/capabilities/lcd/wiring.md:88`:

> | 2 | Q2 | RW — read/write | always `0` (write); the mode never reads |

`docs/capabilities/lcd/wiring.md:53`:

> R/W line is held at write, and the busy flag is not polled.

`docs/capabilities/lcd/display.md:16`:

> The adapter drives the display's 4-bit interface: RS, RW, EN and D4-D7.

Other examples: `README.md:39` "**3.3 V or 5 V** supply rail" versus
`entry-and-setup.md:217` "a Bus Pirate 3.3V / 5.0V rail" versus
`wiring.md:109` "3.3 V logic"; `README.md:50` "hardware not verified on v3 |
adapter path unverified" versus `entry-and-setup.md:21` "not verified on v3 |
unverified."

**Problem:** the same item is written two ways, and the set has no "Names used
in this set" table like the SPI README has. A reader comparing the 595 table to
the display page has to decide whether `RW` and `R/W` are the same pin, and
whether `unverified` and `not verified` mean different states.

**Suggested fix:** add a short table near the top of the README (or in
`wiring.md`): `RW` (pin/firmware form) vs `R/W` (prose); `Vpullup` (warning) /
`Vpu` (reference card) / `VPU` (v table); `3.3V`/`5.0V` in captures vs `3.3 V`/
`5.0 V` in prose; `Multiple` (firmware menu) = two-line; `1` = one-line;
`unverified` vs `not verified`. Use the firmware strings in captures.

### 11. [severity: minor] The README's absolute "No pull-up is required" omits the binary open-drain carry-over

**Location:** `docs/capabilities/lcd/README.md:39-41`:

> Wiring: adapter on **MOSI**, **CLK** and **CS**, plus a **3.3 V or 5 V**
> supply rail and **GND**; **MISO is unused**.  **No pull-up is required.**

`docs/capabilities/lcd/wiring.md:136-141`:

> If a **binary** SPI session left the SPI outputs **open drain**, the LCD
> setup does not change that setting.  High levels are then released rather
> than driven, and the shift register needs pull-ups or will see floating highs.
> Power-cycle the Bus Pirate, or enter the SPI mode, select normal outputs,
> send `W` once, then leave for LCD.

**Problem:** the headline is true on a clean entry but false after a binary SPI
session left the outputs open drain. The exception is on the wiring page, not
the README, so a reader who trusts the headline can end up with weak highs.

**Suggested fix:** add "(unless a prior binary SPI session left the outputs open
drain; see `edge-cases.md`)" to the README bullet.

### 12. [severity: minor] The direct form takes no setup answers, but the docs don't say what `m9 2` does

**Location:** `docs/capabilities/lcd/README.md:30-31`:

> Enter with `m` and answer the menu, or `m<N>`.  There are **no setup
> prompts**.

`docs/generic/navigation.md:203`:

> `m<N> <answers…>` a complete answer set skips the prompts, prints the
> settings summary (when the mode has one), then `Ready`; an incomplete set
> falls back to the prompts

**Problem:** a user may try `m9 2` to select two-line mode, because the shared
grammar allows answers after `m<N>` for modes with prompts. The LCD page says
there are no prompts but does not say whether the extra token is ignored,
becomes the next command, or is an error.

**Suggested fix:** add one sentence to `entry-and-setup.md`: "The direct form
takes no setup answers; select the line mode with `(2) 1` or `(2) 2` after `W`."
If extra tokens are parsed as the next command, say that too.

### 13. [severity: minor] Flashing is a two-hop path from the README

**Location:** `docs/capabilities/lcd/README.md:86`:

> | [docs/dev/building.md](../../dev/building.md) | build and flash the custom image |

**Problem:** the row says "build and flash" but links only the build page; the
flashing page is linked from `entry-and-setup.md` and from `dev/building.md`,
not from the README. A reader looking for the flash step has to double back.

**Suggested fix:** add a `docs/dev/flashing.md` row, or change the row to link
both pages.

## What to keep

- The build-dependent menu table and the "Always read your own `m` list; the
  firmware's menu is authoritative" rule. That is the right way to handle a
  positional menu.
- The `W`/`w` captures and the explicit "`This mode requires an adapter` is a
  fixed reminder, not detection" sentence. It prevents a false hardware-detect
  assumption.
- The `wiring.md` "What to confirm on the board" table. It is the most useful
  part of the wiring page: it turns `unverified` into a concrete checklist.
- The no-pull-up reasoning: 74HCT595 inputs are high impedance, the Bus Pirate
  drives MOSI/CLK/CS, MISO is unused. This is the kind of explanation that
  makes the headline believable.
- The open-drain carry-over section. It is a genuinely non-obvious failure and
  the symptom/behaviour/recovery structure is clear.
- The display geometry section: the 2-line DDRAM map, the `(4) 64` example, the
  `(4) 192` wrap, the `(6)`/`(7)` byte-count rule, and the 20x4 address example.
  This is the best explanation of the display model in the set.
- The `r` no-effect capture and the "keep a copy of what you wrote" advice.
- The macro list with the missing 5 left visible, plus the `(4:0)` rejection.
  The gap is explained instead of hidden.
- The `v` table alignment and the tab-stop caveat. The columns are actually
  aligned, and the page tells you why the spacing is not guaranteed.
- The SPI comparison page. "Not a layer on top of the SPI mode" is the right
  framing, and the comparison table answers the question a SPI user will ask.
- The honesty about unverified hardware. There are no fake bench claims, no
  dates, and no pass counts; the reader knows what is known and what is not.

## Structural opinion

The layout works for this function. The progression
`README → entry-and-setup → wiring → spi-link → display → commands → usage →
edge-cases` is logical: get in, wire it, understand the transport, understand
the glass, then the commands, sessions and failures. The `spi-link.md` page is a
good idea because the LCD mode reuses SPI hardware without being SPI; without
it, a reader would keep looking for SPI-mode behaviour that is not there. The
README is a real map, not a manual, and it is short enough to read.

The weak point is the split of build/availability material across the README,
`entry-and-setup.md` and `dev/building.md`: that is where the contradiction
lives, and none of the three pages owns the final answer. The second weak point
is the wiring page's scope: it owns "the adapter, the pins, the outputs, supply,
contrast, the pull-up statement," but it stops at the adapter's connector and
never describes the display's own power path. The page is honest about the
adapter being unverified, but the display is not an adapter-side detail; it is
the other half of the hardware.

Concrete proposals:

1. **Add a build-status box at the top of `entry-and-setup.md` and a one-line
   pointer in the README.** State the exact working configuration or the exact
   blocker. Do not call the all-features build the "shortest route" until the
   link question is resolved. Move the 30%/95% numbers to `dev/building.md`.
2. **Add a "Display power and contrast" subsection to `wiring.md`.** A four-row
   table (adapter logic supply, display VDD/VSS, V0, backlight) plus a note that
   the display supply decides the clear/keep behaviour. Use the existing v3a
   photo and the v2 photo; they answer questions the text currently cannot.
3. **Add a "Names used in this set" table near the top.** The SPI set has one
   and it works. It would settle `RW`/`R/W`, `Vpullup`/`Vpu`/`VPU`,
   `Multiple`/two-line, `1`/one-line, `3.3V`/`3.3 V`, and `unverified`/
   `not verified` in one place.
4. **Add an "expected display" line to every `usage.md` session.** The terminal
   echo is not the result; the glass is. A tiny "what you should see" table for
   16x2, 20x2 and 20x4 would also make the geometry page more usable.
5. **Add a "wrong mode" callout to the README five-minute version.** One line
   with the v4 PIC / v3 DIO mapping is enough; the edge-case page can keep the
   full explanation.
6. **State the `(2)` rule once.** Put the "missing, `0`, or outside 1-2" rule in
   `commands.md` and link to it from `display.md` and `edge-cases.md`, or copy
   the same sentence verbatim. The current three variants are a small but real
   source of doubt.

## Unknowns

- **Whether the all-features v4 build actually links.** `dev/building.md` says
  "expected to fail at link time; `unverified`." I cannot build it, so I cannot
  tell whether the LCD set's reference image exists or whether the build page is
  stale. This is the biggest open question.
- **Whether the Q0-Q7 assignment and the 7-display + 1-LED split match the v2,
  v3a and v4 adapter revisions.** The firmware-side assignment is stated as
  fixed; the board routing is stated as unverified. I do not have the boards.
- **Whether the v3a board is a revision of the v2 design or a separate product.**
  The repo's art says "Bus Pirate LCD Adapter v3a"; the docs name only v2 and
  v4. I cannot tell whether the docs are intentionally naming firmware
  configurations or whether v3a is simply missing.
- **Whether the adapter's display header supplies the display's VDD/VSS on all
  revisions.** The v2 photo shows the ribbon and the glass; the v3a photo shows
  the 16-pin header. I cannot confirm the power pins from the photos alone.
- **Whether the 1 MHz shift clock and the fixed delays work with all HD44780
  glasses.** The docs label this unverified; I have no display to try.
- **Whether the `i` output-type flag and per-pin open-drain bits behave as
  described.** The explanation is internally consistent, but I cannot inspect
  the firmware or the pads.
- **Whether the v3 `v` column order and the `0.(Bk)` tenth-column label are
  correct on hardware.** `generic/usage.md` says the v3 table is not verified.
- **Whether the external Dangerous Prototypes pages are live.** My fetch got a
  403, which may be bot blocking rather than a dead page. The local v3a/v2
  assets are a safer reference.
- **Whether "about 95% / about 30%" flash usage is accurate.** No build was run
  for the page, and the build it describes is the one in doubt.
- **Whether the v3a Contrast and Backlight trimpots are the "presets" the wiring
  page refers to.** The photo shows them, but the text never names them.
