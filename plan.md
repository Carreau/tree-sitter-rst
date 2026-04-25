# Plan: working through upstream issues and TODOs

This document inventories what's still open at `stsewd/tree-sitter-rst`,
the README `## TODO` list, and in-tree `TODO` comments, then groups
the remaining items into workstreams. Closed work has been pruned;
look at the merge log on `master` for history.

## Inventory

### Upstream issues still open

| # | Title | Type | Status |
|---|---|---|---|
| 16 | Bullet lists with colon in the line | bug | Top-level tree is correct; small inline `ERROR` for the colon is tracked in `test/corpus/partial_fixes.txt`. WS-1 follow-up. |
| 20 | bullet list/enumerated with `:` in paragraph | bug | Same as #16; small inline `ERROR` remains. |
| 27 | Literal block parsed as section title if `::` starts a new line | bug | Section is no longer mis-recognised. The indented block following the standalone `::` still ends up inside the surrounding paragraph rather than as a separate `literal_block`. WS-2 follow-up. |
| 43 | Report heading depth | enhancement | Sections are flat nodes by design; #43 wants hierarchy. See WS-8. |
| 45 | Issue when Footnote with `:` | bug | Same shape as #16; small inline `ERROR` remains. |
| 46 | Directive vs comments when no space after `::` | decision | `.. note::body` parses as a comment; docutils treats it as a directive. Behaviour change pending the maintainer's call. See WS-2. |
| 47 | Detecting missing blank lines before section headers | question | The parser silently absorbs the malformed input into a paragraph. See WS-8. |
| 51 | RST grammar not injected into sub-directives | upstream | Mostly an nvim-treesitter / consumer-side concern; this repo can help by shipping example `injections.scm`. See WS-10. |
| 52 | Tree-sitter grammar turns nested directives into `block_quote` | bug | Outer directive's `content` is `_indented_text_block` (plain text), so nested directives don't parse. See WS-6. |

### README `## TODO`

Still listed on master; trimming as items land:

- Refactor parse citation and footer reference.
- Nested line blocks.
- Option lists.
- Add some nodes to inline?
- Check if there is a way to re-implement some nodes to JS instead of C?
- A definition list with classifiers can't be separated by a blank line.
- tests, tests, and more tests!

### README "Design decisions" still open

- **Adornment length validation.** Docutils warns but parses; the
  README already notes this. Recommend: emit a `MISSING` child node
  rather than failing the parse.

### In-tree TODOs

- `src/tree_sitter_rst/scanner.c` — the fallback text-parse path used
  in tree-sitter's correction mode should move to JS so the C scanner
  stops doing work it doesn't want to do. See WS-12.

## Architecture, quick map

Where the open work lands:

- `grammar.js` — high-level rules (sections, lists, directives, inline
  markup). Most structural decisions live here.
- `src/tree_sitter_rst/scanner.c` — dispatcher calling `parse_*`
  routines based on `valid_symbols`.
- `src/tree_sitter_rst/parser.c` — the actual scanner routines.
  Relevant for the open items: `parse_overline` / `parse_underline`
  (#27 follow-up, #47), `parse_explict_markup_start` (#46, #52).
- `src/tree_sitter_rst/{chars.{c,h},punctuation_chars.h,tokens.h}` —
  character classes and token ids.
- `queries/` — example queries; the missing `injections.scm` would
  partially address #51.
- `test/corpus/*.txt` — regression tests (every fix needs one).
- `test/corpus/partial_fixes.txt` — tests that capture the current
  shape for partially-fixed issues. Updating these is expected as
  follow-ups land.

## Workstreams

### WS-2 — `::` disambiguation, follow-ups

**Closes:** #27 (full), #46.

**#27 follow-up.** The standalone `::` no longer poisons section
parsing, but the indented block that follows ends up inside the
surrounding paragraph rather than as a separate `literal_block`. To
recover it, `T_LITERAL_INDENTED_BLOCK_MARK` would need to be in
`valid_symbols` at the underline position. tree-sitter's GLR currently
commits to the title path before the lexer is asked. Fixing this
needs a grammar conflicts declaration that tree-sitter *accepts* (the
obvious one between `_underline_section` and `paragraph` is reported
as "unnecessary").

**#46.** `.. note::body` without a trailing space still parses as a
comment. Aligning with docutils means accepting the no-space form as
a directive. Recommended: change the `'::'` literal in the directive
rule to allow a directly-following inline character. Needs the
maintainer's call before merging — it's a behaviour change visible to
every consumer.

**Risk.** Low for #27 follow-up (additive). Behaviour change for #46.

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

A first pass at this approach hits a `_list` vs `_directive_body`
conflict that tree-sitter doesn't auto-resolve; resolution likely
needs an explicit `conflicts` entry plus corpus updates.

**Risk.** High. Single biggest parse-tree change in the list. Ship
behind its own PR so regressions are easy to bisect.

### WS-8 — Heading hierarchy and missing-blank-line detection

**Closes:** #43, #47.

**Problem.** Sections are flat nodes containing only the title, by
design (see README). There is no way to know that an `=` title is a
parent of a following `-` title (#43). And there is no dedicated
error node for "section header without a preceding blank line" (#47).

**Approach.**

1. **#43.** Attribute the adornment character on the `section` node —
   either as a `field('adornment', …)` over the existing anonymous
   alias, or as a named `adornment` child. The anonymous-field route
   keeps existing trees identical but is currently blocked by
   tree-sitter's field/alias rules; the named-child route works but
   adds a node to every section, which is a breaking change for
   consumers. Needs a maintainer call on which is acceptable.
2. **#47.** Introduce a new external token `_section_without_blank`
   that fires only when a title underline is seen while the previous
   line is non-empty *and* is not itself a title. Emit it as an error
   node so queries can highlight the problem.

**Risk.** Medium. #43 needs a tree-shape decision; #47 is purely
additive.

### WS-10 — Editor/injection polish

**Closes:** #51 (to the extent this repo can).

**Problem.** Mostly an nvim-treesitter concern, but we can help by
ensuring directive content is a `body` (see WS-6) and by shipping
example `injections.scm` queries so downstream editors don't each
invent their own.

**Approach.** Add `queries/injections.scm` with canonical injections
for `code`, `code-block`, `sourcecode`, `math`, and recursive RST
injection into the `content` of non-code directives. Document in
README. Master already has `queries/diagnostics.scm`; a sibling
injections file is a natural addition.

**Risk.** Low — documentation/queries.

### WS-12 — In-tree TODO: move correction-mode text parsing to JS

**Closes:** the `scanner.c` TODO comment.

**Approach.** Drop the `if (valid_symbols[T_INVALID_TOKEN]) { … return
parse_text(...) }` branch and express the fallback in `grammar.js`
via an explicit `$._text_fallback` rule used inside the paragraph
alternative. Needs conflict analysis in `grammar.js`. Low priority.

### WS-13 — Open design decisions

Not implementable without a decision from the repo owner:

- **Adornment length validation.** Docutils warns but parses; the
  README already notes this. Recommend: emit a `MISSING` child node
  rather than failing the parse.
- **#46 direction.** See WS-2.
- **#43 tree shape.** See WS-8.

## Out of scope for this plan

- Adding a Markdown-style nested `section` tree (would break every
  downstream consumer; see WS-8 for the lighter alternative).
- Rewriting the external scanner in Rust.
- Rewriting the grammar for a strict subset such as numpydoc.
