# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Bus Pirate XSVF player framing, chain probes, and stream chunking."""

from __future__ import annotations

import logging
from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from enum import IntEnum
from types import TracebackType
from typing import Protocol

from bp.errors import ProtocolError, TransportError, TransportTimeout, XsvfHostError, XsvfPlayerError
from bp.protocol.bbio import BbioSession
from bp.transport import SerialTransport, Transport
from bp.types import BpBinaryMode, ConnectionSettings

logger = logging.getLogger(__name__)

XSVF_RESET_COMMAND = 0x01
XSVF_SCAN_COMMAND = 0x02
XSVF_PLAY_COMMAND = 0x03
XSVF_READY_FOR_DATA = 0xFF
XSVF_CHUNK_SIZE = 4096
XSVF_MAX_CHAIN_BYTES = 252
XSVF_REPLY_ATTEMPTS = 5


class XsvfResult(IntEnum):
    """Result byte returned by the firmware XSVF player after a stream run."""

    NONE = 0x00
    UNKNOWN = 0x01
    TDOMISMATCH = 0x02
    MAXRETRIES = 0x03
    ILLEGALCMD = 0x04
    ILLEGALSTATE = 0x05
    DATAOVERFLOW = 0x06
    LAST = 0x07


_RESULT_NAMES = {
    XsvfResult.NONE: "XSVF_ERROR_NONE",
    XsvfResult.UNKNOWN: "XSVF_ERROR_UNKNOWN",
    XsvfResult.TDOMISMATCH: "XSVF_ERROR_TDOMISMATCH",
    XsvfResult.MAXRETRIES: "XSVF_ERROR_MAXRETRIES",
    XsvfResult.ILLEGALCMD: "XSVF_ERROR_ILLEGALCMD",
    XsvfResult.ILLEGALSTATE: "XSVF_ERROR_ILLEGALSTATE",
    XsvfResult.DATAOVERFLOW: "XSVF_ERROR_DATAOVERFLOW",
    XsvfResult.LAST: "XSVF_ERROR_LAST",
}

_RESULT_DETAILS = {
    XsvfResult.NONE: "the stream completed successfully",
    XsvfResult.UNKNOWN: "host stopped feeding, sent data during a wait, or the player hit an unknown condition",
    XsvfResult.TDOMISMATCH: "the captured TDO did not match the expected TDO",
    XsvfResult.MAXRETRIES: "TDO mismatched after the maximum number of retries",
    XsvfResult.ILLEGALCMD: "the stream contains an illegal or unsupported XSVF command",
    XsvfResult.ILLEGALSTATE: "the stream selected an illegal TAP state",
    XsvfResult.DATAOVERFLOW: "a shift exceeds the player's 400-bit buffer",
    XsvfResult.LAST: "the player reported its internal last-error marker",
}


@dataclass(frozen=True, slots=True)
class ChainScan:
    """Chain probe reply: the byte count followed by four IDCODE bytes per device."""

    count: int
    payload: bytes

    def __post_init__(self) -> None:
        _validate_scan_count(self.count)
        if len(self.payload) != self.count:
            raise ValueError(f"Chain scan payload has {len(self.payload)} bytes but the count byte is {self.count}")

    @property
    def raw(self) -> bytes:
        """Return the count byte followed by the payload, as printed by the host tools."""
        return bytes([self.count]) + self.payload

    @property
    def idcodes(self) -> tuple[int, ...]:
        """Return one IDCODE per device, assembled least-significant byte first."""
        return tuple(int.from_bytes(self.payload[index : index + 4], "little") for index in range(0, len(self.payload), 4))


@dataclass(frozen=True, slots=True)
class XsvfPlayReport:
    """Outcome of one successful XSVF stream run."""

    bytes_sent: int
    chunks: int
    result: XsvfResult


class XsvfSession(Protocol):
    """XSVF operations consumed by the command layer."""

    def reset_chain(self) -> None:
        """Send the chain-reset command. The player sends no reply."""
        ...

    def scan_chain(self) -> ChainScan:
        """Probe the JTAG chain and return its IDCODE bytes."""
        ...

    def play(self, data: bytes) -> XsvfPlayReport:
        """Stream an XSVF image until the player returns a result byte."""
        ...


