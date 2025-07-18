TOOLCHAIN:=xc16-
CC:=$(TOOLCHAIN)gcc
AS:=$(TOOLCHAIN)as
OBJCOPY:=$(TOOLCHAIN)objcopy
BIN2HEX:=$(TOOLCHAIN)bin2hex

CFLAGS:=-g -Wall -Wextra
LDFLAGS:=-L/opt/microchip/xc16/lib
INCLUDE:=-I. -Itranslations \
	-I/opt/microchip/xc16/include \
	-I/opt/microchip/xc16/support/PIC24F/h \
	-I/opt/microchip/xc16/support/PIC24F/inc

ifeq ($(BP_VERSION),V4)
MACHINE:=24FJ256GB106
TARGET:=bpv4

SOURCES+=messages_v4.s \
	dp_usb/cdc.c \
	dp_usb/usb_stack.c

INCLUDE+=-Idp_usb

else
ifeq ($(BP_VERSION),V3)
MACHINE:=24FJ64GA002
TARGET:=bpv3

SOURCES+=messages_v3.s

else
$(error BP_VERSION not provided or supported. Available options: V3, V4)
endif
endif

LDSCRIPT:=p$(MACHINE).gld

SOURCES+=jtag/lenval.c \
	jtag/micro.c \
	jtag/ports.c \
	1wire.c \
	aux_pin.c \
	base.c \
	basic.c \
	binary_io.c \
	bitbang.c \
	core.c \
	dio.c \
	hd44780.c \
	i2c.c \
	jtag.c \
	main.c \
	messages.c \
	onboard_eeprom.c \
	openocd.c \
	pc_at_keyboard.c \
	pic.c \
	proc_menu.c \
	raw2wire.c \
	raw3wire.c \
	selftest.c \
	smps.c \
	spi.c \
	sump.c \
	uart.c \
	uart2.c


OBJECTS:=$(patsubst %.s,build/%.o, $(patsubst %.c,build/%.o,$(SOURCES)))

.DEFAULT_GOAL:=$(TARGET).hex

build:
	mkdir -p build/translations build/dp_usb build/jtag

build/%.o: %.s | build
	$(AS) -processor=$(MACHINE) -o $@ $^

build/%.o: %.c | build
	$(CC) -mcpu=$(MACHINE) $(INCLUDE) $(CFLAGS) -c -o $@ $^

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.hex: %.elf
	$(BIN2HEX) $^

$(TARGET).elf: $(OBJECTS)
	$(CC) -mcpu=$(MACHINE) $(INCLUDE) $(CFLAGS) -o $@ $^ $(LDFLAGS) -T $(LDSCRIPT)

.PHONY:=clean
clean:
	rm -rf build

.PHONY:=distclean
distclean: clean
	rm -rf $(TARGET).hex $(TARGET).bin $(TARGET).elf
