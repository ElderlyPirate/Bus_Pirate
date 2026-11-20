"""Tests for the version command service."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager

import pytest

from bp.commands.version import read_version
from bp.errors import ProtocolError
from bp.transport import Transport
from bp.types import ConnectionSettings
from conftest import ScriptedTransport

BANNER = (
    b"\r\nRESET\r\nBus Pirate v4\r\nElderlyPirate Firmware v9.0\r\n"
    b"[HiZ 1WIRE UART I2C SPI 2WIRE 3WIRE KEYB PIC DIO]\r\n"
    b"DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)\r\n"
    b"http://github.com/ElderlyPirate\r\nHiZ>"
)


def _factory(transport: ScriptedTransport) -> Callable[[ConnectionSettings], AbstractContextManager[Transport]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[Transport, None, None]:
        del connection
        yield transport

    return factory


def test_read_version_returns_parsed_banner(connection: ConnectionSettings) -> None:
    transport = ScriptedTransport([BANNER])
    info = read_version(connection, transport_factory=_factory(transport))
    assert info.hardware_type == "Bus Pirate v4"
    assert info.firmware_version == "ElderlyPirate Firmware v9.0"
    assert info.hardware_version == "DEVID:0x1019 REVID:0x0004 (24FJ256GB106 A6)"
    assert info.update_url == "http://github.com/ElderlyPirate"
    transport.assert_consumed()


def test_read_version_propagates_protocol_failure(connection: ConnectionSettings) -> None:
    transport = ScriptedTransport()
    with pytest.raises(ProtocolError, match="no version banner"):
        read_version(connection, transport_factory=_factory(transport))
