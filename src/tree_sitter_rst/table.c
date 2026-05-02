#include "table.h"

#include "chars.h"
#include "parser.h"
#include "scanner.h"
#include "tokens.h"

// =====================================================================
// Tables
// =====================================================================
//
// Tables are exposed with row-level structure: each line of the table
// becomes its own external token, and the JS grammar composes them
// into ``grid_table`` / ``simple_table`` nodes with explicit ``row``
// and border children.
//
// Per-line tokens:
//   * Grid:
//       ``T_GRID_TABLE_SEPARATOR``   '+---+...+'
//       ``T_GRID_TABLE_HEADER_SEP``  '+===+...+'
//       ``T_GRID_TABLE_ROW_LINE``    '|...|...|' (or any non-border
//                                    continuation line whose last
//                                    non-space character is '|' / '+')
//   * Simple:
//       ``T_SIMPLE_TABLE_BORDER``    '=== === ...'
//       ``T_SIMPLE_TABLE_DASHES``    '--- --- ...' colspan separator
//       ``T_SIMPLE_TABLE_ROW_LINE``  any data line
//
// Disambiguation
// --------------
//
// Border characters overlap with sibling constructs:
//
//   * '+' starts a grid border AND a char_bullet ('+ item').
//   * '=' starts a simple-table border AND a section adornment.
//
// Once we advance past the leading character we must commit to a
// token (the external scanner does not roll back lexer state on
// false). Dispatch happens in two places:
//
//   * Top level (overlines / transitions valid): ``parse_overline``
//     runs first and falls back to ``try_grid_table_after_plus`` /
//     ``try_simple_table_after_run`` before ``parse_text``.
//   * Body context (inside lists, directives, block quotes, …):
//     ``scanner.c`` routes the relevant first-character cases to
//     ``parse_table_dispatch``.
//
// State
// -----
//
// The scanner relies on ``valid_symbols`` rather than its own state to
// distinguish "candidate first border" from "subsequent line of an
// already-open table":
//
//   * If ``T_GRID_TABLE_ROW_LINE`` (or its simple-table counterpart) is
//     in ``valid_symbols``, the grammar already entered a table — we
//     classify the current line and emit the matching token.
//   * Otherwise we are at a candidate top border; we validate by
//     looking ahead through the entire table (``mark_end`` is pinned
//     at the end of the first border line, so the lookahead does not
//     leak into the produced token).

/// Per-line accumulators populated by ``scan_remaining_table_line`` and
/// inspected by ``classify_from_state``.
typedef struct {
  // True iff every char on the line was a valid border char or space:
  //   grid:   '+', '-', '=', space
  //   simple: '=', '-', space
  bool only_border_chars;
  // True iff we saw at least one space *between* non-space chars. The
  // simple-table border test uses this to distinguish "=== ===" (table)
  // from "=========" (section adornment).
  bool has_internal_space;
  // True iff we've seen at least one non-space char on the line.
  bool seen_non_space;
  // True iff at least one '-' or '=' appeared.
  bool seen_dash_or_equals;
  // Used to tell '+---+' (regular separator) from '+===+' (header
  // separator) in grid tables, and '=== ===' (border) from '--- ---'
  // (column-spanning underline) in simple tables.
  bool seen_equals;
  bool seen_dashes;
  // Last non-space char on the line.
  int32_t last_non_space;
} TableLineState;

/// Skip up to ``max_indent`` columns of leading inline whitespace.
static void skip_indent_up_to(RSTScanner* scanner, uint32_t max_indent)
{
  uint32_t skipped = 0;
  while (skipped < max_indent && is_inline_space(scanner->lookahead)) {
    scanner->advance(scanner);
    skipped++;
  }
}

/// Read the rest of the current line into the given accumulators. The
/// caller seeds them with whatever has already been consumed of the
/// line. On return the scanner is positioned at the terminating newline
/// (or EOF).
static void scan_remaining_table_line(
    RSTScanner* scanner, bool is_grid, TableLineState* s)
{
  while (!is_newline(scanner->lookahead) && scanner->lookahead != CHAR_EOF) {
    int32_t c = scanner->lookahead;
    if (is_grid) {
      if (c != '+' && c != '-' && c != '=' && !is_inline_space(c)) {
        s->only_border_chars = false;
      }
    } else {
      if (c != '=' && c != '-' && !is_inline_space(c)) {
        s->only_border_chars = false;
      }
    }
    if (is_inline_space(c)) {
      if (s->seen_non_space) {
        s->has_internal_space = true;
      }
    } else {
      s->seen_non_space = true;
      s->last_non_space = c;
      if (c == '-') {
        s->seen_dash_or_equals = true;
        s->seen_dashes = true;
      } else if (c == '=') {
        s->seen_dash_or_equals = true;
        s->seen_equals = true;
      }
    }
    scanner->advance(scanner);
  }
}

