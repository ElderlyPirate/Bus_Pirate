"""Tests for binary UART configuration, send, receive, and bridge setup."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError, TransportTimeout
from bp.protocol.uart import UartConfig, UartController, UartDataBits, UartSpeed
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
CLEANUP = [b"\x00", b"\x0f"]


def test_configuration_bytes_cover_frame_format_and_output() -> None:
    assert UartConfig().configuration_byte == 0x10
    assert UartConfig(data_bits=UartDataBits.EIGHT_EVEN).configuration_byte == 0x14
    assert UartConfig(data_bits=UartDataBits.EIGHT_ODD).configuration_byte == 0x18
    assert UartConfig(data_bits=UartDataBits.NINE_NONE).configuration_byte == 0x1C
    assert (
        UartConfig(
            data_bits=UartDataBits.EIGHT_EVEN,
            stop_bits=2,
            inverted=True,
            normal_output=False,
        ).configuration_byte
        == 0x07
    )


def test_configure_speed_and_config_have_exact_commands() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = UartController(transport)
    controller.configure_speed(UartSpeed.BPS_9600)
    controller.configure(UartConfig())
    assert transport.writes == [b"\x64", b"\x90"]


def test_manual_speed_sends_high_then_low_with_status_after_each_byte() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01", b"\x01"])
    UartController(transport).configure_manual_speed(0x01A0)
    assert transport.writes == [b"\x07", b"\x01", b"\xa0"]


def test_write_bytes_sends_command_status_then_each_byte_status() -> None:
    transport = ScriptedTransport([b"\x01"] * 5)
    UartController(transport).write_bytes(b"\xaa\xbb\xcc\xdd")
    assert transport.writes == [b"\x13", b"\xaa", b"\xbb", b"\xcc", b"\xdd"]


def test_write_bytes_splits_at_sixteen_bytes() -> None:
    payload = bytes(range(17))
    transport = ScriptedTransport([b"\x01"] * 19)
    UartController(transport).write_bytes(payload)
    assert transport.writes == [b"\x1f", *[bytes([value]) for value in payload[:16]], b"\x10", bytes([payload[16]])]


def test_begin_and_end_input_use_echo_commands() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = UartController(transport)
    controller.begin_input()
    controller.end_input()
    assert transport.writes == [b"\x02", b"\x03"]


def test_receive_enables_echo_reads_bytes_and_disables_echo() -> None:
    transport = ScriptedTransport()
    transport.queue_read(b"\x01")
    transport.queue_read(b"hello")
    transport.queue_read(b"\x01")
    assert UartController(transport).receive(5) == b"hello"
    assert transport.writes == [b"\x02", b"\x03"]


def test_receive_timeout_does_not_send_end_input() -> None:
    transport = ScriptedTransport()
    transport.queue_read(b"\x01")
    with pytest.raises(TransportTimeout):
        UartController(transport).receive(1)
    assert transport.writes == [b"\x02"]


def test_bridge_enters_mode_and_suppresses_cleanup() -> None:
    transport = ScriptedTransport([b"BBIO1", b"ART1", b"\x01"])
    with UartController(transport) as controller:
        controller.enter_bridge_mode()
    assert transport.writes == [ENTER, b"\x03", b"\x0f"]


def test_context_manager_cleans_up_normal_session() -> None:
    transport = ScriptedTransport([b"BBIO1", b"ART1", b"BBIO1", b"\x01"])
    with UartController(transport):
        pass
    assert transport.writes == [ENTER, b"\x03", *CLEANUP]


def test_configure_peripherals_uses_submode_flags() -> None:
    transport = ScriptedTransport([b"\x01"])
    UartController(transport).configure_peripherals(power=True, pullups=True, aux=True, cs=True)
    assert transport.writes == [b"\x4f"]


def test_write_bytes_rejects_empty_payload() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        UartController(ScriptedTransport()).write_bytes(b"")


def test_receive_rejects_non_positive_count() -> None:
    with pytest.raises(ValueError, match="count must be positive"):
        UartController(ScriptedTransport()).receive(0)


@pytest.mark.parametrize("brg", [-1, 0x10000])
def test_manual_speed_rejects_out_of_range_brg(brg: int) -> None:
    with pytest.raises(ValueError, match="0 through 0xFFFF"):
        UartController(ScriptedTransport()).configure_manual_speed(brg)


@pytest.mark.parametrize("stop_bits", [0, 3])
def test_config_rejects_invalid_stop_bits(stop_bits: int) -> None:
    with pytest.raises(ValueError, match="1 or 2"):
        UartConfig(stop_bits=stop_bits)


def test_peripheral_failure_is_reported() -> None:
    transport = ScriptedTransport([b"\x00"])
    with pytest.raises(ProtocolError, match="failed binary command"):
        UartController(transport).configure_peripherals()
