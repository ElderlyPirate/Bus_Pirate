# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Binary I2C operations and external EEPROM transfers."""

from __future__ import annotations

import time
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntEnum
from types import TracebackType
from typing import Protocol

from bp.errors import EepromVerifyError, EepromWriteError, I2cNackError, ProtocolError
from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings, EepromLayout

I2C_START = 0x02
I2C_STOP = 0x03
I2C_READ = 0x04
I2C_ACK = 0x06
I2C_NACK = 0x07
I2C_BULK_BASE = 0x10
I2C_SET_SPEED = 0x60
I2C_MAX_BULK = 16
I2C_ACK_BYTE = 0x00
I2C_MAX_ADDRESS = 0x7F
I2C_BUS_COMMAND_REPLIES = (0x00, 0x01)
DEFAULT_WRITE_ATTEMPTS = 100
DEFAULT_RETRY_DELAY = 0.005


class I2cSpeed(IntEnum):
    """Binary I2C clock speed codes."""

    KHZ_5 = 0
    KHZ_50 = 1
    KHZ_100 = 2
    KHZ_400 = 3


@dataclass(frozen=True, slots=True)
class I2cScanHit:
    """One 7-bit address that acknowledged a write and/or read probe."""

    address: int
    write_ack: bool
    read_ack: bool

    def __post_init__(self) -> None:
        if not 0 <= self.address <= I2C_MAX_ADDRESS:
            raise ValueError(f"I2C address must be 0 through 0x7F, got 0x{self.address:02X}")
        if not self.write_ack and not self.read_ack:
            raise ValueError("I2C scan hit requires at least one acknowledgement")

    @property
    def write_address(self) -> int:
        """Return the 8-bit write address."""
        return self.address << 1

    @property
    def read_address(self) -> int:
        """Return the 8-bit read address."""
        return (self.address << 1) | 1


class I2cDeviceSession(Protocol):
    """Register-level I2C operations consumed by the command layer."""

    def read_byte(self, register: int) -> int:
        """Read one byte from a one-byte register address."""
        ...

    def write_byte(self, register: int, value: int) -> None:
        """Write one byte to a one-byte register address."""
        ...

    def read_word(self, register: int) -> int:
        """Read a big-endian 16-bit register value."""
        ...

    def write_word(self, register: int, value: int) -> None:
        """Write a big-endian 16-bit register value."""
        ...

    def command(self, command: int) -> None:
        """Send a one-byte command with no register payload."""
        ...

    def read_block(self, register: int | None, length: int) -> bytes:
        """Read sequential bytes, optionally from a register pointer."""
        ...

    def write_block(self, register: int | None, data: bytes) -> None:
        """Write sequential bytes, optionally after a register pointer."""
        ...


