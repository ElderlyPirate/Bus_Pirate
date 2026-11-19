# AVRISP-MKII — edge cases and failure modes

Part of the AVRISP-MKII documentation set.  Each entry is a concrete trigger,
the observable symptom, and the recovery.

## Entry and transport

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x08` / `0x09` sent from the terminal, not from `BBIO1` | nothing happens; the byte is terminal input | reach binary bitbang mode first (reply `BBIO1`), then send the entry byte — see [entry.md](entry.md) |
| One-argument serial write in PowerShell | ASCII `8` (`0x38`) is sent, not byte `0x08`; the binary layer answers `0x00` | use the byte-array form from [entry.md](entry.md), e.g. `$sp.Write([byte[]]@(0x08),0,1)` |
| Expected a terminal mode name / `AVRISP>` prompt | there is none | this personality is entered only by a binary entry byte; see [protocol.md](protocol.md) |
| Host goes quiet mid-session | after about five seconds the personality exits; the USB CDC port re-appears (the serial-route port stays open) | the binary layer prints `BBIO1`; send `0x0F` for the terminal, or `0x08`/`0x09` to re-enter — see [entry.md](entry.md) |
| Empty OUT packets or an idle handle expected to keep the session alive | the session still times out | they do not renew the idle timer; only a completed request does on the USB route (any received byte renews it on the serial route) |
| The mode ended while `avrdude` was still starting | `avrdude` cannot open the device, or the device has changed identity | prepare the `avrdude` command first; the five-second clock starts at entry, so enter the mode only just before running it |
| First-time Windows driver binding does not finish inside the five-second window | the mkII device disappears and the CDC port returns; the binding is incomplete | re-enter with `0x08` and repeat the one-pass binding; a completed binding is normally kept.  For ISP first use, use the serial route (`0x09`), which needs no libusb driver.  See [host-tools.md](host-tools.md) §Windows USB first use |
| Sending `0x08` or `0x09` on a v3 build | failure byte `0x00`, then `BBIO1` | v3 does not contain the personality; it is v4 only |
| Sending either byte in a v4 all-features custom build | failure byte `0x00`, then `BBIO1` | that fixed feature list omits AVRISP-MKII; add `BP_ENABLE_AVRISP_MKII_SUPPORT` to the feature list and rebuild — see [entry.md](entry.md) §Build availability |
| Sending `0x0F` while the personality is active | the mode does not end; the byte is protocol input | on the USB route it is an unknown V2 command and is answered `0x0F C9`; on the serial route it is ignored outside a frame.  Let the deadline expire or reset the board |
| `0x08` and `0x09` confused | USB route without re-enumeration, or serial route with a missing port | `0x08` gives the USB mkII identity; `0x09` keeps the CDC port and uses AVR068 framing |
| A second `avrdude` run started after the deadline | the tool cannot find the device | re-enter with `0x08`/`0x09`; a second run inside the window is allowed only while the link stayed configured |
| Two commands concatenated into one USB transfer | the whole transfer is answered `C0` | send one command per transfer; the length is validated before anything is touched |

## Host and driver

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| USB route without a bound driver | `03EB:2104` enumerates but `avrdude` cannot open it | bind a libusb/WinUSB driver with a tool such as Zadig (`AVRISP mkII` interface); the first binding may need more than one entry — see [host-tools.md](host-tools.md) §Windows USB first use |
| USB route on Linux without a udev rule | `avrdude` fails with a permission error on `03EB:2104` | add the rule in [host-tools.md](host-tools.md), reload udev, re-plug |
| An Atmel/Studio driver owns the interface | a libusb-based `avrdude` cannot open the device | bind the driver that matches the host tool you use; only one driver can own the interface |
| Serial route used while the terminal still holds the port | `avrdude` cannot open the COM port | close the terminal / release the port before running `avrdude` |
| Host closes the CDC port while the serial route is active | replies stop and the mode ends | re-open the port and re-enter; the `BBIO1` identifier may have no reader and is lost |
| `-c stk500v2` used for a PDI or TPI target with `avrdude` | XPROG is never enabled; the target does not initialise | use `-c avrispmkII -P usb` for PDI/TPI; the serial route is ISP with `avrdude` |
| Host disables the USB port or refuses to enumerate | no device and no COM port appear | the firmware cannot force enumeration; restore host enumeration and retry — no Bus Pirate power cycle is needed once enumeration returns |
| Atmel Studio offers a firmware upgrade | the upgrade command is not implemented | do not accept it; use `avrdude` |
| Host sees `AVRISP mkII` but no COM port | normal for the USB route | the CDC port is absent while the mkII personality is active; it returns when the mode ends |

## ISP target

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| ISP enable is not answered by the target | host reports a failed program-enable, then `Unknown status 0x00` from its secondary check | real cause is no target contact; the status byte is not a firmware fault.  Check wiring, target power and the target clock |
| Data wired to the ordinary SPI pins | no contact on an ATmega whose programming pins differ from SPI | use the target's serial-programming pins (for example PE0/PE1 on an ATmega64A), not PB2/PB3 — see [wiring.md](wiring.md) |
| `-B 125` used believing it means 125 kHz | SCK is 125 µs = 8 kHz | the `-B` number is a period in µs; use `-B 8` for 125 kHz |
| Full-page write at the slowest software rates | command fails with a timeout | the one-second target budget is exceeded; use small blocks with a custom host |
| `RESET` release expected to be driven | the line floats high via the target's own pull-up | release is tristate by design; provide a target reset pull-up if needed |
| ISP enable fails and the host does not send `LEAVE` | the target stays in reset | send `LEAVE_PROGMODE_ISP` or let the mode end; a failed enable leaves `RESET` asserted |
| SCK duration changed after the ISP enable | the clock does not change | the speed is latched at `ENTER_PROGMODE_ISP`; set it before enabling, or `LEAVE` and re-enter |
| SCK duration set above 163 | the parameter reads back unchanged, but the clock is about 51 Hz | the engine clamps the index to 163; use a valid index |
| A fuse write makes the target stop answering ISP | every later session fails to enable programming | a wrong clock-source or reset-disable fuse; restore it with a high-voltage programmer, which this personality is not |

## Clock and voltage

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Target fused for an external clock/crystal that is not running | ISP enable fails even though SCK is driven | wire `CS` → target `XTAL1` and retry; see [rescue-clock-osccal.md](rescue-clock-osccal.md) |
| `CS` wired to `RESET` or `XTAL2` | the target is held in reset or the clock fights an oscillator | move the wire to `XTAL1`; `CS` is not a reset or chip-select line here |
| `OSCCAL` requested without AVR053 firmware on the target | the calibration command times out after about 32 ms nominal | AVR053 requires the target calibration firmware; it is not installed by this personality |
| `OSCCAL` with the target holding `RESET` low | the calibration cannot start; timeout | release the target's reset (reset pin enabled, no external pull-down); the AVR's internal pull-up is enough on a normal part |
| `SCK` still connected during `OSCCAL` | nothing differs — `CLK` is not driven by the command | the AVR053 reference wiring leaves `SCK` off; a connected `SCK` is a wiring-convention difference, not a fault |
| VTARGET reads a nonsense value | the `ADC` pin is floating | VTARGET is informational and is only meaningful if the target supply is wired to the `ADC` pin |
| VTARGET reads low under load but normal when idle | a real supply sag, not a firmware problem | check the target supply's current limit; the programmer does not power the target |
| Target supply below its operating minimum | unstable or failing programming | bring the target supply into its datasheet range before programming |
| A 5 V or 12 V supply rail connected to a signal pin | risk of damage | never do this.  The header inputs are 5 V class, so a 5 V logic signal is allowed, but a supply rail is not; never allow a 12 V reset on any pin. |

## PDI and TPI

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Target reset button pressed during PDI | the PDI clock is disturbed; the session fails | do not touch the target reset while programming (it is PDI_CLK) |
| PDI clock left idle between commands | the target's PDI interface would time out | the firmware keeps a clock running automatically; nothing to do |
| `AUX` wired for PDI | unnecessary; it can hold or disturb the target's reset if a previous ISP state left it driven | leave `AUX` open for PDI; follow [command-model.md](command-model.md) §Changing target interface before switching target type |
| Damaged PDI/TPI link, NVMEN will not clear | bounded cleanup cannot reset the target interface | power-cycle the target; the firmware cannot recover an electrically inaccessible device |
| Failed XPROG enable | reply is FAILED but the interface can stay enabled | send `XPROG LEAVE_PROGMODE`, or let the mode end, before rewiring |
| Interface switched without `LEAVE` | the previous interface's pin state can persist | finish the session with the matching `LEAVE` before changing target type; see [command-model.md](command-model.md) §Changing target interface |
| TPI write with an odd byte count | accepted; the last byte is padded with `0xFF` | this is normal — an odd count is not an error |
| TPI write with an odd start address | the request is rejected (FAILED, result `1`) | TPI addresses must be even; align the write to a word |
| TPI write or read past `0xFFFF` | the request is rejected | TPI has a 16-bit address space; split or truncate the operation |
| XPROG CRC requested on TPI | a failure status, not data | TPI has no memory CRC |
| TPI target not powered at 5 V | no programming; the target may answer nothing | the tinyAVR NVM requires a 5 V target supply; power it externally and level-shift as needed |
| TPI part with `RSTDISBL` set | the target cannot be enabled without 12 V on `RESET` | unsupported; a 12 V reset must never reach a Bus Pirate pin |
| TPI target does not answer at all | no response on a physical TPI part | TPI is **not verified** on a physical target for this port.  Check the wiring and the 5 V supply first, then use ISP or a known-good programmer. |
| Attempting a locked/protected read | the read fails, or the returned data cannot be compared | the target's protection decides the result; unlock/erase the part if its protection settings allow it |

## Related docs

- [entry.md](entry.md) — the lifecycle these entries refer to.
- [wiring.md](wiring.md) — the wiring behind most of these rows.
- [protocol.md](protocol.md) — status codes and request validation.
- [command-model.md](command-model.md) — the session states and cleanup.
- [isp.md](isp.md) — fuses, speeds and host setup.
- [host-tools.md](host-tools.md) — drivers and permissions.
