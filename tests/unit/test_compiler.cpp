#include "nexa/analyzer.h"
#include "nexa/registry.h"
#include "nexa/symbol_table.h"
#include "nexa/type_system.h"

#include <gtest/gtest.h>

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

TEST(AnalyzerTest, ParseFunctionAndForEach) {
    // テスト用のNexaコード
    // 関数宣言と、その中にある forEach 文を定義
    const char* source = R"(
        pub fun update_monsters(delta_time: float32) {
            forEach Monster where is_active {
                Position.x = 1.0;
            }
        }
    )";

    // パーサーのセットアップ
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());
    TSTree* tree = ts_parser_parse_string(parser, nullptr, source, static_cast<uint32_t>(strlen(source)));
    TSNode root_node = ts_tree_root_node(tree);


    // 1. ルート直下の最初のノードが `function_declaration` であることを確認
    TSNode func_node = ts_node_named_child(root_node, 0);
    ASSERT_FALSE(ts_node_is_null(func_node));
    EXPECT_STREQ(ts_node_type(func_node), "function_declaration");

    // 2. 関数の名前 (`name` フィールド) が "update_monsters" であるか確認
    TSNode func_name_node = ts_node_child_by_field_name(func_node, "name", 4);
    ASSERT_FALSE(ts_node_is_null(func_name_node));
    uint32_t name_start = ts_node_start_byte(func_name_node);
    uint32_t name_end = ts_node_end_byte(func_name_node);
    std::string func_name(source + name_start, name_end - name_start);
    EXPECT_EQ(func_name, "update_monsters");

    // 3. 関数のボディ (`block`) を取得
    TSNode body_node = ts_node_child_by_field_name(func_node, "body", 4);
    ASSERT_FALSE(ts_node_is_null(body_node));

    // 4. ボディの中の最初の文が `forEach_statement` であることを確認
    TSNode foreach_node = ts_node_named_child(body_node, 0);
    ASSERT_FALSE(ts_node_is_null(foreach_node));
    EXPECT_STREQ(ts_node_type(foreach_node), "forEach_statement");

    // 5. forEach のターゲット (`target` フィールド) が "Monster" であるか確認
    TSNode target_node = ts_node_child_by_field_name(foreach_node, "target", 6);
    ASSERT_FALSE(ts_node_is_null(target_node));
    uint32_t target_start = ts_node_start_byte(target_node);
    uint32_t target_end = ts_node_end_byte(target_node);
    std::string target_name(source + target_start, target_end - target_start);
    EXPECT_EQ(target_name, "Monster");

    // 6. forEach の条件 (`condition` フィールド) が "is_active" であるか確認
    TSNode condition_node = ts_node_child_by_field_name(foreach_node, "condition", 9);
    ASSERT_FALSE(ts_node_is_null(condition_node));
    uint32_t cond_start = ts_node_start_byte(condition_node);
    uint32_t cond_end = ts_node_end_byte(condition_node);
    std::string cond_name(source + cond_start, cond_end - cond_start);
    EXPECT_EQ(cond_name, "is_active");

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