/// Classify the *current* line based on populated accumulators.
///
/// Returns:
///   2 — clean border line
///   1 — data/continuation line that still belongs to the table
///   0 — line cannot belong to the table
static int classify_from_state(
    bool is_grid, int32_t first, const TableLineState* s)
{
  if (is_grid) {
    if (first == '+' && s->only_border_chars && s->last_non_space == '+'
        && s->seen_dash_or_equals) {
      return 2;
    }
    if (s->last_non_space == '|' || s->last_non_space == '+') {
      return 1;
    }
    return 0;
  }
  if (s->only_border_chars && s->has_internal_space && s->seen_dash_or_equals
      && (first == '=' || first == '-')) {
    return 2;
  }
  // Simple-table data lines can contain anything but must not be blank
  // (caller guards the blank-line case).
  return 1;
}

/// Walk a fresh line from column 0 of the table and classify it.
static int classify_table_line(
    RSTScanner* scanner, bool is_grid, TableLineState* out)
{
  int32_t first = scanner->lookahead;
  if (is_grid && first != '+' && first != '|') {
    return 0;
  }

  *out = (TableLineState) {
    .only_border_chars = true,
  };
  scan_remaining_table_line(scanner, is_grid, out);
  return classify_from_state(is_grid, first, out);
}

/// Pick the right border-flavour token for a grid border line.
static int grid_border_symbol(const TableLineState* s)
{
  return s->seen_equals ? T_GRID_TABLE_HEADER_SEP : T_GRID_TABLE_SEPARATOR;
}

/// Pick the right border-flavour token for a simple-table border line.
static int simple_border_symbol(const TableLineState* s)
{
  // ``=== === …`` is a top/inner/bottom border; ``--- --- …`` is the
  // column-spanning underline used for header colspans. A line mixing
  // both is treated as the dashes flavour iff it contains any '-' that
  // is not just an equals.
  return (s->seen_dashes && !s->seen_equals)
      ? T_SIMPLE_TABLE_DASHES
      : T_SIMPLE_TABLE_BORDER;
}

/// Multi-line lookahead used at top-border emit time. The caller has
/// already pinned ``mark_end`` at the end of the first border line; we
/// advance through subsequent lines purely to verify that a real table
/// follows, then return success/failure. The lexer is reset to the
/// pinned ``mark_end`` whichever way we return.
///
/// Requires at least one row line and at least one closing border to
/// commit. Stops at a non-table line, blank line, or EOF.
static bool validate_table_after_top_border(
    RSTScanner* scanner, bool is_grid, uint32_t start_col)
{
  int border_count = 1;
  int row_line_count = 0;

  while (true) {
    skip_indent_up_to(scanner, start_col);

    if (scanner->lookahead == CHAR_EOF || is_newline(scanner->lookahead)) {
      break;
    }

    TableLineState s;
    int line_kind = classify_table_line(scanner, is_grid, &s);
    if (line_kind == 0) {
      break;
    }
    if (line_kind == 2) {
      border_count++;
    } else {
      row_line_count++;
    }

    if (is_newline(scanner->lookahead)) {
      scanner->advance(scanner);
    }
  }

  return border_count >= 2 && row_line_count >= 1;
}

