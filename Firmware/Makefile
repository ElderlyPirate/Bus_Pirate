# -----------------------------------------------------------------------------
# Bus Pirate v3 / v4 firmware - command line build
#
# Builds the firmware without MPLAB X.  The compiler and linker options used
# here mirror the ones stored in busPirate.X/nbproject/configurations.xml, so
# that this Makefile and the MPLAB X project produce equivalent images.
#
# Usage:
#     make BP_VERSION=V4                # build bpv4.hex
#     make BP_VERSION=V3                # build bpv3.hex
#     make BP_VERSION=V4 clean          # remove build products
#
# Requires the Microchip XC16 compiler to be on PATH, or its location given
# explicitly:
#     make BP_VERSION=V4 XC16_DIR="/opt/microchip/xc16/v2.10"
#
# Non-default optimisation levels are supported, e.g.:
#     make BP_VERSION=V4 OPT=-O0
# -----------------------------------------------------------------------------

BP_VERSION ?= V4

ifeq ($(BP_VERSION),V4)
MACHINE  := 24FJ256GB106
TARGET   := bpv4
LDSCRIPT := p24FJ256GB106.gld
endif

ifeq ($(BP_VERSION),V3)
MACHINE  := 24FJ64GA002
TARGET   := bpv3
LDSCRIPT := p24FJ64GA002.gld
endif

ifeq ($(TARGET),)
$(error BP_VERSION must be V3 or V4 (got '$(BP_VERSION)'))
endif

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------

# If XC16_DIR is not given, fall back to the microchip installer defaults and
# then to whatever is on PATH.
ifeq ($(XC16_DIR),)
ifneq ($(wildcard /opt/microchip/xc16/v2.10/bin),)
XC16_DIR := /opt/microchip/xc16/v2.10
else ifneq ($(wildcard C:/Program Files (x86)/Microchip/MPLABXC16/v2.10/bin),)
XC16_DIR := C:/Program Files (x86)/Microchip/MPLABXC16/v2.10
endif
endif

ifneq ($(XC16_DIR),)
TOOLCHAIN := "$(XC16_DIR)/bin/xc16-"
else
TOOLCHAIN := xc16-
endif

CC      := $(TOOLCHAIN)gcc
BIN2HEX := $(TOOLCHAIN)bin2hex

# -----------------------------------------------------------------------------
# Sources
#
# The list below matches the file list of the MPLAB X project.  Note that the
# BPv3 and BPv4 builds use different source sets: the BPv4 firmware has its own
# USB stack plus the on-board EEPROM driver, while the BPv3 firmware uses the
# JTR OpenOCD bit-shift helper that is written in assembly.
# -----------------------------------------------------------------------------

SOURCES := \
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
	jtag/lenval.c \
	jtag/micro.c \
	jtag/ports.c \
	main.c \
	messages.c \
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

ifeq ($(BP_VERSION),V4)
SOURCES += \
	dp_usb/cdc.c \
	dp_usb/usb_stack.c \
	onboard_eeprom.c \
	messages_v4.s
else
SOURCES += \
	messages_v3.s \
	openocd_asm.s
endif

# Object files live in a per-target directory: the v3 and v4 builds compile
# the same sources with different macros (BUSPIRATEV3 / BUSPIRATEV4), so
# sharing one directory would let make link objects from the wrong hardware
# revision.
OBJECTS := $(addprefix build/$(TARGET)/,$(SOURCES:.c=.o))
OBJECTS := $(OBJECTS:.s=.o)
DEPS    := $(OBJECTS:.o=.d)

# -----------------------------------------------------------------------------
# Flags
# -----------------------------------------------------------------------------

VERSION   := $(shell git rev-parse --short HEAD)
BUILDDATE := $(shell date -u +%Y-%m-%d)

# Optimisation level 1 is what the MPLAB X configuration specifies.
OPT ?= -O1

CFLAGS := \
	-mcpu=$(MACHINE) \
	$(OPT) \
	-finline \
	-std=gnu99 \
	-I. \
	-Wall \
	-MMD -MP \
	-DVERSION=$(VERSION)

# The MPLAB X project requests the small scalar model, which is also the
# compiler default on this target; requesting it explicitly keeps the build
# identical should that default ever change.
CFLAGS += -msmall-scalar

# Hook for one-off additional compiler options, mirroring the "additional
# options" field of the MPLAB X project.
ifdef EXTRA_CFLAGS
CFLAGS += $(EXTRA_CFLAGS)
endif

# -save-temps is enabled in the MPLAB X project; add it here to keep the
# preprocessed and assembly output around for inspection.  The "=obj" form is
# used so that the intermediate files are written next to the object files in
# build/ rather than next to the sources, where they could be confused with
# (or accidentally delete) the tracked assembly sources.
ifdef SAVE_TEMPS
CFLAGS += -save-temps=obj
endif

LDFLAGS := \
	-mcpu=$(MACHINE) \
	-Wl,--gc-sections \
	-Wl,--report-mem \
	-Wl,-Map=$(TARGET).map

ifneq ($(HEAP_SIZE),)
LDFLAGS += -Wl,--heap=$(HEAP_SIZE)
endif

ifneq ($(STACK_SIZE),)
LDFLAGS += -Wl,--stack=$(STACK_SIZE)
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.DEFAULT_GOAL := all

.PHONY: all
all: $(TARGET).hex

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -T $(LDSCRIPT)

%.hex: %.elf
	$(BIN2HEX) -a $<

build/$(TARGET)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/$(TARGET)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf build $(TARGET).elf $(TARGET).hex $(TARGET).map

.PHONY: distclean
distclean: clean

-include $(DEPS)