class XsvfController:
    """Own a binary XSVF player session and its chunk handshake."""

    def __init__(self, transport: Transport, *, reply_attempts: int = XSVF_REPLY_ATTEMPTS) -> None:
        if reply_attempts < 1:
            raise ValueError("XSVF reply attempts must be positive")
        self._transport = transport
        self._bbio = BbioSession(transport)
        self._reply_attempts = reply_attempts

    def __enter__(self) -> XsvfController:
        """Enter binary bitbang mode, then the v4 XSVF player sub-mode."""
        logger.debug("Entering XSVF player mode")
        try:
            self._bbio.enter_binary()
            self._bbio.enter_mode(BpBinaryMode.XSVF)
        except TransportTimeout as exc:
            raise XsvfHostError("Bus Pirate did not enter XSVF mode; XSVF requires a Bus Pirate v4 with player firmware") from exc
        except ProtocolError as exc:
            raise XsvfHostError(str(exc)) from exc
        except TransportError as exc:
            raise XsvfHostError(f"Could not enter XSVF mode: {exc}") from exc
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        """Leave the firmware player in its command loop.

        The native host players intentionally stop there: leaving requires
        ``0x0F`` twice, and sending it after a host timeout can be consumed as
        stream data while the player is still busy. Recovery is documented in
        the package README.
        """
        del exc_type, exc, traceback

    def reset_chain(self) -> None:
        """Send ``0x01`` to reset the JTAG chain; the player does not reply."""
        logger.debug("Resetting JTAG chain")
        self._write(bytes([XSVF_RESET_COMMAND]), action="Failed to send the JTAG reset command")

    def scan_chain(self) -> ChainScan:
        """Send ``0x02`` and read the count byte plus that many IDCODE bytes."""
        logger.debug("Scanning JTAG chain")
        self._write(bytes([XSVF_SCAN_COMMAND]), action="Chain scan: failed to send the command")
        count = self._read_byte("Chain scan: incomplete reply (bounded timeout)")
        try:
            _validate_scan_count(count)
        except ValueError as exc:
            raise XsvfHostError(f"Chain scan: {exc}") from exc
        payload = self._read_bytes(count, "Chain scan: incomplete reply (bounded timeout)")
        scan = ChainScan(count=count, payload=payload)
        logger.debug("Chain scan returned %d byte(s): %s", count, _describe(payload))
        return scan

    def play(self, data: bytes) -> XsvfPlayReport:
        """Stream chunks on each ``0xFF`` request and validate the final result byte."""
        if not data:
            raise XsvfHostError("XSVF stream is empty")

        logger.debug("Starting XSVF stream of %d bytes", len(data))
        self._write(bytes([XSVF_PLAY_COMMAND]), action="Failed to start the XSVF player")
        offset = 0
        chunks = 0

        while True:
            waiting_for_data = offset < len(data)
            timeout_message = (
                "Timed out waiting for the XSVF player data request"
                if waiting_for_data
                else "Timed out waiting for the XSVF player result"
            )
            reply = self._read_byte(timeout_message)
            if reply == XSVF_READY_FOR_DATA:
                if offset == len(data):
                    raise XsvfHostError("End of file reached without a completion result")
                chunk = data[offset : offset + XSVF_CHUNK_SIZE]
                header = bytes([(len(chunk) >> 8) & 0xFF, len(chunk) & 0xFF])
                chunk_number = chunks + 1
                self._write(header, action=f"Failed to send XSVF chunk {chunk_number} length")
                self._write(chunk, action=f"Failed to send XSVF chunk {chunk_number} data")
                offset += len(chunk)
                chunks = chunk_number
                logger.debug("Sent XSVF chunk %d: %d bytes (%d/%d)", chunks, len(chunk), offset, len(data))
                continue

            result = _classify_result(reply)
            name = _RESULT_NAMES[result]
            logger.debug("XSVF player result %s (0x%02X)", name, reply)
            if result is XsvfResult.NONE:
                return XsvfPlayReport(bytes_sent=offset, chunks=chunks, result=result)
            raise XsvfPlayerError(reply, name, _RESULT_DETAILS[result])

    def _write(self, data: bytes, *, action: str) -> None:
        try:
            self._transport.write(data)
        except TransportError as exc:
            raise XsvfHostError(f"{action}: {exc}") from exc

    def _read_byte(self, timeout_message: str) -> int:
        last_timeout: TransportTimeout | None = None
        for _ in range(self._reply_attempts):
            try:
                return self._transport.read_exact(1)[0]
            except TransportTimeout as exc:
                last_timeout = exc
            except TransportError as exc:
                raise XsvfHostError(f"{timeout_message}: {exc}") from exc
        attempts = self._reply_attempts
        raise XsvfHostError(f"{timeout_message}; no reply after {attempts} attempts") from last_timeout

    def _read_bytes(self, size: int, timeout_message: str) -> bytes:
        return bytes(self._read_byte(timeout_message) for _ in range(size))


def result_name(result: XsvfResult) -> str:
    """Return the firmware's symbolic name for a player result."""
    return _RESULT_NAMES[result]


def _validate_scan_count(count: int) -> None:
    if count > XSVF_MAX_CHAIN_BYTES:
        raise ValueError(f"device count 0x{count:02X} exceeds the {XSVF_MAX_CHAIN_BYTES}-byte reply limit")
    if count % 4:
        raise ValueError(f"device count 0x{count:02X} is not a multiple of four bytes")


def _classify_result(value: int) -> XsvfResult:
    try:
        return XsvfResult(value)
    except ValueError:
        return XsvfResult.UNKNOWN


def _describe(payload: bytes) -> str:
    return " ".join(f"0x{value:02X}" for value in payload) if payload else "no data"


XsvfSessionFactory = Callable[[ConnectionSettings], AbstractContextManager[XsvfSession]]


@contextmanager
def open_xsvf_session(connection: ConnectionSettings) -> Generator[XsvfSession, None, None]:
    """Open a serial transport and an initialized binary XSVF player session."""
    with SerialTransport(connection) as transport:
        with XsvfController(transport) as controller:
            yield controller
