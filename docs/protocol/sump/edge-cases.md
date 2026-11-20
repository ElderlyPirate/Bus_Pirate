# SUMP — edge cases and failure modes

Part of the SUMP set (`docs/protocol/sump/`).  Each entry is a concrete trigger,
the observable symptom, and the behaviour or recovery.  Entry and exit detail
is in [entry.md](entry.md); the arithmetic behind the depth and rate rows is in
[depth.md](depth.md) and [rates.md](rates.md).

## Entry and exit

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| twenty NULs sent, then the entry byte | `BBIO1` followed by `I2C1`, not SUMP | the twentieth NUL already entered the binary layer; send `0x00` then `0x0F`, and retry with five to nineteen NULs |
| one to four NULs, then `0x02` | cursor moves left, or a bell at the line start | fewer than five NULs were counted; send five to nineteen NULs immediately before the `0x02` |
| a typed left-arrow (`ESC [ D`) used as the entry | cursor moves; no entry | the `ESC` resets the NUL count before the sequence is decoded; send the raw `0x02` |
| NULs, then any other byte (Enter, a command, `ESC`) | no entry; the count is gone | the count survives further NULs only; send the run immediately before the `0x02` |
| five NULs + `0x02` sent twice | two `1ALS` replies | the first is the entry broadcast, the second is an ordinary ID request; harmless |
| a stray `0x02` after a normal exit | the cursor moves; no re-entry | the exit resets the entry counter; a fresh five-NUL prefix is required |
| ordinary typed characters while in SUMP | nothing happens; the board stays in SUMP | short bytes below `0x80` are ignored; `#`, `$` and `0x0F` are the exits |
| `$` expected to enter the bootloader | the board returns to the terminal instead | `$` is an exit here; type `$` again at the `HiZ>` prompt and answer its confirmation |
| `0x00` expected to leave SUMP | the analyzer resets but stays | `0x00` is the in-SUMP reset; use `0x0F`, `#` or `$` |
| the exit byte sent in the middle of a long command | no exit; the command completes and takes effect | the exit byte is a parameter; send the exit again after the command completes.  A stalled command is discarded by the one-second gap or five `0x00` resets |
| `#` typed then Enter | two `HiZ>` prompts | the `#` leaves SUMP; the Enter that follows is an empty terminal command |
| a client closes the port without an exit | no prompt; the board is still in SUMP | open a terminal and send `#`; the client does not have to exit cleanly |
| the board seems unresponsive | no prompt, no replies | send any exit byte; power-cycle if the link itself is lost |

