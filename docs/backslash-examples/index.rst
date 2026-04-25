===========================
Backslash escape examples
===========================

This page shows how docutils / Sphinx renders each backslash escape
construct so you can compare it with what tree-sitter now exposes as a
named ``backslash_escape`` node.

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

.. rubric:: tree-sitter parse (with ``backslash_escape`` node)

.. code-block:: text

   (paragraph (strong) (backslash_escape))
   (paragraph (backslash_escape) (strong))
   (paragraph (backslash_escape) (strong) (backslash_escape))

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
   (paragraph (emphasis) (backslash_escape) (strong))

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

Escaped markup characters inside failed markup attempts
=========================================================

When RST markup fails to close properly the ``\X`` sequence is now
surfaced as a named ``backslash_escape`` node rather than being
silently absorbed into anonymous text.

The source below uses ``[1\]``, ``|I'm not\|``, and
``\`I'm not\`_`` — constructs where the backslash prevents the closing
delimiter from matching.

.. rubric:: Source

.. code-block:: rst

   [1]_ a text
   [1\] not a footnote.

   |I'm| a text
   |I'm not\| a reference.

   `I'm`__ a text
   `I'm not\`_ a reference.

.. rubric:: Rendered

Docutils treats the failed markup as plain text (it emits warnings for
the unmatched delimiters); only the successful markup is resolved:

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Source fragment
     - Docutils output
   * - ``[1]_ a text``
     - footnote reference resolved
   * - ``[1\] not a footnote.``
     - plain text: ``[1] not a footnote.``
   * - ``|I'm not\| a reference.``
     - plain text: ``|I'm not| a reference.``
   * - :literal:`\`I'm not\`_ a reference.`
     - plain text with unresolved ``\``` characters

.. rubric:: tree-sitter parse

.. code-block:: text

   (paragraph (footnote_reference) (backslash_escape))
   (paragraph (substitution_reference) (backslash_escape))
   (paragraph (reference) (backslash_escape))
