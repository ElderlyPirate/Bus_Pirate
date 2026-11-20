# AVRISP-MKII — fresh-reader feedback (round 1)

## Verdict

**Yes with fixes.** The set is unusually complete, the main naming map is explicit, and I could get from the README to a plausibly wired ISP session without guessing. It is not ready to call done: the Windows USB first-time driver path is a catch-22, and the fuse naming, speed-table completeness, AVR053 command example and interface-switch procedure each need a fix before a fresh user can rely on the set.

## Task log

I read as a Windows user first (v4 board, ATmega328P target, fresh host), then checked the Linux/macOS notes. Times are my reading times, not test times.

**1. First impression from the README.**
`README.md` opening, five-minute version, availability, limits → **~2 min** → **success** → I knew it was AVRISP-MKII, v4-only, included in the default v4 image and omitted from the all-features custom set, with ISP/PDI/TPI paths and the `0x08`/`0x09` entry bytes. Unclear: the five-minute version is a summary, not a runbook — it still needs a separate entry script plus `avrdude`; the README says "20 × on v4" while `entry.md` says 20 × unconditionally; "a terminal cannot send NULs" is stated as absolute; and the exact custom-build switch is never named in the user set.

**2. Host setup.**
`host-tools.md` (identity, `avrdude`, drivers, Atmel Studio, failure map), `entry.md` (host sequence, USB route), `README.md` five-minute version → **~6 min** → **partial** → identity (`03EB:2104`, `AVRISP mkII`, `AVRISP_MK2`, firmware 1.24) and the standard commands were clear in about a minute; the driver table and failure map were clear. The Windows USB path is not executable from zero: there is no `avrdude`/pyserial prerequisite, and the first-time driver step has a timing problem (finding 1). Atmel Studio is advertised but not actionable (finding 6). I also could not tell whether to wait for `03EB:2104` to appear or start `avrdude` immediately; the docs say both "start immediately" and "the clock starts at entry".

**3. ISP wiring and session.**
`wiring.md` (header, 6-pin ISP, programmer table, voltage), `isp.md` (avrdude, fuses, speed, limits), `sessions.md` (worked ISP sequence), README wiring headline → **~5 min** → **success for wiring, partial for a first run** → the five-wire headline is immediate, the 6-pin ISP table matches the target end I expected, and the "two naming layers" paragraph is the best explanation in the set. Unclear: the header pin numbering is called a published convention and pins 9–11 are unconfirmed (finding 8); `RESET` release is tristate, so the target needs its own pull-up; the speed table says "complete mapping" but is not (finding 3); the worked ISP table cannot be typed because the enable delay bytes are prose (finding 15); and no expected `avrdude` output is shown.

**4. PDI and TPI.**
`pdi.md`, `tpi.md`, `wiring.md` PDI/TPI sections, `edge-cases.md` PDI/TPI table, `command-model.md` XPROG sequence → **~5 min** → **success on the differences, partial on switching safely** → in about two minutes I had the four differences: PDI clock on the target's RESET/PDI_CLK, DATA is MOSI+MISO joined through resistors, PDI leaves AUX open, TPI ties AUX to RESET and holds it low, and TPI needs a 5 V target supply. Unclear: there is no single procedure for ending an ISP session and moving to PDI/TPI (finding 5); two PDI rates are given, 25 kHz and 125 kHz (finding 9); the page does not say whether 3.3 V logic is sufficient for a 5 V TPI target (finding 10); and "every other accepted" TPI erase type is not enumerated (finding 14).

**5. Command/state model and AVR053 calibration.**
`command-model.md` (layers, lifecycle, memory, ISP/XPROG sequences, failure table), `sessions.md` (ISP session, end of mode), `rescue-clock-osccal.md` (rescue clock, AVR053 wiring, order, timing, limits), `edge-cases.md` clock/voltage rows → **~6 min** → **partial** → the state model and failure transitions are the clearest part of the set; the 32 ms window, eight level changes, and a fresh window on a second run are all clear. The AVR053 path is not executable: there is no `0x05` worked exchange, no status mapping, and no pointer to the AVR053 firmware (finding 4). The "remove SCK" step is presented as a step but immediately called optional, which made me stop and re-read (finding 21).

