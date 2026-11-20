# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the pyserial transport boundary."""

from __future__ import annotations

import pytest

from bp.errors import TransportError, TransportTimeout
from bp.transport import SerialTransport
from bp.types import ConnectionSettings
from conftest import ScriptedSerial


def _transport(serial: ScriptedSerial, connection: ConnectionSettings) -> SerialTransport:
    return SerialTransport(connection, factory=lambda settings: serial)


def test_connection_settings_reject_invalid_values() -> None:
    with pytest.raises(ValueError, match="must not be empty"):
        ConnectionSettings(port="")
    with pytest.raises(ValueError, match="speed must be positive"):
        ConnectionSettings(port="COM9", speed=0)
    with pytest.raises(ValueError, match="read timeout must be positive"):
        ConnectionSettings(port="COM9", read_timeout=0)
    with pytest.raises(ValueError, match="write timeout must be positive"):
        ConnectionSettings(port="COM9", write_timeout=-1)


def test_open_and_close_context_manager(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    transport = _transport(serial, connection)
    with transport as opened:
        assert opened is transport
    assert serial.closed


def test_open_wraps_factory_failure(connection: ConnectionSettings) -> None:
    def failing_factory(settings: ConnectionSettings) -> ScriptedSerial:
        raise OSError(f"cannot open {settings.port}")

    transport = SerialTransport(connection, factory=failing_factory)
    with pytest.raises(TransportError, match="Could not open serial port COM9"):
        transport.open()


def test_open_wraps_value_error(connection: ConnectionSettings) -> None:
    def failing_factory(settings: ConnectionSettings) -> ScriptedSerial:
        raise ValueError("bad settings")

    transport = SerialTransport(connection, factory=failing_factory)
    with pytest.raises(TransportError, match="bad settings"):
        transport.open()


def test_double_open_is_rejected(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="already open"):
        transport.open()
    transport.close()


def test_operations_before_open_are_rejected(connection: ConnectionSettings) -> None:
    transport = SerialTransport(connection, factory=lambda settings: ScriptedSerial())
    with pytest.raises(TransportError, match="not open"):
        transport.write(b"x")
    with pytest.raises(TransportError, match="not open"):
        transport.read_exact(1)
    with pytest.raises(TransportError, match="not open"):
        transport.read_until_idle(max_bytes=1)
    with pytest.raises(TransportError, match="not open"):
        transport.reset_input()


def test_write_sends_complete_payload(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    transport = _transport(serial, connection)
    transport.open()
    transport.write(b"\x10\x20")
    assert serial.writes == [b"\x10\x20"]
    transport.close()


def test_short_write_is_rejected(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.write_shortfall = 1
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="accepted 2 of 3 bytes"):
        transport.write(b"abc")
    transport.close()


def test_missing_write_count_is_rejected(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.report_no_count = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="did not report a byte count"):
        transport.write(b"abc")
    transport.close()


def test_write_failure_is_wrapped(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.write_raises = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="Serial write to COM9 failed"):
        transport.write(b"abc")
    transport.close()


def test_read_exact_returns_payload(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.feed(b"BBIO1")
    transport = _transport(serial, connection)
    transport.open()
    assert transport.read_exact(5) == b"BBIO1"
    transport.close()


def test_read_exact_times_out_without_data(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportTimeout, match="received 0"):
        transport.read_exact(5)
    transport.close()


def test_read_exact_times_out_after_partial_data(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.feed(b"BB")
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportTimeout, match="received 2"):
        transport.read_exact(5)
    transport.close()


def test_read_exact_rejects_non_positive_size(connection: ConnectionSettings) -> None:
    transport = _transport(ScriptedSerial(), connection)
    transport.open()
    with pytest.raises(ValueError, match="must be positive"):
        transport.read_exact(0)
    transport.close()


def test_read_failure_is_wrapped(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.read_raises = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="Serial read from COM9 failed"):
        transport.read_exact(1)
    transport.close()


def test_read_until_idle_collects_and_caps(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.feed(b"abcdef")
    transport = _transport(serial, connection)
    transport.open()
    assert transport.read_until_idle(max_bytes=3) == b"abc"
    assert transport.read_until_idle(max_bytes=10) == b"def"
    transport.close()


def test_read_until_idle_rejects_non_positive_limit(connection: ConnectionSettings) -> None:
    transport = _transport(ScriptedSerial(), connection)
    transport.open()
    with pytest.raises(ValueError, match="must be positive"):
        transport.read_until_idle(max_bytes=0)
    transport.close()


def test_read_until_idle_wraps_read_failure(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.read_raises = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="Serial read from COM9 failed"):
        transport.read_until_idle(max_bytes=10)
    transport.close()


def test_reset_input_wraps_failure(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.reset_raises = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="Could not reset input buffer"):
        transport.reset_input()
    transport.close()


def test_reset_input_discards_buffered_data(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.feed(b"stale")
    transport = _transport(serial, connection)
    transport.open()
    transport.reset_input()
    assert serial.reset_count == 1
    assert not serial.received
    transport.close()


def test_close_is_idempotent(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    transport = _transport(serial, connection)
    transport.open()
    transport.close()
    transport.close()
    assert serial.closed


def test_close_failure_is_wrapped(connection: ConnectionSettings) -> None:
    serial = ScriptedSerial()
    serial.close_raises = True
    transport = _transport(serial, connection)
    transport.open()
    with pytest.raises(TransportError, match="Could not close serial port COM9"):
        transport.close()
