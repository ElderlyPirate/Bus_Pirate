------------------------------------------------------------------
STOP! If you are upgrading from v2 you must install the v4 bootloader first:
http://dangerousprototypes.com/2010/01/18/how-to-bus-pirate-bootloader-v4-upgrade/
v4 BOOTLOADER ONLY!!!
DO NOT TRY TO LOAD THE BOOTLOADER FROM THIS ARCHIVE!!!! 
DO NOT TRY TO LOAD THE V4 FIRMWARE WITH p24QP!!!
------------------------------------------------------------------
Firmware upgrades:

http://dangerousprototypes.com/docs/Bus_Pirate#Firmware_upgrades

1) Enter the bootloader:

Connect PGC and PGD pins of the ICSP header and plug in the USB cable

--+OR+--

Type $ in the Bus Pirate terminal.

With both methods the MODE LED should light.

2) Load the new firmware:

* ds30Loader: just use the GUI (.net/Mono required)
* Pirate-loader (console app for Windows, Linux, Mac) firmware upgrades:

C:\pl>pirate-loader --dev=COM10 --hex=BPv3-Firmware-v1.0.hex

3) re-plug the USB cable to reset the Bus Pirate. The upgrade is complete.

------------------------------------------------------------------
BPv3-Bootloader-v4.4.hex is the same as v4.3 installed with the updater. This file is only for programming the bootlaoder to a new PIC. Use the upgrade package to upgrade:

http://dangerousprototypes.com/docs/Bus_Pirate#Bootloader_upgrades

------------------------------------------------------------------
vX-Firmware-vY(.z_Z).hex

Normal Bus Pirate firmware.
vX - Bus Pirate hardware version.
vY - The Bus Pirate firmware version.
z_Z - The .z_Z versions are translated compiles (currently Spanish and Italian).

vX-STK500v2-vY(-hiz).hex

ATMEL AVR programmer clone for the Bus Pirate hardware. An STK500 v2 protocol compatible firmware for the Bus Pirate, works with AVRDude, AVR Studio v4.11+, etc.
vX - Bus Pirate hardware version (v2go and v3 use the same firmware).
vY - THe STK500v2 clone firmware version.
hiz - The -hiz version of the fimware uses open drain/high impedance outputs. On-board pull-up resistors are enabled (where applicable). The non -hiz version uses normal 3.3volt outputs.

vX-PIC Bootloader-vY.hex

Bootloader to program in a new PIC microcontroller. This is usually only programmed once.
vX - Bus Pirate hardware version.
vY - The Bus Pirate bootloader version.

BPvX-Firmware-vX.X-extras.hex

The BPv3-Firmware-vX.X-extras.hex has a few modes and new stuff we couldn't fit in the main firmware.
