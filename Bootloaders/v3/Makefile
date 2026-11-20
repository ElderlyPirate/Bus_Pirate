XC16 ?= xc16-
.PHONY: all clean
all: build/bpv3-bootloader.hex
build:
	cmake -E make_directory build
build/bpv3-bootloader.elf: ds30loader.s settings.inc config.c p24FJ64GA002.gld | build
	"$(XC16)gcc" -mcpu=24FJ64GA002 -nostartfiles -Wall -Wextra -Werror -Wa,--fatal-warnings -I. -Wl,--script=p24FJ64GA002.gld,-Map=build/bpv3-bootloader.map ds30loader.s config.c -o $@
build/bpv3-bootloader.hex: build/bpv3-bootloader.elf
	"$(XC16)bin2hex" -a $<
clean:
	cmake -E rm -rf build