**6. Rendering check.**
All 14 `docs/programmers/avrisp/*.md` files, scanned for tabs, fences, table column counts, prose width, code-block alignment, verification prose and naming forms → **~7 min** → **mostly clean** → no TAB characters, no unbalanced fences, no table-column mismatches, no prose line over 80 columns outside tables/code, and no run dates, bench narratives, pass counts or "captured on" provenance. Problems found: two hand-spaced end-of-mode diagrams (finding 18); dense bullet sections in `pdi.md` and `protocol.md` (finding 19); a standalone "Unverified" ledger (finding 11); author-facing hedging (finding 12); the cryptic "LUFA/Table" phrase (finding 13); and Windows-only `COM5` examples (finding 16). The main naming map (`AVRISP-MKII` / `AVRISP mkII` / `AVRISP_MK2` / `STK500_2`) is explicit and consistently used; I did not find an unresolved naming mismatch. The widest tables (`protocol.md` command table, `edge-cases.md` failure maps) are dense but still scannable — not unreadable, just wide.

**7. README navigation.**
`README.md` file table → `entry.md`, `wiring.md`, `isp.md`, `pdi.md`, `tpi.md`, `host-tools.md`, `rescue-clock-osccal.md`, `edge-cases.md` → **~3 min** → **success** → entry, ISP/PDI/TPI, host tools, calibration and edge cases are all one click from the README, and the file-table descriptions are accurate. The only dead-end I hit was `entry.md`'s binary-layer path written as plain code rather than a link (finding 17). The only doubling back was `serial-route.md` sending PDI/TPI readers to `isp.md §Entry` instead of `entry.md` or the PDI/TPI pages (finding 22). The fastest route for a first run is still README → `entry.md` → `host-tools.md` → `wiring.md` → `isp.md`; a single quickstart page would remove that bounce.

## Findings

### Blocker

**1. [severity: blocker] The Windows USB first-time driver setup is a catch-22.**
**Location:** `host-tools.md:66`; `edge-cases.md:16`; `entry.md:124-127`.
> `host-tools.md:66` — a libusb/WinUSB-style driver bound to `03EB:2104` once (for example with Zadig, choosing the `AVRISP mkII` interface)
> `edge-cases.md:16` — First-time driver installation takes longer than five seconds | the mkII device disappears and the CDC port returns | finish the driver installation first, then enter and run `avrdude` immediately
> `entry.md:124-127` — The host must configure the new interface inside the same five-second window.  A slow first-time driver install can exceed it; the firmware then releases the target and restores the CDC device instead of waiting forever.

**Problem:** The `03EB:2104` identity only exists after entering the personality, and it disappears after about five seconds. "Finish the driver installation first" cannot be done because there is no device to bind before entry; and the page itself says a first-time install can exceed the window. A fresh Windows user following the instructions in order is stuck, and the failure-map row "fix the driver first, then enter" repeats the impossible order. The same timing problem applies to the first `avrdude` run: the docs say to start immediately, but do not say whether to wait for `03EB:2104` to appear.
**Fix:** Add a first-time Windows procedure that assumes the device will drop: enter with `0x08`, bind the driver in one Zadig pass, and if the device disappears, re-enter and retry until the binding completes. State that this may take several attempts, and recommend the serial route (`0x09`) for ISP-only first use. If the personality can be held open by a valid `SIGN_ON` loop, give that script; otherwise do not claim the driver is a one-time pre-entry step. Correct the `edge-cases.md` row to say "re-enter and retry the binding" rather than "finish first". Also say whether the first `avrdude` run should wait for enumeration or start at once, and give the retry if it starts too early.

### Major

**2. [severity: major] `efuse` is called the "mid" fuse byte.**
**Location:** `isp.md:57`.
> `lfuse`, `hfuse`, `efuse` name the low/mid/high fuse byte; the value is the part's, not a universal one

**Problem:** The three AVR fuse bytes are low, high and extended. `efuse` is the extended fuse byte, not "mid". A user writing fuses from this table can mis-map the third byte, and a fuse mistake is the one error this page correctly warns can lock the part out.
**Fix:** Change to "`lfuse`, `hfuse`, `efuse` name the low/high/extended fuse byte". Keep the rest of the warning.

