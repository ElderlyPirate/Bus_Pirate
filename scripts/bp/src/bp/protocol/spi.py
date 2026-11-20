"""Binary SPI operations used by the LCD adapter v2 production test."""

from __future__ import annotations

import time
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntEnum
from types import TracebackType
from typing import Protocol

from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings

SPI_CS_LOW = 0x02
SPI_CS_HIGH = 0x03
SPI_WRITE_READ_BASE = 0x10
SPI_SET_SPEED = 0x60
SPI_CONFIG_BASE = 0x80
SPI_OUTPUT_3V3 = 0x08
SPI_IDLE_HIGH = 0x04
SPI_ACTIVE_TO_IDLE = 0x02
SPI_SAMPLE_END = 0x01
SPI_MAX_BULK = 16
DEFAULT_BLINK_DELAY = 0.001


class SpiSpeed(IntEnum):
    """Binary SPI speed codes in the order the firmware accepts them."""

    KHZ_30 = 0
    KHZ_125 = 1
    KHZ_250 = 2
    MHZ_1 = 3
    MHZ_2 = 4
    MHZ_2_6 = 5
    MHZ_4 = 6
    MHZ_8 = 7
    KHZ_50 = 8
    MHZ_1_3 = 9
    MHZ_3_2 = 10
    MHZ_5_3 = 11


@dataclass(frozen=True, slots=True)
class SpiConfig:
    """SPI output type, clock polarity, edge, and sample point."""

    output_3v3: bool = True
    clock_idle_high: bool = False
    active_to_idle: bool = True
    sample_end: bool = False


class SpiSession(Protocol):
    """SPI operations consumed by the command layer."""

    def configure_speed(self, speed: SpiSpeed) -> None:
        """Set the SPI clock speed."""
        ...

    def configure_spi(
        self,
        *,
        output_3v3: bool,
        clock_idle_high: bool,
        active_to_idle: bool,
        sample_end: bool,
    ) -> None:
        """Apply the SPI output type, clock polarity, edge, and sample point."""
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

    def set_cs(self, *, low: bool) -> None:
        """Drive the hardware CS pin low or high."""
        ...

    def transfer(self, value: int) -> int:
        """Clock one byte out and return the byte clocked in."""
        ...

    def bulk_transfer(self, data: bytes) -> bytes:
        """Clock one through sixteen bytes and return the received bytes."""
        ...

    def write_bytes(self, data: bytes) -> bytes:
        """Clock bytes in chunks and return all received bytes."""
        ...

    def read_bytes(self, count: int) -> bytes:
        """Clock ``count`` dummy bytes and return the received bytes."""
        ...


class LcdTestSession(Protocol):
    """One LCD adapter v2 test cycle consumed by the command layer."""

    def run_cycle(self) -> None:
        """Power the adapter, emit the blink pattern, and power it down."""
        ...


class SpiController:
    """Own a binary SPI session and execute the LCD adapter v2 test cycle."""

    def __init__(self, transport: Transport, *, blink_delay: float = DEFAULT_BLINK_DELAY) -> None:
        if blink_delay < 0:
            raise ValueError("LCD blink delay must not be negative")
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._blink_delay = blink_delay

    def __enter__(self) -> SpiController:
        """Enter binary SPI mode."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.SPI)
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        """Return to bitbang mode and reset after a successful session."""
        if exc_type is None:
            self._bbio.exit_to_bitbang()
            self._bbio.reset()

    def configure_spi(
        self,
        *,
        output_3v3: bool,
        clock_idle_high: bool,
        active_to_idle: bool,
        sample_end: bool,
    ) -> None:
        """Configure the SPI output type, clock polarity, edge, and sample point."""
        configuration = 0
        if output_3v3:
            configuration |= SPI_OUTPUT_3V3
        if clock_idle_high:
            configuration |= SPI_IDLE_HIGH
        if active_to_idle:
            configuration |= SPI_ACTIVE_TO_IDLE
        if sample_end:
            configuration |= SPI_SAMPLE_END
        self._transport.write(bytes([SPI_CONFIG_BASE | configuration]))
        self._bbio.expect_success()

    def configure_speed(self, speed: SpiSpeed) -> None:
        """Set the SPI clock speed."""
        self._transport.write(bytes([SPI_SET_SPEED | int(speed)]))
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

    def set_cs(self, *, low: bool) -> None:
        """Drive the hardware CS pin low or high."""
        self._transport.write(bytes([SPI_CS_LOW if low else SPI_CS_HIGH]))
        self._bbio.expect_success()

    def transfer(self, value: int) -> int:
        """Clock one byte out and return the byte clocked in."""
        if not 0 <= value <= 0xFF:
            raise ValueError(f"SPI byte must be 0 through 0xFF, got {value}")
        self._transport.write(bytes([SPI_WRITE_READ_BASE, value]))
        self._bbio.expect_success()
        return self._transport.read_exact(1)[0]

    def bulk_transfer(self, data: bytes) -> bytes:
        """Clock one through sixteen bytes and return the received bytes."""
        if not 1 <= len(data) <= SPI_MAX_BULK:
            raise ValueError(f"SPI bulk transfer requires 1 through {SPI_MAX_BULK} bytes")
        self._transport.write(bytes([SPI_WRITE_READ_BASE | (len(data) - 1)]) + data)
        self._bbio.expect_success()
        return self._transport.read_exact(len(data))

    def write_bytes(self, data: bytes) -> bytes:
        """Clock bytes in chunks and return all received bytes."""
        if not data:
            raise ValueError("SPI byte write requires at least one byte")
        received = bytearray()
        for offset in range(0, len(data), SPI_MAX_BULK):
            received.extend(self.bulk_transfer(data[offset : offset + SPI_MAX_BULK]))
        return bytes(received)

    def read_bytes(self, count: int) -> bytes:
        """Clock ``count`` dummy ``0xFF`` bytes and return the received bytes."""
        if count < 1:
            raise ValueError("SPI byte read count must be positive")
        received = bytearray()
        remaining = count
        while remaining:
            chunk = min(remaining, SPI_MAX_BULK)
            received.extend(self.bulk_transfer(b"\xff" * chunk))
            remaining -= chunk
        return bytes(received)

    def run_cycle(self) -> None:
        """Run one LCD adapter v2 power-on, blink, power-off cycle."""
        self._bbio.configure_peripherals(power=True)
        self.configure_spi(
            output_3v3=True,
            clock_idle_high=False,
            active_to_idle=True,
            sample_end=False,
        )

        self.transfer(0x00)
        self.set_cs(low=False)
        self.set_cs(low=True)
        self.transfer(0xFF)
        self.set_cs(low=False)
        self.set_cs(low=True)
        if self._blink_delay:
            time.sleep(self._blink_delay)

        self._bbio.configure_peripherals(power=False)
        self.configure_spi(
            output_3v3=False,
            clock_idle_high=False,
            active_to_idle=True,
            sample_end=False,
        )


SpiSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[SpiSession]]


@contextmanager
def open_lcd_session(connection: ConnectionSettings) -> Generator[LcdTestSession, None, None]:
    """Open a serial transport and an initialized binary SPI session."""
    with SerialTransport(connection) as transport:
        with SpiController(transport) as controller:
            yield controller


@contextmanager
def open_spi_session(connection: ConnectionSettings) -> Generator[SpiSession, None, None]:
    """Open a serial transport and an initialized SPI session."""
    with SerialTransport(connection) as transport:
        with SpiController(transport) as controller:
            yield controller
