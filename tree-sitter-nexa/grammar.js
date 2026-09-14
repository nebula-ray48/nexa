module.exports = grammar({
  name: 'nexa',

  rules: {
    // ファイルの全体は、複数の「宣言」が繰り返されるもの
    source_file: $ => repeat($._declaration),

    // 宣言（関数か変数）
    _declaration: $ => choice(
      $.function_declaration,
      $.variable_declaration
    ),

    // 関数定義: pub fun update_monsters() { ... }
    function_declaration: $ => seq(
      optional('pub'),
      'fun',
      field('name', $.identifier),
      '(',
      // TODO: 後で引数のルールを追加する
      ')',
      field('body', $.block)
    ),

    // 変数定義: val x: i32 = 0;
    variable_declaration: $ => seq(
      choice('val', 'var'),
      field('name', $.identifier),
      ':',
      field('type', $.type_identifier),
      '=',
      field('value', $.number),
      ';'
    ),

    // ブロック: { ... }
    block: $ => seq(
      '{',
      repeat($._declaration), // とりあえずブロック内にも変数を書けるようにする
      '}'
    ),

    // 識別子（変数名や関数名）と型、数値の正規表現
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
    type_identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
    number: $ => /\d+/
  }
});