**3. [severity: major] The speed section claims a complete mapping and then collapses 157 indices into one row.**
**Location:** `isp.md:82-85`; `isp.md:96`.
> The programmer implements the full AVR069 speed ladder (indices 0–163); the table below is the complete mapping, so the AVR069 document is provenance for the numbering rather than a required lookup.
> | 7 … 163 | below 125 kHz, down to about 51 Hz (computed; approximate) | software |

**Problem:** The table is not the complete mapping; it is seven rows plus a range. If I want index 40, or the actual frequency for a `-B` value below 125 kHz, I cannot look it up. "Provenance" is also author-facing and sounds like an audit note, not a user instruction.
**Fix:** Either publish the real ladder (the AVR069 frequency list is 164 entries, which is too long for this page) or change the sentence to: "The seven fastest steps are tabulated; indices 7–163 are a continuous approximate ladder from about 96 kHz down to 51 Hz. Use `-B` to select a period; you do not need AVR069 for normal use." Remove "provenance". If the full table is kept, generate it rather than hand-writing it.

**4. [severity: major] The AVR053 calibration path has no command-level exchange.**
**Location:** `rescue-clock-osccal.md:121-125`; `protocol.md:108`.
> `rescue-clock-osccal.md:121-125` — `OSCCAL` is reachable on the USB route and, over the STK500v2 command set, on the serial route.  **Host action:** the calibration is requested with command `0x05`; none of the `avrdude` command lines in this set sends it, so a custom host that implements the calibration procedure is needed.
> `protocol.md:108` — | `0x05` | OSCCAL | — | command + status |

**Problem:** This is the one path the docs explicitly say needs a custom host, and the custom host is given no request bytes, no success reply, no timeout reply, and no statement of what the command returns. I cannot tell whether success is `05 00`, whether a timeout is `05 80` or `05 C0`, or whether the target's OSCCAL value is returned at all. The page also does not say where the AVR053 target firmware comes from, only that the personality does not install it.
**Fix:** Add a short "Worked calibration exchange": `host -> 05`, `host <- 05 00` for success, `host <- 05 80` for the 32 ms timeout, `host <- 05 C0` for a rejected request; state that the command performs only the clock/sense window and neither returns nor writes an OSCCAL value; and point to the AVR053 application note or say the target firmware is the operator's own and out of scope. This is the difference between a description and a path a user can run.

**5. [severity: major] There is no single procedure for switching between ISP, PDI and TPI.**
**Location:** `command-model.md:40-44`; `edge-cases.md:72`; `wiring.md:120-121`.
> `command-model.md:40-44` — The states are cooperative, not strictly enforced.  The firmware does not refuse a memory command sent before `ENTER_PROGMODE`, and it does not police a switch from one target interface to the other without a `LEAVE`.  In those cases the command is executed against a target that is not listening, or the previous interface's pin state can persist.
> `edge-cases.md:72` — leave `AUX` open for PDI; send `LEAVE_PROGMODE_ISP` before switching target type
> `wiring.md:120-121` — On PDI the clock line is the target's **RESET/PDI_CLK** pin; do not wire `AUX` and do not press the target's reset button while programming.

**Problem:** The docs know an interface switch without `LEAVE` is unsafe, but the user has to assemble the rule from three pages. There is no ordered procedure for "I finished ISP, I now want PDI": `LEAVE` the current session, let the mode end or re-enter, rewire, then `XPROG_SETMODE`. A user may rewire while the personality is active (explicitly forbidden elsewhere) or leave `AUX` driven into the target's RESET.
**Fix:** Add a short "Changing target interface" checklist in `wiring.md` or `command-model.md` with the exact order and the pin-state danger. One table with three rows (ISP→PDI, ISP→TPI, PDI↔TPI) and four columns (LEAVE, wait/re-enter, rewire, SETMODE) would remove the guesswork.

**6. [severity: major] Atmel Studio is advertised as a host tool but is not a usable procedure.**
**Location:** `README.md:33`; `host-tools.md:88-99`.
> `README.md:33` — | [host-tools.md](host-tools.md) | device identity, `avrdude`, Atmel Studio, host drivers |
> `host-tools.md:95-97` — Atmel Studio's tool discovery and programming flow are **not verified** for this port.  The identity is the compatibility mechanism; whether a particular Atmel Studio release accepts the clone is outside the firmware.

