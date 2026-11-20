"""Tests for binary SPI framing, transfers, and the LCD adapter v2 cycle."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.spi import SpiConfig, SpiController, SpiSpeed
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
BASE = [ENTER, b"\x01"]
CYCLE = [
    b"\x48",
    b"\x8a",
    b"\x10\x00",
    b"\x03",
    b"\x02",
    b"\x10\xff",
    b"\x03",
    b"\x02",
    b"\x40",
    b"\x82",
]
CLEANUP = [b"\x00", b"\x0f"]
CYCLE_RESPONSES = [
    b"\x01",
    b"\x01",
    b"\x01\x00",
    b"\x01",
    b"\x01",
    b"\x01\x00",
    b"\x01",
    b"\x01",
    b"\x01",
    b"\x01",
]


def test_one_lcd_cycle_has_exact_wire_sequence() -> None:
    transport = ScriptedTransport([b"BBIO1", b"SPI1", *CYCLE_RESPONSES, b"BBIO1", b"\x01"])
    with SpiController(transport, blink_delay=0) as controller:
        controller.run_cycle()

    assert transport.writes == [*BASE, *CYCLE, *CLEANUP]
    transport.assert_consumed()


def test_repeated_lcd_cycles_repeat_only_the_test_body() -> None:
    transport = ScriptedTransport([b"BBIO1", b"SPI1", *CYCLE_RESPONSES, *CYCLE_RESPONSES, b"BBIO1", b"\x01"])
    with SpiController(transport, blink_delay=0) as controller:
        controller.run_cycle()
        controller.run_cycle()

    assert transport.writes == [*BASE, *CYCLE, *CYCLE, *CLEANUP]


def test_lcd_cycle_rejects_transfer_failure_status() -> None:
    transport = ScriptedTransport([b"BBIO1", b"SPI1", b"\x01", b"\x01", b"\x00"])
    with pytest.raises(ProtocolError, match="failed binary command"):
        with SpiController(transport, blink_delay=0) as controller:
            controller.run_cycle()
    assert transport.writes[-1] == b"\x10\x00"


def test_transfer_rejects_out_of_range_byte() -> None:
    controller = SpiController(ScriptedTransport())
    with pytest.raises(ValueError, match="0 through 0xFF"):
        controller.transfer(0x100)


def test_constructor_rejects_negative_blink_delay() -> None:
    with pytest.raises(ValueError, match="must not be negative"):
        SpiController(ScriptedTransport(), blink_delay=-1)


def test_set_cs_and_configure_are_checked() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = SpiController(transport)
    controller.set_cs(low=True)
    controller.configure_spi(
        output_3v3=True,
        clock_idle_high=False,
        active_to_idle=True,
        sample_end=False,
    )
    assert transport.writes == [b"\x02", b"\x8a"]


def test_configuration_byte_covers_output_polarity_edge_and_sample() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = SpiController(transport)
    controller.configure_spi(
        output_3v3=False,
        clock_idle_high=True,
        active_to_idle=False,
        sample_end=True,
    )
    controller.configure_spi(
        output_3v3=True,
        clock_idle_high=False,
        active_to_idle=True,
        sample_end=False,
    )
    assert transport.writes == [b"\x85", b"\x8a"]


def test_configure_speed_uses_firmware_index() -> None:
    transport = ScriptedTransport([b"\x01"])
    SpiController(transport).configure_speed(SpiSpeed.MHZ_8)
    assert transport.writes == [b"\x67"]


def test_configuration_defaults_are_normal_output_msb_mode() -> None:
    config = SpiConfig()
    assert config.output_3v3 is True
    assert config.clock_idle_high is False
    assert config.active_to_idle is True
    assert config.sample_end is False


def test_bulk_transfer_writes_and_reads_identical_length() -> None:
    transport = ScriptedTransport([b"\x01\xaa\xbb"])
    response = SpiController(transport).bulk_transfer(b"\x9f\x00")
    assert response == b"\xaa\xbb"
    assert transport.writes == [b"\x11\x9f\x00"]


def test_bulk_transfer_rejects_unsupported_lengths() -> None:
    controller = SpiController(ScriptedTransport())
    with pytest.raises(ValueError, match="1 through 16"):
        controller.bulk_transfer(b"")
    with pytest.raises(ValueError, match="1 through 16"):
        controller.bulk_transfer(b"\x00" * 17)


def test_write_bytes_splits_and_concatenates_received_bytes() -> None:
    payload = bytes(range(17))
    transport = ScriptedTransport([b"\x01" + bytes(range(16)), b"\x01\x10"])
    response = SpiController(transport).write_bytes(payload)
    assert response == bytes(range(17))
    assert transport.writes == [b"\x1f" + payload[:16], b"\x10\x10"]


def test_read_bytes_uses_ff_dummy_bytes_in_chunks() -> None:
    transport = ScriptedTransport([b"\x01" + bytes(range(16)), b"\x01\x10"])
    response = SpiController(transport).read_bytes(17)
    assert response == bytes(range(17))
    assert transport.writes == [b"\x1f" + b"\xff" * 16, b"\x10\xff"]


def test_read_bytes_rejects_non_positive_count() -> None:
    with pytest.raises(ValueError, match="count must be positive"):
        SpiController(ScriptedTransport()).read_bytes(0)


def test_write_bytes_rejects_empty_payload() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        SpiController(ScriptedTransport()).write_bytes(b"")
