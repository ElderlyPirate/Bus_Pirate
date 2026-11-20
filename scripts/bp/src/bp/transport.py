"""Serial transport boundary for the Bus Pirate host frontend."""

from __future__ import annotations

import logging
from collections.abc import Callable
from contextlib import AbstractContextManager
from types import TracebackType
from typing import Protocol

import serial

from bp.errors import TransportError, TransportTimeout
from bp.types import ConnectionSettings

logger = logging.getLogger(__name__)


class SerialPort(Protocol):
    """Minimal pyserial surface used by :class:`SerialTransport`."""

    def write(self, data: bytes, /) -> int | None:
        """Write bytes and return the number accepted."""
        ...

    def read(self, size: int, /) -> bytes:
        """Read up to ``size`` bytes, honoring the configured timeout."""
        ...

    def reset_input_buffer(self) -> None:
        """Discard buffered input from the device."""
        ...

    def close(self) -> None:
        """Close the port."""
        ...


class Transport(Protocol):
    """Byte transport contract consumed by protocol modules."""

    def write(self, data: bytes) -> None:
        """Write all bytes or raise."""
        ...

    def read_exact(self, size: int) -> bytes:
        """Read exactly ``size`` bytes or raise :class:`TransportTimeout`."""
        ...

    def read_until_idle(self, *, max_bytes: int) -> bytes:
        """Read until the device stops sending or ``max_bytes`` is reached."""
        ...

    def reset_input(self) -> None:
        """Discard stale input."""
        ...


SerialFactory = Callable[[ConnectionSettings], SerialPort]
TransportFactory = Callable[[ConnectionSettings], AbstractContextManager[Transport]]


def _open_serial(settings: ConnectionSettings) -> SerialPort:
    """Open a pyserial port with the Bus Pirate's 8N1 framing."""
    return serial.Serial(
        port=settings.port,
        baudrate=settings.speed,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=settings.read_timeout,
        write_timeout=settings.write_timeout,
    )


class SerialTransport:
    """Open, read, and write a serial port without protocol knowledge."""

    def __init__(self, settings: ConnectionSettings, *, factory: SerialFactory = _open_serial) -> None:
        self._settings = settings
        self._factory = factory
        self._port: SerialPort | None = None

    def open(self) -> None:
        """Open the serial port, translating pyserial failures."""
        if self._port is not None:
            raise TransportError("Serial port is already open")
        logger.debug("Opening serial port %s at %d baud", self._settings.port, self._settings.speed)
        try:
            self._port = self._factory(self._settings)
        except (OSError, ValueError) as exc:
            raise TransportError(f"Could not open serial port {self._settings.port}: {exc}") from exc

    def close(self) -> None:
        """Close the serial port once."""
        port = self._port
        if port is None:
            return
        self._port = None
        logger.debug("Closing serial port %s", self._settings.port)
        try:
            port.close()
        except OSError as exc:
            raise TransportError(f"Could not close serial port {self._settings.port}: {exc}") from exc

    def __enter__(self) -> SerialTransport:
        """Open the port and return this transport."""
        self.open()
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        """Close the port without suppressing an active exception."""
        self.close()

    def write(self, data: bytes) -> None:
        """Write every byte in ``data``."""
        port = self._require_port()
        try:
            written = port.write(data)
        except OSError as exc:
            raise TransportError(f"Serial write to {self._settings.port} failed: {exc}") from exc
        if written is None:
            raise TransportError(f"Serial write to {self._settings.port} did not report a byte count")
        if written != len(data):
            raise TransportError(f"Serial write to {self._settings.port} accepted {written} of {len(data)} bytes")

    def read_exact(self, size: int) -> bytes:
        """Read exactly ``size`` bytes, raising on a timeout."""
        if size < 1:
            raise ValueError("Read size must be positive")
        port = self._require_port()
        payload = bytearray()
        while len(payload) < size:
            try:
                chunk = port.read(size - len(payload))
            except OSError as exc:
                raise TransportError(f"Serial read from {self._settings.port} failed: {exc}") from exc
            if not chunk:
                raise TransportTimeout(f"Timed out reading {size} bytes from {self._settings.port}; received {len(payload)}")
            payload.extend(chunk)
        return bytes(payload)

    def read_until_idle(self, *, max_bytes: int) -> bytes:
        """Read one byte at a time until the port's timeout elapses between bytes."""
        if max_bytes < 1:
            raise ValueError("Maximum read size must be positive")
        port = self._require_port()
        payload = bytearray()
        while len(payload) < max_bytes:
            try:
                chunk = port.read(1)
            except OSError as exc:
                raise TransportError(f"Serial read from {self._settings.port} failed: {exc}") from exc
            if not chunk:
                break
            payload.extend(chunk)
        return bytes(payload)

    def reset_input(self) -> None:
        """Discard bytes the device sent before the current command."""
        port = self._require_port()
        try:
            port.reset_input_buffer()
        except OSError as exc:
            raise TransportError(f"Could not reset input buffer for {self._settings.port}: {exc}") from exc

    def _require_port(self) -> SerialPort:
        if self._port is None:
            raise TransportError("Serial port is not open")
        return self._port


def open_transport(settings: ConnectionSettings) -> AbstractContextManager[Transport]:
    """Create a lazily opened serial transport for command orchestration."""
    return SerialTransport(settings)
