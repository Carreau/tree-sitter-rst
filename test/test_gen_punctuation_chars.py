"""Tests for utils/gen_punctuation_chars.py.

The docutils punctuation strings are regex character-class fragments, so
the generator must resolve escapes (``\\\\`` is one literal backslash,
``\\-`` a literal dash) instead of copying the escape backslashes into
the generated C tables, and must not duplicate range ends as singles.
"""

import importlib.util
import sys
from pathlib import Path

import pytest

_SCRIPT = Path(__file__).parent.parent / "utils" / "gen_punctuation_chars.py"
_spec = importlib.util.spec_from_file_location("gen_punctuation_chars", _SCRIPT)
gen = importlib.util.module_from_spec(_spec)
sys.modules["gen_punctuation_chars"] = gen
_spec.loader.exec_module(gen)


def entries(lines: list) -> list:
    """Extract the singles emitted between the braces of the first table."""
    out = []
    for line in lines[1:]:
        if line == "};":
            break
        out.append(line.strip().rstrip(","))
    return out


def test_escaped_backslash_is_one_entry():
    # The python string "\\\\" is the regex fragment \\ : one literal backslash.
    lines = gen.generate_c_chars_define("t", "\\\\.,")
    assert entries(lines) == ["'\\\\'", "'.'", "','"]


def test_escaped_dash_is_literal_not_range():
    # \- is a literal dash; it must not produce a backslash entry nor a range.
    lines = gen.generate_c_chars_define("t", "\\-/:")
    assert entries(lines) == ["'-'", "'/'", "':'"]


def test_escaped_bracket():
    lines = gen.generate_c_chars_define("t", "(<\\[{")
    assert entries(lines) == ["'('", "'<'", "'['", "'{'"]


def test_duplicates_removed():
    lines = gen.generate_c_chars_define("t", "abcb")
    assert entries(lines) == ["'a'", "'b'", "'c'"]


def test_range_end_not_duplicated_as_single():
    lines = gen.generate_c_chars_define("t", "a՚-՟b", expects_range=True)
    assert entries(lines) == ["'a'", "'b'"]
    assert "  {0x55a, 0x55f}," in lines


def test_checked_in_header_is_current():
    """The checked-in header must match the generator's output.

    Guards both against forgetting to regenerate after editing the script
    and against silent docutils data drift. Skipped when docutils (the
    generator's data source) is not installed.
    """
    pytest.importorskip("docutils")
    import contextlib
    import io

    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer):
        gen.main()
    header = Path(__file__).parent.parent / "src" / "tree_sitter_rst" / "punctuation_chars.h"
    assert buffer.getvalue() == header.read_text()
