"""Tests for binary SPI framing and the LCD adapter v2 cycle."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.spi import SpiController
from conftest import ScriptedTransport

ENTER = b"\x00" * 20
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
