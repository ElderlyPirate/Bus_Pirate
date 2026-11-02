# Bus Pirate

>
> THIS REPOSITORY IS ONLY FOR THE OLD BUS PIRATE v3.x and v4.x HARDWARE!
>
> If you own a Bus Pirate 5 or later, please see https://buspirate.com
>

The Bus Pirate is an open source hacker multi-tool that talks to electronic stuff.
It's got a bunch of features an intrepid hacker might need to prototype their next project.

This community firmware was forked from the official Dangerous Prototypes firmware for these devices.
This repository represents the hard work of community members to sustain and continue the legacy of the Bus Pirate v3 and v4 devices.

Public domain (Creative Commons Zero) source: prototype with the Bus Pirate, then use the code in your project however you want — see [LICENSE](LICENSE).

## Documentation

One central location: **[docs/](docs/)**.  This README intentionally does not repeat capabilities — the specification does that:

* **[docs/SPEC.md](docs/SPEC.md)** — the specification: every mode, function and sub-function (e.g. UART *and* HDUART, SUMP *and* its extended logic analyzer) with its v3/v4 status.  Planned items are marked `tbi`.
* **[docs/generic/](docs/generic/)** — how to use the prompt (the generic layer: navigation, formatting, macros, elements).
* per-function documentation under `docs/<function>/` — being consolidated from the existing [Documentation/](Documentation/) files.
* developer documentation (build, flash, bootloaders, tooling) — see [docs/dev/](docs/dev/).

*(Documentation restructure in progress — 2026-11.)*

## Application support

The Bus Pirate is used through a simple terminal interface; these programs can drive it for specific jobs (each covered with its function's documentation):

* [AVRDude] — AVR programming via the built-in AVRISP-MKII personality (BPv4)
* [OpenOCD] — JTAG debugging (binary JTAG mode)
* [flashrom] — BIOS/flash programming
* [sigrok/PulseView] — logic analyzer (SUMP)
* `picprog` — PIC programming

## Purchasing

> 2025 - If you're looking to purchase an old Bus Pirate v3, or v4.. don't
> spend over $30-40 USD. Spend your hard-earned cash on a Bus Pirate 5+ https://buspirate.com

* Bus Pirate v4.0 - *Newer, more space for bigger firmwares*
  * Worldwide: amazon.com has some overpriced old stock boards.

* Bus Pirate v3.6 - *Proven, more well tested*
  * Worldwide: amazon.com has some overpriced old stock boards.
  * Europe: <https://www.robotshop.com/uk/bus-pirate-v36a.html>
  * USA: <https://anibit.com/product/ptt07001>

## Firmware

Refer to [docs/dev/](docs/dev/) to see how to build and flash this firmware.

[OpenOCD]: http://openocd.org
[AVRDude]: http://www.nongnu.org/avrdude
[flashrom]: https://www.flashrom.org/Flashrom