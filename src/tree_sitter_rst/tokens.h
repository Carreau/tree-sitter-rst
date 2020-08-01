#ifndef TREE_SITTER_RST_TOKENS_H_
#define TREE_SITTER_RST_TOKENS_H_

enum TokenType {
  T_NEWLINE,
  T_BLANKLINE,
  T_INDENT,
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
  T_LITERAL_BLOCK_MARK,
  T_QUOTED_LITERAL_BLOCK,

  // Line blocks
  T_LINE_BLOCK_MARK,

  // Block quotes
  T_ATTRIBUTION_MARK,

  // Doctest blocks
  T_DOCTEST_BLOCK_MARK,

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
  T_DIRECTIVE_MARK,
  T_SUBSTITUTION_MARK,
  T_EMPTY_COMMENT,
};

#endif /* TREE_SITTER_RST_TOKENS_H_ */
