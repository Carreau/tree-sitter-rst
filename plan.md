# Plan: working through upstream issues and TODOs

This document inventories the open upstream issues at
`stsewd/tree-sitter-rst`, the README `## TODO` list, and in-tree `TODO`
comments, then groups them into workstreams that can be shipped
incrementally. For each workstream it notes the probable code locations,
the approach, and which issues it would close.

## Session scoreboard

Of the 16 open upstream issues, this branch (`claude/plan-upstream-issues-7elcx`)
now has:

- **Fully fixed**, with regression tests: #14, #28, #44, #48, #57, #64.
- **Structurally fixed** (correct top-level tree, small contained
  `ERROR` node in place of the contested token): #16, #20, #27, #45.
- **Open**, summarised below: #29, #43, #46, #47, #51, #52.

The fixes were shipped across these commits on this branch:

- `b0794ed` — WS-1: classifier-separator indent guard.
- `d65ba06` — WS-11: Python lint CI; WS-5/#48 corpus test.
- `4cd54bf` — WS-2: refuse `::` as a section underline.
- `d493a78` — WS-7: incremental-parse regression tests for #44/#64.

## Inventory

### Upstream open issues (stsewd/tree-sitter-rst)

The "Status in this fork" column tracks work that has already landed on
`carreau/tree-sitter-rst`. The upstream issues remain open at
`stsewd/tree-sitter-rst` until a PR is sent.

| # | Title | Type | Status in this fork |
|---|---|---|---|
| 14 | Grammar sometimes treats numbered lists as definition lists | bug | **Fixed** by `a41a933` + `c80ef74`; corpus test in `b0794ed` |
| 16 | Bullet lists with colon in the line | bug | Structurally **fixed** by `b0794ed`; small inline `ERROR` for the colon remains |
| 20 | bullet list/enumerated with `:` in paragraph | bug | Structurally **fixed** by `b0794ed`; small inline `ERROR` remains |
| 27 | Literal block parsed as section title if `::` starts a new line | bug | Structurally **fixed** in this batch; the indented block is paragraph content rather than `literal_block` (parser-state limitation) |
| 28 | interpreted text should only allow 1 backtick? | bug | **Fixed** by `34d6d8b` (with corpus test) |
| 29 | Allow blank line between list items? | enhancement | Open |
| 43 | Report heading depth | enhancement | Open |
| 44 | AST depends on order of edit actions | bug | **Fixed** by `1760b36`; verified by new incremental-parse test |
| 45 | Issue when Footnote with `:` | bug | Structurally **fixed** by `b0794ed`; small inline `ERROR` remains |
| 46 | Directive vs comments when no space after `::` | decision | Open (needs maintainer call) |
| 47 | Detecting missing blank lines before section headers | question | Open |
| 48 | Parsing a document with an incomplete definition list | bug | **Fixed** by `b0794ed`; matches the reporter's requested behaviour |
| 51 | RST grammar not injected into sub-directives | upstream | Open (mostly nvim-treesitter side) |
| 52 | Tree-sitter grammar turns nested directives into `block_quote` | bug | Open |
| 57 | Add some CI checks for the Python script | chore | **Done** in this batch (ruff + black job) |
| 64 | code-block directive seems to break parsing in Neovim | bug | **Fixed** by `1760b36`; verified by new incremental-parse test |

### Closed-but-related fork commits

