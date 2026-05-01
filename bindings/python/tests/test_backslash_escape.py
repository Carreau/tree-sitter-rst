"""Regression tests for backslash-escape handling in parse_text.

PR #25 (commit ed6df39) fixed a bug where backslash-escaped inline markup
start chars (``\\`x\\```, ``\\*y\\*``) did not actually prevent inline
markup from opening on the next scanner dispatch. ``parse_text`` consumed
only the leading backslash as a one-char text token; the very next call
saw the literal backtick / star in lookahead and dispatched to
``parse_inline_markup``. If a real markup pair appeared later in the
paragraph, the buggy parse formed a single overlong markup node spanning
the escaped chars *plus* the real ones.

The corpus tests added by PR #25 only checked the *count* of markup
nodes, which doesn't change when the bug regresses — the buggy parse
still emits one ``interpreted_text``, just with the wrong byte range.
This file asserts the range explicitly so a regression in
``parse_text`` is caught.
"""

from unittest import TestCase

import tree_sitter
import tree_sitter_rst


def _lang():
    return tree_sitter.Language(tree_sitter_rst.language())


def _parser():
    return tree_sitter.Parser(_lang())


def _find(node, type_name):
    """Yield every descendant whose ``type`` matches ``type_name``.

    Stops descending once a match is found — the grammar wraps
    ``interpreted_text`` inside another ``interpreted_text`` (the
    default-role alias), and we only want to count each markup span
    once.
    """
    if node.type == type_name:
        yield node
        return
    for child in node.children:
        yield from _find(child, type_name)


class TestBackslashEscapeRange(TestCase):
    def assertSpan(self, node, source: bytes, expected_text: str):
        """Assert ``node``'s byte slice equals ``expected_text``."""
        actual = source[node.start_byte:node.end_byte].decode("utf-8")
        self.assertEqual(
            actual,
            expected_text,
            f"node {node.type} at {node.start_point}-{node.end_point} "
            f"covers {actual!r}, expected {expected_text!r}",
        )

    def test_escaped_then_real_backticks_only_real_is_interpreted(self):
        """``A \\`fake\\` and `real` here.`` should mark only `real` as
        interpreted_text. Without the parse_text escape fix, the buggy
        parse claims ``\\`fake\\` and `real`` as one interpreted_text."""
        src = b"A \\`fake\\` and `real` here.\n"
        tree = _parser().parse(src)
        nodes = list(_find(tree.root_node, "interpreted_text"))
        self.assertEqual(
            len(nodes), 1, f"expected exactly one interpreted_text, got {len(nodes)}"
        )
        self.assertSpan(nodes[0], src, "`real`")

    def test_escaped_then_real_backticks_with_intervening_text(self):
        """``A \\`foo with `bar` later.`` should mark only `bar`. The
        buggy parse claims ``\\`foo with `bar`` as the interpreted_text."""
        src = b"A \\`foo with `bar` later.\n"
        tree = _parser().parse(src)
        nodes = list(_find(tree.root_node, "interpreted_text"))
        self.assertEqual(len(nodes), 1)
        self.assertSpan(nodes[0], src, "`bar`")

    def test_escaped_emphasis_followed_by_real_emphasis(self):
        """``A \\*fake\\* and *real* end.`` should mark only `*real*`."""
        src = b"A \\*fake\\* and *real* end.\n"
        tree = _parser().parse(src)
        nodes = list(_find(tree.root_node, "emphasis"))
        self.assertEqual(len(nodes), 1)
        self.assertSpan(nodes[0], src, "*real*")

    def test_two_escaped_pairs_around_two_real_pairs(self):
        """``A \\`a\\` and `r1` and `r2` end.`` keeps both real pairs and
        absorbs neither escaped pair into them."""
        src = b"A \\`a\\` and `r1` and `r2` end.\n"
        tree = _parser().parse(src)
        nodes = list(_find(tree.root_node, "interpreted_text"))
        self.assertEqual(len(nodes), 2)
        self.assertSpan(nodes[0], src, "`r1`")
        self.assertSpan(nodes[1], src, "`r2`")