**Problem:** The README table tells a Studio user this page covers Studio; the page says discovery is unverified, gives no steps, and warns that Studio may demand a firmware upgrade that the personality answers with `0xC9`. That is a dead end for the Studio path, and the failure map only says "do not rely on the upgrade".
**Fix:** Either add a minimal Studio procedure (connect, select `AVRISP mkII`, choose the part, program; if Studio demands an upgrade, stop and use `avrdude`), or demote the row to "Atmel Studio compatibility note — not a supported host path" and add a clear "use `avrdude` instead" recommendation. Do not list Studio as a covered host without a next step.

**7. [severity: major] The host setup assumes tools that were never introduced.**
**Location:** `README.md:54-56`; `host-tools.md:36-42`.
> `README.md:54-56` — **USB route:** the device re-enumerates as `AVRISP mkII`; program with `avrdude -c avrispmkII -P usb -p m328p -U flash:w:app.hex:i`.  Host drivers and permissions are in [host-tools.md](host-tools.md).
> `host-tools.md:36` — ## avrdude

**Problem:** The set never says what to install. An engineer with a fresh laptop needs an `avrdude` build that supports libusb (the Windows build is not automatically that), Python 3 + pyserial for the entry snippet, and a terminal program. The page talks about "a libusb-based `avrdude`" without saying how to tell whether the installed one is. "No minimum `avrdude` version is claimed" is honest but leaves the prerequisite unstated.
**Fix:** Add a short "Prerequisites" table at the top of `host-tools.md`: `avrdude` with libusb support, Python 3 + pyserial (or the built-in PowerShell path), a terminal, and the OS driver. Include one check command (`avrdude -c avrispmkII -P usb -p m328p -n`) and say what a successful check looks like. This is what makes the setup executable rather than assumed.

### Minor

**8. [severity: minor] The wiring page owns the pin numbering but calls it a convention and leaves the supply pins unconfirmed.**
**Location:** `wiring.md:4-5`; `wiring.md:22`; `wiring.md:27-29`.
> `wiring.md:4-5` — This page owns the full wiring tables **and the v4 header pin numbering** that the ADC and AUX sets quote
> `wiring.md:22` — | 9–11 | `Vpu`, `+3V3`, `+5V` (reference/supply; order within 9–11 unconfirmed) |
> `wiring.md:27-29` — Locate a pin by its silkscreen name where the board prints one; the pin numbers are this set's published convention for the v4 header.

**Problem:** For a page whose first line claims the pin numbering, "this set's published convention" and "unconfirmed" are a trust problem. I have a board; I want a pin map, not a documentation convention. The personality only uses named pins, so the numbers are doing more harm than good here.
**Fix:** Confirm the header against the board and state the numbering as fact, or drop the numeric column for this personality and use a silkscreen-name-only table with one caveat. At minimum, make pins 9–11 three separate rows with an explicit "order unconfirmed" note rather than a range row.

**9. [severity: minor] PDI is given two different clock rates without saying which applies when.**
**Location:** `pdi.md:76-77`; `pdi.md:102-103`.
> The rate is about 25 kHz; the operator does not need to do anything for this.
> **No host speed setting.**  The PDI/TPI physical layer runs at about 125 kHz and the host cannot change it.

**Problem:** A reader sees the PDI clock line described as about 25 kHz and then about 125 kHz. They may be the idle keep-alive clock and the transfer rate, but the page does not say so; it reads like one of the numbers is wrong.
**Fix:** Say "the idle keep-alive clock is about 25 kHz; data transfers run at about 125 kHz" in both places, or use one number consistently if that is the real behaviour.

**10. [severity: minor] TPI says "5 V target supply" and "3.3 V logic" but does not say whether 3.3 V is enough.**
**Location:** `tpi.md:56-58`; `wiring.md:178-180`.
> `tpi.md:56-58` — The Bus Pirate drives 3.3 V logic; a 5 V target supply is not a 5 V Bus Pirate output.  Power the target at 5 V from its own supply and use suitable level translation where the logic levels demand it
> `wiring.md:178-180` — Use bidirectional level translation for targets whose logic thresholds or supply voltage do not match 3.3 V.  The series resistors in the PDI and TPI DATA join are **not** level shifters.

