#ifndef TREE_SITTER_RST_TOKENS_H_
#define TREE_SITTER_RST_TOKENS_H_

enum TokenType {
  // Whitespace
  T_NEWLINE,
  T_BLANKLINE,
  T_INDENT,
  T_NEWLINE_INDENT,
  T_DEDENT,

  // Sections
  T_OVERLINE,
  T_UNDERLINE,

  // Transitions
  T_TRANSITION,

  // Lists
  T_CHAR_BULLET,
  T_NUMERIC_BULLET,
  T_FIELD_MARK,
  T_FIELD_MARK_END,

  // Literal blocks
  T_LITERAL_INDENTED_BLOCK_MARK,
  T_LITERAL_QUOTED_BLOCK_MARK,
  T_QUOTED_LITERAL_BLOCK,

  // Line blocks
  T_LINE_BLOCK_MARK,

  // Block quotes
  T_ATTRIBUTION_MARK,

  // Doctest blocks
  T_DOCTEST_BLOCK_MARK,

  // Tables — structural tokens, one per line.
  //
  // Grid tables emit a ``+---+...+`` border, a ``+===+...+`` header
  // separator, or a ``|...|...|`` content line. Simple tables emit
  // ``=== ===`` borders, ``--- ---`` column-spanning header underlines,
  // and content lines. The grammar composes these into ``grid_table`` /
  // ``simple_table`` nodes with row sub-structure.
  T_GRID_TABLE_SEPARATOR,
  T_GRID_TABLE_HEADER_SEP,
  T_GRID_TABLE_ROW_LINE,
  T_SIMPLE_TABLE_BORDER,
  T_SIMPLE_TABLE_DASHES,
  T_SIMPLE_TABLE_ROW_LINE,

  // Inline markup
  T_TEXT,
  T_EMPHASIS,
  T_STRONG,
  T_INTERPRETED_TEXT,
  T_INTERPRETED_TEXT_PREFIX,
  T_ROLE_NAME_PREFIX,
  T_ROLE_NAME_SUFFIX,
  T_LITERAL,
  T_SUBSTITUTION_REFERENCE,
  T_INLINE_TARGET,
  T_FOOTNOTE_REFERENCE,
  T_CITATION_REFERENCE,
  T_REFERENCE,
  T_STANDALONE_HYPERLINK,

  // Markup blocks
  T_EXPLICIT_MARKUP_START,
  T_FOOTNOTE_LABEL,
  T_CITATION_LABEL,
  T_TARGET_NAME,
  T_ANONYMOUS_TARGET_MARK,
  T_DIRECTIVE_NAME,
  T_SUBSTITUTION_MARK,
  T_EMPTY_COMMENT,

  T_INVALID_TOKEN,

  T_CLASSIFIER_INDENT_CHECK,
};

#endif /* TREE_SITTER_RST_TOKENS_H_ */
