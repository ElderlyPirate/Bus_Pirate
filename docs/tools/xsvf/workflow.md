# XSVF workflow — from file to configured target

Part of the XSVF host-tool set (`docs/tools/xsvf/`).  This page is the
end-to-end sequence: get a file, wire the target, connect, run a host player,
watch the run, and leave the firmware player.  The player options are in
[players.md](players.md), the converter in [converter.md](converter.md), and
the failure cases in [limits-and-errors.md](limits-and-errors.md).

## Scope

A single-target CPLD/FPGA configuration run using one of the shipped host
players.  A multi-device chain uses `mergexsvf` first
([converter.md](converter.md)); the run steps are the same.  The firmware
protocol is not repeated here — see
[protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md).

## 1. Get the XSVF file

- Preferred: generate the XSVF directly with iMPACT for the device(s) in the
  chain.
- If you have only an SVF: convert it with `svf2xsvf502`
  ([converter.md](converter.md)).
- **Shift gate:** the largest `XSDRSIZE` (or `XSIR`/`XSIR2` length) in the
  file must be **≤ 400 bits**; divide by 8 and round up, the result must be
  50 or less (`ceil(value / 8) ≤ 50`).  The `-a` text listing shows the values
  ([converter.md](converter.md) §Checking the shift size).

Do not feed an `.svf` file to the host player: the firmware player reads
XSVF only.

## 2. Wire the target

| Bus Pirate | JTAG |
|---|---|
| MOSI | TDI |
| MISO | TDO |
| CLK | TCK |
| CS | TMS |
| GND | GND |

- **No pull-up needed**: the firmware player drives TDI/TCK/TMS push-pull.
  SRST and TRST are not used by the firmware player.
- **The firmware player powers the target.**  Entry (`0x18`) enables the
  on-board regulator, which feeds the VCC pin.  Do not also power the target
  from its own supply while the player runs; remove that supply first.
- Logic level is 3.3 V; the interface is 5 V safe (5.5 V maximum).  Do not
  drive a lower-voltage chain push-pull — see
  [wiring.md](../../protocol/jtag/wiring.md).

## 3. Connect and prepare

1. Connect the Bus Pirate by USB; note the serial port.
2. Close the terminal program and any other program using the port.
3. Leave the board at the terminal prompt (`HiZ>`).  If it is in a menu or
   still in the firmware player, press reset.
4. Check the target's JTAG header pin 1 and ground before applying power.

## 4. Run a host player

Windows:

```
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f design.xsvf -r
```

Linux (cross-platform build):

```
$ BPXSVFplayer -p /dev/ttyACM0 -s 115200 -f design.xsvf -r
```

- `-p` is the port; `-s 115200` is the normal speed; `-f` is the XSVF file.
- Add `-x` if you want the chain scan before the run.  It is informational;
  the file run is what configures the target.  On the portable build `-x` has
  no overall limit and can wait indefinitely on Linux
  ([players.md](players.md)).
- Add `-r` to reset the chain first.  It is a good default before a run.
- The host tool enters `BBIO1` and `XSV1` itself; do not pre-enter them.

## 5. Watch the run

A clean Windows run of the shipped sample looks like this (banner and chunk
lines elided; the full transcript is in [players.md](players.md)):

```
> BP4_Full_bitbang_XSVFplayer_Win.exe -p COM5 -s 115200 -f xc95test.xsvf -r
 Entering binary mode ... ok
 Switching to XSVF mode ... ok
 Performing Reset..
 Done

 File is 53289 bytes,  Read 53289 bytes
 Opening Bus Pirate on COM5 at 115200bps, using XSVF file xc95test.xsvf
 Entering XSVF Player Mode
 Waiting for first data request...ok
 Sending 4096 Bytes (1000)...ok
 ...
 Sending 41 Bytes (D029)...ok
 End of operation reply: XSVF_ERROR_NONE
 Success!
 Thank you for playing! :-)
```

The full line-by-line reading is in
[players.md](players.md#reading-the-output).  For the run, the line that
matters is `End of operation reply:` followed by `Success!` (no error); the
host tool then exits.

**The tool finishing is not the firmware player finishing.**  `Thank you for
playing!` means the host stopped.  A long `XRUNTEST`/`XWAIT` can outlast the
host's patience, and the firmware player may still complete the run, so the
target may already be configured; `0x0F` sent during the wait aborts it.
Wait, check the target, then reset before replaying.

## 6. Leave the player

**The host tools do not exit the firmware player.**  After a normal run the
board is still in the firmware player loop, and the on-board regulator is
still on (it feeds VCC).  After `End of file reached.` or
`No reply.... Quitting.` the player may instead still be inside the run.

1. **Press reset.**  This is the safe exit in every case and returns to the
   terminal.
2. Or send `0x0F` twice from a serial terminal or script — only after the
   firmware player has returned to its command loop (a result was printed, or
   the ~5 s stall has expired):
   - the first `0x0F` resets the chain and returns to the binary loop
     (silent — `BBIO1` is not reprinted);
   - the second `0x0F` returns to the terminal: it replies `0x01` and runs
     the terminal reset, which switches the regulator off.

Do **not** send `0x0F` while the firmware player is still waiting for a chunk
or busy in a timed wait: it is consumed as data or as a busy byte, and two
bytes can leave the board deeper in the player.  In the two non-result cases:

| After the tool stopped at | What the firmware player is doing | Exit |
|---|---|---|
| `End of file reached.` | waiting for a chunk | press reset; or wait for the ~5 s stall, then `0x0F` twice |
| `No reply.... Quitting.` | possibly still running | do not replay; wait/check, then press reset |

Do not start another run first: the firmware player recognises `0x00` only at
its command loop, so the NUL entry cannot reach `BBIO1` until the board is out
of the player.

Once back at `HiZ>`, the target's own supply can be reconnected (if it has
one).

## 7. If it fails

The tool prints a result name; the mapping and the recovery actions are in
[limits-and-errors.md](limits-and-errors.md).  The most common causes are a
file for a different chain, swapped TDI/TDO or TCK/TMS, a missing ground, or
the target's VCC not reaching it — the firmware player feeds VCC on entry;
check the VCC pin, ground and the regulator (the terminal power-supply check
reports `VREG too low, is there a short?`).

## Related docs

| Doc | Why |
|---|---|
| [players.md](players.md) | options, serial settings, full transcripts |
| [converter.md](converter.md) | producing and checking the XSVF file |
| [limits-and-errors.md](limits-and-errors.md) | limits, messages, recovery |
| [protocol/jtag/xsvf.md](../../protocol/jtag/xsvf.md) | the firmware player and stream framing |
| [protocol/jtag/wiring.md](../../protocol/jtag/wiring.md) | the full pin map and levels |
| [../../protocol/binary/entering-and-framing.md](../../protocol/binary/entering-and-framing.md) | `BBIO1` entry and framing |