**Problem:** "Where the logic levels demand it" leaves the user to decide whether 3.3 V is within the ATtiny's VIH at a 5 V supply. For a path that is already "not verified", that is the one electrical question I need answered before trusting it.
**Fix:** State the requirement explicitly: either "3.3 V is within the target's VIH at 5 V; a level shifter is optional but recommended" or "use a 5 V level shifter for reliable NVM programming; 3.3 V is marginal/unverified". If the value is unknown, label it `unverified` right there rather than making the reader find a datasheet.

**11. [severity: minor] The standalone "Unverified" section is an audit ledger, not a failure table.**
**Location:** `edge-cases.md:84-92`.
> ## Unverified
> - TPI is **not verified** on a physical target.
> - PDI is **not verified for every XMEGA part**.
> - The software-clocked end of the ISP speed ladder is approximate and is **not verified for every rate/page combination**.
> - Atmel Studio tool discovery and programming are **not verified** for this port.
> - AVR053 calibration is **not verified** on a physical target.

**Problem:** This is five verification statements with no trigger, symptom or recovery, at the end of the failure page. I had already seen every one of these caveats inline where it mattered; the section adds no action and reads like a note left for a reviewer.
**Fix:** Delete the section and keep each caveat next to the feature it limits. If a caveat needs a failure row, give it a trigger and a recovery (for example, "TPI on a real part: no response — treat as unverified, use ISP or a known-good programmer").

**12. [severity: minor] Several sentences are addressed to an auditor, not a user.**
**Location:** `sessions.md:16`; `host-tools.md:58-60`; `protocol.md:165-166`.
> `sessions.md:16` — The target-dependent bytes are marked as such; nothing below invents a target's flash contents.
> `host-tools.md:58-60` — Host-side behaviour varies by release; the command lines above are the standard mkII and STK500v2 invocations, not a claim about a specific `avrdude` build.
> `protocol.md:165-166` — For the ISP connection-status parameter the value `0x00` means *ISP ready*; there are no host-visible connection-fault bits, and this is deliberate.

**Problem:** "Nothing below invents", "not a claim", and "this is deliberate" are defensive notes. They make me wonder what was being audited; they do not help me use the programmer. The same is true of the repeated "preserved deliberately" phrasing around CRC byte order.
**Fix:** Rewrite in plain user terms or delete. "Target-dependent bytes are marked" is enough; "output varies by `avrdude` release" is enough; "there are no connection-fault bits; `0xA1` reads `0x00` when ISP is ready" is enough.

**13. [severity: minor] "LUFA/Table sequence" is unexplained.**
**Location:** `pdi.md:85-87`.
> **Section and page erases follow the LUFA/Table sequence.**  The XMEGA AU manual is not consistent on application/boot section erase; this personality keeps the sequence LUFA uses and does not add a mode switch.

**Problem:** I do not know what "Table" means here, and "LUFA/Table" reads like an internal symbol or a file name. The sentence is trying to tell me which erase order is used, but it sends me to the source instead.
**Fix:** Say "Section and page erases follow the same order as LUFA's reference implementation" and state the observable order (application page then boot page, for example), or drop the implementation name and say what the command does.

**14. [severity: minor] The TPI erase types are never enumerated.**
**Location:** `protocol.md:221-222`; `tpi.md:92-94`.
> PDI supports erase types 1–8 and rejects type 9.  TPI treats type 1 as chip erase and every other accepted type as a section erase.
> Type `1` is a chip erase; every other accepted erase type uses the section-erase command.

**Problem:** "Every other accepted type" is circular: it does not say which types TPI accepts. A custom host cannot validate a TPI erase request from this. PDI's range is explicit; TPI's is not.
**Fix:** Give the TPI accepted range in the same form: "TPI accepts types 1–8; type 1 is chip erase, 2–8 are section erase" (or the actual set), and say which types are rejected before the target is touched.

**15. [severity: minor] The "worked" ISP session cannot be typed because the enable delays are prose.**
**Location:** `sessions.md:161`.
> | enter ISP | `10` + timeout, stabilisation, execution, sync loops, byte delay, `53`, `03`, `AC 53 00 00` | `10 00` on success, `10 C0` on failure | `SCK` is latched here |

