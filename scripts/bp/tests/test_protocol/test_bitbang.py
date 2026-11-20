"""Tests for binary bitbang pin, bit, ADC, PWM, and self-test operations."""

from __future__ import annotations

import pytest

from bp.errors import ProtocolError
from bp.protocol.bbio import BBIO_ENTER_SEQUENCE, BitbangController, SelfTestReport
from conftest import ScriptedTransport


def test_configure_directions_sets_five_bit_mask_and_reads_state() -> None:
    transport = ScriptedTransport([b"\x00"])
    controller = BitbangController(transport)
    assert controller.configure_directions(0x13) == 0
    assert transport.writes == [b"\x53"]


def test_read_pins_reuses_tracked_direction_mask() -> None:
    transport = ScriptedTransport([b"\x1d"])
    controller = BitbangController(transport)
    controller.configure_directions(0x13)
    transport.responses.append(b"\x1d")
    assert controller.read_pins() == 0x1D
    assert transport.writes == [b"\x53", b"\x53"]


def test_power_pullup_aux_and_cs_accumulate_and_clear() -> None:
    transport = ScriptedTransport([b"\x40", b"\x60", b"\x70", b"\x71", b"\x01", b"\x00"])
    controller = BitbangController(transport)
    controller.set_power(True)
    controller.set_pullups(True)
    controller.set_aux(True)
    controller.set_cs(True)
    controller.set_cs(False)
    controller.set_power(False)
    assert transport.writes == [b"\xc0", b"\xe0", b"\xf0", b"\xf1", b"\xf0", b"\xb0"]


def test_write_bits_clocks_msb_first_with_exact_state_commands() -> None:
    transport = ScriptedTransport([b"\x00"] * 14)
    controller = BitbangController(transport)
    controller.write_bits(0b1010, 4)
    assert transport.writes == [
        b"\x53",
        b"\x80",
        b"\x88",
        b"\x8c",
        b"\x88",
        b"\x80",
        b"\x84",
        b"\x80",
        b"\x88",
        b"\x8c",
        b"\x88",
        b"\x80",
        b"\x84",
        b"\x80",
    ]


def test_read_bits_assembles_msb_first() -> None:
    transport = ScriptedTransport([b"\x00", b"\x00", b"\x06", b"\x02", b"\x04", b"\x00", b"\x06", b"\x02", b"\x04", b"\x00"])
    controller = BitbangController(transport)
    assert controller.read_bits(4) == 0b1010


def test_read_bytes_wraps_eight_bit_reads() -> None:
    transport = ScriptedTransport([b"\x00"] * 35)
    controller = BitbangController(transport)
    assert controller.read_bytes(2) == b"\x00\x00"


def test_self_test_reads_error_count_and_exits_loop() -> None:
    transport = ScriptedTransport([b"\x00", b"\x01"])
    controller = BitbangController(transport)
    assert controller.self_test() == SelfTestReport(error_count=0, long_test=False)
    assert transport.writes == [b"\x10", b"\xff"]


def test_long_self_test_uses_full_test_command() -> None:
    transport = ScriptedTransport([b"\x02", b"\x01"])
    controller = BitbangController(transport)
    assert controller.self_test(long_test=True).error_count == 2
    assert transport.writes == [b"\x11", b"\xff"]


def test_self_test_rejects_missing_exit_acknowledgement() -> None:
    transport = ScriptedTransport([b"\x00", b"\x00"])
    with pytest.raises(ProtocolError, match="did not acknowledge exit"):
        BitbangController(transport).self_test()


def test_read_adc_returns_big_endian_sample() -> None:
    transport = ScriptedTransport([b"\x0a\xbc"])
    assert BitbangController(transport).read_adc() == 0x0ABC
    assert transport.writes == [b"\x14"]


def test_read_adc_rejects_out_of_range_sample() -> None:
    transport = ScriptedTransport([b"\xff\xff"])
    with pytest.raises(ProtocolError, match="exceeds the 12-bit range"):
        BitbangController(transport).read_adc()


def test_setup_and_clear_pwm_have_exact_frames() -> None:
    transport = ScriptedTransport([b"\x01", b"\x01"])
    controller = BitbangController(transport)
    controller.setup_pwm(prescaler=2, duty=0x1234, period=0x5678)
    controller.clear_pwm()
    assert transport.writes == [b"\x12\x02\x12\x34\x56\x78", b"\x13"]


def test_version_requires_bbio_banner() -> None:
    transport = ScriptedTransport([b"BBIO1"])
    assert BitbangController(transport).version() == b"BBIO1"
    assert transport.writes == [b"\x00"]


def test_version_rejects_wrong_banner() -> None:
    transport = ScriptedTransport([b"SPI1X"])
    with pytest.raises(ProtocolError, match="Expected BBIO1 banner"):
        BitbangController(transport).version()


def test_context_manager_resets_unless_disabled() -> None:
    transport = ScriptedTransport([b"BBIO1", b"\x01", b"BBIO1", b"\x01"])
    with BitbangController(transport):
        pass
    with BitbangController(transport, reset_on_exit=False):
        pass
    assert transport.writes == [BBIO_ENTER_SEQUENCE, b"\x0f", BBIO_ENTER_SEQUENCE]


@pytest.mark.parametrize("mask", [-1, 0x20])
def test_configure_directions_rejects_masks_outside_five_bits(mask: int) -> None:
    with pytest.raises(ValueError, match="0 through 0x1F"):
        BitbangController(ScriptedTransport()).configure_directions(mask)


@pytest.mark.parametrize("mask", [-1, 0x80])
def test_set_pins_rejects_masks_outside_state_bits(mask: int) -> None:
    with pytest.raises(ValueError, match="0 through 0x7F"):
        BitbangController(ScriptedTransport()).set_pins(mask)


@pytest.mark.parametrize("count", [0, 33])
def test_bit_count_is_bounded(count: int) -> None:
    controller = BitbangController(ScriptedTransport())
    with pytest.raises(ValueError, match="1 through 32"):
        controller.write_bits(0, count)


def test_bit_value_must_fit_count() -> None:
    with pytest.raises(ValueError, match="does not fit in 4 bits"):
        BitbangController(ScriptedTransport()).write_bits(0x10, 4)


def test_write_bytes_rejects_empty_payload() -> None:
    with pytest.raises(ValueError, match="at least one byte"):
        BitbangController(ScriptedTransport()).write_bytes(b"")


def test_read_bytes_rejects_non_positive_count() -> None:
    with pytest.raises(ValueError, match="count must be positive"):
        BitbangController(ScriptedTransport()).read_bytes(0)


@pytest.mark.parametrize(
    ("kwargs", "message"),
    [
        ({"prescaler": -1, "duty": 0, "period": 0}, "prescaler"),
        ({"prescaler": 4, "duty": 0, "period": 0}, "prescaler"),
        ({"prescaler": 0, "duty": -1, "period": 0}, "duty"),
        ({"prescaler": 0, "duty": 0x10000, "period": 0}, "duty"),
        ({"prescaler": 0, "duty": 0, "period": -1}, "period"),
        ({"prescaler": 0, "duty": 0, "period": 0x10000}, "period"),
    ],
)
def test_pwm_parameter_ranges(kwargs: dict[str, int], message: str) -> None:
    controller = BitbangController(ScriptedTransport())
    with pytest.raises(ValueError, match=message):
        controller.setup_pwm(**kwargs)
