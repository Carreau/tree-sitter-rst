; Syntax highlighting queries for tree-sitter-rst.
;
; Capture names follow the modern tree-sitter / nvim-treesitter
; highlighting convention (@markup.*, @punctuation.*, @string.escape, …).
; Node names come from src/node-types.json; quoted strings match the
; anonymous tokens aliased in grammar.js ('adornment', 'bullet', '..',
; '::', ':', '|', '--', '__', '<', '>', '`', '_').

; ========
; Sections
; ========

(section (title) @markup.heading)
("adornment") @punctuation.special

(transition) @punctuation.special

; =====
; Lists
; =====

; Bullet and enumerated list markers ('-', '*', '1.', '#.', …).
("bullet") @markup.list

; Definition lists.
(list_item (term) @markup.strong)
(list_item (classifier) @markup.italic)

; Field lists (:Date: …, directive :options:, …).
(field (field_name) @variable.member)
(field (":") @punctuation.delimiter)

; ============
; Body blocks
; ============

(literal_block) @markup.raw.block
("::") @punctuation.special

; Line blocks.
(line ("|") @punctuation.special)

; Block quotes.
(block_quote) @markup.quote
(attribution) @markup.italic
(attribution ("--") @punctuation.delimiter)

(doctest_block) @markup.raw.block

; Tables are exposed as flat blocks.
(grid_table) @markup.raw.block
(simple_table) @markup.raw.block

; ==============
; Explicit markup
; ==============

; Explicit markup start ('..' of directives, targets, footnotes, …).
("..") @punctuation.special

; Directives.
(directive
  name: (type) @keyword.directive)
(directive
  body: (body (arguments) @string))
(directive
  body: (body (content) @markup.raw.block))

; Comments.
(comment) @comment

; Footnotes and citations.
(footnote
  name: (label) @label)
(citation
  name: (label) @label)

; Hyperlink targets.
(target
  name: (name) @markup.link.label)
(target
  link: (link) @markup.link.url)
("__") @punctuation.special

; Substitution definitions.
(substitution_definition
  name: (substitution) @string.special)

; =============
; Inline markup
; =============

(emphasis) @markup.italic
(strong) @markup.bold
(literal) @markup.raw

; Interpreted text and roles (:role:`text`).
(interpreted_text) @markup.raw
(interpreted_text (role) @function.macro)

(substitution_reference) @string.special
(inline_target) @markup.link.label

(footnote_reference) @markup.link.label
(citation_reference) @markup.link.label

; References (reference_, `phrase reference`_, `text <uri>`_).
(reference) @markup.link
(reference (name) @markup.link.label)
(reference (uri) @markup.link.url)
(reference ("`") @punctuation.delimiter)
(reference ("_") @punctuation.delimiter)
(reference ("<") @punctuation.bracket)
(reference (">") @punctuation.bracket)

(standalone_hyperlink) @markup.link.url

(escape_sequence) @string.escape
