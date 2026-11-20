# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for XSVF file handling and command-service orchestration."""

from __future__ import annotations

from collections.abc import Callable, Generator
from contextlib import AbstractContextManager, contextmanager
from pathlib import Path

import pytest

from bp.commands.xsvf import XsvfRunReport, read_xsvf_file, run_xsvf
from bp.errors import TransportError, XsvfHostError
from bp.protocol.xsvf import ChainScan, XsvfPlayReport, XsvfResult, XsvfSession
from bp.types import ConnectionSettings


class FakeXsvfSession:
    """Record XSVF operations without touching a serial port."""

    def __init__(
        self,
        *,
        scan: ChainScan | None = None,
        play: XsvfPlayReport | None = None,
    ) -> None:
        self.scan = scan or ChainScan(count=0, payload=b"")
        self.play_report = play or XsvfPlayReport(bytes_sent=0, chunks=0, result=XsvfResult.NONE)
        self.calls: list[tuple[str, bytes | None]] = []

    def reset_chain(self) -> None:
        self.calls.append(("reset", None))

    def scan_chain(self) -> ChainScan:
        self.calls.append(("scan", None))
        return self.scan

    def play(self, data: bytes) -> XsvfPlayReport:
        self.calls.append(("play", data))
        return self.play_report


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
        session_factory=_factory(session, captured),
    )

    assert report == XsvfRunReport(
        reset=True,
        scan=ChainScan(count=4, payload=b"\x93\x40\x60\x59"),
        play=XsvfPlayReport(bytes_sent=3, chunks=1, result=XsvfResult.NONE),
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
        session_factory=_factory(session),
    )
    assert report == XsvfRunReport(reset=True, scan=None, play=None)
    assert session.calls == [("reset", None)]


def test_run_xsvf_rejects_no_selected_operation(connection: ConnectionSettings) -> None:
    session = FakeXsvfSession()
    with pytest.raises(XsvfHostError, match="No XSVF file or player operation was selected"):
        run_xsvf(
            connection,
            None,
            reset=False,
            scan=False,
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
            session_factory=factory,
        )
    assert raised.value.exit_code == 2
