Important files:
base.h - Hardware version and interface language configuration. Mode data structure, command definitions/enumeration. Also brings delay, terminal IO and UI functions via baseIO.c baseUI.c.

baseIO.c - Terminal IO functions, convert byte to ASCII, string printing, etc.

baseUI.c - User interface functions, function to display text messages from the lookup table. Text abstraction to translation files in \translate.

main.c - Main program loop.

procMenu.c - Processes single character menu commands, such as display mode, terminal speed, etc. Includes supporting functions.

procSyntax.c - Processes syntax arrays and executes commands through the protocol abstraction function in busPirateCore.c.

busPirateCore.c - A function with a switch statement that forwards commands to the proper protocol library depending on the currently set bus mode.

AUXpin.c - Functions of the AUX pin. High/low/read (Hi-Z), frequency measurement, pulse-width modulator.

1wire,I2C,jtag,m_1wire_213.h,m_i2c_1.h,pc_at_keyboard,raw2wire,raw3wire,SPI,UART - Functions that implement the individual protocols.

------------------------

Basic code overview:

A loop in main.c grabs terminal input. 

On enter, terminal input is checked for a menu command (procMenu.c).

If there's no menu command, the input is processed as syntax (procSyntax.c).

Synax is scanned. The function ASCII2CMD in procSyntax.c identifies commands according to the defines in base.h. 

A valid syntax command is processed with bpProcess in busPirateCore.c. bpProcess() is a switch statement that abstracts the many protocol library processing functions.

Each protocol library has a processing function that acts on the command as defined in base.h.