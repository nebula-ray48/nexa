#pragma once

#include <vector>
#include <string>
#include <string_view>
#include "compiler.h"
#include "registry.h"
#include "type_system.h"
#include "symbol_table.h"

namespace nexa {

// 型エラーの情報を保持するフラットな構造体
struct TypeError {
    StringID function_name_id;
    std::string message;
};

class TypeChecker {
public:
    // Analyzerが抽出した関数リストと、TypeRegistryを受け取る
    TypeChecker(const std::vector<FunctionInfo>& functions,
                TypeRegistry& type_registry,
                StringInterner& interner);

    // 型検査を実行。1つでもエラーがあれば false を返す
    [[nodiscard]] bool check_all();

    // 蓄積されたエラーの一覧を取得
    [[nodiscard]] const std::vector<TypeError>& get_errors() const noexcept { return errors_; }

private:
    const std::vector<FunctionInfo>& functions_;
    TypeRegistry& type_registry_;
    StringInterner& interner_;

    // 検査中に状態が変化するデータ
    SymbolTable symbol_table_;
    std::vector<TypeError> errors_;

    // --- 内部の検証ロジック ---
    void check_function(const FunctionInfo& func);

    // エラーを追加するユーティリティ関数
    void report_error(StringID func_id, std::string_view message);

    // ブロック（文の集まり）を処理する
    void check_block(const std::vector<ParameterInfo>& statements);

    // 変数宣言の配列を一括チェック
    void check_variables(StringID func_name, const std::vector<VariableInfo>& variables);

    // ForEachループの配列を一括チェック
    void check_for_each_loops(StringID func_name, const std::vector<ForEachInfo>& loops);

    // If文の配列を一括チェック
    void check_if_statements(StringID func_name, const std::vector<IfInfo>& if_stmts);

    // Whileループの配列を一括チェック
    void check_while_loops(StringID func_name, const std::vector<WhileInfo>& while_loops);
};

} // namespace nexa