class I2cControlSession(Protocol):
    """I2C control operations consumed by the command layer."""

    def configure_speed(self, speed: I2cSpeed) -> None:
        """Set the binary I2C bus speed."""
        ...

    def scan(self, *, start: int = 0, end: int = I2C_MAX_ADDRESS) -> tuple[I2cScanHit, ...]:
        """Probe 7-bit addresses and return every address that acknowledged."""
        ...

    def device(self, address: int) -> I2cDeviceSession:
        """Return a register-level helper for one 7-bit address."""
        ...


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
        power: bool = True,
        pullups: bool = True,
    ) -> None:
        if write_attempts < 1:
            raise ValueError("EEPROM write attempts must be positive")
        if retry_delay < 0:
            raise ValueError("EEPROM retry delay must not be negative")
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._write_attempts = write_attempts
        self._retry_delay = retry_delay
        self._power = power
        self._pullups = pullups

    def __enter__(self) -> I2CController:
        """Enter binary I2C mode and apply the configured power/pull-up flags."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.I2C)
        self._bbio.configure_peripherals(power=self._power, pullups=self._pullups)
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
        self._expect_bus_command_reply("start")

    def stop(self) -> None:
        """Issue an I2C stop condition."""
        self._transport.write(bytes([I2C_STOP]))
        self._expect_bus_command_reply("stop")

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
        self._expect_bus_command_reply("ACK")

    def not_acknowledge(self) -> None:
        """Send a NACK for the previous read byte."""
        self._transport.write(bytes([I2C_NACK]))
        self._expect_bus_command_reply("NACK")

    def configure_speed(self, speed: I2cSpeed) -> None:
        """Set the binary I2C bus speed."""
        self._transport.write(bytes([I2C_SET_SPEED | int(speed)]))
        self._bbio.expect_success()

    def device(self, address: int) -> I2cDevice:
        """Return a register-level helper for one 7-bit address."""
        return I2cDevice(self, address)

    def scan(self, *, start: int = 0, end: int = I2C_MAX_ADDRESS) -> tuple[I2cScanHit, ...]:
        """Probe every 7-bit address in the range and return the acknowledgements.

        Raises:
            ValueError: If the range is invalid or outside the 7-bit address space.
            ProtocolError: If the firmware rejects an I2C command.
        """
        if not 0 <= start <= end <= I2C_MAX_ADDRESS:
            raise ValueError(f"I2C scan range must satisfy 0 <= start <= end <= 0x7F; got 0x{start:02X} through 0x{end:02X}")
        hits = []
        for address in range(start, end + 1):
            write_ack = self._probe_address(address << 1)
            read_ack = self._probe_address((address << 1) | 1, read_byte=True)
            if write_ack or read_ack:
                hits.append(I2cScanHit(address=address, write_ack=write_ack, read_ack=read_ack))
        return tuple(hits)

    def _expect_bus_command_reply(self, operation: str) -> None:
        """Accept the two observed replies for an I2C bus-control command.

        The protocol reference specifies ``0x01`` for success, but the flashed
        BPv4 v9.0 firmware answers start, stop, ACK, and NACK with ``0x00``.
        Both values mean the command was consumed; data-phase failures are
        still reported by the bulk-write status and ACK bits.
        """
        status = self._transport.read_exact(1)[0]
        if status not in I2C_BUS_COMMAND_REPLIES:
            raise ProtocolError(f"I2C {operation} command returned 0x{status:02X}; expected 0x00 or 0x01")

    def _probe_address(self, value: int, *, read_byte: bool = False) -> bool:
        self.start()
        acknowledgements = self.write_bytes(bytes([value]))
        if not all(acknowledgements):
            self.stop()
            return False
        if read_byte:
            self.read_byte()
            self.not_acknowledge()
        self.stop()
        return True

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


class I2cDevice:
    """Register-level I2C helpers for one 7-bit device address."""

    def __init__(self, controller: I2CController, address: int) -> None:
        if not 0 <= address <= I2C_MAX_ADDRESS:
            raise ValueError(f"I2C address must be 0 through 0x7F, got 0x{address:02X}")
        self._controller = controller
        self._address = address

    @property
    def address(self) -> int:
        """Return the 7-bit device address."""
        return self._address

    @property
    def write_address(self) -> int:
        """Return the 8-bit write address."""
        return self._address << 1

    @property
    def read_address(self) -> int:
        """Return the 8-bit read address."""
        return (self._address << 1) | 1

    def read_byte(self, register: int) -> int:
        """Read one byte from a one-byte register address."""
        self._begin_register_read(register)
        value = self._controller.read_byte()
        self._controller.not_acknowledge()
        self._controller.stop()
        return value

    def write_byte(self, register: int, value: int) -> None:
        """Write one byte to a one-byte register address."""
        _validate_byte(register, "I2C register")
        _validate_byte(value, "I2C value")
        self._controller.start()
        acknowledgements = self._controller.write_bytes(bytes([self.write_address, register, value]))
        self._controller.stop()
        if not all(acknowledgements):
            raise I2cNackError(self._address, "register write")

    def read_word(self, register: int) -> int:
        """Read a big-endian 16-bit register value."""
        self._begin_register_read(register)
        high = self._controller.read_byte()
        self._controller.acknowledge()
        low = self._controller.read_byte()
        self._controller.not_acknowledge()
        self._controller.stop()
        return (high << 8) | low

    def write_word(self, register: int, value: int) -> None:
        """Write a big-endian 16-bit register value."""
        _validate_byte(register, "I2C register")
        if not 0 <= value <= 0xFFFF:
            raise ValueError(f"I2C word value must be 0 through 0xFFFF, got 0x{value:04X}")
        payload = bytes([self.write_address, register, (value >> 8) & 0xFF, value & 0xFF])
        self._controller.start()
        acknowledgements = self._controller.write_bytes(payload)
        self._controller.stop()
        if not all(acknowledgements):
            raise I2cNackError(self._address, "register word write")

    def command(self, command: int) -> None:
        """Send a one-byte command with no register payload."""
        _validate_byte(command, "I2C command")
        self._controller.start()
        acknowledgements = self._controller.write_bytes(bytes([self.write_address, command]))
        self._controller.stop()
        if not all(acknowledgements):
            raise I2cNackError(self._address, "command")

    def read_block(self, register: int | None, length: int) -> bytes:
        """Read ``length`` sequential bytes, optionally from a register pointer."""
        if length < 1:
            raise ValueError("I2C block read length must be positive")
        if register is not None:
            _validate_byte(register, "I2C register")
            self._controller.start()
            acknowledgements = self._controller.write_bytes(bytes([self.write_address, register]))
            if not all(acknowledgements):
                self._controller.stop()
                raise I2cNackError(self._address, "register address")
            self._controller.start()
        else:
            self._controller.start()
        acknowledgements = self._controller.write_bytes(bytes([self.read_address]))
        if not all(acknowledgements):
            self._controller.stop()
            raise I2cNackError(self._address, "read address")
        payload = bytearray()
        for index in range(length):
            payload.append(self._controller.read_byte())
            if index + 1 < length:
                self._controller.acknowledge()
            else:
                self._controller.not_acknowledge()
        self._controller.stop()
        return bytes(payload)

    def write_block(self, register: int | None, data: bytes) -> None:
        """Write sequential bytes, optionally after a register pointer."""
        if not data:
            raise ValueError("I2C block write requires at least one byte")
        if register is not None:
            _validate_byte(register, "I2C register")
        prefix = bytes([self.write_address]) + (bytes([register]) if register is not None else b"")
        payload = prefix + data
        self._controller.start()
        acknowledgements: list[bool] = []
        for offset in range(0, len(payload), I2C_MAX_BULK):
            chunk = payload[offset : offset + I2C_MAX_BULK]
            acknowledgements.extend(self._controller.write_bytes(chunk))
        self._controller.stop()
        if not all(acknowledgements):
            raise I2cNackError(self._address, "block write")

    def _begin_register_read(self, register: int) -> None:
        _validate_byte(register, "I2C register")
        self._controller.start()
        acknowledgements = self._controller.write_bytes(bytes([self.write_address, register]))
        if not all(acknowledgements):
            self._controller.stop()
            raise I2cNackError(self._address, "register address")
        self._controller.start()
        acknowledgements = self._controller.write_bytes(bytes([self.read_address]))
        if not all(acknowledgements):
            self._controller.stop()
            raise I2cNackError(self._address, "read address")


def _validate_byte(value: int, label: str) -> None:
    if not 0 <= value <= 0xFF:
        raise ValueError(f"{label} must be 0 through 0xFF, got {value}")


I2cControlSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[I2cControlSession]]
EepromSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[EepromSession]]


@contextmanager
def open_eeprom_session(connection: ConnectionSettings) -> Generator[EepromSession, None, None]:
    """Open a serial transport and an initialized binary I2C session."""
    with SerialTransport(connection) as transport:
        with I2CController(transport) as controller:
            yield controller


@contextmanager
def open_i2c_session(
    connection: ConnectionSettings,
    *,
    power: bool = False,
    pullups: bool = True,
) -> Generator[I2cControlSession, None, None]:
    """Open a serial transport and an initialized I2C control session."""
    with SerialTransport(connection) as transport:
        with I2CController(transport, power=power, pullups=pullups) as controller:
            yield controller
