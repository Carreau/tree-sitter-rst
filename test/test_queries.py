"""Tests for the tree-sitter queries shipped in queries/*.scm.

Two things are verified:

1. Every ``queries/*.scm`` file compiles with :class:`tree_sitter.Query`.
   Compilation validates node and capture names against the generated
   grammar, so a query referencing a node type that does not exist (e.g.
   after a grammar rename) fails here instead of silently matching
   nothing in editors.

2. ``queries/highlights.scm`` actually highlights: parsing a small but
   representative RST document produces the expected captures (heading,
   bold, italic, raw, directive type, list bullets, links, …).
"""

from __future__ import annotations

from pathlib import Path

import pytest
import tree_sitter
import tree_sitter_rst

QUERIES_DIR = Path(__file__).resolve().parent.parent / "queries"


@pytest.fixture(scope="module")
def language() -> tree_sitter.Language:
    return tree_sitter.Language(tree_sitter_rst.language())


def _query_files() -> list[Path]:
    return sorted(QUERIES_DIR.glob("*.scm"))


def test_queries_dir_has_highlights() -> None:
    files = [p.name for p in _query_files()]
    assert files, f"no .scm files found in {QUERIES_DIR}"
    assert "highlights.scm" in files


@pytest.mark.parametrize(
    "query_file", _query_files(), ids=lambda p: p.name
)
def test_query_compiles(language: tree_sitter.Language, query_file: Path) -> None:
    """Every shipped query must compile against the current grammar.

    tree_sitter.Query raises QueryError on unknown node types, bad
    fields, or malformed patterns.
    """
    query = tree_sitter.Query(language, query_file.read_text())
    assert query.pattern_count > 0
    assert query.capture_count > 0


# A small document exercising the main constructs covered by
# highlights.scm: a section, inline markup, a bullet list, a directive
# with argument and content, a literal block, and hyperlink targets.
SAMPLE = b"""\
Section Title
=============

A paragraph with *emphasis*, **strong**, ``literal`` and a reference_.

- first bullet
- second bullet

.. note:: Directive argument

   Directive content.

::

    a literal block

.. _reference: https://example.com/
"""


@pytest.fixture(scope="module")
def highlight_captures(language: tree_sitter.Language) -> dict[str, list[tree_sitter.Node]]:
    parser = tree_sitter.Parser(language)
    tree = parser.parse(SAMPLE)
    assert not tree.root_node.has_error, tree.root_node
    query = tree_sitter.Query(
        language, (QUERIES_DIR / "highlights.scm").read_text()
    )
    cursor = tree_sitter.QueryCursor(query)
    return cursor.captures(tree.root_node)


def _texts(captures: dict[str, list[tree_sitter.Node]], name: str) -> list[str]:
    return [node.text.decode() for node in captures.get(name, [])]


def test_heading_capture(highlight_captures) -> None:
    assert "Section Title" in _texts(highlight_captures, "markup.heading")


def test_section_adornment_capture(highlight_captures) -> None:
    assert "=============" in _texts(highlight_captures, "punctuation.special")


def test_bold_and_italic_captures(highlight_captures) -> None:
    assert "**strong**" in _texts(highlight_captures, "markup.bold")
    assert "*emphasis*" in _texts(highlight_captures, "markup.italic")


def test_inline_literal_capture(highlight_captures) -> None:
    assert "``literal``" in _texts(highlight_captures, "markup.raw")


def test_directive_captures(highlight_captures) -> None:
    assert "note" in _texts(highlight_captures, "keyword.directive")
    raw_blocks = _texts(highlight_captures, "markup.raw.block")
    assert any("Directive content." in text for text in raw_blocks)


def test_bullet_capture(highlight_captures) -> None:
    assert _texts(highlight_captures, "markup.list") == ["-", "-"]


def test_literal_block_capture(highlight_captures) -> None:
    raw_blocks = _texts(highlight_captures, "markup.raw.block")
    assert any("a literal block" in text for text in raw_blocks)


def test_reference_and_target_captures(highlight_captures) -> None:
    assert "reference_" in _texts(highlight_captures, "markup.link")
    assert "https://example.com/" in _texts(highlight_captures, "markup.link.url")
