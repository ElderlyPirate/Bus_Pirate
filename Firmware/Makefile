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
# then to whatever is on PATH.  The Windows default contains spaces
# ("Program Files (x86)"); a literal space in $(wildcard ...) is treated as a
# list separator, so match the spaces with '?' and then assign the real path.
ifeq ($(XC16_DIR),)
ifneq ($(wildcard /opt/microchip/xc16/v2.10/bin),)
XC16_DIR := /opt/microchip/xc16/v2.10
else ifneq ($(wildcard C:/Program?Files?(x86)/Microchip/MPLABXC16/v2.10/bin),)
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
	protocol_wire/1wire.c \
	capability_aux/aux_pin.c \
	programmer_avrisp/avrisp_isp.c \
	programmer_avrisp/avrisp_mkii.c \
	programmer_avrisp/avrisp_nvm_pdi.c \
	programmer_avrisp/avrisp_nvm_tpi.c \
	programmer_avrisp/avrisp_v2.c \
	programmer_avrisp/avrisp_xprog.c \
	core_system/base.c \
	capability_adc/adc.c \
	tool_basic/basic.c \
	protocol_binary/binary_io.c \
	core_bitbang/bitbang.c \
	core_system/core.c \
	protocol_dio/dio.c \
	capability_hd44780/hd44780.c \
	protocol_i2c/i2c.c \
	protocol_jtag/jtag.c \
	tool_xsvf/lenval.c \
	tool_xsvf/micro.c \
	tool_xsvf/ports.c \
	core_system/main.c \
	core_messages/messages.c \
	tool_openocd/openocd.c \
	protocol_keyboard/pc_at_keyboard.c \
	programmer_pic/pic.c \
	core_system/proc_menu.c \
	protocol_wire/raw2wire.c \
	protocol_wire/raw3wire.c \
	capability_selftest/selftest.c \
	capability_smps/smps.c \
	protocol_spi/spi.c \
	protocol_sump/sump.c \
	protocol_uart/uart.c \
	protocol_uart/uart2.c

ifeq ($(BP_VERSION),V4)
SOURCES += \
	core_usb/cdc.c \
	core_usb/usb_stack.c \
	core_eeprom/onboard_eeprom.c \
	core_messages/messages_v4.s
else
SOURCES += \
	core_messages/messages_v3.s \
	tool_openocd/openocd_asm.s
endif

# Object files live in a per-target directory: the v3 and v4 builds compile
# the same sources with different macros (BUSPIRATEV3 / BUSPIRATEV4), so
# sharing one directory would let make link objects from the wrong hardware
# revision.
OBJECTS := $(addprefix build/$(TARGET)/,$(SOURCES:.c=.o))
OBJECTS := $(OBJECTS:.s=.o)
DEPS    := $(OBJECTS:.o=.d)

# Stamp recording the effective compiler flags (see the object rules below).
FLAG_STAMP := build/$(TARGET)/.build-flags

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
	-I. -Icore_system -Icore_bitbang -Icore_messages -Icore_eeprom \
	-Icapability_hd44780 -Itool_basic -Iprotocol_sump -Icapability_aux \
	-Iprotocol_i2c -Iprotocol_spi -Iprotocol_uart -Iprotocol_wire \
	-Iprotocol_jtag -Itool_openocd -Itool_xsvf -Iprogrammer_pic \
	-Iprogrammer_avrisp -Icapability_adc -Icapability_smps \
	-Icapability_selftest -Iprotocol_binary -Iprotocol_keyboard -Iprotocol_dio \
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

# The object path depends only on the target and source, and -MMD tracks
# headers but not compiler flags.  Without an extra dependency a change to
# OPT/EXTRA_CFLAGS would leave stale objects in place and link the old image.
# The stamp holds the effective flag set and is rewritten only when its
# contents change, so a flag change forces a recompile while an ordinary
# rebuild stays a no-op.
$(FLAG_STAMP): FORCE
	@mkdir -p $(@D)
	@printf '%s\n' '$(CFLAGS)' > $@.tmp
	@cmp -s $@.tmp $@ || mv -f $@.tmp $@
	@rm -f $@.tmp

.PHONY: FORCE
FORCE:

build/$(TARGET)/%.o: %.c $(FLAG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/$(TARGET)/%.o: %.s $(FLAG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf build $(TARGET).elf $(TARGET).hex $(TARGET).map

.PHONY: distclean
distclean: clean

-include $(DEPS)
