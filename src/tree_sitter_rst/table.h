#ifndef TREE_SITTER_RST_TABLE_H_
#define TREE_SITTER_RST_TABLE_H_

#include <stdbool.h>
#include <tree_sitter/parser.h>

#include "scanner.h"
#include "tokens.h"

// Body-context entry point: dispatches lines starting with '+', '|', '='
// or '-' into a per-line table token (T_GRID_TABLE_*, T_SIMPLE_TABLE_*),
// a char_bullet, or text. Called from rst_scanner_scan when neither
// T_OVERLINE nor T_TRANSITION is valid.
static bool parse_table_dispatch(RSTScanner* scanner);

// Top-level entry points: called from parse_overline at the points
// where it would otherwise fall through to parse_text. They take
// responsibility for whatever state parse_overline has already left
// the lexer in (a single leading '+', or a run of '=' plus trailing
// whitespace), validate that a real table follows, and emit only the
// first border line as ``T_GRID_TABLE_SEPARATOR`` /
// ``T_SIMPLE_TABLE_BORDER``. Subsequent lines are scanned via the
// body-context dispatch.
static bool try_grid_table_after_plus(RSTScanner* scanner, uint32_t start_col);
static bool try_simple_table_after_run(RSTScanner* scanner, uint32_t start_col);

#endif /* ifndef TREE_SITTER_RST_TABLE_H_ */