**Problem:** The table gives names for the first five request bytes but no values, so the row is a shape, not a worked request. The text says the delay fields are host choices, but a reader trying to reproduce the session has nothing to send.
**Fix:** Give one concrete valid request (for example `10 C8 64 64 20 00 53 03 AC 53 00 00`) and note that the delays only need to be long enough, or relabel the section "Request shape" and point to `avrdude` for a real run.

**16. [severity: minor] `COM5` is used as the serial example without the Linux/macOS substitution.**
**Location:** `README.md:58`; `serial-route.md:41`; `entry.md:63`.
> `README.md:58` — `avrdude -c stk500v2 -P COM5 -p m328p -U flash:w:app.hex:i`
> `entry.md:63` — use the port the host lists (`/dev/ttyACM0` on Linux, a `/dev/cu.usbmodem` name on macOS).

**Problem:** The README and `serial-route.md` present `COM5` as the command to copy. The Linux/macOS names appear only in `entry.md`. A non-Windows reader can follow the wrong command and get "port not found".
**Fix:** Add a one-line note under the command table: "`COM5` is a Windows example; use `/dev/ttyACM0` on Linux or the `/dev/cu.usbmodem…` name on macOS." Or use `<port>` in the command and give the three forms once.

**17. [severity: minor] The binary-layer path in `entry.md` is plain code, not a link.**
**Location:** `entry.md:181-182`.
> - `docs/protocol/binary/entering-and-framing.md` — the shared binary layer (`BBIO1`, result bytes, `0x0F`).

**Problem:** This is in a "Related docs" list, but it is the only item that is not a Markdown link. I had to navigate to the folder and find the file by hand; it is exactly the page I wanted when the personality ended unexpectedly.
**Fix:** Make it `[docs/protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md)` or point to the binary README that already exists.

**18. [severity: minor] The two end-of-mode diagrams have misaligned annotations.**
**Location:** `entry.md:155-160`; `sessions.md:186-192`.

`entry.md:155-160`:

```
personality ends -> "BBIO1"        (binary bitbang layer)
host  -> 0x0F
host  <- 0x01
                    banner + "HiZ>" (terminal)
```

`sessions.md:186-192`:

```
host <- 42 42 49 4F 31          "BBIO1" on the returned CDC port
host -> 0F
host <- 01
          the terminal banner and "HiZ>" follow
```

**Problem:** The annotation columns are hand-spaced and do not line up; the last line of each block is an indented note rather than a labelled step. It reads like a capture that lost its formatting, and it is the one place where I had to stop and work out which line the note belonged to.
**Fix:** Convert each to a three-column table (actor, bytes, meaning) or align the annotations and label the final note as "then:". A table would also be easier to copy.

**19. [severity: minor] Two long bold-lead bullet sections should be tables.**
**Location:** `pdi.md:74-91`; `protocol.md:241-257`.
> `pdi.md:74-77` — ## Operational notes / - **PDI needs a running clock.**  The XMEGA PDI interface times out if PDI_CLK is idle for too long, so the firmware keeps a clock on the clock line between commands and during long reply transfers.  The rate is about 25 kHz; the operator does not need to do anything for this.
> `protocol.md:241-243` — ## XPROG properties / - **CRC reply order** is high byte, low byte, middle byte — LUFA's order, preserved deliberately, not a normalised ordering.

**Problem:** These are six or seven mini-paragraphs in a row. They are scannable only by reading every bold lead-in; a table with "Topic | Behaviour" would let me find the one item I need (erase order, CRC byte order, clock) without reading the rest.
**Fix:** Convert each section to a two- or three-column table: `Topic | Behaviour | Limit/caveat`. Keep the prose only for the one item that needs a full explanation.

**20. [severity: minor] The custom-build switch is never named in the user set.**
**Location:** `README.md:85-86`; `entry.md:19-21`.
> `README.md:85-86` — Add the AVRISP-MKII switch to the custom feature list, or use the default v4 feature set, to keep it.
> `entry.md:19-21` — A custom build that adds the personality to its feature list has it; the build route is [dev/building.md](../../dev/building.md).

