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

    symbol_table_.enter_scope();

    for (const auto& param : func.parameters) {

        // 1. 引数の型が辞書にあるか確認
        if (!type_registry_.is_builtin(param.type_id)) {
            report_error(func.name_id, "Unknown parameter type");
        }

        bool success = symbol_table_.declare(param.name_id, param.type_id, false);

        if (!success) {
            report_error(func.name_id, "Duplicate parameter name");
        }
    }

    symbol_table_.exit_scope();
}

} // namespace nexa