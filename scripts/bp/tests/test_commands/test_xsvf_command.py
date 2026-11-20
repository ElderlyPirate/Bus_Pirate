# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for XSVF file handling and command-service orchestration."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from pathlib import Path

import pytest

from bp.commands.xsvf import XsvfRunReport, read_xsvf_file, run_xsvf
from bp.errors import TransportError, XsvfHostError, XsvfPlayerError
from bp.protocol.xsvf import ChainScan, XsvfPlayReport, XsvfResult, XsvfSession
from bp.types import ConnectionSettings


class FakeXsvfSession:
    """Record XSVF operations without touching a serial port."""

    def __init__(
        self,
        *,
        scan: ChainScan | None = None,
        play: XsvfPlayReport | None = None,
        scan_error: XsvfHostError | None = None,
        play_error: XsvfPlayerError | None = None,
        leave_error: XsvfHostError | None = None,
    ) -> None:
        self.scan = scan or ChainScan(count=0, payload=b"")
        self.play_report = play or XsvfPlayReport(bytes_sent=0, chunks=0, result=XsvfResult.NONE)
        self.scan_error = scan_error
        self.play_error = play_error
        self.leave_error = leave_error
        self.calls: list[tuple[str, bytes | None]] = []

    def reset_chain(self) -> None:
        self.calls.append(("reset", None))

    def scan_chain(self) -> ChainScan:
        self.calls.append(("scan", None))
        if self.scan_error is not None:
            raise self.scan_error
        return self.scan

    def play(self, data: bytes) -> XsvfPlayReport:
        self.calls.append(("play", data))
        if self.play_error is not None:
            raise self.play_error
        return self.play_report

    def leave_to_terminal(self) -> None:
        self.calls.append(("leave", None))
        if self.leave_error is not None:
            raise self.leave_error


def _factory(
    session: FakeXsvfSession,
    captured: list[ConnectionSettings] | None = None,
) -> Callable[[ConnectionSettings], AbstractContextManager[XsvfSession]]:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[XsvfSession, None, None]:
        if captured is not None:
            captured.append(connection)
        yield session

    return factory


def test_read_xsvf_file_returns_raw_bytes(tmp_path: Path) -> None:
    path = tmp_path / "design.xsvf"
    path.write_bytes(b"\x00\x01\xff")
    assert read_xsvf_file(path) == b"\x00\x01\xff"


def test_read_xsvf_file_wraps_missing_file(tmp_path: Path) -> None:
    with pytest.raises(XsvfHostError, match="Could not read XSVF file"):
        read_xsvf_file(tmp_path / "missing.xsvf")


def test_read_xsvf_file_rejects_empty_file(tmp_path: Path) -> None:
    path = tmp_path / "empty.xsvf"
    path.write_bytes(b"")
    with pytest.raises(XsvfHostError, match=r"XSVF file .* is empty"):
        read_xsvf_file(path)


def test_run_xsvf_executes_reset_scan_then_play(
    connection: ConnectionSettings,
) -> None:
    session = FakeXsvfSession(
        scan=ChainScan(count=4, payload=b"\x93\x40\x60\x59"),
        play=XsvfPlayReport(bytes_sent=3, chunks=1, result=XsvfResult.NONE),
    )
    captured: list[ConnectionSettings] = []
    report = run_xsvf(
        connection,
        b"abc",
        reset=True,
        scan=True,
        leave=False,
        session_factory=_factory(session, captured),
    )

    assert report == XsvfRunReport(
        reset=True,
        scan=ChainScan(count=4, payload=b"\x93\x40\x60\x59"),
        play=XsvfPlayReport(bytes_sent=3, chunks=1, result=XsvfResult.NONE),
        left_player=False,
    )
    assert session.calls == [("reset", None), ("scan", None), ("play", b"abc")]
    assert captured == [connection]


def test_run_xsvf_runs_scan_without_file(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession(scan=ChainScan(count=0, payload=b""))
    report = run_xsvf(
        connection,
        None,
        reset=False,
        scan=True,
        leave=False,
        session_factory=_factory(session),
    )
    assert report.scan == ChainScan(count=0, payload=b"")
    assert report.play is None
    assert session.calls == [("scan", None)]


def test_run_xsvf_runs_reset_without_file(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession()
    report = run_xsvf(
        connection,
        None,
        reset=True,
        scan=False,
        leave=False,
        session_factory=_factory(session),
    )
    assert report == XsvfRunReport(reset=True, scan=None, play=None, left_player=False)
    assert session.calls == [("reset", None)]


def test_run_xsvf_rejects_no_selected_operation(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession()
    with pytest.raises(XsvfHostError, match="No XSVF file or player operation was selected"):
        run_xsvf(
            connection,
            None,
            reset=False,
            scan=False,
            leave=False,
            session_factory=_factory(session),
        )
    assert session.calls == []


def test_run_xsvf_rejects_empty_stream_before_opening_session(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession()
    with pytest.raises(XsvfHostError, match="XSVF stream is empty"):
        run_xsvf(
            connection,
            b"",
            reset=False,
            scan=False,
            leave=False,
            session_factory=_factory(session),
        )
    assert session.calls == []


def test_run_xsvf_wraps_session_transport_failure(connection: ConnectionSettings) -> None:
    @contextmanager
    def factory(connection: ConnectionSettings) -> Generator[XsvfSession, None, None]:
        del connection
        raise TransportError("Could not open serial port COM9")
        yield FakeXsvfSession()

    with pytest.raises(XsvfHostError, match="Could not open serial port COM9") as raised:
        run_xsvf(
            connection,
            b"abc",
            reset=False,
            scan=False,
            leave=False,
            session_factory=factory,
        )
    assert raised.value.exit_code == 2


def test_run_xsvf_leave_runs_after_reset_scan_and_play(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession(
        scan=ChainScan(count=4, payload=b"\x93\x40\x60\x59"),
        play=XsvfPlayReport(bytes_sent=3, chunks=1, result=XsvfResult.NONE),
    )
    report = run_xsvf(
        connection,
        b"abc",
        reset=True,
        scan=True,
        leave=True,
        session_factory=_factory(session),
    )

    assert report.left_player is True
    assert session.calls == [("reset", None), ("scan", None), ("play", b"abc"), ("leave", None)]


def test_run_xsvf_does_not_leave_after_player_failure(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession(
        play_error=XsvfPlayerError(0x02, "XSVF_ERROR_TDOMISMATCH", "captured TDO did not match expected TDO")
    )
    with pytest.raises(XsvfPlayerError, match="XSVF_ERROR_TDOMISMATCH"):
        run_xsvf(
            connection,
            b"abc",
            reset=False,
            scan=False,
            leave=True,
            session_factory=_factory(session),
        )
    assert session.calls == [("play", b"abc")]


def test_run_xsvf_propagates_leave_failure(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession(
        scan=ChainScan(count=0, payload=b""),
        leave_error=XsvfHostError("Leaving XSVF player: missing reset acknowledgement"),
    )
    with pytest.raises(XsvfHostError, match="missing reset acknowledgement") as raised:
        run_xsvf(
            connection,
            None,
            reset=False,
            scan=True,
            leave=True,
            session_factory=_factory(session),
        )

    assert raised.value.exit_code == 2
    assert session.calls == [("scan", None), ("leave", None)]
