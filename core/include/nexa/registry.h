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

struct ForEachInfo {
    StringID target_entity_id; // 対象エンティティ（例: "Monster" のID）
    StringID condition_id;     // 条件式フラグ（例: "is_active" のID、無ければ kInvalidStringID）
    TSNode condition_node;

    [[nodiscard]] bool has_condition() const noexcept {
        return is_valid(condition_id);
    }
};

struct FunctionInfo {
    StringID name_id;
    StringID return_type_id;
    std::vector<ParameterInfo> parameters;
    std::vector<ForEachInfo> for_each_loops;
};

class Analyzer {
public:
    Analyzer(std::string_view source_code, StringInterner& interner);

    void analyze_root(TSNode root_node);
    const std::vector<FunctionInfo>& get_functions() const { return functions_; }

private:
    std::string_view source_;
    StringInterner& interner_;
    std::vector<FunctionInfo> functions_;

    StringID get_node_string_id(TSNode node);
    void analyze_function(TSNode func_node);
    void analyze_block(TSNode block_node, FunctionInfo& current_func);
};

}  // namespace nexa