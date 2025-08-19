TOOLCHAIN:=xc16-
CC:=$(TOOLCHAIN)gcc
AS:=$(TOOLCHAIN)as
OBJCOPY:=$(TOOLCHAIN)objcopy

MACHINE:=-mcpu=24FJ256GB106

CFLAGS:=-g -Wall -Wextra
LDFLAGS:=-L/opt/microchip/xc16/lib
INCLUDE:=-I. -I/opt/microchip/xc16/include \
	-I/opt/microchip/xc16/support/PIC24F/h \
	-I/opt/microchip/xc16/support/PIC24F/inc
LDSCRIPT:=p24FJ256GB106.gld
TARGET:=bpv4-bootloader

SOURCES:=$(wildcard *.c)

OBJECTS:=$(patsubst %.s,build/%.o, $(patsubst %.c,build/%.o,$(SOURCES)))

.DEFAULT_GOAL:=$(TARGET).hex

build:
	mkdir -p build

build/%.o: %.s | build
	$(AS) -o $@ $^

build/%.o: %.c | build
	$(CC) $(MACHINE) $(INCLUDE) $(CFLAGS) -c -o $@ $^

%.hex: %.elf
	$(OBJCOPY) -O ihex $^ $@

$(TARGET).elf: $(OBJECTS)
	$(CC) $(MACHINE) $(INCLUDE) $(CFLAGS) -o $@ $^ $(LDFLAGS) -T $(LDSCRIPT)

.PHONY:=clean
clean:
	rm -rf build

.PHONY:=distclean
distclean: clean
	rm -rf $(TARGET).hex $(TARGET).elf
