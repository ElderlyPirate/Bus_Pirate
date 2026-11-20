"""Binary raw 2-wire and 3-wire operations."""

from __future__ import annotations

import logging
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntEnum
from types import TracebackType
from typing import Protocol

from bp.errors import ProtocolError
from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings

logger = logging.getLogger(__name__)

RAW_WIRE_EXIT = 0x00
RAW_WIRE_VERSION = 0x01
RAW_WIRE_START = 0x02
RAW_WIRE_STOP = 0x03
RAW_WIRE_CS_LOW = 0x04
RAW_WIRE_CS_HIGH = 0x05
RAW_WIRE_READ_BYTE = 0x06
RAW_WIRE_READ_BIT = 0x07
RAW_WIRE_PEEK = 0x08
RAW_WIRE_CLOCK_TICK = 0x09
RAW_WIRE_CLOCK_LOW = 0x0A
RAW_WIRE_CLOCK_HIGH = 0x0B
RAW_WIRE_DATA_LOW = 0x0C
RAW_WIRE_DATA_HIGH = 0x0D
RAW_WIRE_BULK_BYTE = 0x10
RAW_WIRE_BULK_CLOCK = 0x20
RAW_WIRE_SET_SPEED = 0x60
RAW_WIRE_CONFIG = 0x80
RAW_WIRE_MAX_BULK = 16
RAW_WIRE_CONFIG_OUTPUT = 0x00
RAW_WIRE_CONFIG_HIGH_IMPEDANCE = 0x08
RAW_WIRE_CONFIG_THREE_WIRE = 0x04
RAW_WIRE_CONFIG_LSB_FIRST = 0x02


class RawWireSpeed(IntEnum):
    """Raw-wire clock speed codes."""

    KHZ_5 = 0
    KHZ_50 = 1
    KHZ_100 = 2
    KHZ_400 = 3


@dataclass(frozen=True, slots=True)
class RawWireConfig:
    """Raw-wire output type, wire count, and bit order.

    The firmware's configuration bit 3 selects high-impedance output, so
    ``output_3v3=True`` clears it.
    """

    output_3v3: bool = True
    three_wire: bool = True
    lsb_first: bool = False

    @property
    def configuration_byte(self) -> int:
        """Return the ``0x80`` configuration payload for this mode."""
        value = RAW_WIRE_CONFIG_OUTPUT if self.output_3v3 else RAW_WIRE_CONFIG_HIGH_IMPEDANCE
        if self.three_wire:
            value |= RAW_WIRE_CONFIG_THREE_WIRE
        if self.lsb_first:
            value |= RAW_WIRE_CONFIG_LSB_FIRST
        return value


class RawWireSession(Protocol):
    """Raw-wire operations consumed by the command layer."""

    def configure(self, config: RawWireConfig) -> None:
        """Apply output type, wire count, and bit order."""
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

    def set_speed(self, speed: RawWireSpeed) -> None:
        """Set the raw-wire clock speed."""
        ...

    def set_cs(self, *, low: bool) -> None:
        """Drive CS low or high."""
        ...

    def start(self) -> None:
        """Emit a 2-wire start condition."""
        ...

    def stop(self) -> None:
        """Emit a 2-wire stop condition."""
        ...

    def clock_tick(self) -> None:
        """Advance the clock by one tick."""
        ...

    def clock_high(self) -> None:
        """Drive the clock high."""
        ...

    def clock_low(self) -> None:
        """Drive the clock low."""
        ...

    def data_high(self) -> None:
        """Drive the data line high."""
        ...

    def data_low(self) -> None:
        """Drive the data line low."""
        ...

    def read_byte(self) -> int:
        """Read one byte from the bus."""
        ...

    def read_bit(self) -> bool:
        """Read one bit from the bus."""
        ...

    def peek(self) -> bool:
        """Read the input pin without clocking."""
        ...

    def bulk_clock(self, ticks: int) -> None:
        """Emit one through sixteen clock ticks."""
        ...

    def write_bit(self, high: bool) -> None:
        """Write one bit to the bus."""
        ...

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        """Write ``count`` bits to the bus."""
        ...

    def write_byte(self, value: int) -> None:
        """Write one byte to the bus."""
        ...

    def write_bytes(self, data: bytes) -> None:
        """Write bytes to the bus."""
        ...

    def read_bytes(self, count: int) -> bytes:
        """Read bytes from the bus."""
        ...


