# SPDX-FileCopyrightText: 2026 Bus Pirate contributors
# SPDX-License-Identifier: Apache-2.0
"""Click command implementations and shared command-layer helpers."""

from __future__ import annotations

import click

from bp.config import AppContext


def get_app_context(ctx: click.Context) -> AppContext:
    """Return the validated application context attached by the root group."""
    app = ctx.find_object(AppContext)
    if app is None:
        raise RuntimeError("Application context is not attached to the Click context")
    return app
