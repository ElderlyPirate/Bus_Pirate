"""Binary SPI operations used by the LCD adapter v2 production test."""

from __future__ import annotations

import time
from collections.abc import Generator
from contextlib import contextmanager
from types import TracebackType
from typing import Protocol

from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings

SPI_CS_LOW = 0x02
SPI_CS_HIGH = 0x03
SPI_WRITE_READ_BASE = 0x10
SPI_CONFIG_BASE = 0x80
SPI_OUTPUT_3V3 = 0x08
SPI_IDLE_HIGH = 0x04
SPI_ACTIVE_TO_IDLE = 0x02
SPI_SAMPLE_END = 0x01
DEFAULT_BLINK_DELAY = 0.001


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


@contextmanager
def open_lcd_session(connection: ConnectionSettings) -> Generator[LcdTestSession, None, None]:
    """Open a serial transport and an initialized binary SPI session."""
    with SerialTransport(connection) as transport:
        with SpiController(transport) as controller:
            yield controller