- `34d6d8b` — `:role:`` ``…`` ``` no longer parses as interpreted text
  (closes #28).
- `c80ef74` — bullet list items with `* a : b` no longer trigger field
  marks. `e516a89` is the regression follow-up that keeps
  `:stub-columns: 1` working in directive option lists.
- `1760b36` — three scanner bugs: T_INDENT vs T_BLANKLINE ordering,
  single-dash bullet vs attribution mark, and the swapped
  `memcpy` in `rst_scanner_serialize`/`deserialize`. The serialize
  fix is the real meat for incremental-parse correctness (#44, #64).
- `58ddd50` — regression tests for the bullet-list + block-quote
  scenarios above.
- `b0794ed` — zero-width `_classifier_indent_check` external token
  gates the JS-level `<ws>:<ws>` classifier separator on a deeper
  indented continuation. Restores correct top-level structure for
  #16, #20, #45, #48 inputs. Adds 5 corpus tests (#14, #16, #20, #45,
  #48).
- `a41a933` (upstream, already merged) — recovery-mode awareness and
  numeric-list misparse fix; closes upstream #41 and most of #14.

### README `## TODO`

- Allow lists with blank lines between items — same as #29.
- Refactor parse citation and footer reference.
- Nested line blocks.
- Option lists.
- Add some nodes to inline?
- Move some nodes from C to JS?
- Definition list with classifiers can't be separated by a blank line.
- More tests.

### README "Design decisions" (still open)

- Implement tables?
- Validate length of adornments in sections?

### In-tree TODOs

- `src/tree_sitter_rst/scanner.c:120` — the fallback text-parse path used in
  tree-sitter's correction mode should move to JS so the C scanner stops
  doing work it doesn't want to do.

## Current architecture, quick map

Knowing where the fixes land:

- `grammar.js` — high-level rules (sections, lists, directives, inline
  markup). Most structural decisions live here.
- `src/tree_sitter_rst/scanner.c` — dispatcher calling `parse_*` routines
  based on `valid_symbols`.
- `src/tree_sitter_rst/parser.c` — the actual scanner routines:
  `parse_overline`/`parse_underline` (#27, #47), `parse_char_bullet` /
  `parse_numeric_bullet` / `parse_field_mark` (#14, #16, #20, #45, #48),
  `parse_label` (#45), `parse_explict_markup_start` (#46, #52, #64),
  `parse_inline_markup` / backtick handling (#28).
- `src/tree_sitter_rst/chars.{c,h}`, `punctuation_chars.h`, `tokens.h` —
  character classes and token ids.
- `utils/gen_punctuation_chars.py` — Python script flagged by #57.
- `test/corpus/*.txt` — regression tests (every fix needs one).

## Workstreams

Ordered by a blend of user impact, risk, and dependency. "Closes" is the
list of upstream issues the workstream would resolve if shipped as
described.

### WS-1 — Colon disambiguation in lists, footnotes, and paragraphs ✅ shipped

**Closes:** #14 (test added), #48. Structurally fixes #16, #20, #45.

**Status.** `b0794ed` added an external zero-width
`_classifier_indent_check` token that the scanner emits right after the
JS-level `<ws>:<ws>` classifier separator only when the next non-blank
line is indented strictly deeper than the current scope. Without that
guard the GLR parser committed to the definition-list term branch in
any context (paragraphs, bullet items, footnote bodies). The five
corpus tests in `lists.txt` and `body_elements.txt` lock the new
behaviour in.

**Remaining work.** The contested ` : ` still produces a small inline
`ERROR` node for #16, #20, #45. The JS-level classifier separator is
matched and then rejected by the guard; the consumed characters become
the ERROR. Eliminating this requires moving the separator itself to an
external scanner token, which I tried in earlier iterations and which
perturbed the parser tables enough to break the unrelated `("::")`
paragraph test. A follow-up that keeps the table stable would close
the inline-ERROR window — see WS-2 area for the related `::` work.

**Risk.** Done — landed.

### WS-2 — `::` disambiguation: literal block vs adornment vs directive 🟡 partial

**Closes:** #27 structurally; #46 still open (decision needed).

**Status.** `parse_underline` now refuses to emit `T_UNDERLINE` for a
two-character `::` adornment. The structural misreading from #27 is
gone -- the line above is no longer turned into a section title.

**Remaining work.**

1. The indented block following the standalone `::` ends up inside the
   surrounding paragraph rather than as a separate `literal_block`
   node. To recover it, `T_LITERAL_INDENTED_BLOCK_MARK` would need to
   be in `valid_symbols` at the underline position. tree-sitter's GLR
   currently commits to the title path before the lexer is asked.
   Fixing this needs a grammar conflicts declaration that tree-sitter
   *accepts* (the obvious one between `_underline_section` and
   `paragraph` is reported as "unnecessary").
2. **#46 directive vs comment.** `.. note::body` without a trailing
   space still parses as a comment. Aligning with docutils means
   accepting the no-space form as a directive. Recommended: change the
   `'::'` literal in the directive rule to allow a directly-following
   inline character. Needs the maintainer's call before merging.

**Risk.** Done for #27. #46 remains a behaviour change pending the
maintainer's decision.

### WS-3 — Interpreted text backtick count ✅ done

**Closes:** #28.

**Status.** Shipped in `34d6d8b`. The role-name-prefix path now peeks
one character past the opening backtick and falls through to plain
text when it sees a second backtick, so `:attr:`` ``numpy.ufunc.identity`` ``
parses as `(paragraph (literal))`. A regression test sits at
`test/corpus/inline_markup.txt:217` ("Interpreted text with roles -
double backtick (invalid)").

**Remaining work.** None for the reported case. Optional polish: the
plain `` ``one`` `` form is already covered, but a corpus case for
three-or-more backticks would lock the boundary down further.

### WS-4 — Lists with blank lines, and list/paragraph boundary polish

**Closes:** #29; clears the matching README TODO; addresses the README
"definition list with classifiers can't be separated by a blank line"
note.

**Problem.** A bullet or enumerated list currently closes on any blank
line, so the idiomatic "spaced list" renders as two separate lists.

**Approach.**

1. Track list context on the indent stack (`scanner->indent_stack`)
   with a marker that says "we are inside a list of kind K at column
   C". In `parse_char_bullet` / `parse_numeric_bullet`, a subsequent
   bullet of the same kind at the same column after a single blank
   line should emit another `list_item` under the current list rather
   than terminating the list.
2. Mirror the same logic for `field_list` and for the definition-list
   classifier-continuation case.
3. Corpus: add spaced bullet/enum/field examples.

**Risk.** Medium. Touches the indent stack, so incremental-parse
correctness matters (cross-reference WS-7).

### WS-5 — Incomplete definition list recovery

**Closes:** #48. Unlocks better partial parses for numpydoc.

**Problem.** A term+classifier without a definition body currently
swallows all trailing content into `(ERROR (classifier))`.

**Approach.** Make the definition body required at the grammar level
(`_definition_list_item` already requires `$.body` after
`$._newline_indent`) but give the scanner a cheap lookahead: if the
line after a candidate term has no deeper indent, reject the
definition-list path entirely so the fallback paragraph path runs. In
`parse_field_mark`/classifier emission, refuse to emit the classifier
token when the next non-space character is `\n` (end of file or blank
line). Corpus: add truncated numpydoc examples in
`test/corpus/lists.txt`.

**Risk.** Low–medium. Pure pruning of false positives.

### WS-6 — Nested directives stay directives

**Closes:** #52. Makes #51 tractable (see WS-10).

**Problem.** A directive inside a directive's content block is parsed
as `block_quote` because the outer directive's `content` is
`_indented_text_block` (plain text), not a body.

**Approach.** In `grammar.js`, change `_directive_body` so its content
branch is `alias($.body, $.content)` instead of
`alias($._indented_text_block, $.content)`. That lets the outer
directive contain nested directives, lists, etc. Audit existing
corpus; several fixtures that deliberately expect `literal_block`
inside directives will need updating. Consider adding a `raw_content`
branch for directives that really want opaque content (`code`,
`raw`, `math`) — these should keep today's behaviour so syntax
highlighting via injection still works.

**Risk.** High. The single biggest parse-tree change in the list.
Ship behind its own PR so regressions are easy to bisect.

### WS-7 — Incremental-parse correctness (state in the external scanner) ✅ shipped

**Closes:** #44, #64.

**Status.** `1760b36` fixed three concrete bugs in the external
scanner -- `T_INDENT` vs `T_BLANKLINE` ordering, the single-dash
attribution-vs-bullet conflict, and the swapped `memcpy` in
`rst_scanner_serialize`/`deserialize` (the real meat). The new
`bindings/python/tests/test_incremental.py` replays the edit scripts
from #44 and #64 and asserts that the incremental reparse matches a
full reparse. Both pass, plus a stress test that appends multiple
lines inside a `code-block::` directive.

**Remaining work.** None for the reported cases. Future regressions
can be added to the same test file -- the harness is generic.

**Risk.** Done.

### WS-8 — Heading hierarchy and missing-blank-line detection

**Closes:** #43, #47.

**Problem.** Sections are flat nodes containing only the title, by
design (see README). There is no way to know that an `=` title is a
parent of a following `-` title. And there is no dedicated error node
for "section header without a preceding blank line".

**Approach.**

1. Attribute the adornment character on the `section` node — emit an
   `adornment_char` field from `parse_overline` / `parse_underline`.
   Tools can derive hierarchy from the adornment sequence. This is
   cheaper than changing the tree to be nested.
2. Optionally: add a top-level post-processing rule that nests
   sections. Not recommended — breaks every existing consumer. Document
   the adornment-char field in README as the supported approach.
3. For #47, introduce a new external token `_section_without_blank`
   that fires only when a title underline is seen while the previous
   line is non-empty *and* is not itself a title. Emit it as an error
   node so queries can highlight the problem.

**Risk.** Medium. Pure additive — no existing field names change.

### WS-9 — README TODO list cleanup: line blocks, option lists, etc.

**Closes:** the README `## TODO` items not covered by issues above.

1. **Nested line blocks.** Change `line_block` to allow indented inner
   lines: `line_block: $ => repeat1(choice($.line, $._indented_line_block))`.
   Straightforward.
2. **Option lists.** Add a new external token `_option_mark` matching
   `-o`, `--long`, `/V`, with the two-space gap before the description.
   New `option_list` rule modelled on `field_list`.
3. **Add some nodes to inline.** Audit `_inline_markup` — candidates
   are `target` shorthand `` `anon`__ `` and `standalone_hyperlink`
   variants. Treat as a polish pass after WS-3.
4. **Refactor citation and footnote references.** Merge
   `footnote_reference` and `citation_reference` scanner paths into a
   single `parse_bracket_reference` that dispatches on label shape.

**Risk.** Low per item; ship each as its own PR.

### WS-10 — Editor/injection polish

**Closes:** #51 (to the extent this repo can).

**Problem.** Mostly an nvim-treesitter concern, but we can help by
ensuring directive content is a `body` (see WS-6) and by shipping
example `injections.scm` queries so downstream editors don't each
invent their own.

**Approach.** Add `queries/injections.scm` with canonical injections
for `code`, `code-block`, `sourcecode`, `math`, and recursive RST
injection into the `content` of non-code directives. Document in
README. This is a no-risk documentation/queries change.

### WS-11 — Python util CI checks ✅ shipped

**Closes:** #57.

**Status.** `.github/workflows/ci.yml` now has a `python` job running
`ruff check utils/` and `black --check utils/` on Python 3.10. Tool
config sits in `pyproject.toml` under `[tool.ruff]` / `[tool.black]`;
`utils/gen_punctuation_chars.py` was reformatted (import sort, line
condensation, no behaviour change).

### WS-12 — In-tree TODO: move correction-mode text parsing to JS

**Closes:** `scanner.c:120` TODO.

**Approach.** Drop the `if (valid_symbols[T_INVALID_TOKEN]) { … return
parse_text(...) }` branch and express the fallback in `grammar.js` via
an explicit `$._text_fallback` rule used inside the paragraph
alternative. Needs conflict analysis in `grammar.js`. Low priority.

### WS-13 — Open design decisions (need maintainer call)

Not implementable without a decision from the repo owner:

- **Tables.** Grid and simple tables. Large addition; a separate
  external scanner section. Flag as a distinct plan once signed off.
- **Adornment length validation.** Docutils warns but parses; the
  README already notes this. Recommend: emit a `MISSING` child node
  rather than failing the parse.
- **#46 direction.** See WS-2.

## Recommended ordering

WS-3 is fully shipped; WS-1 and WS-7 are partially shipped — start by
verifying and locking those down with corpus tests, then move on:

1. **Verify already-fixed work:** add corpus regression tests for #16,
   #20, #45 (WS-1) and incremental-parse repros for #44, #64 (WS-7).
   Send the closing PRs upstream.
2. **WS-11** (CI for the Python util) — 10 minutes, independent.
3. **WS-1 cleanup** (corpus tests + #45 verify) — finish the colon
   work that already shipped.
4. **WS-7 cleanup** (incremental-parse harness, finish #44/#64) —
   prerequisite for trusting WS-6.
5. **WS-5** (incomplete definition list).
6. **WS-2** (`::` disambiguation) — unblocks the #46 decision.
7. **WS-6** (nested directives as body) — ship separately, after
   WS-7 cleanup.
8. **WS-4** (blank-line-separated lists) — touches indent stack.
9. **WS-8** (heading depth + missing-blank detection).
10. **WS-10** (injection queries).
11. **WS-9** (line blocks, option lists, inline nodes, reference refactor).
12. **WS-12** (scanner TODO cleanup).
13. **WS-13** (tables, adornment validation) — only after maintainer
    decisions.

## Mapping back to issues

| Issue | Workstream | Status |
|---|---|---|
| #14 | WS-1 | **fixed** by `a41a933`+`c80ef74`; test in `b0794ed` |
| #16 | WS-1 | **structurally fixed** by `b0794ed`; small inline ERROR remains |
| #20 | WS-1 | **structurally fixed** by `b0794ed`; small inline ERROR remains |
| #27 | WS-2 | **structurally fixed** in this batch; literal block content remains paragraph content |
| #28 | WS-3 | **fixed** by `34d6d8b` |
| #29 | WS-4 | open |
| #43 | WS-8 | open |
| #44 | WS-7 | **fixed** by `1760b36`; verified by incremental test |
| #45 | WS-1 | **structurally fixed** by `b0794ed`; small inline ERROR remains |
| #46 | WS-2 | open (needs decision) |
| #47 | WS-8 | open |
| #48 | WS-5/WS-1 | **fixed** by `b0794ed` |
| #51 | WS-10 (+WS-6 dependency; partially upstream) | open |
| #52 | WS-6 | open |
| #57 | WS-11 | **fixed** in this batch |
| #64 | WS-7 | **fixed** by `1760b36`; verified by incremental test |

## Out of scope for this plan

- Adding a Markdown-style nested `section` tree (would break every
  downstream consumer; see WS-8 for the lighter alternative).
- Rewriting the external scanner in Rust.
- Rewriting the grammar for a strict subset such as numpydoc (#48 can
  stand on its own).
