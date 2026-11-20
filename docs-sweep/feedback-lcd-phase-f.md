# LCD Phase F delta audit — RA-17

**Verdict: no — the RA-17 delta is not applied; five passage groups still
describe the pre-fix open-drain behavior.** The rest of the LCD set is
unchanged and was not re-reviewed.

## Source check (docs worktree `Firmware/`)

- `capability_hd44780/hd44780.c:175-187`: `LCDsetup()` sets
  `mode_configuration.high_impedance=NO`; `LCDsetup_exc()` writes
  `BP_MOSI_ODC=OFF`, `BP_CLK_ODC=OFF`, `BP_CS_ODC=OFF`.
- `core_system/proc_menu.c:1610-1615`: `high_impedance == NO` selects
  `BPMSG1121`; `core_messages/messages_v4.s:389` (and `messages_v3.s:359`)
  is `Normal outputs (H=3.3v, L=GND)`.
- `core_system/core.c:370-371`: `setup_prepare = LCDsetup`,
  `setup_execute = LCDsetup_exc`; `setup_prepare` runs at mode entry,
  `setup_execute` on `W`.
- Consequence: `i` reports normal outputs; a successful `W` writes the three
  SPI open-drain bits off, so a previous binary SPI session's open-drain
  setting is cleared. No carry-over and no workaround.

## Findings

### 1. [major] `entry-and-setup.md` still prints and explains the old open-drain status

**Location:** `docs/capabilities/lcd/entry-and-setup.md:134-146`.

**Quote:**

