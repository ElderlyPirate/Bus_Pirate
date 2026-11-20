# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Binary 1-Wire operations and Dallas/Maxim ROM helpers."""

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

ONEWIRE_RESET = 0x02
ONEWIRE_READ_BYTE = 0x04
ONEWIRE_ROM_SEARCH = 0x08
ONEWIRE_ALARM_SEARCH = 0x09
ONEWIRE_BULK_BASE = 0x10
ONEWIRE_SET_SPEED = 0x20
ONEWIRE_MAX_BULK = 16
ONEWIRE_ROM_SIZE = 8
ONEWIRE_SEARCH_TERMINATOR = b"\xff" * ONEWIRE_ROM_SIZE
ONEWIRE_MAX_SEARCH_DEVICES = 64

ONEWIRE_READ_ROM = 0x33
ONEWIRE_MATCH_ROM = 0x55
ONEWIRE_SKIP_ROM = 0xCC
ONEWIRE_READ_SCRATCHPAD = 0xBE
ONEWIRE_WRITE_SCRATCHPAD = 0x4E
ONEWIRE_COPY_SCRATCHPAD = 0x48
ONEWIRE_READ_MEMORY = 0xF0
ONEWIRE_READ_STATUS = 0xAA


class OneWireSpeed(IntEnum):
    """Binary 1-Wire bus speed."""

    STANDARD = 0
    OVERDRIVE = 1


@dataclass(frozen=True, slots=True)
class OneWireRom:
    """An eight-byte 1-Wire ROM identifier."""

    data: bytes

    def __post_init__(self) -> None:
        if len(self.data) != ONEWIRE_ROM_SIZE:
            raise ValueError(f"1-Wire ROM must contain {ONEWIRE_ROM_SIZE} bytes, got {len(self.data)}")

    @property
    def family(self) -> int:
        """Return the family code."""
        return self.data[0]

    @property
    def serial(self) -> bytes:
        """Return the six serial-number bytes."""
        return self.data[1:7]

    @property
    def crc(self) -> int:
        """Return the stored CRC byte."""
        return self.data[7]

    @property
    def crc_valid(self) -> bool:
        """Return whether the stored CRC matches the first seven bytes."""
        return onewire_crc8(self.data[:7]) == self.crc

    @property
    def hex(self) -> str:
        """Return the ROM as uppercase hex."""
        return self.data.hex().upper()


def onewire_crc8(data: bytes) -> int:
    """Return the Dallas/Maxim CRC-8 of ``data``."""
    crc = 0
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8C if crc & 0x01 else crc >> 1
    return crc


class OneWireSession(Protocol):
    """1-Wire operations consumed by the command layer."""

    def configure_speed(self, speed: OneWireSpeed) -> None:
        """Set standard or overdrive speed."""
        ...

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = True,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        """Set power, pull-up, AUX, and CS peripheral flags."""
        ...

    def reset(self) -> None:
        """Perform a 1-Wire bus reset."""
        ...

    def read_byte(self) -> int:
        """Read one byte from the bus."""
        ...

    def read_bytes(self, count: int) -> bytes:
        """Read sequential bytes from the bus."""
        ...

    def write_bytes(self, data: bytes) -> None:
        """Write sequential bytes to the bus."""
        ...

    def search_rom(self) -> tuple[OneWireRom, ...]:
        """Enumerate every device with a ROM search."""
        ...

    def alarm_search(self) -> tuple[OneWireRom, ...]:
        """Enumerate every device in alarm state."""
        ...

    def read_rom(self) -> OneWireRom:
        """Reset the bus and read the ROM of a single device."""
        ...

    def match_rom(self, rom: OneWireRom) -> None:
        """Send a MATCH ROM command without resetting the bus."""
        ...

    def skip_rom(self) -> None:
        """Send a SKIP ROM command without resetting the bus."""
        ...

    def read_scratchpad(self, rom: OneWireRom | None, *, length: int = 9) -> bytes:
        """Read a device scratchpad after reset and ROM selection."""
        ...

    def write_scratchpad(self, rom: OneWireRom | None, data: bytes) -> None:
        """Write a device scratchpad after reset and ROM selection."""
        ...

    def copy_scratchpad(self, rom: OneWireRom | None) -> None:
        """Copy scratchpad contents after reset and ROM selection."""
        ...

    def read_memory(self, rom: OneWireRom | None, *, address: int, length: int) -> bytes:
        """Read device memory after reset and ROM selection."""
        ...

    def read_status(self, rom: OneWireRom | None, *, address: int) -> int:
        """Read one device status byte after reset and ROM selection."""
        ...


