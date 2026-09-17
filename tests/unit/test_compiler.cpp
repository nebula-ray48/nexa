#include <gtest/gtest.h>
#include "nexa/compiler.h"

using namespace nexa;

void ParseAndAnalyze(std::string_view code, StringInterner& interner, VariableRegistry_DOD& var_reg, ComponentRegistry_DOD& comp_reg) {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        code.data(),
        static_cast<uint32_t>(code.length())
    );

    TSNode root_node = ts_tree_root_node(tree);

    TreeSitterSymbols symbols;
    symbols.Initialize(tree_sitter_nexa());

    AnalyzeAST(root_node, code, interner, var_reg, comp_reg, symbols);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

// 1. 変数解析のテスト
TEST(AnalyzerTest, ParseVariable) {
    StringInterner interner;
    VariableRegistry_DOD var_reg;
    ComponentRegistry_DOD comp_reg;

    ParseAndAnalyze("val hp: int32 = 100;", interner, var_reg, comp_reg);

    // 抽出された変数が1つだけであることを保証
    ASSERT_EQ(var_reg.names.size(), 1);

    // 名前、型、初期値が正しくDODメモリに格納されていることを検証
    EXPECT_EQ(interner.GetString(var_reg.names[0]), "hp");
    EXPECT_EQ(interner.GetString(var_reg.types[0]), "int32");
    EXPECT_EQ(interner.GetString(var_reg.initial_values[0]), "100");
}

// 2. コンポーネント解析のテスト
TEST(AnalyzerTest, ParseComponent) {
    StringInterner interner;
    VariableRegistry_DOD var_reg;
    ComponentRegistry_DOD comp_reg;

    ParseAndAnalyze("component Position { x: float32, y: float32 }", interner, var_reg, comp_reg);

    // 抽出されたコンポーネントが1つであることを保証
    ASSERT_EQ(comp_reg.names.size(), 1);
    EXPECT_EQ(interner.GetString(comp_reg.names[0]), "Position");

    // フィールドが2つ抽出されていることを保証
    ASSERT_EQ(comp_reg.field_counts[0], 2);

    // フラットなSoA配列からフィールド情報を検証
    uint32_t start = comp_reg.field_starts[0];

    EXPECT_EQ(interner.GetString(comp_reg.field_names[start + 0]), "x");
    EXPECT_EQ(interner.GetString(comp_reg.field_types[start + 0]), "float32");

    EXPECT_EQ(interner.GetString(comp_reg.field_names[start + 1]), "y");
    EXPECT_EQ(interner.GetString(comp_reg.field_types[start + 1]), "float32");
}