"""Standard-library logging configuration for the host frontend."""

from __future__ import annotations

import logging


def configure_logging(*, verbose: bool) -> None:
    """Configure stdlib logging once for the console entry point."""
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.WARNING,
        format="%(levelname)s %(name)s: %(message)s",
    )
