# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Domain exceptions for the Bus Pirate host frontend."""

from __future__ import annotations


class BpError(Exception):
    """Base class for expected Bus Pirate frontend failures."""

    exit_code: int = 1


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


class I2cNackError(ProtocolError):
    """Raised when an I2C device does not acknowledge an address or data byte."""

    def __init__(self, address: int, operation: str) -> None:
        self.address = address
        self.operation = operation
        super().__init__(f"I2C address 0x{address:02X} did not acknowledge {operation}")


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


class XsvfHostError(BpError):
    """Raised when an XSVF host, transport, or setup operation fails."""

    exit_code = 2


class XsvfPlayerError(BpError):
    """Raised when the XSVF player reports a non-success result byte."""

    def __init__(self, result_code: int, result_name: str, detail: str) -> None:
        self.result_code = result_code
        self.result_name = result_name
        self.detail = detail
        super().__init__(f"End of operation reply: {result_name} (0x{result_code:02X}); {detail}")