TEST(AnalyzerTest, ExtractFunctionAndForEach) {
    // テスト用のNexaコード
    const char* source = R"(
        pub fun update_monsters(delta_time: float32) {
            forEach Monster where is_active {
                Position.x = 1.0;
            }
        }
    )";

    // 1. パーサーと Interner の準備
    nexa::StringInterner interner;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    uint32_t source_length = static_cast<uint32_t>(strlen(source));
    TSTree* tree = ts_parser_parse_string(parser, nullptr, source, source_length);
    TSNode root_node = ts_tree_root_node(tree);

    char* tree_str = ts_node_string(root_node);
    std::cout << "\n==== AST DUMP ====\n" << tree_str << "\n==================\n" << std::endl;
    free(tree_str);

    nexa::Analyzer analyzer(source, interner);
    analyzer.analyze_root(root_node);

    const auto& functions = analyzer.get_functions();

    // 関数が1つだけ見つかっているか？
    ASSERT_EQ(functions.size(), 1);
    const auto& func = functions[0];

    // 関数の名前が "update_monsters" になっているか？
    // IDを Interner に渡して文字列に戻して確認する
    EXPECT_EQ(interner.GetString(func.name_id), "update_monsters");

    ASSERT_EQ(func.parameters.size(), 1);
    const auto& param = func.parameters[0];
    EXPECT_EQ(interner.GetString(param.name_id), "delta_time");
    EXPECT_EQ(interner.GetString(param.type_id), "float32");

    // forEach ループが1つ見つかっているか？
    ASSERT_EQ(func.for_each_loops.size(), 1);
    const auto& loop = func.for_each_loops[0];

    // ループの対象（ターゲット）が "Monster" か？
    EXPECT_EQ(interner.GetString(loop.target_entity_id), "Monster");

    // 条件（where）がちゃんと存在しているか？
    EXPECT_TRUE(loop.has_condition());

    // 条件の文字が "is_active" か？
    EXPECT_EQ(interner.GetString(loop.condition_id), "is_active");

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

TEST(AnalyzerTest, ExtractIfStatement) {
    const char* source = R"(
        fun check_status(is_alive: bool) {
            if is_alive {
                forEach Monster where is_active {
                    Position.x = 1.0;
                }
            }
        }
    )";

    nexa::StringInterner interner;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, source, static_cast<uint32_t>(strlen(source)));
    TSNode root_node = ts_tree_root_node(tree);

    nexa::Analyzer analyzer(source, interner);
    analyzer.analyze_root(root_node);

    const auto& functions = analyzer.get_functions();
    ASSERT_EQ(functions.size(), 1);
    const auto& func = functions[0];

    // if文が抽出できているか
    ASSERT_EQ(func.if_statements.size(), 1);
    EXPECT_EQ(interner.GetString(func.if_statements[0].condition_id), "is_alive");

    // ifブロック内のforEachも再帰的に拾えているか
    ASSERT_EQ(func.for_each_loops.size(), 1);
    EXPECT_EQ(interner.GetString(func.for_each_loops[0].target_entity_id), "Monster");

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

TEST(AnalyzerTest, ExtractWhileStatement) {
    const char* source = R"(
        pub fun wait_for_ready() {
            while is_waiting {
                if ready_flag {
                    is_waiting = 0;
                }
            }
        }
    )";

    nexa::StringInterner interner;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, source, static_cast<uint32_t>(strlen(source)));
    TSNode root_node = ts_tree_root_node(tree);

    nexa::Analyzer analyzer(source, interner);
    analyzer.analyze_root(root_node);

    const auto& functions = analyzer.get_functions();
    ASSERT_EQ(functions.size(), 1);
    const auto& func = functions[0];

    // whileループが抽出できているか
    ASSERT_EQ(func.while_loops.size(), 1);
    EXPECT_EQ(interner.GetString(func.while_loops[0].condition_id), "is_waiting");

    // whileの中のif文も再帰的に拾えているか
    ASSERT_EQ(func.if_statements.size(), 1);
    EXPECT_EQ(interner.GetString(func.if_statements[0].condition_id), "ready_flag");

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

TEST(AnalyzerTest, ExtractVariableDeclaration) {
    const char* source = R"(
        pub fun setup_player() {
            val max_health: float32 = 100.0;
            var current_state = 1;
        }
    )";

    nexa::StringInterner interner;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    TSTree* tree = ts_parser_parse_string(parser, nullptr, source, static_cast<uint32_t>(strlen(source)));
    TSNode root_node = ts_tree_root_node(tree);

    nexa::Analyzer analyzer(source, interner);
    analyzer.analyze_root(root_node);

    const auto& functions = analyzer.get_functions();
    ASSERT_EQ(functions.size(), 1);
    const auto& func = functions[0];

    // 変数が2つ抽出できているか
    ASSERT_EQ(func.variables.size(), 2);

    // 1つ目: val max_health: float32 = 100.0;
    const auto& var1 = func.variables[0];
    EXPECT_EQ(interner.GetString(var1.name_id), "max_health");
    EXPECT_TRUE(var1.has_explicit_type());
    EXPECT_EQ(interner.GetString(var1.type_id), "float32");
    EXPECT_FALSE(var1.is_mutable);
    EXPECT_FALSE(ts_node_is_null(var1.value_node));

    // 2つ目: var current_state = 1;
    const auto& var2 = func.variables[1];
    EXPECT_EQ(interner.GetString(var2.name_id), "current_state");
    EXPECT_FALSE(var2.has_explicit_type()); // 型指定がない
    EXPECT_TRUE(var2.is_mutable);
    EXPECT_FALSE(ts_node_is_null(var2.value_node));

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