class RawWireController:
    """Own a binary raw-wire session and implement its wire operations."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._config = RawWireConfig(output_3v3=False, three_wire=False, lsb_first=False)

    def __enter__(self) -> RawWireController:
        """Enter binary bitbang mode, then raw-wire mode."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.RAW_WIRE)
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

    def configure(self, config: RawWireConfig) -> None:
        """Apply output type, wire count, and bit order."""
        self._config = config
        self._send_checked(bytes([RAW_WIRE_CONFIG | config.configuration_byte]))

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

    def set_speed(self, speed: RawWireSpeed) -> None:
        """Set the raw-wire clock speed."""
        self._send_checked(bytes([RAW_WIRE_SET_SPEED | int(speed)]))

    def set_cs(self, *, low: bool) -> None:
        """Drive CS low or high."""
        self._send_checked(bytes([RAW_WIRE_CS_LOW if low else RAW_WIRE_CS_HIGH]))

    def start(self) -> None:
        """Emit a 2-wire start condition."""
        self._send_checked(bytes([RAW_WIRE_START]))

    def stop(self) -> None:
        """Emit a 2-wire stop condition."""
        self._send_checked(bytes([RAW_WIRE_STOP]))

    def clock_tick(self) -> None:
        """Advance the clock by one tick."""
        self._send_checked(bytes([RAW_WIRE_CLOCK_TICK]))

    def clock_high(self) -> None:
        """Drive the clock high."""
        self._send_checked(bytes([RAW_WIRE_CLOCK_HIGH]))

    def clock_low(self) -> None:
        """Drive the clock low."""
        self._send_checked(bytes([RAW_WIRE_CLOCK_LOW]))

    def data_high(self) -> None:
        """Drive the data line high."""
        self._send_checked(bytes([RAW_WIRE_DATA_HIGH]))

    def data_low(self) -> None:
        """Drive the data line low."""
        self._send_checked(bytes([RAW_WIRE_DATA_LOW]))

    def read_byte(self) -> int:
        """Read one byte from the bus."""
        self._transport.write(bytes([RAW_WIRE_READ_BYTE]))
        return self._transport.read_exact(1)[0]

    def read_bit(self) -> bool:
        """Read one bit from the bus."""
        self._transport.write(bytes([RAW_WIRE_READ_BIT]))
        value = self._transport.read_exact(1)[0]
        if value not in (0, 1):
            raise ProtocolError(f"Raw-wire bit read returned 0x{value:02X}, expected 0x00 or 0x01")
        return value == 1

    def peek(self) -> bool:
        """Read the input pin without clocking."""
        self._transport.write(bytes([RAW_WIRE_PEEK]))
        value = self._transport.read_exact(1)[0]
        if value not in (0, 1):
            raise ProtocolError(f"Raw-wire pin peek returned 0x{value:02X}, expected 0x00 or 0x01")
        return value == 1

    def bulk_clock(self, ticks: int) -> None:
        """Emit one through sixteen clock ticks."""
        if not 1 <= ticks <= RAW_WIRE_MAX_BULK:
            raise ValueError(f"Raw-wire bulk clock requires 1 through {RAW_WIRE_MAX_BULK} ticks, got {ticks}")
        self._send_checked(bytes([RAW_WIRE_BULK_CLOCK | (ticks - 1)]))

    def write_bit(self, high: bool) -> None:
        """Write one bit to the bus."""
        if high:
            self.data_high()
        else:
            self.data_low()
        self.clock_tick()

    def write_bits(self, value: int, count: int, *, lsb_first: bool = False) -> None:
        """Write ``count`` bits to the bus, MSB first unless asked otherwise."""
        if not 1 <= count <= 32:
            raise ValueError(f"Bit count must be 1 through 32, got {count}")
        if not 0 <= value < (1 << count):
            raise ValueError(f"Bit value 0x{value:X} does not fit in {count} bits")
        for index in range(count):
            bit_index = index if lsb_first else count - 1 - index
            self.write_bit(bool(value & (1 << bit_index)))

    def write_byte(self, value: int) -> None:
        """Write one byte to the bus."""
        if not 0 <= value <= 0xFF:
            raise ValueError(f"Raw-wire byte must be 0 through 0xFF, got {value}")
        self.write_bytes(bytes([value]))

    def write_bytes(self, data: bytes) -> None:
        """Write bytes to the bus.

        Two-wire mode uses the firmware bulk byte command and consumes its
        per-byte acknowledgement. Three-wire mode clocks each bit explicitly
        because the firmware's three-wire bulk path returns no per-byte status
        and re-clocks its received value.
        """
        if not data:
            raise ValueError("Raw-wire byte write requires at least one byte")
        if self._config.three_wire:
            self.clock_low()
            for value in data:
                self.write_bits(value, 8, lsb_first=self._config.lsb_first)
            return

        for offset in range(0, len(data), RAW_WIRE_MAX_BULK):
            chunk = data[offset : offset + RAW_WIRE_MAX_BULK]
            self._transport.write(bytes([RAW_WIRE_BULK_BYTE | (len(chunk) - 1)]))
            self._bbio.expect_success()
            for value in chunk:
                self._transport.write(bytes([value]))
                self._bbio.expect_success()

    def read_bytes(self, count: int) -> bytes:
        """Read ``count`` bytes from the bus."""
        if count < 1:
            raise ValueError("Raw-wire byte read count must be positive")
        return bytes(self.read_byte() for _ in range(count))

    def _send_checked(self, command: bytes) -> None:
        self._transport.write(command)
        self._bbio.expect_success()


RawWireSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[RawWireSession]]


@contextmanager
def open_raw_wire_session(connection: ConnectionSettings) -> Generator[RawWireSession, None, None]:
    """Open a serial transport and an initialized binary raw-wire session."""
    with SerialTransport(connection) as transport:
        with RawWireController(transport) as controller:
            yield controller
