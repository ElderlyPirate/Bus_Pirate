"""Domain exceptions for the Bus Pirate host frontend."""

from __future__ import annotations


class BpError(Exception):
    """Base class for expected Bus Pirate frontend failures."""


class ConfigError(BpError):
    """Raised when an optional YAML configuration cannot be loaded or validated."""


class CommandError(BpError):
    """Raised when a command input or command operation violates its contract."""


class TransportError(BpError):
    """Raised when the serial transport cannot be opened or used."""


class TransportTimeout(TransportError):
    """Raised when the serial transport does not deliver the requested bytes."""


class ProtocolError(BpError):
    """Raised when the Bus Pirate rejects or misanswers a protocol command."""


class EepromWriteError(ProtocolError):
    """Raised when an EEPROM byte cannot be acknowledged within the retry limit."""

    def __init__(self, address: int, value: int, attempts: int) -> None:
        self.address = address
        self.value = value
        self.attempts = attempts
        super().__init__(
            f"EEPROM write at address 0x{address:04X} (value 0x{value:02X}) was not acknowledged after {attempts} attempts"
        )


class EepromVerifyError(BpError):
    """Raised when EEPROM contents differ from the expected byte sequence."""

    def __init__(self, offset: int, expected: int, actual: int) -> None:
        self.offset = offset
        self.expected = expected
        self.actual = actual
        super().__init__(f"EEPROM verification failed at offset 0x{offset:04X}: expected 0x{expected:02X}, read 0x{actual:02X}")
