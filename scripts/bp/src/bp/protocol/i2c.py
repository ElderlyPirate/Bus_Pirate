"""Binary I2C operations and external EEPROM transfers."""

from __future__ import annotations

import time
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from types import TracebackType
from typing import Protocol

from bp.errors import EepromVerifyError, EepromWriteError, ProtocolError
from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings, EepromLayout

I2C_START = 0x02
I2C_STOP = 0x03
I2C_READ = 0x04
I2C_ACK = 0x06
I2C_NACK = 0x07
I2C_BULK_BASE = 0x10
I2C_MAX_BULK = 16
I2C_ACK_BYTE = 0x00
DEFAULT_WRITE_ATTEMPTS = 100
DEFAULT_RETRY_DELAY = 0.005


@dataclass(frozen=True, slots=True)
class EepromWriteReport:
    """Outcome of a sequential, non-paged EEPROM write."""

    bytes_written: int
    retries: int


class EepromSession(Protocol):
    """EEPROM operations consumed by the ``eeprom`` command layer."""

    def read_eeprom(self, layout: EepromLayout, length: int) -> bytes:
        """Read ``length`` sequential bytes."""
        ...

    def write_eeprom(self, layout: EepromLayout, data: bytes) -> EepromWriteReport:
        """Write bytes one at a time without page writes."""
        ...

    def verify_eeprom(self, layout: EepromLayout, expected: bytes) -> None:
        """Read and compare bytes, raising on the first mismatch."""
        ...


class I2CController:
    """Own a binary I2C session and implement EEPROM transfers."""

    def __init__(
        self,
        transport: Transport,
        *,
        write_attempts: int = DEFAULT_WRITE_ATTEMPTS,
        retry_delay: float = DEFAULT_RETRY_DELAY,
    ) -> None:
        if write_attempts < 1:
            raise ValueError("EEPROM write attempts must be positive")
        if retry_delay < 0:
            raise ValueError("EEPROM retry delay must not be negative")
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._write_attempts = write_attempts
        self._retry_delay = retry_delay

    def __enter__(self) -> I2CController:
        """Enter binary I2C mode and enable power and pull-ups."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.I2C)
        self._bbio.configure_peripherals(power=True, pullups=True)
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

    def start(self) -> None:
        """Issue an I2C start condition."""
        self._transport.write(bytes([I2C_START]))
        self._bbio.expect_success()

    def stop(self) -> None:
        """Issue an I2C stop condition."""
        self._transport.write(bytes([I2C_STOP]))
        self._bbio.expect_success()

    def write_bytes(self, data: bytes) -> tuple[bool, ...]:
        """Bulk-write one to sixteen bytes and return each ACK result."""
        if not 1 <= len(data) <= I2C_MAX_BULK:
            raise ValueError(f"I2C bulk writes require 1 through {I2C_MAX_BULK} bytes")
        self._transport.write(bytes([I2C_BULK_BASE | (len(data) - 1)]) + data)
        self._bbio.expect_success()
        acknowledgements = self._transport.read_exact(len(data))
        return tuple(value == I2C_ACK_BYTE for value in acknowledgements)

    def read_byte(self) -> int:
        """Read one I2C byte without sending its ACK or NACK yet."""
        self._transport.write(bytes([I2C_READ]))
        return self._transport.read_exact(1)[0]

    def acknowledge(self) -> None:
        """Send an ACK for the previous read byte."""
        self._transport.write(bytes([I2C_ACK]))
        self._bbio.expect_success()

    def not_acknowledge(self) -> None:
        """Send a NACK for the previous read byte."""
        self._transport.write(bytes([I2C_NACK]))
        self._bbio.expect_success()

    def read_eeprom(self, layout: EepromLayout, length: int) -> bytes:
        """Read sequential bytes using a write-address-set then read transaction."""
        layout.ensure_span(length)
        self._set_memory_pointer(layout, layout.memory_address)
        self.start()
        read_address = layout.write_address_for(layout.memory_address) | 0x01
        read_acks = self.write_bytes(bytes([read_address]))
        if not all(read_acks):
            self.stop()
            raise ProtocolError(
                f"EEPROM at chip address {layout.chip.value} did not acknowledge read address 0x{read_address:02X}"
            )

        payload = bytearray()
        for index in range(length):
            payload.append(self.read_byte())
            if index + 1 < length:
                self.acknowledge()
            else:
                self.not_acknowledge()
        self.stop()
        return bytes(payload)

    def write_eeprom(self, layout: EepromLayout, data: bytes) -> EepromWriteReport:
        """Write bytes sequentially without page writes, retrying NACKed writes."""
        layout.ensure_span(len(data))
        retries = 0
        for offset, value in enumerate(data):
            address = layout.memory_address + offset
            frame = bytes([layout.write_address_for(address)]) + layout.address_bytes_for(address) + bytes([value])
            for attempt in range(1, self._write_attempts + 1):
                self.start()
                acknowledgements = self.write_bytes(frame)
                self.stop()
                if all(acknowledgements):
                    break
                if attempt == self._write_attempts:
                    raise EepromWriteError(address, value, self._write_attempts)
                retries += 1
                if self._retry_delay:
                    time.sleep(self._retry_delay)
        return EepromWriteReport(bytes_written=len(data), retries=retries)

    def verify_eeprom(self, layout: EepromLayout, expected: bytes) -> None:
        """Read the expected span and raise at the first differing byte."""
        actual = self.read_eeprom(layout, len(expected))
        for offset, (wanted, received) in enumerate(zip(expected, actual, strict=True)):
            if wanted != received:
                raise EepromVerifyError(offset, wanted, received)

    def _set_memory_pointer(self, layout: EepromLayout, address: int) -> None:
        frame = bytes([layout.write_address_for(address)]) + layout.address_bytes_for(address)
        self.start()
        acknowledgements = self.write_bytes(frame)
        self.stop()
        if not all(acknowledgements):
            raise ProtocolError(f"EEPROM at chip address {layout.chip.value} did not acknowledge memory address 0x{address:04X}")


EepromSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[EepromSession]]


@contextmanager
def open_eeprom_session(connection: ConnectionSettings) -> Generator[EepromSession, None, None]:
    """Open a serial transport and an initialized binary I2C session."""
    with SerialTransport(connection) as transport:
        with I2CController(transport) as controller:
            yield controller
