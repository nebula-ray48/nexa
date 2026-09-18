#include "nexa/registry.h"

namespace nexa {

Analyzer::Analyzer(std::string_view source_code, StringInterner& interner)
    : source_(source_code), interner_(interner) {}

void Analyzer::analyze_root(TSNode root_node) {
    uint32_t count = ts_node_named_child_count(root_node);
    for (uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_named_child(root_node, i);
        std::string_view type = ts_node_type(child);
        if (type == "function_declaration") {
            analyze_function(child);
        }
    }
}

StringID Analyzer::get_node_string_id(TSNode node) {
    if (ts_node_is_null(node)) {
        return kInvalidStringID;
    }
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    std::string_view text = source_.substr(start, end - start);
    return interner_.Intern(text);
}

void Analyzer::analyze_function(TSNode func_node) {
    FunctionInfo info;
    TSNode name_node = ts_node_child_by_field_name(func_node, "name", 4);
    info.name_id = get_node_string_id(name_node);

    TSNode body_node = ts_node_child_by_field_name(func_node, "body", 4);
    if (!ts_node_is_null(body_node)) {
        analyze_block(body_node, info);
    }
    functions_.push_back(std::move(info));
}

void Analyzer::analyze_block(TSNode block_node, FunctionInfo& current_func) {
    uint32_t count = ts_node_named_child_count(block_node);
    for (uint32_t i = 0; i < count; ++i) {
        TSNode statement = ts_node_named_child(block_node, i);
        std::string_view type = ts_node_type(statement);

        if (type == "forEach_statement") {
            ForEachInfo loop_info;
            TSNode target_node = ts_node_child_by_field_name(statement, "target", 6);
            loop_info.target_entity_id = get_node_string_id(target_node);

            TSNode condition_node = ts_node_child_by_field_name(statement, "condition", 9);
            if (!ts_node_is_null(condition_node)) {
                loop_info.condition_id = get_node_string_id(condition_node);
                loop_info.condition_node = condition_node;
            } else {
                loop_info.condition_id = kInvalidStringID;
                loop_info.condition_node = TSNode{}; // 空の波括弧で安全にゼロ初期化
            }
            current_func.for_each_loops.push_back(std::move(loop_info));
        }
    }
}

} // namespace nexa