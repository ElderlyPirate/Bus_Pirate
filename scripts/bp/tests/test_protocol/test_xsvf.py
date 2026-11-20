# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for XSVF player entry, chain probing, and chunk streaming."""

from __future__ import annotations

from collections import deque

import pytest

from bp.errors import BpError, TransportError, TransportTimeout, XsvfHostError, XsvfPlayerError
from bp.protocol.xsvf import (
    ChainScan,
    XsvfController,
    XsvfPlayReport,
    XsvfResult,
    result_name,
)
from conftest import ScriptedTransport

ENTER = b"\x00" * 21
BASE_WRITES = [ENTER, b"\x18"]
BASE_RESPONSES = [b"BBIO1", b"XSV1"]
TERMINAL_BANNER = b"\r\nRESET\r\nBus Pirate v4\r\nElderlyPirate Firmware v9.0\r\nHiZ>"


def _entered(responses: list[bytes]) -> ScriptedTransport:
    return ScriptedTransport([*BASE_RESPONSES, *responses])


def test_enter_uses_binary_then_xsvf_banners() -> None:
    transport = _entered([])
    with XsvfController(transport):
        pass
    assert transport.writes == BASE_WRITES
    assert transport.reset_count == 2
    transport.assert_consumed()


def test_enter_rejects_wrong_bitbang_banner() -> None:
    transport = ScriptedTransport([b"SPI1X"])
    with pytest.raises(XsvfHostError, match="Expected BBIO1 banner"):
        with XsvfController(transport):
            pass


def test_enter_rejects_wrong_xsvf_banner() -> None:
    transport = ScriptedTransport([b"BBIO1", b"XSV0"])
    with pytest.raises(XsvfHostError, match="Expected XSV1 banner, received 0x58 0x53 0x56 0x30"):
        with XsvfController(transport):
            pass


def test_enter_reports_v3_or_old_firmware_timeout() -> None:
    transport = ScriptedTransport([b"BBIO1", b"\x00"])
    with pytest.raises(XsvfHostError, match="XSVF requires a Bus Pirate v4"):
        with XsvfController(transport):
            pass


def test_reset_sends_command_without_reading_a_reply() -> None:
    transport = _entered([])
    with XsvfController(transport) as controller:
        controller.reset_chain()
    assert transport.writes == [*BASE_WRITES, b"\x01"]
    transport.assert_consumed()


def test_scan_reads_count_and_idcode_payload() -> None:
    transport = _entered([b"\x04\x93\x40\x60\x59"])
    with XsvfController(transport) as controller:
        scan = controller.scan_chain()

    assert scan == ChainScan(count=4, payload=b"\x93\x40\x60\x59")
    assert scan.raw == b"\x04\x93\x40\x60\x59"
    assert scan.idcodes == (0x59604093,)
    assert transport.writes == [*BASE_WRITES, b"\x02"]
    transport.assert_consumed()


def test_scan_accepts_zero_devices() -> None:
    transport = _entered([b"\x00"])
    with XsvfController(transport) as controller:
        scan = controller.scan_chain()
    assert scan == ChainScan(count=0, payload=b"")
    assert scan.idcodes == ()


@pytest.mark.parametrize("count", [0x03, 0xFD])
def test_scan_rejects_invalid_count(count: int) -> None:
    transport = _entered([bytes([count])])
    with pytest.raises(XsvfHostError, match=f"Chain scan: device count 0x{count:02X}"):
        with XsvfController(transport) as controller:
            controller.scan_chain()


def test_scan_reports_missing_count_after_bounded_retries() -> None:
    transport = _entered([])
    with pytest.raises(XsvfHostError, match="Chain scan: incomplete reply \\(bounded timeout\\); no reply after 5 attempts"):
        with XsvfController(transport) as controller:
            controller.scan_chain()


def test_scan_reports_truncated_payload() -> None:
    transport = _entered([b"\x04\x93\x40"])
    with pytest.raises(XsvfHostError, match="Chain scan: incomplete reply"):
        with XsvfController(transport) as controller:
            controller.scan_chain()


def test_scan_reports_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1"],
    )
    transport.fail_after_writes = 2
    with pytest.raises(XsvfHostError, match="Chain scan: failed to send the command: serial write failed"):
        with XsvfController(transport) as controller:
            controller.scan_chain()


def test_reset_reports_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1"],
    )
    transport.fail_after_writes = 2
    with pytest.raises(XsvfHostError, match="Failed to send the JTAG reset command: serial write failed"):
        with XsvfController(transport) as controller:
            controller.reset_chain()


def test_play_small_file_uses_big_endian_length_and_single_chunk() -> None:
    transport = _entered([b"\xff", b"", b"\x00"])
    with XsvfController(transport) as controller:
        report = controller.play(b"abc")

    assert report == XsvfPlayReport(bytes_sent=3, chunks=1, result=XsvfResult.NONE)
    assert transport.writes == [*BASE_WRITES, b"\x03", b"\x00\x03", b"abc"]
    transport.assert_consumed()


