# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Serial-port enumeration and likely Bus Pirate classification."""

from __future__ import annotations

import logging
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from enum import Enum

from serial.tools import list_ports

from bp.errors import CommandError

logger = logging.getLogger(__name__)


class BusPiratePortKind(Enum):
    """Likely Bus Pirate model implied by a known USB vendor/product ID."""

    V4_APPLICATION = "Bus Pirate v4 application"
    V4_BOOTLOADER = "Bus Pirate v4 bootloader"
    V3_CANDIDATE = "FTDI (Bus Pirate v3 possible)"


_BUS_PIRATE_KINDS = {
    (0x04D8, 0xFB00): BusPiratePortKind.V4_APPLICATION,
    (0x04D8, 0xFAFF): BusPiratePortKind.V4_BOOTLOADER,
    (0x0403, 0x6001): BusPiratePortKind.V3_CANDIDATE,
}


@dataclass(frozen=True, slots=True)
class SerialPortInfo:
    """Validated description of one serial port returned by pyserial."""

    device: str
    vid: int | None
    pid: int | None
    description: str

    def __post_init__(self) -> None:
        if not self.device:
            raise ValueError("Serial port device must not be empty")
        if (self.vid is None) != (self.pid is None):
            raise ValueError("Serial port VID and PID must be provided together")
        if self.vid is not None and not 0 <= self.vid <= 0xFFFF:
            raise ValueError(f"Serial port VID must be 0 through 0xFFFF, got {self.vid}")
        if self.pid is not None and not 0 <= self.pid <= 0xFFFF:
            raise ValueError(f"Serial port PID must be 0 through 0xFFFF, got {self.pid}")

    @property
    def vid_pid(self) -> str | None:
        """Return the USB VID:PID as uppercase hex, or ``None`` when unavailable."""
        if self.vid is None or self.pid is None:
            return None
        return f"{self.vid:04X}:{self.pid:04X}"

    @property
    def bus_pirate_kind(self) -> BusPiratePortKind | None:
        """Return the likely Bus Pirate model for this port, if any."""
        if self.vid is None or self.pid is None:
            return None
        return _BUS_PIRATE_KINDS.get((self.vid, self.pid))


PortLister = Callable[[], Iterable[object]]


def list_serial_ports(*, lister: PortLister = list_ports.comports) -> tuple[SerialPortInfo, ...]:
    """Enumerate serial ports and narrow pyserial's port objects.

    The external port objects are validated once at this boundary. Entries
    without usable device, description, or USB ID values are rejected rather
    than passed through as weakly typed data.

    Raises:
        CommandError: If enumeration fails or an entry violates the port-info
            contract.
    """
    logger.debug("Enumerating serial ports")
    try:
        entries = tuple(lister())
    except OSError as exc:
        raise CommandError(f"Could not enumerate serial ports: {exc}") from exc
    ports = tuple(_validate_port_entry(entry) for entry in entries)
    logger.debug("Serial port enumeration returned %d port(s)", len(ports))
    return ports


def _validate_port_entry(entry: object) -> SerialPortInfo:
    device = _string_field(entry, "device")
    description = _string_field(entry, "description")
    vid = _optional_int_field(entry, "vid")
    pid = _optional_int_field(entry, "pid")
    try:
        return SerialPortInfo(device=device, vid=vid, pid=pid, description=description)
    except ValueError as exc:
        raise CommandError(f"Serial port entry is invalid: {exc}") from exc


def _string_field(entry: object, name: str) -> str:
    value = _field(entry, name)
    if not isinstance(value, str):
        raise CommandError(f"Serial port entry has a non-string '{name}' value")
    return value


def _optional_int_field(entry: object, name: str) -> int | None:
    value = _field(entry, name)
    if value is None:
        return None
    if not isinstance(value, int) or isinstance(value, bool):
        raise CommandError(f"Serial port entry has an invalid '{name}' value: {value!r}")
    return value


def _field(entry: object, name: str) -> object:
    try:
        return getattr(entry, name)
    except AttributeError as exc:
        raise CommandError(f"Serial port entry is missing the required '{name}' attribute") from exc
