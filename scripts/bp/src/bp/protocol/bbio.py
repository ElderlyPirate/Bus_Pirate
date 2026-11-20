"""Bus Pirate binary bitbang framing and terminal banner handling."""

from __future__ import annotations

import logging

from bp.errors import ProtocolError, TransportTimeout
from bp.transport import Transport
from bp.types import BpBinaryMode, VersionInfo

logger = logging.getLogger(__name__)

BBIO_BANNER = b"BBIO1"
BBIO_ENTER_SEQUENCE = b"\x00" * 20
BBIO_RESET_COMMAND = b"\x0f"
BBIO_SUCCESS = 0x01
BBIO_FAILURE = 0x00
PERIPHERAL_COMMAND = 0x40
POWER_BIT = 0x08
PULLUP_BIT = 0x04
AUX_BIT = 0x02
CS_BIT = 0x01
VERSION_MAX_BYTES = 1000
VERSION_COMMAND = b"\n" * 10 + b"#\n"


class BbioSession:
    """Own the transition between the user terminal and binary modes."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport

    def enter_binary(self) -> None:
        """Send the documented NUL sequence and require a ``BBIO1`` banner."""
        logger.debug("Entering binary bitbang mode")
        self._transport.reset_input()
        self._transport.write(BBIO_ENTER_SEQUENCE)
        try:
            banner = self._transport.read_exact(len(BBIO_BANNER))
        except TransportTimeout as exc:
            raise ProtocolError("Bus Pirate did not enter binary mode; check the port and reset the device") from exc
        if banner != BBIO_BANNER:
            raise ProtocolError(f"Expected BBIO1 banner, received {_describe(banner)}")
        self._transport.reset_input()

    def enter_mode(self, mode: BpBinaryMode) -> None:
        """Enter a binary protocol mode and verify its banner."""
        logger.debug("Entering binary %s mode", mode.name)
        self._transport.write(bytes([mode.value]))
        banner = self._transport.read_exact(len(mode.banner))
        if banner != mode.banner:
            raise ProtocolError(f"Expected {mode.banner.decode('ascii')} banner, received {_describe(banner)}")

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        """Set the binary-mode power, pull-up, AUX, and CS pins."""
        flags = 0
        if power:
            flags |= POWER_BIT
        if pullups:
            flags |= PULLUP_BIT
        if aux:
            flags |= AUX_BIT
        if cs:
            flags |= CS_BIT
        self._transport.write(bytes([PERIPHERAL_COMMAND | flags]))
        self.expect_success()

    def expect_success(self) -> None:
        """Consume one status byte and require the binary success value."""
        status = self._transport.read_exact(1)
        if status == bytes([BBIO_FAILURE]):
            raise ProtocolError("Bus Pirate reported a failed binary command")
        if status != bytes([BBIO_SUCCESS]):
            raise ProtocolError(f"Expected binary success status 0x01, received {_describe(status)}")

    def exit_to_bitbang(self) -> None:
        """Return from the active binary mode to binary bitbang mode."""
        self._transport.write(b"\x00")
        banner = self._transport.read_exact(len(BBIO_BANNER))
        if banner != BBIO_BANNER:
            raise ProtocolError(f"Expected BBIO1 banner while leaving mode, received {_describe(banner)}")

    def reset(self) -> None:
        """Reset the Bus Pirate and require its acknowledgement."""
        logger.debug("Resetting Bus Pirate")
        self._transport.write(BBIO_RESET_COMMAND)
        self.expect_success()


def read_version_info(transport: Transport) -> VersionInfo:
    """Read and parse the firmware/hardware banner from the user terminal."""
    transport.reset_input()
    transport.write(VERSION_COMMAND)
    raw = transport.read_until_idle(max_bytes=VERSION_MAX_BYTES)
    if not raw:
        raise ProtocolError("Bus Pirate returned no version banner; reset the device and try again")
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ProtocolError("Version banner contained non-ASCII data; reset the device and try again") from exc
    return parse_version_banner(text)


def parse_version_banner(text: str) -> VersionInfo:
    """Extract the reset banner fields from terminal text.

    The reset banner contains a hardware line, a firmware line, a
    ``DEVID:...`` line, and an update URL. The firmware also prints a protocol
    list between the firmware and DEVID lines, so fields are identified by
    their labels rather than by a fixed line offset. Any text before the final
    ``RESET`` line and any prompt after the URL are ignored.

    Raises:
        ProtocolError: If a complete labelled banner is not present.
    """
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    reset_index: int | None = None
    for index, line in enumerate(lines):
        if line == "RESET":
            reset_index = index
    if reset_index is None:
        raise ProtocolError("Incomplete version banner; reset the Bus Pirate and try again")

    banner = lines[reset_index + 1 :]
    hardware_type = next((line for line in banner if line.startswith("Bus Pirate")), None)
    firmware_version = next((line for line in banner if "Firmware" in line), None)
    hardware_version = next((line for line in banner if line.startswith("DEVID:")), None)
    update_url = next((line for line in banner if line.startswith(("http://", "https://"))), None)
    if hardware_type is None or firmware_version is None or hardware_version is None or update_url is None:
        raise ProtocolError("Incomplete version banner; reset the Bus Pirate and try again")

    return VersionInfo(
        hardware_type=hardware_type,
        firmware_version=firmware_version,
        hardware_version=hardware_version,
        update_url=update_url,
    )


def _describe(payload: bytes) -> str:
    return " ".join(f"0x{value:02X}" for value in payload) if payload else "no data"
