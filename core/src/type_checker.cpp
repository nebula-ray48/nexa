#include "nexa/type_checker.h"

namespace nexa {

TypeChecker::TypeChecker(const std::vector<FunctionInfo>& functions,
                         TypeRegistry& type_registry,
                         StringInterner& interner)
    : functions_(functions), type_registry_(type_registry), interner_(interner) {}

void TypeChecker::report_error(StringID func_id, std::string_view message) {

    std::string func_name = std::string(interner_.GetString(func_id));
    std::string full_message = func_name + ": " + std::string(message);

    errors_.push_back(TypeError{ .function_name_id=func_id, .message=full_message });
}

bool TypeChecker::check_all() {

    for (const auto& func : functions_) {
        check_function(func);
    }

    return errors_.empty();
}

void TypeChecker::check_function(const FunctionInfo& func) {
    if (!type_registry_.is_builtin(func.return_type_id)) {
        report_error(func.name_id, "Unknown return type");
    }

    symbol_table_.enter_scope(); // 関数のスコープに入る

    // 1. 引数の登録
    for (const auto& param : func.parameters) {
        if (!type_registry_.is_builtin(param.type_id)) {
            report_error(func.name_id, "Unknown parameter type");
        }
        bool success = symbol_table_.declare(param.name_id, param.type_id, false);
        if (!success) {
            report_error(func.name_id, "Duplicate parameter name");
        }
    }

    // 2. 変数宣言の一括検査
    check_variables(func.name_id, func.variables);

    // 3. ループや条件分岐の一括検査（今回は空の枠組みだけ作ります）
    check_for_each_loops(func.name_id, func.for_each_loops);
    check_if_statements(func.name_id, func.if_statements);
    check_while_loops(func.name_id, func.while_loops);

    symbol_table_.exit_scope(); // 関数のスコープから出る
}

// 変数の型チェックとシンボルテーブル登録
void TypeChecker::check_variables(StringID func_name, const std::vector<VariableInfo>& variables) {
    for (const auto& var : variables) {

        // 1. 型が明示されている場合（has_explicit_type()）、その型が存在するかチェック
        if (var.has_explicit_type()) {
            if (!type_registry_.is_builtin(var.type_id)) {
                report_error(func_name, "Unknown variable type");
            }
        } else {
            // TODO: 型推論（右辺の var.value_node から型を特定する処理）は後で実装
        }

        // 2. シンボルテーブルへ登録し、名前被りをチェック
        bool success = symbol_table_.declare(var.name_id, var.type_id, var.is_mutable);
        if (!success) {
            report_error(func_name, "Duplicate variable name");
        }
    }
}

void TypeChecker::check_for_each_loops(StringID func_name, const std::vector<ForEachInfo>& loops) {
    for (const auto& loop : loops) {

        // 対象のエンティティ型（例: Monster）が辞書に存在するかチェック
        if (!type_registry_.is_builtin(loop.target_entity_id)) {
            report_error(func_name, "Unknown target entity in forEach");
        }

        // 条件フラグ（例: is_active）が指定されている場合のみチェック
        if (loop.has_condition()) {
            StringID cond_type = symbol_table_.lookup(loop.condition_id);

            if (cond_type == kInvalidStringID) {
                report_error(func_name, "Undefined variable in forEach condition");
            } else if (cond_type != type_registry_.get_bool()) {
                report_error(func_name, "forEach condition must be bool");
            }
        }
    }
}

void TypeChecker::check_if_statements(StringID func_name, const std::vector<IfInfo>& if_stmts) {
    for (const auto& stmt : if_stmts) {
        StringID type_id = symbol_table_.lookup(stmt.condition_id);

        if (type_id == kInvalidStringID) {
            report_error(func_name, "Undefined variable in if condition");
        } else if (type_id != type_registry_.get_bool()) {
            report_error(func_name, "If condition must be bool");
        }
    }
}

void TypeChecker::check_while_loops(StringID func_name, const std::vector<WhileInfo>& while_loops) {
    for (const auto& loop : while_loops) {
        StringID type_id = symbol_table_.lookup(loop.condition_id);

        if (type_id == kInvalidStringID) {
            report_error(func_name, "Undefined variable in while condition");
        } else if (type_id != type_registry_.get_bool()) {
            report_error(func_name, "While condition must be bool");
        }
    }
}

} // namespace nexa