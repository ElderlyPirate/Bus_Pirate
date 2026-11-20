"""Tests for binary raw-wire configuration, bits, bytes, and clocking."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.raw_wire import RawWireConfig, RawWireController, RawWireSpeed
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
CLEANUP = [b"\x00", b"\x0f"]


def test_configuration_bytes_select_output_wires_and_order() -> None:
    assert RawWireConfig().configuration_byte == 0x04
    assert RawWireConfig(output_3v3=False, three_wire=False, lsb_first=True).configuration_byte == 0x0A
    assert RawWireConfig(output_3v3=True, three_wire=True, lsb_first=True).configuration_byte == 0x06


def test_configure_and_speed_have_exact_commands() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = RawWireController(transport)
    controller.configure(RawWireConfig(output_3v3=True, three_wire=True, lsb_first=False))
    controller.set_speed(RawWireSpeed.KHZ_400)
    assert transport.writes == [b"\x84", b"\x63"]


def test_cs_start_stop_and_clock_data_commands() -> None:
    transport = ScriptedTransport([b"\x01"] * 10)
    controller = RawWireController(transport)
    controller.set_cs(low=True)
    controller.set_cs(low=False)
    controller.start()
    controller.stop()
    controller.clock_tick()
    controller.clock_high()
    controller.clock_low()
    controller.data_high()
    controller.data_low()
    assert transport.writes == [
        b"\x04",
        b"\x05",
        b"\x02",
        b"\x03",
        b"\x09",
        b"\x0b",
        b"\x0a",
        b"\x0d",
        b"\x0c",
    ]


def test_read_byte_bit_and_peek_return_raw_values() -> None:
    transport = ScriptedTransport([b"\x5a", b"\x01", b"\x00"])
    controller = RawWireController(transport)
    assert controller.read_byte() == 0x5A
    assert controller.read_bit() is True
    assert controller.peek() is False
    assert transport.writes == [b"\x06", b"\x07", b"\x08"]


@pytest.mark.parametrize("command", [0x07, 0x08])
def test_bit_and_peek_reject_non_boolean_replies(command: int) -> None:
    transport = ScriptedTransport([b"\x02"])
    controller = RawWireController(transport)
    with pytest.raises(ProtocolError, match="expected 0x00 or 0x01"):
        if command == 0x07:
            controller.read_bit()
        else:
            controller.peek()


def test_bulk_clock_uses_four_bit_count_minus_one() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = RawWireController(transport)
    controller.bulk_clock(1)
    controller.bulk_clock(16)
    assert transport.writes == [b"\x20", b"\x2f"]


def test_write_bit_writes_data_then_clock_tick() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01", b"\x01", b"\x01"])
    controller = RawWireController(transport)
    controller.write_bit(True)
    controller.write_bit(False)
    assert transport.writes == [b"\x0d", b"\x09", b"\x0c", b"\x09"]


def test_two_wire_write_uses_bulk_and_per_byte_acknowledgement() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01", b"\x01"])
    controller = RawWireController(transport)
    controller.write_bytes(b"\xaa\xbb")
    assert transport.writes == [b"\x11", b"\xaa", b"\xbb"]


def test_three_wire_write_clocks_each_bit_instead_of_bulk() -> None:
    transport = ScriptedTransport([b"\x01"] * 18)
    controller = RawWireController(transport)
    controller.configure(RawWireConfig(output_3v3=True, three_wire=True, lsb_first=False))
    controller.write_byte(0x01)
    bits = transport.writes[1:]
    assert bits[0] == b"\x0a"
    for index in range(7):
        assert bits[1 + index * 2] == b"\x0c"
        assert bits[2 + index * 2] == b"\x09"
    assert bits[15] == b"\x0d"
    assert bits[16] == b"\x09"
    assert b"\x10" not in transport.writes


def test_three_wire_lsb_first_starts_with_the_low_bit() -> None:
    transport = ScriptedTransport([b"\x01"] * 18)
    controller = RawWireController(transport)
    controller.configure(RawWireConfig(output_3v3=True, three_wire=True, lsb_first=True))
    controller.write_byte(0x01)
    assert transport.writes[1] == b"\x0a"
    assert transport.writes[2] == b"\x0d"
    assert transport.writes[4] == b"\x0c"


def test_read_bytes_reads_one_command_per_byte() -> None:
    transport = ScriptedTransport([b"\x11", b"\x22"])
    controller = RawWireController(transport)
    assert controller.read_bytes(2) == b"\x11\x22"
    assert transport.writes == [b"\x06", b"\x06"]


def test_peripheral_configuration_uses_submode_flags() -> None:
    transport = ScriptedTransport([b"\x01"])
    RawWireController(transport).configure_peripherals(power=True, pullups=True)
    assert transport.writes == [b"\x4c"]


def test_context_manager_enters_and_cleans_up() -> None:
    transport = ScriptedTransport([b"BBIO1", b"RAW1", b"BBIO1", b"\x01"])
    with RawWireController(transport):
        pass
    assert transport.writes == [ENTER, b"\x05", *CLEANUP]


@pytest.mark.parametrize("ticks", [0, 17])
def test_bulk_clock_is_bounded(ticks: int) -> None:
    with pytest.raises(ValueError, match="1 through 16"):
        RawWireController(ScriptedTransport()).bulk_clock(ticks)


@pytest.mark.parametrize("value", [-1, 0x100])
def test_write_byte_rejects_out_of_range(value: int) -> None:
    with pytest.raises(ValueError, match="0 through 0xFF"):
        RawWireController(ScriptedTransport()).write_byte(value)


def test_write_bytes_rejects_empty_payload() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        RawWireController(ScriptedTransport()).write_bytes(b"")


def test_read_bytes_rejects_non_positive_count() -> None:
    with pytest.raises(ValueError, match="count must be positive"):
        RawWireController(ScriptedTransport()).read_bytes(0)


def test_bit_write_rejects_value_that_does_not_fit() -> None:
    with pytest.raises(ValueError, match="does not fit in 2 bits"):
        RawWireController(ScriptedTransport()).write_bits(0b100, 2)
