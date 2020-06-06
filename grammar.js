const WHITE_SPACE = choice(' ', '\t', '\v', '\f')

const OPTION = /[a-zA-Z0-9][a-zA-Z0-9_-]*/
const OPTION_STRING = choice(
  seq('-', OPTION),
  seq('+', OPTION),
  seq('--', OPTION),
  seq('/', OPTION),
)
const OPTION_GROUP = seq(
  OPTION_STRING,
  optional(
    seq(choice(' ', '='), OPTION),
  ),
)


module.exports = grammar({
  name: 'rst',

  extras: $ => [$._eol],

  conflicts: $ => [],

  externals: $ => [
    $._eol,
    $._blank_line,
  ],

  supertypes: $ => [],

  rules: {
    document: $ => repeat(
      choice(
        $._body_elements,
        $._blank_line,
      )
    ),

    // =============
    // Body elements
    // =============

    _body_elements: $ => seq(
      choice(
        $.paragraph,
        $._lists,
        $.line_block,
        $._markup_blocks,
      ),
      $._blank_line,
    ),


    // Paragraph
    // =========

    paragraph: $ => seq(
      repeat(seq($._line, $._eol)),
      $._line,
    ),


    // Lists
    // =====

    _lists: $ => choice(
      $.bullet_list,
      $.enumerated_list,
      $.field_list,
      $.option_list,
    ),

    list_item: $ => choice(
      $._bullet_list_item,
      $._enumerated_list_item,
    ),

    // Bullet lists
    // ------------

    bullet_list: $ => seq(
      repeat(seq(alias($._bullet_list_item, $.list_item), $._eol)),
      alias($._bullet_list_item, $.list_item),
    ),

    _bullet_list_item: $ => seq($._char_bullet, $._line),
    _char_bullet: $ => token(seq(
      choice('*', '+', '-', '•', '‣', '⁃'),
      WHITE_SPACE,
    )),

    // Enumerated lists
    // ----------------

    enumerated_list: $ => seq(
      repeat(seq(alias($._enumerated_list_item, $.list_item), $._eol)),
      alias($._enumerated_list_item, $.list_item),
    ),

    _enumerated_list_item: $ => seq($._numeric_bullet, $._line),
    _numeric_bullet: $ => token(seq(
      choice(/[0-9]+\./, /[a-z]\./, /[A-Z]\./, /[IVXLCDM]+\./, /[ivxlcdm]+\./, '#.'),
      WHITE_SPACE,
    )),

    // Definition list
    // ---------------

    // TODO

    // Field list
    // ----------

    field_list: $ => seq(
      repeat(seq($.field, $._eol)),
      $.field,
    ),

    field: $ => seq(
      ':',
      $._field_name,
      ':',
      WHITE_SPACE,
      $._field_body,
    ),

    _field_name: $ => /[^:]+/,
    _field_body: $ => $._line,

    // Option list
    // -----------

    option_list: $ => seq(
      repeat(seq($.option_list_item, $._eol)),
      $.option_list_item,
    ),

    option_list_item: $ => seq(
      token(
        seq(
          repeat(seq(OPTION_GROUP, ', ')),
          OPTION_GROUP,
          WHITE_SPACE,
          WHITE_SPACE,
        ),
      ),
      $._line,
    ),


    // Line block
    // ==========

    line_block: $ => seq(
      repeat(seq($._single_line_block, $._eol)),
      $._single_line_block,
    ),
    _single_line_block: $ => choice(
      '|',
      seq('|', WHITE_SPACE, $._line),
    ),


    // Markup blocks
    // =============

    _markup_blocks: $ => choice(
      $._footnote_block,
      $._citation_block,
      $._hyperlink_target_block,
      $._anoynymous_hyperlink_target_block,
    ),
    _markup_start: $ => token(seq('..', WHITE_SPACE)),

    // Footnotes
    // ---------

    _footnote_block: $ => seq(
      repeat(seq($.footnote, $._eol)),
      $.footnote,
    ),
    footnote: $ => seq(
      $._markup_start,
      '[',
      $._label,
      ']',
      WHITE_SPACE,
      $._line,
    ),
    _label: $ => choice(
      /[0-9]+/,
      '#',
      /#[a-zA-Z0-9][a-zA-Z0-9_]*/,
      '*',
    ),

    // Citations
    // ---------

    _citation_block: $ => seq(
      repeat(seq($.citation, $._eol)),
      $.citation,
    ),
    citation: $ => seq(
      $._markup_start,
      '[',
      $._citation_label,
      ']',
      WHITE_SPACE,
      $._line,
    ),
    _citation_label: $ => /[a-zA-Z0-9]+([a-zA-Z0-9._-]+[a-zA-Z0-9])?/,

    // Hyperlink targets
    // -----------------

    _hyperlink_target_block: $ => seq(
      repeat(seq($.target, $._eol)),
      $.target,
    ),
    target: $ => seq(
      $._markup_start,
      choice(
        seq('_', $._reference_name),
        '__',
      ),
      ':',
      optional(seq(WHITE_SPACE, $._line)),
    ),
    _reference_name: $ => choice(
      /[^_:]([^:]+[^_:])?/,
      /`[^`]+`/,
    ),

    // Anonymous hyperlink targets
    // ---------------------------

    _anoynymous_hyperlink_target_block: $ => seq(
      repeat(seq(alias($._anonymous_target, $.target), $._eol)),
      alias($._anonymous_target, $.target),
    ),
    _anonymous_target: $ => seq(
      '__',
      optional(seq(WHITE_SPACE, $._line)),
    ),


    // ==============
    // General tokens
    // ==============

    // TODO: this should be body
    _line: $ => repeat1(/./),
  },
});
