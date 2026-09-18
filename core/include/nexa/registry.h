#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <tree_sitter/api.h>

#include "compiler.h"

namespace nexa {

struct ParameterInfo {
    StringID name_id;
    StringID type_id;
};

struct VariableInfo {
    StringID name_id;
    StringID type_id;
    bool is_mutable;
    TSNode value_node;

    [[nodiscard]] bool has_explicit_type() const noexcept { return is_valid(type_id); }
};

struct IfInfo {
    StringID condition_id;
};

struct ForEachInfo {
    StringID target_entity_id; // 対象エンティティ（例: "Monster" のID）
    StringID condition_id;     // 条件式フラグ（例: "is_active" のID、無ければ kInvalidStringID）
    TSNode condition_node;

    [[nodiscard]] bool has_condition() const noexcept {
        return is_valid(condition_id);
    }
};

struct WhileInfo {
    StringID condition_id;
};

struct FunctionInfo {
    StringID name_id;
    StringID return_type_id;
    std::vector<ParameterInfo> parameters;
    std::vector<ForEachInfo> for_each_loops;
    std::vector<IfInfo> if_statements;
    std::vector<WhileInfo> while_loops;
    std::vector<VariableInfo> variables;
};

}  // namespace nexa