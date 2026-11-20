XC16 ?= xc16-
CFLAGS = -mcpu=24FJ256GB106 -O1 -msmall-code -Wall -Wextra -Werror -std=gnu99
SOURCES = bootloader.c flash.c cdc.c main.c usb_stack.c
HEADERS = $(wildcard *.h)
.PHONY: all clean
all: build/bpv4-bootloader.hex
build:
	cmake -E make_directory build
build/bpv4-bootloader.elf: $(SOURCES) $(HEADERS) p24FJ256GB106.gld | build
	"$(XC16)gcc" $(CFLAGS) -I. -Wl,--script=p24FJ256GB106.gld,-Map=build/bpv4-bootloader.map $(SOURCES) -o $@
build/bpv4-bootloader.hex: build/bpv4-bootloader.elf
	"$(XC16)bin2hex" -a $<
clean:
	cmake -E rm -rf build
