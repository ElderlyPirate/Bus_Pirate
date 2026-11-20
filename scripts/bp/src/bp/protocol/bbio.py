# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Bus Pirate binary bitbang framing, mode operations, and terminal banners."""

from __future__ import annotations

import logging
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntFlag
from types import TracebackType
from typing import Protocol

from bp.errors import ProtocolError, TransportTimeout
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings, VersionInfo

logger = logging.getLogger(__name__)

BBIO_BANNER = b"BBIO1"
BBIO_ENTER_SEQUENCE = b"\x00" * 21
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

BITBANG_DIRECTION_COMMAND = 0x40
BITBANG_STATE_COMMAND = 0x80
BITBANG_MODE_VERSION_COMMAND = 0x00
BITBANG_SHORT_SELF_TEST = 0x10
BITBANG_FULL_SELF_TEST = 0x11
BITBANG_SETUP_PWM = 0x12
BITBANG_CLEAR_PWM = 0x13
BITBANG_ADC_ONESHOT = 0x14
BITBANG_SELF_TEST_EXIT = 0xFF
BITBANG_ADC_MAX = 0x0FFF


class BitbangDirection(IntFlag):
    """Binary-bitbang direction mask bits; a set bit configures an input."""

    CS = 0x01
    MISO = 0x02
    CLK = 0x04
    MOSI = 0x08
    AUX = 0x10


class BitbangState(IntFlag):
    """Binary-bitbang pin-state mask bits returned by the firmware."""

    CS = 0x01
    MISO = 0x02
    CLK = 0x04
    MOSI = 0x08
    AUX = 0x10
    PULLUP = 0x20
    POWER = 0x40


@dataclass(frozen=True, slots=True)
class SelfTestReport:
    """Result of a binary bitbang self-test run."""

    error_count: int
    long_test: bool


class BitbangSession(Protocol):
    """Binary-bitbang operations consumed by the command layer."""

    def version(self) -> bytes:
        """Request and return the ``BBIO1`` mode identifier."""
        ...

    def configure_directions(self, mask: int) -> int:
        """Set the pin-direction mask and return the resulting pin state."""
        ...

    def set_pins(self, mask: int) -> int:
        """Set the pin/power/pull-up state mask and return the pin state."""
        ...

    def read_pins(self) -> int:
        """Read the pin state without changing pin directions."""
        ...

    def set_power(self, enabled: bool) -> int:
        """Enable or disable the on-board power supplies."""
        ...

    def set_pullups(self, enabled: bool) -> int:
        """Enable or disable the on-board pull-up resistors."""
        ...

    def set_aux(self, high: bool) -> int:
        """Drive the AUX pin high or low."""
        ...

    def set_cs(self, high: bool) -> int:
        """Drive the CS pin high or low."""
        ...

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        """Clock ``count`` bits out on MOSI."""
        ...

    def read_bits(self, count: int, *, lsb_first: bool = False) -> int:
        """Clock ``count`` bits in from MISO."""
        ...

    def write_bytes(self, data: bytes, *, lsb_first: bool = False) -> None:
        """Clock each byte out on MOSI."""
        ...

    def read_bytes(self, count: int, *, lsb_first: bool = False) -> bytes:
        """Clock ``count`` bytes in from MISO."""
        ...

    def self_test(self, *, long_test: bool = False) -> SelfTestReport:
        """Run the firmware self-test and return its error count."""
        ...

    def read_adc(self) -> int:
        """Read one raw ADC sample."""
        ...

    def setup_pwm(self, *, prescaler: int, duty: int, period: int) -> None:
        """Configure the AUX PWM output."""
        ...

    def clear_pwm(self) -> None:
        """Stop and release the AUX PWM output."""
        ...

    def reset_to_terminal(self) -> None:
        """Send the terminal reset and require its acknowledgement."""
        ...