/// Inside-table per-line scan for grid tables. Called when the grammar
/// has already entered a ``grid_table`` (``T_GRID_TABLE_ROW_LINE`` is
/// in ``valid_symbols``). Classifies the current line and emits the
/// matching token. Returns false (without consuming any input — we
/// only inspect ``lookahead`` first) if the line is plainly not a
/// grid-table line, so the grammar can conclude the table.
static bool scan_grid_table_inner_line(RSTScanner* scanner)
{
  TSLexer* lexer = scanner->lexer;
  const bool* valid = scanner->valid_symbols;

  int32_t first = scanner->lookahead;
  if (first != '+' && first != '|') {
    return false;
  }

  TableLineState s = {
    .only_border_chars = true,
  };
  scan_remaining_table_line(scanner, /* is_grid */ true, &s);
  int kind = classify_from_state(/* is_grid */ true, first, &s);

  lexer->mark_end(lexer);

  if (kind == 2) {
    int symbol = grid_border_symbol(&s);
    if (valid[symbol]) {
      lexer->result_symbol = symbol;
      return true;
    }
    // Border flavour not expected here (e.g. header_sep after we've
    // already seen one). Fall through to text fallback below.
  } else if (kind == 1 && valid[T_GRID_TABLE_ROW_LINE]) {
    lexer->result_symbol = T_GRID_TABLE_ROW_LINE;
    return true;
  }

  // We've advanced past the line but cannot match any expected table
  // token. Emit T_TEXT for the consumed span as a safety net so the
  // parser can recover.
  if (valid[T_TEXT]) {
    lexer->result_symbol = T_TEXT;
    return true;
  }
  return false;
}

/// Inside-table per-line scan for simple tables. Same contract as
/// ``scan_grid_table_inner_line``. A blank line / EOF terminates the
/// table cleanly.
static bool scan_simple_table_inner_line(RSTScanner* scanner)
{
  TSLexer* lexer = scanner->lexer;
  const bool* valid = scanner->valid_symbols;

  int32_t first = scanner->lookahead;
  if (first == CHAR_EOF || is_newline(first) || is_inline_space(first)) {
    // A blank/leading-space line never starts a simple-table line at
    // column ``start_col``. Let the parser conclude the table.
    return false;
  }

  TableLineState s = {
    .only_border_chars = true,
  };
  scan_remaining_table_line(scanner, /* is_grid */ false, &s);
  int kind = classify_from_state(/* is_grid */ false, first, &s);

  lexer->mark_end(lexer);

  if (kind == 2) {
    int symbol = simple_border_symbol(&s);
    if (valid[symbol]) {
      lexer->result_symbol = symbol;
      return true;
    }
  } else if (kind == 1 && valid[T_SIMPLE_TABLE_ROW_LINE]) {
    lexer->result_symbol = T_SIMPLE_TABLE_ROW_LINE;
    return true;
  }

  if (valid[T_TEXT]) {
    lexer->result_symbol = T_TEXT;
    return true;
  }
  return false;
}

/// Pin ``mark_end`` at the current position (end of the first border
/// line, before its terminating newline) and emit the first-border
/// token. Subsequent lines of the table are picked up by the inner
/// per-line scanners.
static bool emit_grid_first_border(
    RSTScanner* scanner,
    const TableLineState* s,
    uint32_t start_col)
{
  TSLexer* lexer = scanner->lexer;
  lexer->mark_end(lexer);

  // Multi-line lookahead to verify the table is well-formed end-to-
  // end. The lexer is reset to ``mark_end`` whether we return true or
  // false, so it is safe to advance past it here.
  if (is_newline(scanner->lookahead)) {
    scanner->advance(scanner);
  }
  if (!validate_table_after_top_border(scanner, /* is_grid */ true, start_col)) {
    return false;
  }
  lexer->result_symbol = grid_border_symbol(s);
  return true;
}

static bool emit_simple_first_border(
    RSTScanner* scanner,
    const TableLineState* s,
    uint32_t start_col)
{
  TSLexer* lexer = scanner->lexer;
  lexer->mark_end(lexer);

  if (is_newline(scanner->lookahead)) {
    scanner->advance(scanner);
  }
  if (!validate_table_after_top_border(scanner, /* is_grid */ false, start_col)) {
    return false;
  }
  lexer->result_symbol = simple_border_symbol(s);
  return true;
}

