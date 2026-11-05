1-Wire Mode
===================
1-Wire uses a single data signal wire. Most devices also require a power and ground connection. Some parts draw power parasitically through the 1-Wire bus and don't require a separate power source. 

  - **Bus:** [1-Wire](http://en.wikipedia.org/wiki/1-Wire) (Dallas/Maxim 1-Wire protocol)
  - **Connections:** one pin (OWD) and ground
  - **Output types:** [open drain/open collector](http://en.wikipedia.org/wiki/High_impedence) (high=Hi-Z, low=ground), [pull-up resistors](http://dangerousprototypes.com/docs/Practical_guide_to_Bus_Pirate_pull-up_resistors) required
  - **Pull-up resistors:** pull-ups always required (2K - 10K ohms, 2K or less for parasitic power parts)
  - **Maximum voltage:** 5.5volts (5 volt safe)

Starting 1-Wire Mode
---------------------

    HiZ>m    <<<open the mode menu
    1. HiZ
    2. 1-WIRE
    …
    (1)>2    <<<choose 1-wire mode
    Clutch disengaged!!!
    To finish setup, start up the power supplies with command 'W'

    Ready
    1-WIRE>

Command Syntax
-------------------

  * A/a/@
	  * Toggle auxiliary pin.
	  * Capital "A" sets AUX high
	  * Small "a" sets to ground.
	  * @ sets aux to input (high impedance mode) and reads the pin value.
  * D/d
	  * Measure voltage on the ADC pin (v1+ hardware only).
  * W/w
	  * Capital 'W' enables the on-board power supplies.
	  * Small 'w' disables them. (v1+ hardware only).
  * { or [
	  * 1-Wire bus reset.
  * ] or }
	  * -
  * r
	  * Read one byte. (r:1…255 for bulk reads)
  * 0b
	  * Write this binary value.
	  * Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
  * 0x
	  * Write this HEX value, check ACK.
	  * Format is 0x01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
  * 0-255
	  * Write this decimal value.
	  * Any number not preceded by 0x or 0b is interpreted as a decimal value.
  * , space
	  * Value delimiter.
	  * Use a comma or space to separate numbers.
	  * Any combination is fine, no delimiter is required between non-number values: {0xa6,0, 0 16 5 0b111 0xaF}.
  * &
	  * Delay 1uS. (&:1…255 for multiple delays)
  * (#) 
	  * Run macro, (0) for macro list.
  * ^
      * One clock tick, use data state from previous – or _ operation. (^:1…255 for multiple clock ticks)
  * / or \
      * -
  * - or _
      * Set the 1-Wire data state to 1 (-) or 0 (_). This will be used on the next ^ command, no actual bus change. (updated in v5.2, this previously set the state and sent a bit)
  * !
      * Read one bit with clock.
  * .
      * Read current data state setting from last - or _ command, no actual bus change.
  * O
      * Toggle the 1-Wire speed in place between standard (~16.3kbps) and overdrive (~160kbps) without turning off the power supplies or the pull-up resistors, and print the speed that is now active. This lets a device be put into overdrive with `0x3C` (Overdrive Skip ROM) at standard speed and then be driven at overdrive speed without leaving the mode. Only valid in 1-Wire mode.

Macros
------------------

| (#) | Description |
|:---:| ----------- |
| 0 | Macro menu |
| 1-50 | Reserved for device address shortcuts. |
| 51 | READ ROM (0x33) *for single device bus. |
| 85 | MATCH ROM (0x55) *followed by 64bit address. |
| 204 | SKIP ROM (0xCC) *followed by command. |
| 236 | ALARM SEARCH (0xEC). |
| 240 | SEARCH ROM (0xF0). |

### Notes

1-Wire specifies a 2K or smaller resistor when working with parasitically powered devices. Since v3a the on-board pull-up resistor on MOSI are 2K. Use an external 2K pull-up resistor if you have a v2go. Parasitically powered parts may appear to work with resistors larger than 2K ohms, but will fail certain operations (like EEPROM writes).

The 1-wire reset command can detect two bus errors. If no 1-wire chips respond to the reset command by pulling the bus low, it will report *No device detected (0x02). If the bus stays low for too long after the reset, because the pull-up resistor isn't working or there's a short circuit, it will report *Short or no pull-up (0x01).

One wire is a time sensitive protocol. There's no actual data wire to set high or low with the - and _ commands, so we just store the desired value and send it with the next clock tick (^).

The _ and - commands were updated in firmware v5.2. They previously set the data state and sent a bit. Now they just set the data state that will be used on the next clock tick command (^). Example: previously you could write 4 high bits with -^^^, now you must use -^^^^. We feel this is more consistent with the operation of the other modes.

Connections
------------------

| Bus Pirate | Dir. | Circuit  | Description         |
| ----------:|:----:|:-------- | ------------------- |
| MOSI       | ↔    | 1-wire   | Serial Data & Clock |
| GND        | ⏚    | GND      | Signal Ground       |
