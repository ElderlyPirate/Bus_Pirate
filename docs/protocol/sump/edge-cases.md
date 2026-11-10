# SUMP logic analyzer — edge cases & failure modes

Part of the SUMP documentation set (`docs/protocol/sump/`). Each entry is a
concrete trigger, the observable symptom, and the behaviour or recovery.

## Entry and exit

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Twenty NULs sent, then the entry byte | enters binary bitbang (`BBIO1`), not SUMP | the 20th NUL already switched to the binary layer; exit with `0x0F` and retry with five to nineteen NULs |
| One to four NULs then raw `0x02` | cursor moves (or beeps at the line start); still in the terminal | fewer than five NULs were counted; send more NULs immediately before the `0x02` |
| A typed left-arrow (`ESC [ D`) used as the entry byte | does not enter SUMP; the cursor moves | the `ESC` resets the NUL count before the sequence is decoded; send the raw `0x02` instead |
| NULs arrive from a script or line noise, then any other byte (Enter, a command, `ESC`) | no entry; the count is gone | the count survives further NULs only; any other byte resets it, so send the NULs immediately before the raw `0x02`; exit an unexpected SUMP session with `#` |
| Normal command typed while in SUMP | no effect; the board stays in SUMP | SUMP has no protocol exit; use `0x0F`, `#` or `$` |
| Board left in SUMP | unresponsive to ordinary commands until reset | always leave with `0x0F`, `#` or `$`; otherwise power-cycle |
| `$` expected to enter the bootloader | returns to the terminal instead | `$` is treated as a plain reset here; use the normal bootloader path |
| An unknown first byte in SUMP | nothing happens | it is read as a long (5-byte) command and ignored |
| Host stalls partway through a long command | parser misaligned; later bytes swallowed as parameters | the in-flight command is abandoned after one second; or send the reset byte five times |

## Rates

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Rate above 16 MHz requested | not possible | 16 000 000 Hz is the maximum advertised rate |
| Rate between the five engines | mapped to one of the engines | the requested nominal rate is rounded to one of 1, 2, 4, 8 or 16 MHz |
| A reliable capture at 16 MHz | short window; no headroom | use a timer-paced rate (≤ 1 MHz) for a longer window; 1 MHz and 16 MHz are exact, 4/8 MHz have one cycle of per-pass jitter |
| Exact sample spacing required at 4 or 8 MHz | up to one cycle of per-pass jitter | the long-term rate is exact; use 1 MHz (or 16 MHz) when every interval must be cycle-exact |

## Depth and channels

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| 8k / 16k / 32k capture requested | only the lowest 4 / 2 / 1 channels carry data | larger captures trade channels for depth; the other channels show the alternating pattern |
| Clock channel expected in a 32k capture | channel absent (only one channel is kept) | capture at a smaller depth, or accept the reduced channel set |
| A capture above 2048 samples on v3 (the 4096 tier, 8k+, or a fast request) | exceeds the v3 2048-byte buffer | v3 large-capture behaviour is unverified; expect truncation or unstable data — stay within the 2048-byte buffer (2048 samples at 2/4 MHz, 1024 at 8/16 MHz) |
| Fast rate combined with a depth tier | the tier is not applied; the capture is capped at the fast capacity and keeps every channel | request no more than 2048 samples on v4 when using a fast rate |
| Sample count not a multiple of four | rounded by the encoding | SUMP encodes `(raw + 1) × 4` samples; counts are always multiples of four |

## Trigger

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Client set to a high/low level trigger | behaves as an any-edge trigger | only the channel mask is implemented; there is no level match |
| Trigger position set at 2 MHz or above | ignored; capture starts at the trigger | the fast loops cannot buffer ahead; use a timer-paced rate for pre-trigger data |
| Trigger on a channel that the chosen depth does not record | trigger fires, but that channel is absent from the capture | the trigger mask is independent of the recorded-channel set; choose a depth that keeps the channel |
| Trigger on bit 6 | no effect | bit 6 has no pin and cannot arm a trigger |
| Delay count larger than the window | clamped to the window | no post-trigger samples beyond the captured window |

## Client and buffer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| A host byte sent during a capture | a timer-paced capture stops and no samples are returned; at a fast rate the byte is checked only while the capture waits for its trigger (a post-trigger fast window is sub-millisecond) | the byte is handled as a command (for example `#` exits); an aborted timer-paced capture has no partial readout |
| Client sample-order/reverse setting changed | the trace is displayed backwards | samples stream newest-first; leave the SUMP/OLS profile default |
| Bus Pirate-specific driver/profile selected | device not detected, or the capture fails | use a SUMP / OLS-compatible profile (see [capture.md](capture.md)); the device reports `1ALS` |
| A terminal and the SUMP client both target the port | only one program can own a serial port at a time | the client opens the port and sends the entry sequence itself; close the terminal first |
| Client seeks RLE, flags demux/filter, external clock or trigger values | the option has no effect | those features are accepted or ignored, not implemented |
| Capture cannot supply the full requested count | the missing samples are padding | the host is always sent the requested number of bytes; padding alternates `0x55` / `0xAA` |
| Unrecorded channel bits read as data | channels show a regular alternating pattern | that pattern marks bits with no recorded pin, not a signal |

## Unverified paths

- The whole v3 column is build-only and **not verified**.
- The v3 4096-sample tier and larger, and the v3 fast engines, are
  **unverified**; the v3 buffer is 2048 bytes.
- The exact SUMP client driver/profile selection and the sample-order setting
  are **unverified** (the entry sequence the client sends is documented in
  [capture.md](capture.md) §Entry and exit; the firmware recognises it on the
  port regardless of which program sends it).
