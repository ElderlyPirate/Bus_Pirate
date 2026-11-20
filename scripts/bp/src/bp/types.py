"""Shared value types for the Bus Pirate host frontend."""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum

DEFAULT_SERIAL_SPEED = 115_200
DEFAULT_SERIAL_TIMEOUT = 1.0
EEPROM_ONE_BYTE_ADDRESS_SPACE = 0x800
EEPROM_TWO_BYTE_ADDRESS_SPACE = 0x10000


class BpBinaryMode(IntEnum):
    """Binary Bus Pirate protocol modes used by this frontend."""

    SPI = 0x01
    I2C = 0x02
    XSVF = 0x18

    @property
    def banner(self) -> bytes:
        """Return the four-byte mode banner the firmware emits."""
        match self:
            case BpBinaryMode.SPI:
                return b"SPI1"
            case BpBinaryMode.I2C:
                return b"I2C1"
            case BpBinaryMode.XSVF:
                return b"XSV1"


class AddressWidth(IntEnum):
    """Number of memory-address bytes sent to an I2C EEPROM."""

    ONE_BYTE = 1
    TWO_BYTE = 2


@dataclass(frozen=True, slots=True)
class ConnectionSettings:
    """Serial-port parameters shared by every command."""

    port: str
    speed: int = DEFAULT_SERIAL_SPEED
    read_timeout: float = DEFAULT_SERIAL_TIMEOUT
    write_timeout: float = DEFAULT_SERIAL_TIMEOUT

    def __post_init__(self) -> None:
        if not self.port:
            raise ValueError("Serial port must not be empty")
        if self.speed < 1:
            raise ValueError("Serial speed must be positive")
        if self.read_timeout <= 0:
            raise ValueError("Serial read timeout must be positive")
        if self.write_timeout <= 0:
            raise ValueError("Serial write timeout must be positive")


@dataclass(frozen=True, slots=True)
class ChipAddress:
    """The three-bit EEPROM chip-select value (0 through 7)."""

    value: int

    def __post_init__(self) -> None:
        if not 0 <= self.value <= 7:
            raise ValueError(f"EEPROM chip address must be 0 through 7, got {self.value}")

    @property
    def write_address(self) -> int:
        """Return the base I2C write address for this chip-select value."""
        return 0xA0 | (self.value << 1)


@dataclass(frozen=True, slots=True)
class EepromLayout:
    """Addressing contract for an external I2C EEPROM transfer."""

    chip: ChipAddress
    address_width: AddressWidth
    memory_address: int

    def __post_init__(self) -> None:
        if not 0 <= self.memory_address <= 0xFFFF:
            raise ValueError(f"Memory address must be 0 through 0xFFFF, got {self.memory_address}")
        if self.address_width is AddressWidth.ONE_BYTE and self.memory_address >= EEPROM_ONE_BYTE_ADDRESS_SPACE:
            raise ValueError(
                f"One-byte EEPROM addressing supports memory addresses 0 through 0x7FF; got 0x{self.memory_address:04X}"
            )

    @property
    def address_space(self) -> int:
        """Return the highest exclusive memory address for this addressing width."""
        if self.address_width is AddressWidth.ONE_BYTE:
            return EEPROM_ONE_BYTE_ADDRESS_SPACE
        return EEPROM_TWO_BYTE_ADDRESS_SPACE

    def ensure_span(self, length: int) -> None:
        """Reject a transfer that is empty or extends past the addressable space."""
        if length < 1:
            raise ValueError("Transfer length must be at least one byte")
        if self.memory_address + length > self.address_space:
            space = f"0x{self.address_space:04X}"
            raise ValueError(
                f"Transfer of {length} bytes at 0x{self.memory_address:04X} extends past the {space}-byte address space"
            )

    def write_address_for(self, address: int) -> int:
        """Return the I2C write address, including page bits for one-byte devices."""
        if self.address_width is AddressWidth.ONE_BYTE:
            return self.chip.write_address | ((address >> 7) & 0x0E)
        return self.chip.write_address

    def address_bytes_for(self, address: int) -> bytes:
        """Return the EEPROM memory-address bytes in wire order."""
        if self.address_width is AddressWidth.ONE_BYTE:
            return bytes([address & 0xFF])
        return bytes([(address >> 8) & 0xFF, address & 0xFF])


@dataclass(frozen=True, slots=True)
class VersionInfo:
    """Firmware and hardware banner fields reported by the Bus Pirate."""

    hardware_type: str
    firmware_version: str
    hardware_version: str
    update_url: str