/// Top-level integration hook for grid tables. Called from
/// ``parse_overline`` after it has consumed the leading '+' (so
/// ``overline_length`` == 1) and bumped into a non-adornment, non-space
/// character that would otherwise fall through to ``parse_text``. The
/// first '+' is already consumed; we read the rest of the first line
/// and decide.
static bool try_grid_table_after_plus(RSTScanner* scanner, uint32_t start_col)
{
  const bool* valid_symbols = scanner->valid_symbols;

  if (!valid_symbols[T_GRID_TABLE_SEPARATOR]
      && !valid_symbols[T_GRID_TABLE_HEADER_SEP]) {
    return false;
  }

  TableLineState s = {
    .only_border_chars = true,
    .seen_non_space = true, // we already consumed '+'
    .last_non_space = '+',
  };
  scan_remaining_table_line(scanner, /* is_grid */ true, &s);
  if (classify_from_state(/* is_grid */ true, '+', &s) != 2) {
    return false;
  }
  int symbol = grid_border_symbol(&s);
  if (!valid_symbols[symbol]) {
    return false;
  }

  return emit_grid_first_border(scanner, &s, start_col);
}

/// Top-level integration hook for simple tables. Called from
/// ``parse_overline`` after the leading run of '=' and trailing inline
/// whitespace have been consumed.
static bool try_simple_table_after_run(RSTScanner* scanner, uint32_t start_col)
{
  const bool* valid_symbols = scanner->valid_symbols;

  if (!valid_symbols[T_SIMPLE_TABLE_BORDER]) {
    return false;
  }

  TableLineState s = {
    .only_border_chars = true,
    .has_internal_space = true,
    .seen_non_space = true,
    .seen_dash_or_equals = true,
    .seen_equals = true,
    .last_non_space = '=',
  };
  scan_remaining_table_line(scanner, /* is_grid */ false, &s);
  if (classify_from_state(/* is_grid */ false, '=', &s) != 2) {
    return false;
  }

  return emit_simple_first_border(scanner, &s, start_col);
}

/// Body-context dispatcher. Called from ``rst_scanner_scan`` when
/// neither overline nor transition is in play and a table-relevant
/// token is in ``valid_symbols``. Once we begin advancing we cannot
/// safely return false, so this function always commits to a token
/// (table line, char_bullet, or text).
static bool parse_table_dispatch(RSTScanner* scanner)
{
  TSLexer* lexer = scanner->lexer;
  const bool* valid_symbols = scanner->valid_symbols;

  // Inside an open grid table: any first character is fair game so
  // long as the line classifies as one of the table tokens.
  if (valid_symbols[T_GRID_TABLE_ROW_LINE]) {
    return scan_grid_table_inner_line(scanner);
  }
  if (valid_symbols[T_SIMPLE_TABLE_ROW_LINE]) {
    return scan_simple_table_inner_line(scanner);
  }

  // Candidate first border. Only '+' (grid) and '=' (simple) reach
  // this branch via the dispatch in scanner.c; anything else means
  // we shouldn't be here.
  int32_t first = scanner->lookahead;
  bool is_grid = (first == '+');
  if (!is_grid && first != '=') {
    return false;
  }

  uint32_t start_col = lexer->get_column(lexer);

  // Consume the leading run of the same character. For grid tables a
  // single '+' is typical; simple tables have several '='s.
  int run_length = 0;
  while (scanner->lookahead == first) {
    scanner->advance(scanner);
    run_length++;
  }

  // Bullet shortcut: a single '+' followed by inline whitespace is a
  // char_bullet. ``parse_inner_list_element`` expects exactly this
  // state (the bullet char already consumed, lookahead at the trailing
  // whitespace).
  if (is_grid && run_length == 1 && is_inline_space(scanner->lookahead)
      && valid_symbols[T_CHAR_BULLET]) {
    return parse_inner_list_element(scanner, 1, T_CHAR_BULLET);
  }

  TableLineState s = {
    .only_border_chars = true,
    .seen_non_space = (run_length > 0),
    .seen_dash_or_equals = !is_grid,
    .seen_equals = !is_grid,
    .last_non_space = first,
  };
  scan_remaining_table_line(scanner, is_grid, &s);
  int kind = classify_from_state(is_grid, first, &s);

  if (kind == 2) {
    bool ok = is_grid
        ? emit_grid_first_border(scanner, &s, start_col)
        : emit_simple_first_border(scanner, &s, start_col);
    if (ok) {
      return true;
    }
    // Validation failed — not a real table. Fall through to T_TEXT.
  }

  // Not a table. We've already consumed up to the newline, so the
  // best fallback is to emit the line as text.
  if (valid_symbols[T_TEXT]) {
    lexer->mark_end(lexer);
    lexer->result_symbol = T_TEXT;
    return true;
  }
  return false;
}
