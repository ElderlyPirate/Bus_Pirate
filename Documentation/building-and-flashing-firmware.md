# Prerequisites

Before we can build the firmware, we need to download and install XC16 and MPLAB-X, the toolchain and IDE used to compile firmware for the Bus Pirate's PIC24 microcontroller. 

## XC16

XC16 is freely available from [Microchip's website](http://www.microchip.com/mplab/compilers).  Open the page, Scroll down, Click on the Downloads tab, and download **XC16** (not XC8 or XC32) for your platform.

On Linux, you may have to make the file executable and run it from the command line

```bash
chmod +x xc16-*.run
./xc16-*.run
```

Follow the prompts and select the option to Add XC16 to the PATH environment variable.  If you accidentally forget to select said option, you can let the IDE know where the toolchain is located.  Keep in mind that right before the end of the installation you will be prompted to choose a particular edition of the toolchain - select the **free** edition, unless you happen to have a licence for the compiler itself.

## MPLAB-X

MPLAB-X is freely available from [Microchip's website](http://www.microchip.com/mplab/mplab-x-ide) as well.  Open the page, Scroll down, Click on the Downloads tab, and download MPLAB-X for your platform.

On Linux, you have to un-tar the installation script and make it executable before you can run it

```bash
tar xf MPLABX-*.tar
chmod +x MPLABX-*.sh 
./MPLABX-*.sh
```

Follow the prompts to install MPLAB-X.

# Configuration

If you selected to add the XC16 toolchain to your PATH variable you can safely skip this step.

Open the preferences dialog, click on the Embedded icon, then open the Build Tools tab.  Click on the Scan for Build Tools button and make sure it automatically adds a toolchain to your IDE.  If it doesn't then you have to click on the Add button and navigate to the root directory of your XC16 installation, then click OK.

# Building

Open MPLAB-X.

Click File &rarr; Open Project and navigate to `Bus_Pirate/Firmware/busPirate.X`.

Select `BusPirate_v3` or `BusPirate_v4` from the dropdown menu in the toolbar.

Click the hammer in the toolbar.

The firmware will be saved in `/Firmware/busPirate.X/dist/BusPirate_v3/production/busPirate.X.production.hex` or `/Firmware/busPirate.X/dist/BusPirate_v4/production/busPirate.X.production.hex`, depending on which hardware version you chose to build the firmware for.

# Flashing

To flash the firmware onto a Bus Pirate, there is a tool called Pirate-Loader. This tool is included in the Bus Pirate Github repository, so there's nothing extra to download. 

Simply navigate to `Bus_Pirate/package/BPv3-firmware` if you have a Bus Pirate v3 hardware board. Here you will find pirate-loader_lnx, pirate-loader_mac and pirate-loader.exe, for the respective platforms. When you see `pirate-loader` in a command in this section replace it with either `./pirate-loader_lnx` `./pirate-loader_mac` or `pirate-loader.exe`, depending on your platform. You may wish to copy pirate-loader to another location or add it to your PATH environment variable. 

Before we can flash the firmware, we need to trigger the Bus Pirate bootloader. There are two ways to do this. On a Bus Pirate with bootloader v4+, you can simply type `$` at the Bus Pirate TTY prompt and press enter, then close the TTY to free up the connection. If this doesn't work for you, you can use the second method, which is to connect the PGC and PGD pins with a jumper cable. When you are in the bootloader, the MODE LED of the Bus Pirate will stay on.

Once the Bus Pirate prints BOOTLOADER you will need to quit the terminal application e.g. `screen` by typing `Ctrl+a Ctrl+k` Otherwise the loader will not be able to connect.

Now we are ready to use pirate-loader. The usage is as follows: 

`pirate-loader --dev=X --hex=Y`

X is the Bus Pirate port name on your system
Y is the firmware file name to load

On a Windows system, the port name will be some COM port, something like `COM10`. On a Mac or Linux System, it's the path to the device file, in my case this is `/dev/ttyUSB0`.

Recall that our firmware file is saved in `Bus_Pirate/Firmware/busPirate.X/dist/BusPirate_v*/production/busPirate.X.production.hex`

So our command will be something like
`pirate-loader --dev=COM10 --hex=../../Firmware/busPirate.X/dist/BusPirate_v[3 or 4]/production/busPirate.X.production.hex`

You should see pirate-loader report it's progress, finishing with `Firmware updated successfully :)!`

If you see `Error updating firmware :(`, but it progresses past page 41, this is actually a successful result. 

All that's left now is to reboot the Bus Pirate (removing the jumper if necessary), and type `i` at the prompt to see the version number. At the time of this writing, the most current version of the firmware is v7.1 and I see the following:

```
Bus Pirate v3.5
Community Firmware v7.1 - goo.gl/gCzQnW [HiZ 1-WIRE UART I2C SPI 2WIRE 3WIRE PIC DIO]
Bootloader v4.4
DEVID:0x0447 REVID:0x3046 (24FJ64GA00 2 B8)
```