def test_play_exactly_one_full_chunk() -> None:
    data = bytes(range(256)) * 16
    transport = _entered([b"\xff", b"", b"\x00"])
    with XsvfController(transport) as controller:
        report = controller.play(data)

    assert report == XsvfPlayReport(bytes_sent=4096, chunks=1, result=XsvfResult.NONE)
    assert transport.writes == [*BASE_WRITES, b"\x03", b"\x10\x00", data]
    transport.assert_consumed()


def test_play_oversized_file_splits_at_4096_byte_boundary() -> None:
    data = bytes([0x5A]) * 4097
    transport = _entered([b"\xff", b"", b"\xff", b"", b"\x00"])
    with XsvfController(transport) as controller:
        report = controller.play(data)

    assert report == XsvfPlayReport(bytes_sent=4097, chunks=2, result=XsvfResult.NONE)
    assert transport.writes == [
        *BASE_WRITES,
        b"\x03",
        b"\x10\x00",
        data[:4096],
        b"\x00\x01",
        data[4096:],
    ]
    transport.assert_consumed()


def test_play_multiple_full_chunks_and_remainder() -> None:
    data = bytes([0xA5]) * (2 * 4096 + 3)
    responses = [b"\xff", b"", b"\xff", b"", b"\xff", b"", b"\x00"]
    transport = _entered(responses)
    with XsvfController(transport) as controller:
        report = controller.play(data)

    assert report == XsvfPlayReport(bytes_sent=len(data), chunks=3, result=XsvfResult.NONE)
    headers = [write for write in transport.writes if len(write) == 2 and write not in {b"\x18"}]
    assert headers == [b"\x10\x00", b"\x10\x00", b"\x00\x03"]
    transport.assert_consumed()


def test_play_accepts_immediate_success_result() -> None:
    transport = _entered([b"\x00"])
    with XsvfController(transport) as controller:
        report = controller.play(b"abc")

    assert report == XsvfPlayReport(bytes_sent=0, chunks=0, result=XsvfResult.NONE)
    assert transport.writes == [*BASE_WRITES, b"\x03"]


def test_play_rejects_empty_stream_before_start_command() -> None:
    transport = _entered([])
    with pytest.raises(XsvfHostError, match="XSVF stream is empty"):
        with XsvfController(transport) as controller:
            controller.play(b"")
    assert transport.writes == BASE_WRITES


@pytest.mark.parametrize(
    ("result_code", "result_name_value"),
    [
        (0x01, "XSVF_ERROR_UNKNOWN"),
        (0x02, "XSVF_ERROR_TDOMISMATCH"),
        (0x03, "XSVF_ERROR_MAXRETRIES"),
        (0x04, "XSVF_ERROR_ILLEGALCMD"),
        (0x05, "XSVF_ERROR_ILLEGALSTATE"),
        (0x06, "XSVF_ERROR_DATAOVERFLOW"),
        (0x07, "XSVF_ERROR_LAST"),
        (0x80, "XSVF_ERROR_UNKNOWN"),
    ],
)
def test_play_raises_player_error_for_nonzero_result(result_code: int, result_name_value: str) -> None:
    transport = _entered([b"\xff", b"", bytes([result_code])])
    with pytest.raises(XsvfPlayerError) as raised:
        with XsvfController(transport) as controller:
            controller.play(b"abc")

    assert raised.value.result_code == result_code
    assert raised.value.result_name == result_name_value
    assert raised.value.exit_code == 1
    assert f"End of operation reply: {result_name_value}" in str(raised.value)


def test_play_reports_end_of_file_without_result() -> None:
    transport = _entered([b"\xff", b"", b"\xff"])
    with pytest.raises(XsvfHostError, match="End of file reached without a completion result"):
        with XsvfController(transport) as controller:
            controller.play(b"abc")
    assert transport.writes[-1] == b"abc"


def test_play_reports_missing_first_data_request() -> None:
    transport = _entered([])
    with pytest.raises(
        XsvfHostError,
        match="Timed out waiting for the XSVF player data request; no reply after 5 attempts",
    ):
        with XsvfController(transport) as controller:
            controller.play(b"abc")
    assert transport.writes == [*BASE_WRITES, b"\x03"]


def test_play_reports_missing_result_after_last_chunk() -> None:
    transport = _entered([b"\xff", b"", b""])
    with pytest.raises(XsvfHostError, match="Timed out waiting for the XSVF player result; no reply after 5 attempts"):
        with XsvfController(transport) as controller:
            controller.play(b"abc")


def test_play_reports_start_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1"],
    )
    transport.fail_after_writes = 2
    with pytest.raises(XsvfHostError, match="Failed to start the XSVF player: serial write failed"):
        with XsvfController(transport) as controller:
            controller.play(b"abc")


