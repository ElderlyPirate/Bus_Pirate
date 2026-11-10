# AVRISP-MKII — edge cases and failure modes

Part of the AVRISP-MKII documentation set (`docs/avrisp/`).  Each entry is
a concrete trigger, the observable symptom, and the recovery.

## Entry and transport

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| `0x08` / `0x09` sent from the terminal, not from BBIO1 | nothing happens; the byte is just terminal input | reach binary bitbang mode first (reply `BBIO1`), then send the entry byte — see `docs/generic/` |
| One-argument serial write in PowerShell | ASCII `8` (`0x38`) is sent, not byte `0x08`; BBIO replies `0x00` | use the three-argument byte-array form, e.g. `$sp.Write([byte[]]@(0x08),0,1)` |
| Expected a terminal mode name / `AVRISP>` prompt | there is none | this personality is entered only by a binary entry byte; see [protocol.md](protocol.md) |
| Host goes quiet mid-session | the personality exits after a few seconds; the USB CDC port re-appears | re-enter with the entry byte; a hardware reset always leaves the mode |
| Empty OUT packets or an idle handle expected to keep the session alive | the session still times out | they do not renew the idle timer; only real host requests do |
| Sending `0x08` or `0x09` on a v3 build | failure byte `0x00` | v3 does not contain the personality; it is v4 only |

## Host and driver (Windows)

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| USB route without a bound driver | `03EB:2104` enumerates but `avrdude` cannot open it (driver problem) | bind a libusb/WinUSB driver once with a tool such as Zadig (`AVRISP mkII` interface) |
| Serial route used while the terminal still holds the port | `avrdude` cannot open the COM port | close the terminal / release the port before running `avrdude` |
| `-c stk500v2` used for a PDI or TPI target | XPROG is never enabled; the target does not initialise | the serial route is ISP-only; use `-c avrispmkII -P usb` for PDI/TPI |
| Host disables the USB port or refuses to enumerate | no COM port appears | the firmware cannot force one; restore host enumeration and retry — no Bus Pirate power cycle is needed once enumeration returns |

## ISP target

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| ISP enable is not answered by the target | host reports a failed program-enable, then `Unknown status 0x00` from its secondary check | real cause is no target contact; the status byte is not a firmware fault. Check wiring, target power and the target clock |
| Data wired to the ordinary SPI pins | no contact on an ATmega whose programming pins differ from SPI | use the target's serial-programming pins (for example PE0/PE1 on an ATmega64A), not PB2/PB3 — see [wiring.md](wiring.md) |
| `-B 125` used believing it means 125 kHz | SCK is 125 µs = 8 kHz | the `-B` number is a period in µs; use `-B 8` for 125 kHz |
| Full-page write at the slowest software rates | command fails with a timeout | the one-second target budget is exceeded; use small blocks with a custom host |
| `/RESET` release expected to be driven | the line floats high via the target's own pull-up | release is tristate by design; provide a target reset pull-up if needed |

## Clock and voltage

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Target fused for an external clock/crystal that is not running | ISP enable fails even though SCK is driven | wire `CS` → target XTAL1 and retry; see [rescue-clock-osccal.md](rescue-clock-osccal.md) |
| `OSCCAL` requested without AVR053 firmware on the target | the calibration command times out | AVR053 requires the target calibration firmware; it is not installed by this personality |
| VTARGET reads a nonsense value | the ADC pin is floating | VTARGET is informational and is only meaningful if the target supply is wired to the ADC pin |
| VTARGET reads low under load but normal when idle | a real supply sag, not a firmware problem | check the target supply's current limit; the programmer does not power the target |
| Target supply below its operating minimum | unstable or failing programming | bring the target supply into its datasheet range before programming |
| 5 V or 12 V applied to a Bus Pirate pin | risk of damage | never do this; use level translation and never a 12 V reset |

## PDI and TPI

| Trigger | Symptom | Behaviour / recovery |
|---|---|---|
| Target reset button pressed during PDI | the PDI clock is disturbed; the session fails | do not touch the target reset while programming (it is PDI_CLK) |
| PDI clock left idle between commands | the target's PDI interface would time out | the firmware keeps a clock running automatically; nothing to do |
| Damaged PDI/TPI link, NVMEN will not clear | bounded cleanup cannot reset the target interface | power-cycle the target; the firmware cannot recover an electrically inaccessible device |
| TPI write with an odd byte count or odd address | the request is rejected | TPI writes are word-based; use even addresses and whole words |
| XPROG CRC requested on TPI | a failure status, not data | TPI has no memory CRC |
| Attempting a locked/protected read | data returns as an address pattern, not real contents | protection denies verification as well as programming; chip erase can clear the lock on a part that allows it |

## Unverified

- TPI is **not verified** on hardware.
- PDI is **not verified for every XMEGA part**.
- The software-clocked end of the ISP speed ladder is approximate and is
  **not verified for every rate/page combination**.