class OneWireController:
    """Own a binary 1-Wire session and implement ROM and memory operations."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport
        self._bbio = BbioSession(transport)

    def __enter__(self) -> OneWireController:
        """Enter binary bitbang mode, then 1-Wire mode."""
        self._bbio.enter_binary()
        self._bbio.enter_mode(BpBinaryMode.ONEWIRE)
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

    def configure_speed(self, speed: OneWireSpeed) -> None:
        """Set standard or overdrive speed."""
        self._send_checked(bytes([ONEWIRE_SET_SPEED | int(speed)]))

    def configure_peripherals(
        self,
        *,
        power: bool = False,
        pullups: bool = True,
        aux: bool = False,
        cs: bool = False,
    ) -> None:
        """Set power, pull-up, AUX, and CS peripheral flags."""
        self._bbio.configure_peripherals(power=power, pullups=pullups, aux=aux, cs=cs)

    def reset(self) -> None:
        """Perform a 1-Wire bus reset."""
        self._send_checked(bytes([ONEWIRE_RESET]))

    def read_byte(self) -> int:
        """Read one byte from the bus."""
        self._transport.write(bytes([ONEWIRE_READ_BYTE]))
        return self._transport.read_exact(1)[0]

    def read_bytes(self, count: int) -> bytes:
        """Read sequential bytes from the bus."""
        if count < 1:
            raise ValueError("1-Wire byte read count must be positive")
        return bytes(self.read_byte() for _ in range(count))

    def write_bytes(self, data: bytes) -> None:
        """Write sequential bytes to the bus."""
        if not data:
            raise ValueError("1-Wire byte write requires at least one byte")
        for offset in range(0, len(data), ONEWIRE_MAX_BULK):
            chunk = data[offset : offset + ONEWIRE_MAX_BULK]
            self._transport.write(bytes([ONEWIRE_BULK_BASE | (len(chunk) - 1)]))
            self._bbio.expect_success()
            for value in chunk:
                self._transport.write(bytes([value]))
                self._bbio.expect_success()

    def search_rom(self) -> tuple[OneWireRom, ...]:
        """Enumerate every device with a ROM search."""
        return self._search(ONEWIRE_ROM_SEARCH)

    def alarm_search(self) -> tuple[OneWireRom, ...]:
        """Enumerate every device in alarm state."""
        return self._search(ONEWIRE_ALARM_SEARCH)

    def read_rom(self) -> OneWireRom:
        """Reset the bus and read the ROM of a single device."""
        self.reset()
        self.write_bytes(bytes([ONEWIRE_READ_ROM]))
        return OneWireRom(self.read_bytes(ONEWIRE_ROM_SIZE))

    def match_rom(self, rom: OneWireRom) -> None:
        """Send a MATCH ROM command without resetting the bus."""
        self.write_bytes(bytes([ONEWIRE_MATCH_ROM]) + rom.data)

    def skip_rom(self) -> None:
        """Send a SKIP ROM command without resetting the bus."""
        self.write_bytes(bytes([ONEWIRE_SKIP_ROM]))

    def read_scratchpad(self, rom: OneWireRom | None, *, length: int = 9) -> bytes:
        """Read a device scratchpad after reset and ROM selection."""
        if length < 1:
            raise ValueError("1-Wire scratchpad read length must be positive")
        self._select(rom)
        self.write_bytes(bytes([ONEWIRE_READ_SCRATCHPAD]))
        return self.read_bytes(length)

    def write_scratchpad(self, rom: OneWireRom | None, data: bytes) -> None:
        """Write a device scratchpad after reset and ROM selection."""
        if not data:
            raise ValueError("1-Wire scratchpad write requires at least one byte")
        self._select(rom)
        self.write_bytes(bytes([ONEWIRE_WRITE_SCRATCHPAD]) + data)

    def copy_scratchpad(self, rom: OneWireRom | None) -> None:
        """Copy scratchpad contents after reset and ROM selection."""
        self._select(rom)
        self.write_bytes(bytes([ONEWIRE_COPY_SCRATCHPAD]))

    def read_memory(self, rom: OneWireRom | None, *, address: int, length: int) -> bytes:
        """Read device memory after reset and ROM selection."""
        _validate_address(address)
        if length < 1:
            raise ValueError("1-Wire memory read length must be positive")
        self._select(rom)
        self.write_bytes(bytes([ONEWIRE_READ_MEMORY, (address >> 8) & 0xFF, address & 0xFF]))
        return self.read_bytes(length)

    def read_status(self, rom: OneWireRom | None, *, address: int) -> int:
        """Read one device status byte after reset and ROM selection."""
        _validate_address(address)
        self._select(rom)
        self.write_bytes(bytes([ONEWIRE_READ_STATUS, (address >> 8) & 0xFF, address & 0xFF]))
        return self.read_byte()

    def _select(self, rom: OneWireRom | None) -> None:
        self.reset()
        if rom is None:
            self.skip_rom()
        else:
            self.match_rom(rom)

    def _search(self, command: int) -> tuple[OneWireRom, ...]:
        self._transport.write(bytes([command]))
        self._bbio.expect_success()
        devices: list[OneWireRom] = []
        for _ in range(ONEWIRE_MAX_SEARCH_DEVICES + 1):
            group = self._transport.read_exact(ONEWIRE_ROM_SIZE)
            if group == ONEWIRE_SEARCH_TERMINATOR:
                return tuple(devices)
            devices.append(OneWireRom(group))
        raise ProtocolError(f"1-Wire search returned more than {ONEWIRE_MAX_SEARCH_DEVICES} devices without a terminator")

    def _send_checked(self, command: bytes) -> None:
        self._transport.write(command)
        self._bbio.expect_success()


def _validate_address(address: int) -> None:
    if not 0 <= address <= 0xFFFF:
        raise ValueError(f"1-Wire memory address must be 0 through 0xFFFF, got 0x{address:04X}")


OneWireSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[OneWireSession]]


@contextmanager
def open_onewire_session(connection: ConnectionSettings) -> Generator[OneWireSession, None, None]:
    """Open a serial transport and an initialized 1-Wire session."""
    with SerialTransport(connection) as transport:
        with OneWireController(transport) as controller:
            yield controller
