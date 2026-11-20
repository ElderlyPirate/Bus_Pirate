"""Binary UART operations."""

from __future__ import annotations

import logging
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntEnum
from types import TracebackType
from typing import Protocol

from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings

logger = logging.getLogger(__name__)

UART_BEGIN_INPUT = 0x02
UART_END_INPUT = 0x03
UART_SET_MANUAL_SPEED = 0x07
UART_BRIDGE = 0x0F
UART_BULK_BASE = 0x10
UART_SET_SPEED = 0x60
UART_CONFIGURE = 0x80
UART_MAX_BULK = 16
UART_MAX_BRG = 0xFFFF

UART_OUTPUT_NORMAL = 0x10
UART_STOP_TWO = 0x02
UART_INVERTED = 0x01


class UartSpeed(IntEnum):
    """Binary UART speed codes in firmware table order."""

    BPS_300 = 0
    BPS_1200 = 1
    BPS_2400 = 2
    BPS_4800 = 3
    BPS_9600 = 4
    BPS_19200 = 5
    BPS_38400 = 6
    BPS_57600 = 7
    BPS_115200 = 8
    BPS_31250 = 9


class UartDataBits(IntEnum):
    """Binary UART data-bit and parity configuration values."""

    EIGHT_NONE = 0x00
    EIGHT_EVEN = 0x04
    EIGHT_ODD = 0x08
    NINE_NONE = 0x0C


@dataclass(frozen=True, slots=True)
class UartConfig:
    """Binary UART frame format and output type."""

    data_bits: UartDataBits = UartDataBits.EIGHT_NONE
    stop_bits: int = 1
    inverted: bool = False
    normal_output: bool = True

    def __post_init__(self) -> None:
        if self.stop_bits not in (1, 2):
            raise ValueError(f"UART stop bits must be 1 or 2, got {self.stop_bits}")

    @property
    def configuration_byte(self) -> int:
        """Return the ``0x80`` configuration payload for this frame format."""
        value = int(self.data_bits)
        if self.normal_output:
            value |= UART_OUTPUT_NORMAL
        if self.stop_bits == 2:
            value |= UART_STOP_TWO
        if self.inverted:
            value |= UART_INVERTED
        return value


class UartSession(Protocol):
    """UART operations consumed by the command layer."""

    def configure_speed(self, speed: UartSpeed) -> None:
        """Set the UART baud rate from the firmware table."""
        ...

    def configure_manual_speed(self, brg: int) -> None:
        """Set the UART baud rate from a raw 16-bit BRG value."""
        ...

    def configure(self, config: UartConfig) -> None:
        """Apply the UART frame format and output type."""
        ...

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        """Set power, pull-up, AUX, and CS peripheral flags."""
        ...

    def write_bytes(self, data: bytes) -> None:
        """Send bytes over the UART."""
        ...

    def receive(self, count: int) -> bytes:
        """Enable receive echo, collect ``count`` bytes, and disable echo."""
        ...

    def begin_input(self) -> None:
        """Enable UART receive echo."""
        ...

    def end_input(self) -> None:
        """Disable UART receive echo."""
        ...

    def enter_bridge_mode(self) -> None:
        """Enter the firmware transparent bridge; only a reset exits it."""
        ...


class UartController:
    """Own a binary UART session and implement send, receive, and bridge setup."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._bridge_active = False

    def __enter__(self) -> UartController:
        """Enter binary bitbang mode, then UART mode."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.UART)
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        """Return to bitbang mode and reset after a successful non-bridge session."""
        if exc_type is None and not self._bridge_active:
            self._bbio.exit_to_bitbang()
            self._bbio.reset()

    def configure_speed(self, speed: UartSpeed) -> None:
        """Set the UART baud rate from the firmware table."""
        self._transport.write(bytes([UART_SET_SPEED | int(speed)]))
        self._bbio.expect_success()

    def configure_manual_speed(self, brg: int) -> None:
        """Set the UART baud rate from a raw 16-bit BRG value."""
        if not 0 <= brg <= UART_MAX_BRG:
            raise ValueError(f"UART BRG value must be 0 through 0xFFFF, got {brg}")
        self._transport.write(bytes([UART_SET_MANUAL_SPEED]))
        self._bbio.expect_success()
        self._transport.write(bytes([(brg >> 8) & 0xFF]))
        self._bbio.expect_success()
        self._transport.write(bytes([brg & 0xFF]))
        self._bbio.expect_success()

    def configure(self, config: UartConfig) -> None:
        """Apply the UART frame format and output type."""
        self._transport.write(bytes([UART_CONFIGURE | config.configuration_byte]))
        self._bbio.expect_success()

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = False,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        """Set power, pull-up, AUX, and CS peripheral flags."""
        self._bbio.configure_peripherals(power=power, pullups=pullups, aux=aux, cs=cs)

    def write_bytes(self, data: bytes) -> None:
        """Send bytes over the UART in firmware-sized bulk chunks."""
        if not data:
            raise ValueError("UART byte write requires at least one byte")
        for offset in range(0, len(data), UART_MAX_BULK):
            chunk = data[offset : offset + UART_MAX_BULK]
            self._transport.write(bytes([UART_BULK_BASE | (len(chunk) - 1)]))
            self._bbio.expect_success()
            for value in chunk:
                self._transport.write(bytes([value]))
                self._bbio.expect_success()

    def begin_input(self) -> None:
        """Enable UART receive echo."""
        self._transport.write(bytes([UART_BEGIN_INPUT]))
        self._bbio.expect_success()

    def end_input(self) -> None:
        """Disable UART receive echo."""
        self._transport.write(bytes([UART_END_INPUT]))
        self._bbio.expect_success()

    def receive(self, count: int) -> bytes:
        """Enable receive echo, collect ``count`` bytes, and disable echo."""
        if count < 1:
            raise ValueError("UART receive count must be positive")
        self.begin_input()
        data = self._transport.read_exact(count)
        self.end_input()
        return data

    def enter_bridge_mode(self) -> None:
        """Enter the firmware transparent bridge; only a hardware reset exits it."""
        logger.debug("Entering UART bridge mode")
        self._transport.write(bytes([UART_BRIDGE]))
        self._bbio.expect_success()
        self._bridge_active = True


UartSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[UartSession]]


@contextmanager
def open_uart_session(connection: ConnectionSettings) -> Generator[UartSession, None, None]:
    """Open a serial transport and an initialized UART session."""
    with SerialTransport(connection) as transport:
        with UartController(transport) as controller:
            yield controller
