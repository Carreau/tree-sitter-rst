#!/usr/bin/env python3
"""Append hand-maintained pyproject.toml sections after `tree-sitter generate`.

`tree-sitter generate` regenerates pyproject.toml from a template and drops any
sections that aren't part of that template (e.g. ``[tool.cibuildwheel]``). The
sections we want to preserve live in ``pyproject.extra.toml`` and are appended
to ``pyproject.toml`` by this script.

Behaviour:
- Idempotent: a marker block delimits the appended content, so re-running the
  script does not produce duplicates.
- The script is invoked automatically by ``make generate-bindings`` and by the
  ``generate`` CI job after ``tree-sitter generate``. Run it manually if you
  edit ``pyproject.extra.toml`` directly.
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PYPROJECT = ROOT / "pyproject.toml"
EXTRAS = ROOT / "pyproject.extra.toml"

BEGIN_MARKER = "# >>> pyproject.extra.toml (managed by utils/apply_pyproject_extras.py) >>>"
END_MARKER = "# <<< pyproject.extra.toml <<<"


def strip_existing_block(text: str) -> str:
    begin = text.find(BEGIN_MARKER)
    if begin == -1:
        return text
    end = text.find(END_MARKER, begin)
    if end == -1:
        raise SystemExit(
            f"Found {BEGIN_MARKER!r} but no matching {END_MARKER!r} in {PYPROJECT}; "
            "refusing to modify the file."
        )
    end += len(END_MARKER)
    # Trim trailing newline that follows the end marker, if any.
    if end < len(text) and text[end] == "\n":
        end += 1
    # Trim the leading newline that precedes the begin marker, if any.
    if begin > 0 and text[begin - 1] == "\n":
        begin -= 1
    return text[:begin] + text[end:]


def main() -> int:
    if not PYPROJECT.exists():
        print(f"error: {PYPROJECT} does not exist", file=sys.stderr)
        return 1
    if not EXTRAS.exists():
        print(f"error: {EXTRAS} does not exist", file=sys.stderr)
        return 1

    pyproject = PYPROJECT.read_text()
    extras = EXTRAS.read_text().rstrip() + "\n"

    base = strip_existing_block(pyproject).rstrip() + "\n"
    merged = f"{base}\n{BEGIN_MARKER}\n{extras}{END_MARKER}\n"

    if merged == pyproject:
        return 0

    PYPROJECT.write_text(merged)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