**Problem:** I am told to "add the AVRISP-MKII switch" but not what it is called. The dev page has `BP_ENABLE_AVRISP_MKII_SUPPORT`; I had to leave the user set to find it. For a build-availability question, that is an unnecessary hop.
**Fix:** Name the switch once in `entry.md` §Build availability: "the custom-build switch is `BP_ENABLE_AVRISP_MKII_SUPPORT`" and keep the link to `dev/building.md` for the build procedure. If the switch name is not stable, say "the AVRISP-MKII feature switch (see the build page)".

**21. [severity: minor] The AVR053 "remove SCK" step is presented as required and then called optional.**
**Location:** `rescue-clock-osccal.md:99-106`; `edge-cases.md:60`.
> `rescue-clock-osccal.md:99-106` — With the mode inactive, remove the `SCK` wire to match the AVR053 reference wiring; leave `MOSI`, `MISO`, `RESET` and ground in place. ... Step 3 is a reference-wiring convention, not a firmware requirement — the personality stops driving `CLK` for the command either way.
> `edge-cases.md:60` — SCK still connected during OSCCAL | nothing differs — CLK is not driven by the command | the AVR053 reference wiring leaves SCK off; a connected SCK is a wiring-convention difference, not a fault

**Problem:** The numbered procedure tells me to remove a wire, then immediately says the step is optional. I do not know whether leaving `SCK` connected is safe, merely cosmetically wrong, or a risk. The edge-case row says "nothing differs", which makes the step look pointless.
**Fix:** Mark step 3 "Optional — reference wiring only" and state plainly that leaving `SCK` connected does not affect the command. If the reference wiring matters for a reason the firmware does not enforce (for example, the target's own schematic), say why; otherwise remove the step.

**22. [severity: minor] The serial-route page sends PDI/TPI readers back to the ISP page for entry.**
**Location:** `serial-route.md:23-24`.
> For PDI/TPI with `avrdude` and for the mkII USB identity, use the USB route ([isp.md](isp.md) §Entry).

**Problem:** A PDI/TPI reader arrives on the serial page, is told to use the USB route, and the link lands on `isp.md` §Entry. The entry information is generic and also lives in `entry.md`; following this link is a loop through the ISP page before reaching PDI/TPI.
**Fix:** Link to `entry.md` for the USB entry byte and deadline, or directly to `pdi.md` / `tpi.md` for the target path. Keep the serial page focused on the ISP serial route.

## What to keep

- The README's opening paragraph and 13-row file table. I knew what the feature was, which build had it, and where to go in under a minute. The "map, not the manual" framing is accurate.
- The explicit name mapping: `AVRISP-MKII` (feature), `AVRISP mkII` (USB product), `AVRISP_MK2` (USB `SIGN_ON`), `STK500_2` (serial `SIGN_ON`). This removed the biggest potential confusion before it started.
- The wiring page's "two naming layers" paragraph and the 6-pin ISP table. I could wire `CLK`→`SCK`, `MOSI`→`MOSI`, `MISO`→`MISO`, `AUX`→`RESET`, `GND`→`GND` without translating names in my head, and the warning that `CS` is the rescue clock, not chip select, is exactly the kind of thing that prevents a bad connection.
- The five-second deadline warning repeated at every entry point. I did not enter the mode and then go looking for a command.
- The PDI/TPI difference set: DATA join through 220 Ω, PDI clock on the target's RESET/PDI_CLK, PDI leaves AUX open, TPI holds AUX/RESET low, TPI needs 5 V. Those are the four things I actually needed, and they are stated in three places consistently.
- The `edge-cases.md` trigger → symptom → recovery format and the `host-tools.md` host-side failure map. These are the pages I would keep open at the bench.
- The command-model state table and the failure-behaviour table. I understood what the personality remembers between commands and what a failed enable leaves behind (`RESET` asserted) without reading the protocol catalogue.
- The serial-route AVR068 worked frame. I checked the checksums and the byte counts and they add up; that gave me confidence in the rest of the byte tables.
- The `-B 8` vs `-B 125` explanation. It is repeated, but it is the classic mistake and the repetition is justified.
- The honesty about what is unverified (PDI parts, TPI, Atmel Studio, AVR053). I would rather know before I wire a target.

## Structural opinion

The layout is right for this function. A README map, a separate entry/lifecycle page, one wiring page, one page per target interface, a host-tools page, a protocol reference, a state model, worked sessions, edge cases and a licence page is the correct decomposition. It avoids the usual 600-line README and keeps the target paths parallel. The "Related docs" footer on every page is a good navigational habit.

The one structural gap is a **first-run runbook**. The README's "five-minute version" is a summary, not a procedure: it points at `entry.md`, then `host-tools.md`, then `wiring.md`, then `isp.md`. A fresh user has to bounce. Add a single `quickstart.md` (or a top section in `host-tools.md`) with two tracks — Windows USB and serial/ISP — and a decision table at the top:

| Host / route | First step | Driver | First command |
|---|---|---|---|
| Windows, USB | enter `0x08`, bind WinUSB, retry if it drops | Zadig / WinUSB | `avrdude -c avrispmkII -P usb ...` |
| Windows, serial ISP | enter `0x09` | normal CDC driver | `avrdude -c stk500v2 -P COM5 ...` |
| Linux, USB | enter `0x08`, install udev rule | udev | `avrdude -c avrispmkII -P usb ...` |
| macOS, USB | enter `0x08` | none | `avrdude -c avrispmkII -P usb ...` |

That one page would also solve the Windows driver catch-22 and the missing prerequisites without bloating the README (which is already at 121 lines).

Second structural proposal: fold the `edge-cases.md` "Unverified" section into the rows and pages it belongs to. The set currently repeats the same caveats in the README availability table, the page limits, and the edge-cases ledger. One inline caveat per feature is enough; the ledger reads like an audit appendix and adds length without action.

Third: if `protocol.md` grows, split the USB transport narrative from the command/parameter catalogue. At 284 lines it is still usable, but the command table is the densest part and a reader looking for "how do I send a request" has to pass the whole catalogue first.

Fourth: in `serial-route.md`, send PDI/TPI readers to `entry.md` or the PDI/TPI pages, not `isp.md §Entry`. That is the only navigation loop I hit: the serial page is the ISP route, so a PDI/TPI reader following it lands on another ISP page.

## Unknowns

- **Firmware behaviour.** I did not test the personality on a board. Availability in a specific v4 image, the actual five-second timing, the real speed ladder, the PDI/TPI signal integrity, and the 32 ms calibration window are all taken from the docs. The docs themselves label several of these unverified, which is the right posture, but I cannot confirm them.
- **Board pinout.** I did not have a v4 to check the 2×6 header numbering or the silkscreen. The page's own "published convention" and "order within 9–11 unconfirmed" caveats are the only evidence I have; I cannot tell whether the used pins 1–5, 8 and 12 are correct.
- **Windows driver timing.** I could not determine whether a first-time Zadig/WinUSB bind can complete inside the five-second window, whether Windows caches enough of a failed install to finish on the next enumeration, or whether `avrdude` waits for the mkII device. This is the core of finding 1; the docs should not depend on the answer being favourable.
- **Atmel Studio acceptance.** The docs say it is not verified; I could not test whether a particular Studio release accepts the clone or whether the firmware-upgrade prompt blocks it. That is why finding 6 asks for either a procedure or an explicit "not supported" label.
- **`avrdude` build/version.** The docs deliberately avoid a minimum version. I read the command lines as standard mkII/STK500v2 invocations, but I did not run them against a current `avrdude` 8.x build, and I did not verify that a Windows `avrdude` package includes libusb support.
- **Code snippets.** I read the Python/pyserial and PowerShell entry snippets but did not execute them. I did not verify the PowerShell `New-Object System.IO.Ports.SerialPort COM5,115200,None,8,one` constructor on a real Windows host, nor the pyserial `read(5)` timing.
- **AVR053 target firmware.** I did not have the AVR053 application firmware, so I could not judge whether its MISO output produces the eight level changes the personality expects within 32 ms. The wiring and timing description is internally consistent, but the target side is out of reach from the docs alone.
- **External link.** I could not reach `https://www.lufa-lib.org` from this environment; the licence text is reproduced in `license.md`, so this does not block reading, but the link may be stale. A GitHub project URL would be more durable.