def test_play_reports_chunk_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1", b"\xff"],
    )
    # Fail after the play command and the chunk header have been accepted.
    transport.fail_after_writes = 4
    with pytest.raises(XsvfHostError, match="Failed to send XSVF chunk 1 data: serial write failed"):
        with XsvfController(transport) as controller:
            controller.play(b"abc")


def test_leave_sends_reset_pair_and_drains_terminal_banner() -> None:
    transport = _entered([b"", b"\x01" + TERMINAL_BANNER])
    with XsvfController(transport) as controller:
        controller.leave_to_terminal()

    assert transport.writes == [*BASE_WRITES, b"\x0f", b"\x0f"]
    assert not transport.received
    transport.assert_consumed()


def test_leave_rejects_failed_acknowledgement() -> None:
    transport = _entered([b"", b"\x00"])
    with pytest.raises(XsvfHostError, match="Leaving XSVF player: Bus Pirate reported a failed binary command") as raised:
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()

    assert raised.value.exit_code == 2
    assert transport.writes == [*BASE_WRITES, b"\x0f", b"\x0f"]


def test_leave_rejects_unexpected_ack_byte() -> None:
    transport = _entered([b"", b"\x02"])
    with pytest.raises(
        XsvfHostError,
        match="Leaving XSVF player: Expected binary success status 0x01, received 0x02",
    ) as raised:
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()

    assert raised.value.exit_code == 2
    assert transport.writes == [*BASE_WRITES, b"\x0f", b"\x0f"]


def test_leave_reports_missing_acknowledgement() -> None:
    transport = _entered([b"", b""])
    with pytest.raises(XsvfHostError, match="Leaving XSVF player: missing reset acknowledgement") as raised:
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()

    assert raised.value.exit_code == 2


def test_leave_reports_first_command_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1"],
    )
    transport.fail_after_writes = 2
    with pytest.raises(XsvfHostError, match="Failed to send the XSVF player leave command: serial write failed"):
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()


def test_leave_reports_reset_write_failure() -> None:
    transport = FaultyTransport(
        write_error=TransportError("serial write failed"),
        responses=[b"BBIO1", b"XSV1"],
    )
    transport.fail_after_writes = 3
    with pytest.raises(XsvfHostError, match="Leaving XSVF player: serial write failed"):
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()


def test_leave_reports_terminal_banner_drain_failure() -> None:
    transport = DrainFaultTransport(
        write_error=TransportError("unused"),
        responses=[b"BBIO1", b"XSV1", b"", b"\x01"],
    )
    with pytest.raises(XsvfHostError, match="Leaving XSVF player: could not read the terminal banner: serial read failed"):
        with XsvfController(transport) as controller:
            controller.leave_to_terminal()


def test_controller_rejects_invalid_reply_attempts() -> None:
    with pytest.raises(ValueError, match="attempts must be positive"):
        XsvfController(ScriptedTransport(), reply_attempts=0)


def test_chain_scan_validates_payload_length() -> None:
    with pytest.raises(ValueError, match="payload has 2 bytes"):
        ChainScan(count=4, payload=b"\x00\x01")


def test_chain_scan_rejects_non_multiple_of_four_count() -> None:
    with pytest.raises(ValueError, match="not a multiple of four"):
        ChainScan(count=3, payload=b"\x00\x01\x02")


def test_result_name_is_exposed_for_command_output() -> None:
    assert result_name(XsvfResult.NONE) == "XSVF_ERROR_NONE"
    assert result_name(XsvfResult.TDOMISMATCH) == "XSVF_ERROR_TDOMISMATCH"


def test_xsvf_host_error_exit_code_is_two() -> None:
    assert issubclass(XsvfHostError, BpError)
    assert XsvfHostError("setup").exit_code == 2


class FaultyTransport:
    """Fake transport that fails the requested write after a fixed number of writes."""

    def __init__(self, *, write_error: TransportError, responses: list[bytes]) -> None:
        self.write_error = write_error
        self.responses: deque[bytes] = deque(responses)
        self.fail_after_writes: int | None = None
        self.received = bytearray()
        self.writes: list[bytes] = []

    def write(self, data: bytes) -> None:
        if self.fail_after_writes is not None and len(self.writes) >= self.fail_after_writes:
            raise self.write_error
        self.writes.append(bytes(data))
        if self.responses:
            self.received.extend(self.responses.popleft())

    def read_exact(self, size: int) -> bytes:
        if len(self.received) < size:
            raise TransportTimeout(f"fake transport has {len(self.received)} of {size} bytes")
        chunk = bytes(self.received[:size])
        del self.received[:size]
        return chunk

    def read_until_idle(self, *, max_bytes: int) -> bytes:
        chunk = bytes(self.received[:max_bytes])
        del self.received[: len(chunk)]
        return chunk

    def reset_input(self) -> None:
        self.received.clear()


class DrainFaultTransport(FaultyTransport):
    """Fake transport whose terminal-banner drain fails."""

    def read_until_idle(self, *, max_bytes: int) -> bytes:
        del max_bytes
        raise TransportError("serial read failed")