TEST(TypeSystemTest, BuiltinTypeRegistration) {
    nexa::StringInterner interner;
    nexa::TypeRegistry type_registry(interner);

    // float32 や bool という文字列をIDに変換してみる
    nexa::StringID float32_id = interner.Intern("float32");
    nexa::StringID bool_id = interner.Intern("bool");
    nexa::StringID unknown_id = interner.Intern("Monster"); // 組み込み型ではない適当な名前

    // TypeRegistryが正しくIDを保持し、組み込み型として判定できるか確認
    EXPECT_TRUE(type_registry.is_builtin(float32_id));
    EXPECT_TRUE(type_registry.is_builtin(bool_id));

    // ユーザー定義の型は組み込み型ではないと判定されるか確認
    EXPECT_FALSE(type_registry.is_builtin(unknown_id));

    // ゲッター経由で取得したIDが一致するか確認
    EXPECT_EQ(type_registry.get_float32(), float32_id);
    EXPECT_EQ(type_registry.get_bool(), bool_id);
}

TEST(SymbolTableTest, BasicDeclarationAndLookup) {
    nexa::StringInterner interner;
    nexa::SymbolTable table;

    nexa::StringID var_x = interner.Intern("x");
    nexa::StringID type_i32 = interner.Intern("int32");

    // 未定義の変数は検索失敗する
    EXPECT_EQ(table.lookup(var_x), nexa::kInvalidStringID);

    // 変数登録と検索
    EXPECT_TRUE(table.declare(var_x, type_i32, /*is_mutable=*/false));
    EXPECT_EQ(table.lookup(var_x), type_i32);

    // 同一スコープでの同名定義は弾かれる
    EXPECT_FALSE(table.declare(var_x, type_i32, /*is_mutable=*/true));
}

TEST(SymbolTableTest, ScopeNestingAndShadowing) {
    nexa::StringInterner interner;
    nexa::SymbolTable table;

    nexa::StringID var_x = interner.Intern("x");
    nexa::StringID var_y = interner.Intern("y");
    nexa::StringID type_i32 = interner.Intern("int32");
    nexa::StringID type_f32 = interner.Intern("float32");

    // グローバル / 最外周スコープ
    EXPECT_TRUE(table.declare(var_x, type_i32, false));

    // 内側スコープへ突入
    table.enter_scope();
    {
        EXPECT_TRUE(table.declare(var_y, type_f32, true));
        // 外側の変数は内側からも見える
        EXPECT_EQ(table.lookup(var_x), type_i32);
        EXPECT_EQ(table.lookup(var_y), type_f32);

        // 別スコープであれば同名変数を定義可能（シャドウイング）
        EXPECT_TRUE(table.declare(var_x, type_f32, false));
        // 内側では新しい型（float32）で解決される
        EXPECT_EQ(table.lookup(var_x), type_f32);
    }
    // 内側スコープを脱出
    table.exit_scope();

    // 脱出後は外側の型（int32）に復帰している
    EXPECT_EQ(table.lookup(var_x), type_i32);
    // 内側で定義された y は消滅している
    EXPECT_EQ(table.lookup(var_y), nexa::kInvalidStringID);
}

TEST(SymbolTableTest, RedundantExitScopeSafety) {
    nexa::SymbolTable table;

    // スコープが空の状態で exit_scope を呼んでもクラッシュしない
    EXPECT_NO_THROW(table.exit_scope());
}