class BbioSession:
    """Own the transition between the user terminal and binary modes."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport

    def enter_binary(self) -> None:
        """Send the NUL entry sequence and require a ``BBIO1`` banner.

        One leading NUL exits any active binary submode; the remaining twenty
        NULs are the documented binary-bitbang entry sequence. Keeping them in
        one write avoids leaving a stale mode banner between the two steps.
        """
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


class BitbangController:
    """Own binary bitbang mode and drive pins, bits, and BBIO housekeeping."""

    def __init__(self, transport: Transport, *, reset_on_exit: bool = True) -> None:
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._reset_on_exit = reset_on_exit
        self._direction_mask = int(BitbangDirection(0x1F))
        self._state_mask = 0

    def __enter__(self) -> BitbangController:
        """Enter binary bitbang mode."""
        self._bbio.enter_binary()
        self._direction_mask = int(BitbangDirection(0x1F))
        self._state_mask = 0
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        """Reset to the user terminal after a successful operation."""
        if exc_type is None and self._reset_on_exit:
            self._bbio.reset()

    def version(self) -> bytes:
        """Request the mode identifier with ``0x00`` and require ``BBIO1``."""
        self._transport.write(bytes([BITBANG_MODE_VERSION_COMMAND]))
        banner = self._transport.read_exact(len(BBIO_BANNER))
        if banner != BBIO_BANNER:
            raise ProtocolError(f"Expected BBIO1 banner, received {_describe(banner)}")
        return banner

    def configure_directions(self, mask: int) -> int:
        """Set the pin-direction mask and return the resulting pin state."""
        if not 0 <= mask <= 0x1F:
            raise ValueError(f"Pin direction mask must be 0 through 0x1F, got {mask}")
        self._direction_mask = mask
        self._transport.write(bytes([BITBANG_DIRECTION_COMMAND | mask]))
        return self._transport.read_exact(1)[0]

    def set_pins(self, mask: int) -> int:
        """Set output/power/pull-up state and return the resulting pin state."""
        if not 0 <= mask <= 0x7F:
            raise ValueError(f"Pin state mask must be 0 through 0x7F, got {mask}")
        self._state_mask = mask
        self._transport.write(bytes([BITBANG_STATE_COMMAND | mask]))
        return self._transport.read_exact(1)[0]

    def read_pins(self) -> int:
        """Read the pin state without changing directions."""
        self._transport.write(bytes([BITBANG_DIRECTION_COMMAND | self._direction_mask]))
        return self._transport.read_exact(1)[0]

    def set_power(self, enabled: bool) -> int:
        """Enable or disable the on-board power supplies."""
        return self._set_state_bit(BitbangState.POWER, enabled)

    def set_pullups(self, enabled: bool) -> int:
        """Enable or disable the on-board pull-up resistors."""
        return self._set_state_bit(BitbangState.PULLUP, enabled)

    def set_aux(self, high: bool) -> int:
        """Drive the AUX pin high or low."""
        return self._set_state_bit(BitbangState.AUX, high)

    def set_cs(self, high: bool) -> int:
        """Drive the CS pin high or low."""
        return self._set_state_bit(BitbangState.CS, high)

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        """Clock ``count`` bits out on MOSI, MSB first unless asked otherwise."""
        _validate_bit_count(count)
        if not 0 <= value < (1 << count):
            raise ValueError(f"Bit value 0x{value:X} does not fit in {count} bits")
        self._ensure_output_pins()
        self._set_state_mask(self._state_mask & ~int(BitbangState.CLK))
        for index in range(count):
            bit_index = index if lsb_first else count - 1 - index
            high = bool(value & (1 << bit_index))
            mosi_mask = int(BitbangState.MOSI)
            next_mask = self._state_mask | mosi_mask if high else self._state_mask & ~mosi_mask
            self._set_state_mask(next_mask)
            self._set_state_mask(self._state_mask | int(BitbangState.CLK))
            self._set_state_mask(self._state_mask & ~int(BitbangState.CLK))

    def read_bits(self, count: int, *, lsb_first: bool = False) -> int:
        """Clock ``count`` bits in from MISO, assembling MSB first by default."""
        _validate_bit_count(count)
        self._ensure_output_pins()
        self._set_state_mask(self._state_mask & ~int(BitbangState.CLK))
        value = 0
        for index in range(count):
            high_state = self._set_state_mask(self._state_mask | int(BitbangState.CLK))
            bit = 1 if high_state & int(BitbangState.MISO) else 0
            if lsb_first:
                value |= bit << index
            else:
                value = (value << 1) | bit
            self._set_state_mask(self._state_mask & ~int(BitbangState.CLK))
        return value

    def write_bytes(self, data: bytes, *, lsb_first: bool = False) -> None:
        """Clock each byte out on MOSI."""
        if not data:
            raise ValueError("Bitbang byte write requires at least one byte")
        for value in data:
            self.write_bits(value, 8, lsb_first=lsb_first)

    def read_bytes(self, count: int, *, lsb_first: bool = False) -> bytes:
        """Clock ``count`` bytes in from MISO."""
        if count < 1:
            raise ValueError("Bitbang byte read count must be positive")
        return bytes(self.read_bits(8, lsb_first=lsb_first) for _ in range(count))

    def self_test(self, *, long_test: bool = False) -> SelfTestReport:
        """Run the firmware self-test, exit its loop, and return the error count."""
        command = BITBANG_FULL_SELF_TEST if long_test else BITBANG_SHORT_SELF_TEST
        self._transport.write(bytes([command]))
        error_count = self._transport.read_exact(1)[0]
        self._transport.write(bytes([BITBANG_SELF_TEST_EXIT]))
        status = self._transport.read_exact(1)
        if status != bytes([BBIO_SUCCESS]):
            raise ProtocolError(f"Self-test did not acknowledge exit; received {_describe(status)}")
        return SelfTestReport(error_count=error_count, long_test=long_test)

    def read_adc(self) -> int:
        """Request one raw ADC sample."""
        self._transport.write(bytes([BITBANG_ADC_ONESHOT]))
        sample = self._transport.read_exact(2)
        value = int.from_bytes(sample, "big")
        if value > BITBANG_ADC_MAX:
            raise ProtocolError(f"ADC sample 0x{value:04X} exceeds the 12-bit range")
        return value

    def setup_pwm(self, *, prescaler: int, duty: int, period: int) -> None:
        """Configure the AUX PWM output with the firmware's raw parameters."""
        if not 0 <= prescaler <= 3:
            raise ValueError(f"PWM prescaler must be 0 through 3, got {prescaler}")
        if not 0 <= duty <= 0xFFFF:
            raise ValueError(f"PWM duty must be 0 through 0xFFFF, got {duty}")
        if not 0 <= period <= 0xFFFF:
            raise ValueError(f"PWM period must be 0 through 0xFFFF, got {period}")
        frame = bytes(
            [
                BITBANG_SETUP_PWM,
                prescaler,
                (duty >> 8) & 0xFF,
                duty & 0xFF,
                (period >> 8) & 0xFF,
                period & 0xFF,
            ]
        )
        self._transport.write(frame)
        self._bbio.expect_success()

    def clear_pwm(self) -> None:
        """Stop and release the AUX PWM output."""
        self._transport.write(bytes([BITBANG_CLEAR_PWM]))
        self._bbio.expect_success()

    def reset_to_terminal(self) -> None:
        """Send the terminal reset and require its acknowledgement."""
        self._bbio.reset()

    def _set_state_bit(self, bit: BitbangState, enabled: bool) -> int:
        mask = self._state_mask | int(bit) if enabled else self._state_mask & ~int(bit)
        return self._set_state_mask(mask)

    def _set_state_mask(self, mask: int) -> int:
        self._state_mask = mask
        self._transport.write(bytes([BITBANG_STATE_COMMAND | mask]))
        return self._transport.read_exact(1)[0]

    def _ensure_output_pins(self) -> None:
        output_mask = self._direction_mask & ~int(BitbangDirection.MOSI | BitbangDirection.CLK)
        if output_mask != self._direction_mask:
            self.configure_directions(output_mask)


def _validate_bit_count(count: int) -> None:
    if not 1 <= count <= 32:
        raise ValueError(f"Bit count must be 1 through 32, got {count}")


BitbangSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[BitbangSession]]


@contextmanager
def open_bitbang_session(
    connection: ConnectionSettings,
    *,
    reset_on_exit: bool = True,
) -> Generator[BitbangSession, None, None]:
    """Open a serial transport and an initialized binary-bitbang session."""
    with SerialTransport(connection) as transport:
        with BitbangController(transport, reset_on_exit=reset_on_exit) as controller:
            yield controller


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
