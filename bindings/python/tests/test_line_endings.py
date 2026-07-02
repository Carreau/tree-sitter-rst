"""Line-ending regression tests.

The scanner must treat \r\n as a single newline without leaking the \r
into token spans, and a lone \r (old-Mac line endings) must terminate a
line just like \n does.
"""

from unittest import TestCase

import tree_sitter
import tree_sitter_rst

LANGUAGE = tree_sitter.Language(tree_sitter_rst.language())


def parse(source: bytes) -> tree_sitter.Tree:
    return tree_sitter.Parser(LANGUAGE).parse(source)


def node_types(node) -> list:
    return [child.type for child in node.children]


class TestCrlf(TestCase):
    def test_spans_exclude_carriage_return(self):
        source = b"Title\r\n=====\r\n\r\nPara *em* text.\r\n"
        tree = parse(source)
        self.assertFalse(tree.root_node.has_error)
        section, paragraph = tree.root_node.children
        self.assertEqual(section.type, "section")
        self.assertEqual(paragraph.type, "paragraph")
        title = section.children[0]
        self.assertEqual(source[title.start_byte : title.end_byte], b"Title")
        self.assertEqual(
            source[paragraph.start_byte : paragraph.end_byte], b"Para *em* text."
        )

    def test_literal_block(self):
        source = b"para::\r\n\r\n   lit\r\n"
        tree = parse(source)
        self.assertFalse(tree.root_node.has_error)
        (paragraph,) = tree.root_node.children
        literal = paragraph.children[-1]
        self.assertEqual(literal.type, "literal_block")
        self.assertEqual(source[literal.start_byte : literal.end_byte], b"lit")


class TestLoneCarriageReturn(TestCase):
    def test_paragraph_break(self):
        tree = parse(b"para one\r\rpara two\r")
        self.assertFalse(tree.root_node.has_error)
        self.assertEqual(node_types(tree.root_node), ["paragraph", "paragraph"])

    def test_bullet_list(self):
        tree = parse(b"- a\r- b\r")
        self.assertFalse(tree.root_node.has_error)
        (bullet_list,) = tree.root_node.children
        self.assertEqual(bullet_list.type, "bullet_list")
        self.assertEqual(len(bullet_list.named_children), 2)

    def test_mixed_line_endings_single_paragraph(self):
        tree = parse(b"line a\r\nline b\nline c\r")
        self.assertFalse(tree.root_node.has_error)
        self.assertEqual(node_types(tree.root_node), ["paragraph"])