## Rates

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a 3 MHz request | the capture runs at 4 MHz | requests between engine points select the fixed engine; use the achieved rate in the timing maths |
| a 1.5 MHz request | the capture runs at 2 MHz | same rule: the 2 MHz engine is the first point above about 1.06 MHz |
| a 1.05 MHz request | the capture runs at 1 MHz | the timer band starts at divider 94; the 2 MHz engine starts at divider 93 |
| a request slower than about 6 Hz | the capture runs at a different rate | only the low 24 bits of the divider fit the command; the request wraps |
| a request for exact spacing at 4 or 8 MHz | up to one cycle of jitter per pass (up to two on the v3 4 MHz loop) | the long-term rate is exact; use 1 MHz, 2 MHz or 16 MHz when every interval must match |
| no divider sent | a 4096-sample capture at about 9.994 kHz, not 1 MHz | every reset restores the 1601-tick default timer period; no divider is set, so send `0x80` before every capture |
| a trigger wait longer than the 32-bit sample counter | no visible symptom; the reply length, order and pre-trigger split are unchanged | the counter is rebased at a whole number of ring revolutions; no action needed ([rates.md](rates.md#counter-rollover)) |
| a 16 MHz capture expected to last | the window is at most 128 µs on v4 | the fast buffer caps the count at 2048; use a timer rate for a longer window |
| the rate set before a capture seems lost | the next capture uses the default rate | the analyzer resets after every capture; re-send the divider, counts and mask |

## Depth and channels

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| an 8192, 16 384 or 32 768 count | only the low 4, 2 or 1 channel bits carry data | larger captures trade channels for depth; the other bits show the alternating pattern |
| a trigger set on a channel the depth drops | the trigger fires, but that channel is absent from the reply | the mask is independent of the recorded set; choose a count that keeps the channel |
| an 8188 count at 1 MHz | 4096 real samples and 4092 padding bytes | the tier is chosen from the request; use 8192 to reach the packed tier |
| a count above 32 768 | a long reply that is mostly padding | the capture stays at 32 768; the surplus is padding |
| a fast capture with a deep count | the channel set is not reduced, but the reply is padded | fast captures record every channel and cap at the fast buffer |
| a 4096-sample request at 16 MHz on v4 | half the reply is padding | the v4 fast buffer holds 2048 samples; the v3 8/16 MHz paths are also capped at 2048 |
| a 4096-sample request at 2 or 4 MHz on v3 | the reply is full data | the v3 2/4 MHz paths store one byte per sample and hold 4096 |
| an unrecorded channel read as a signal | a regular alternating square wave | that is the `0x55` / `0xAA` pattern, not data |
| the oldest samples of a packed capture read low | recorded bits are all 0 | those samples were overwritten by the final word; they cannot be recovered |
| a real all-low sample at the oldest edge | looks the same as an overwritten sample | the protocol has no marker for the difference; shorten the window if it matters |

## Trigger

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a client set to a high or low level trigger | the capture fires on any edge of that pin | only the mask is implemented; there is no level match |
| trigger values or stages configured | no effect | the commands are consumed and ignored |
| a trigger edge arrives before the trigger arms (timer rates) | ignored; the capture keeps waiting | the edge detection is armed only after the first pre-fill burst; see [trigger.md](trigger.md#how-the-window-is-filled) |
| a selected pin that never changes | the capture waits forever; the MODE LED stays on | send any byte to abort, or `0x00` to reset; an edge is required |
| a mask with only empty bits (`0x40` on v4) | the capture runs without waiting | no wired pin is selected, so no trigger is armed |
| unconnected pins left in the mask | spurious triggers from noise | tie unused pins to a level, or remove them from the mask |
| a trigger position at 2 MHz or above | ignored; the window starts at the trigger | the fast loops cannot buffer ahead; use a timer rate for pre-trigger data |
| a delay larger than the window | clamped to the window; the trigger sits at the start | no post-trigger samples exist beyond the window |
| a delay of raw 0 | 4 samples after the trigger | the delay encoding has a minimum of four samples |
| several edges before the window is complete | the first accepted edge anchors the window | later edges do not retrigger or extend it |

## Client and host

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a profile for a different analyzer selected | the device is not detected, or the capture fails | use the SUMP/OLS-compatible profile; the device reports `1ALS` |
| a client that expects `SLA0` or `SLA1` | the identification is rejected | the device sends `1ALS` by design; use the OLS/sigrok profile |
| a client that sends fewer than five reset bytes | no `1ALS`; the device seems dead | the terminal requires five; use a client or script that sends the standard five-reset initialisation |
| a client that sends twenty or more reset bytes | `BBIO1` instead of `1ALS` | the binary boundary; see the first row of this page |
| the client's sample-order setting changed | the trace is displayed backwards | samples are newest-first; leave the profile default |
| channel checkboxes cleared in the client | the capture still records those channels | the flags byte is ignored; the read count decides the recorded set |
| RLE, demux, filter or external clock requested | no effect | only the divider, counts and mask change the capture |
| a terminal and the client both target the port | one of them cannot open the port, or data is lost | only one program can own a serial port; close the terminal first |
| a client timeout shorter than the capture | the client gives up before the first sample | allow capture time plus trigger wait plus transfer; see [client.md](client.md#capture-time-and-host-timeouts) |
| a v3 readout at 115200 baud | a long pause after the capture | 4096 bytes take about 0.36 s; 32 768 bytes about 2.8 s |

## Readout and transfer

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| a byte sent during a timer-paced capture | the capture stops; no samples arrive | the capture stops at the next burst boundary (up to 2.7 s at the slowest rate); the byte is handled as a command; there is no partial readout |
| a byte sent during a fast trigger wait | the wait stops; no samples arrive | the byte is handled as a command |
| a byte sent during the fast post-trigger window | not polled; the window and the readout complete before the byte is handled | about 1 ms on v4 (2.05 ms for 4096 samples at 2 MHz on v3), then the readout (about 0.36 s for 4096 bytes at 115200 baud on v3) |
| a byte sent during the readout | it is handled after the last sample | one byte waits; on v3 a long burst can overrun the UART and lose bytes.  The loop clears the overrun before polling the next command (including the first pass), discards the partial command and capture configuration, and reads the next complete command; resend it.  No power cycle is needed ([commands.md](commands.md#parser-rules)) |
| the host stops reading | the device stalls mid-reply | back-pressure, not data loss; resume reading or reset |
| the first bytes of a reply are `0x55 0xAA` | they are padding, not samples | the request was larger than the capture; request a count the engine can supply |
| a reply expected to start with a header | the first byte is sample data | there is no header or length prefix; the read count is the length |
| an unimplemented command sent | no reply; the next bytes may be out of step | the command is consumed (one or five bytes); the one-second gap or five resets restore the parser |