> The mode-relevant status lines after `W` are:
>
> ```
> POWER SUPPLIES ON, Pull-up resistors OFF, Open drain outputs (H=Hi-Z, L=GND)
> MSB set: MOST sig bit first, Number of bits read/write: 8
> a/A/@ controls AUX pin
> ```
>
> Before `W`, the same lines read `POWER SUPPLIES OFF`.  The `Open drain outputs`
> text is the mode's output-type flag: entering the mode sets that flag, but the
> LCD setup does not write the per-pin open-drain bits.  On a clean entry those
> bits are clear, so the SPI pins are driven normally
> ([spi-link.md](spi-link.md#output-type)).

**Why:** current tree: `LCDsetup()` sets `high_impedance=NO` and
`LCDsetup_exc()` writes `BP_MOSI_ODC/CLK/CS=OFF`. `i` therefore prints
`Normal outputs (H=3.3v, L=GND)`, and the setup **does** write the bits. The
capture and the explanation are the pre-RA-17 behavior.

**Exact fix:**

- Change the capture line to:

  ```
  POWER SUPPLIES ON, Pull-up resistors OFF, Normal outputs (H=3.3v, L=GND)
  ```

  Use the exact firmware casing: the status message is `3.3v` (lowercase);
  the selection prompt is `Normal (H=3.3V, L=GND)` (uppercase).
- Replace the paragraph with: "Before `W`, the supply field reads
  `POWER SUPPLIES OFF`.  The `Normal outputs` text is the mode's output-type
  setting: entering the mode selects normal outputs, and the LCD setup writes
  the MOSI/CLK/CS open-drain bits off, so the SPI pins are driven push-pull."
- In the `W` section at lines 187-189, change "The setup configures the SPI
  pins and selects the **data** register." to "The setup configures the SPI
  pins, writes the MOSI/CLK/CS open-drain bits off, and selects the **data**
  register." This states the initialised ODC state at the point where `W`
  runs it.

### 2. [major] `spi-link.md` "Output type" says the mode sets open drain and the carry-over persists

**Location:** `docs/capabilities/lcd/spi-link.md:112-125`.

**Quote:**

> The LCD mode sets the shared output-type flag to open drain, so `i` reports
> `Open drain outputs (H=Hi-Z, L=GND)`.  Its own setup does not write the
> per-pin open-drain bits, and on a clean entry they are clear, so the SPI pins
> are driven normally.  The status text is a mode flag, not proof of the pad
> state.
>
> If a **binary** SPI session left the SPI outputs open drain, the bits persist:
> the LCD setup does not clear them and the v4 terminal reset does not either.
> The shift register then sees released highs instead of driven highs.
> Power-cycle the Bus Pirate, or enter the SPI mode, select normal outputs, send
> `W` once, then leave for LCD
> ([edge-cases.md](edge-cases.md#open-drain-carry-over)).

**Why:** opposite of the current tree. The mode flag is normal, the ODC bits
are written off by `LCDsetup_exc()`, and the binary-session carry-over is
cleared on `W`. The recovery paragraph is obsolete.

**Exact fix:** replace the whole section with:

> The LCD mode uses normal push-pull outputs.  Entering the mode selects normal
> output, so `i` reports `Normal outputs (H=3.3v, L=GND)`.  The LCD setup
> writes the MOSI/CLK/CS open-drain bits off, so a previous binary SPI session's
> open-drain setting is cleared when `W` runs.  No pull-up or recovery step is
> needed.

Remove the `#open-drain-carry-over` link; the anchor should no longer exist.
Optionally add an output-type row to the "Fixed settings" table at lines
33-43: `output type | normal push-pull | MOSI/CLK/CS ODC bits written off`.

### 3. [major] `README.md` still calls a binary session an exception and says `i` reports open drain

**Location:** `docs/capabilities/lcd/README.md:53-57`.

**Quote:**

> **No pull-up is required** in the normal case; a binary SPI session that left
> the outputs open drain is the exception
> ([edge-cases.md](edge-cases.md#open-drain-carry-over)).  `i` still says
> `Open drain outputs`; that is the mode's flag, not the pad state.

**Why:** the exception is gone; `i` reports normal outputs; the LCD setup
clears the ODC bits. The bullet tells the reader the opposite and links to a
section that should be deleted.

**Exact fix:** replace the bullet's pull-up/`i` sentences with:

> **No pull-up is required**: the LCD setup selects normal push-pull outputs
> and clears the SPI open-drain bits; `i` reports `Normal outputs (H=3.3v,
> L=GND)`.

Keep the following contrast sentence unchanged.

### 4. [major] `wiring.md` still lists a carry-over exception and the old recovery

**Location:** `docs/capabilities/lcd/wiring.md:166-182`.

**Quote:**

> **No pull-up is required** on MOSI, CLK or CS in the normal case.  The Bus
> Pirate drives all three actively, and the shift register's inputs are high
> impedance; MISO is unused, so there is no open-drain bus and no idle-high
> level to hold.
> Two board-level and carry-over exceptions:
>
> - The shift register's output-enable must be tied low and its master-reset
>   high for the outputs to drive.  The published designs do this; confirm it on
>   a different revision (`unverified`).
> - If a **binary** SPI session left the SPI outputs **open drain**, the LCD
>   setup does not change that setting.  High levels are then released rather
>   than driven, and the shift register needs pull-ups or will see floating
>   highs.  Power-cycle the Bus Pirate, or enter the SPI mode, select normal
>   outputs, send `W` once, then leave for LCD
>   ([edge-cases.md](edge-cases.md#open-drain-carry-over)).

**Why:** the second bullet is false. `LCDsetup_exc()` writes the three ODC
bits off, so the setting is changed and no pull-up/recovery is needed.

**Exact fix:**

- Change "Two board-level and carry-over exceptions:" to "One board-level
  requirement:".
- Delete the second bullet entirely.
- Add one sentence to the first paragraph: "The LCD setup selects normal
  push-pull outputs and writes the SPI open-drain bits off."
- Remove the `#open-drain-carry-over` link.

### 5. [major] `edge-cases.md` still documents a non-existent failure and workaround

**Location:** `docs/capabilities/lcd/edge-cases.md:31` and `99-113`.

**Quote (failure table row):**

> | open-drain carry-over from a binary SPI session | weak highs; the adapter
> may not latch reliably | the open-drain bits persist and the LCD setup does
> not clear them | power-cycle; or select normal SPI outputs and send `W`
> ([details](#open-drain-carry-over)) |

**Quote (section):**

> ## Open-drain carry-over
>
> - **Trigger:** an earlier **binary** SPI session configured open-drain SPI
>   outputs, and the terminal was entered again without a power cycle. ...
> - **Symptom:** the adapter may not see clean logic highs; `i` reports
>   `Open drain outputs`, which is also the mode's normal status text.
> - **Behaviour:** the LCD setup does not write the per-pin open-drain bits, and
>   neither the mode change nor the v4 terminal reset clears them. ...
> - **Recovery:** power-cycle the Bus Pirate ... Alternatively, enter the SPI
>   mode, select normal outputs, send `W` once, then leave for LCD.

**Why:** the failure does not exist after RA-17: the LCD setup writes the ODC
bits off, and `i` reports normal outputs. The workaround is unnecessary and
misleading.

**Exact fix:**

- Delete the failure-table row at line 31.
- Delete the entire `## Open-drain carry-over` section (lines 99-113).
- No replacement section is needed; the positive statement belongs in
  `spi-link.md` / `entry-and-setup.md` (findings 1 and 2).
- After the edits, confirm no remaining links to `#open-drain-carry-over`
  (README, wiring and spi-link are covered above).

## Cross-set list

No cross-set edit is required; the LCD carry-over claims are local to the set.

- `docs/capabilities/auxiliary/aux-pin.md:152,202` — generic "if the active
  mode configured that pin as open drain, `A` releases it" / "no per-pin
  open-drain selection" text. Owned by `auxiliary` (already accepted); it is
  about the AUX pin, not MOSI/CLK/CS, and does not claim an LCD carry-over.
  No change.
- `docs/protocol/spi/binary.md:44-57` — the binary SPI entry defaults are open
  drain. Scoped to the binary path; it does not say the LCD terminal setup
  inherits them. No change.
- `docs/generic/navigation.md:104` — the generic `i` status example uses
  `Open drain outputs (H=Hi-Z, L=GND)`. It is a generic example, not an LCD
  claim; the LCD page now uses the normal-output string. No change.
- No page outside `docs/capabilities/lcd/` links to `#open-drain-carry-over`.

## Presentation check (changed passages)

- No TAB characters in the eight LCD files.
- No prose line over 80 columns; the current changed passages are within the
  limit.
- The replacement status capture is 77 characters, a code line, under the
  prose limit.
- The proposed replacements are prose and should be wrapped at ~80 columns.
- No dates, bench prose, pass counts or resource percentages in the changed
  passages.
- Removing the failure-table row and the `## Open-drain carry-over` bullet
  section removes one very long table row and one dense bullet chain.
- **Casing note:** the status message is `Normal outputs (H=3.3v, L=GND)`
  (lowercase `v`), while the selection prompt is `Normal (H=3.3V, L=GND)`
  (uppercase `V`). The README "Names used in this set" table currently lists
  `3.3V` / `5.0V` for captures; add `3.3v` (status line) there or qualify the
  table so the new capture is not read as a typo.

## Fix order

1. `spi-link.md` output type (the owning explanation).
2. `entry-and-setup.md` status capture, paragraph and `W` bullet.
3. `README.md` five-minute bullet.
4. `wiring.md` pull-up statement.
5. `edge-cases.md` row and section; remove all `#open-drain-carry-over` links.

Then re-run this delta audit against the same worktree `Firmware/`.
