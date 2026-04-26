===========================
Backslash escape examples
===========================

This page shows how docutils / Sphinx renders each backslash escape
construct so you can compare it with what tree-sitter exposes.

tree-sitter distinguishes two cases:

- ``\X`` (backslash + non-space character) — emits a named
  ``backslash_escape`` node spanning **both** characters so consumers can
  identify and process the escape.
- ``\ `` (backslash + space, the zero-width separator) — emits an
  **anonymous** token that produces **no named node** in the parse tree,
  matching docutils semantics where the construct contributes nothing to
  the rendered output.

.. contents:: Examples
   :local:
   :depth: 1

----

Escaped space: joining inline markup to adjacent text
======================================================

RST requires whitespace around inline markup delimiters.
A backslash followed by a space (written ``\`` + space in source) acts
as a zero-width separator so that markup can directly adjoin surrounding
words.

.. rubric:: Source

.. code-block:: rst

   **bold**\ word

   word\ **bold**

   a\ **b**\ c

.. rubric:: Rendered

**bold**\ word

word\ **bold**

a\ **b**\ c

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (strong))
   (paragraph (strong))
   (paragraph (strong))

Because ``\ `` is the zero-width separator, it emits an anonymous token
with no named node.  Only the ``(strong)`` node is named; the surrounding
text (``word`` / ``a`` / ``c``) is also anonymous.

----

Backslash before a letter: separator after closing inline markup
================================================================

When inline markup closes immediately before a letter (e.g. ``*mid*\dle``),
a backslash before that letter acts as a separator satisfying RST's
end-string recognition rules while producing just the letter in the output.
Without the backslash, ``*mid*dle`` would not form valid emphasis because
the closing ``*`` must be immediately followed by a non-alphanumeric
character.

.. rubric:: Source

.. code-block:: rst

   I'm in the *mid*\dle.

.. rubric:: Rendered

I'm in the *mid*\dle.

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (emphasis) (backslash_escape))

The ``backslash_escape`` node spans ``\d`` (backslash + the letter ``d``,
two characters); the trailing ``le.`` is anonymous text.

----

Escaped asterisk: prevent emphasis
====================================

A backslash before ``*`` or ``**`` prevents the parser from treating
it as an inline-markup delimiter.

.. rubric:: Source

.. code-block:: rst

   \*not emphasis\*

   I'm in the mid\*dle.

   I'm **valid\\**

.. rubric:: Rendered

\*not emphasis\*

I'm in the mid\*dle.

I'm **valid\\**

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (backslash_escape))
   (paragraph (backslash_escape))
   (paragraph (strong) (backslash_escape))

----

Escaped backtick: prevent interpreted text
==========================================

.. rubric:: Source

.. code-block:: rst

   \`not interpreted text

   I'm *mid*\ **dle**

.. rubric:: Rendered

\`not interpreted text

I'm *mid*\ **dle**

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (backslash_escape))
   (paragraph (emphasis) (strong))

The first example's ``\``` produces a named ``backslash_escape``.
In the second example, ``\ `` (backslash + space between ``*mid*`` and
``**dle**``) is the zero-width separator and produces **no named node**.

----

Double backslash: literal backslash
====================================

``\\`` produces a single literal backslash character.

.. rubric:: Source

.. code-block:: rst

   \\double backslash

   http://www.example.org\\using\\DOS\\paths\\

.. rubric:: Rendered

\\double backslash

http://www.example.org\\using\\DOS\\paths\\

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (backslash_escape))
   (paragraph (standalone_hyperlink) (backslash_escape) (backslash_escape)
              (backslash_escape) (backslash_escape))

----

Escaped colon in a field name
==============================

A field name normally ends at the first unescaped ``:`` after the opening
``:``; writing ``\:`` inside the field name lets you include a literal
colon without closing the field marker — which is how you embed a role
annotation (single-backtick markup) inside a field name.

.. rubric:: Source

.. code-block:: rst

   :field\:`name`: interpreted text (standard role) requires
                   escaping the leading colon in a field name

.. rubric:: Rendered

:field\:`name`: interpreted text (standard role) requires
                escaping the leading colon in a field name

.. rubric:: tree-sitter parse

.. code-block:: text

   (field_list
     (field
       (field_name
         (backslash_escape)
         (interpreted_text))
       (field_body (paragraph))))

The ``backslash_escape`` node spans ``\:``; ``(interpreted_text)`` covers
the `` `name` `` role that follows it.

----

Backslash inside a failed substitution reference
=================================================

These two lines form a **single paragraph** (no blank line between them).
The first line contains a valid substitution reference; the second has
``\|`` inside a would-be reference, which prevents the closing ``|`` from
matching — so the reference fails and ``\|`` surfaces as a named
``backslash_escape`` node instead of being silently absorbed.

.. rubric:: Source

.. code-block:: rst

   |I'm| a text
   |I'm not\| an reference.

   .. |I'm| replace:: *I'm*

.. rubric:: Rendered

Docutils substitutes ``|I'm|`` with the replacement text and emits a
warning for the unmatched ``|I'm not\|`` delimiter.  The text output looks
like::

   *I'm* a text |I'm not| an reference.

(``|I'm not\|`` becomes plain ``|I'm not|`` after the backslash is
consumed; the substitution reference is not resolved.)

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph
     (substitution_reference)
     (backslash_escape))

``(substitution_reference)`` covers ``|I'm|`` on the first line;
``(backslash_escape)`` covers ``\|`` on the second line.  The surrounding
text (`` a text``, `` an reference.``) is anonymous and not shown as
named nodes.

----

Backslash inside other failed markup
======================================

The same pattern applies to footnote references, inline targets, citation
references, and hyperlink references.

.. rubric:: Source

.. code-block:: rst

   [1]_ a text
   [1\] not a footnote.

   _`I'm` a text
   _`I'm not\` a target.

   [1one]_ a text
   [not\] a citation

   `I'm`__ a text
   `I'm not\`_ a reference.

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (footnote_reference) (backslash_escape))

   (paragraph (inline_target) (backslash_escape))

   (paragraph (citation_reference) (backslash_escape))

   (paragraph (reference) (backslash_escape))

In each case the first line provides a successful match; the backslash on
the second line prevents the closing delimiter from matching, and ``\X``
becomes a named ``backslash_escape`` node.
