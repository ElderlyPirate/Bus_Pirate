# Bus Pirate

This repository was forked on 03 May 2018 from the repo "BusPirate/Bus_Pirate" maintained by Agatti (Community version 7.1). Therefore, all the installation instructions, features are the same as the main repo's.

The aim of this fork is to fix I2C protocol bugs that are present in the Bus Pirate Firmware, with the intention that it will be merged again into the main repo.


## Protocols
    
* 1-Wire
* I2C
* SPI
* JTAG
* Asynchronous serial
* MIDI
* PC keyboard
* HD44780 LCD
* 2- and 3-wire libraries with bitwise pin control
* Scriptable binary bitbang, 1-Wire, I2C, SPI, and UART modes.

## Features

* 0 - 5.5v tolerant pins
* 0 - 6v measurement probe
* 1Hz - 40MHz frequency measurement
* 1kHz - 4MHz pulse-width modulator, frequency generator
* On-board multi-voltage pull-up resistors
* On-board 3.3v and 5v power supplies with software reset
* Macros for common operations
* Bus traffic sniffers (SPI, I2C)
* A bootloader for easy firmware updates
* Transparent USB&rarr;serial mode
* 10Hz - 1MHz low-speed logic analyzer
* Servo driver
* Can program many AVR microcontrollers
  * Supported by AVRdude
  * Can emulate the AVR STK500 v2 with alternate ST500 Clone firmware 
  * Programs FPGAs and CPLDs with alternate XSVF firmware
* Scriptable from Perl, Python, etc.
* Public domain (Creative Commons Zero) source. Prototype with the Bus Pirate, then use the code in your project however you want. 

## Application Support

The Bus Pirate is used through a simple terminal interface, but these applications also support the Bus Pirate as a programming device, etc.

* [AVRDude] AVR programmer (AVRDude v5.8+, firmware v4 (any) or v5.9+)
* [OpenOCD] JTAG debugger
* [flashrom] bios/flash programmer 

# Purchasing

* Bus Pirate v4.0 - *Newer, more space for bigger firmwares*
  * Worldwide: <http://www.seeedstudio.com/depot/bus-pirate-v4-for-developers-p-740.html>

* Bus Pirate v3.6 - *Proven, more well tested*
  * Worldwide: <http://www.seeedstudio.com/depot/bus-pirate-v3-assembled-p-609.html?cPath=61_68>
  * USA: <https://www.adafruit.com/products/237?&main_page=product_info&cPath=8&products_id=237>
  * USA: <http://www.epictinker.com/Bus-Pirate-v3-assembled-p/thm180c4m.htm>
  * Germany: <http://www.watterott.com/en/Bus-Pirate>
  * Europe: <http://evola.fr/en/dangerous-prototypes/164-bus-pirate-v3-6.html>
  * USA: <https://anibit.com/product/ptt07001>
  * USA: <http://store.hackaday.com/products/buspirate-v3-6-thm180c4m>

[OpenOCD]: http://openocd.org
[AVRDude]: http://www.nongnu.org/avrdude
[flashrom]: https://www.flashrom.org/Flashrom

## Firmware Flashing

Refer to [building-and-flashing-firmware](Documentation/building-and-flashing-firmware.md) to see how to build and flash this firmware.
