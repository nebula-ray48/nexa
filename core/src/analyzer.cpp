#include "nexa/registry.h"
#include "nexa/analyzer.h"

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

    uint32_t child_count = ts_node_named_child_count(func_node);
    for (uint32_t i = 0; i < child_count; ++i) {
        TSNode child = ts_node_named_child(func_node, i);
        if (ts_node_type(child) == std::string_view("parameter_list")) {
            analyze_parameters(child, info);
            break;
        }
    }

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
        } else if (type == "if_statement") {
            analyze_if(statement, current_func);
        } else if (type == "while_statement") {
            analyze_while(statement, current_func);
        } else if (type == "variable_declaration") {
            analyze_variable(statement, current_func);
        }
    }
}

void Analyzer::analyze_variable(TSNode var_node, FunctionInfo& current_func) {
    VariableInfo info;

    TSNode kind_node = ts_node_child(var_node, 0);
    info.is_mutable = (ts_node_type(kind_node) == std::string_view("var"));

    TSNode name_node = ts_node_child_by_field_name(var_node, "name", 4);
    info.name_id = get_node_string_id(name_node);

    TSNode type_node = ts_node_child_by_field_name(var_node, "type", 4);
    if (!ts_node_is_null(type_node)) {
        info.type_id = get_node_string_id(type_node);
    } else {
        info.type_id = kInvalidStringID;
    }
    
    info.value_node = ts_node_child_by_field_name(var_node, "value", 5);

    current_func.variables.push_back(std::move(info));
}

void Analyzer::analyze_if(TSNode if_node, FunctionInfo& current_func) {
    IfInfo if_info;

    TSNode condition_node = ts_node_child_by_field_name(if_node, "condition", 9);
    if (!ts_node_is_null(condition_node)) {
        if_info.condition_id = get_node_string_id(condition_node);
    } else {
        if_info.condition_id = kInvalidStringID;
    }

    current_func.if_statements.push_back(std::move(if_info));

    TSNode consequence_node = ts_node_child_by_field_name(if_node, "consequence", 11);
    if (!ts_node_is_null(consequence_node)) {
        analyze_block(consequence_node, current_func);
    }
}

void Analyzer::analyze_while(TSNode while_node, FunctionInfo& current_func) {
    WhileInfo info;

    TSNode cond_node = ts_node_child_by_field_name(while_node, "condition", 9);
    if (!ts_node_is_null(cond_node)) {
        info.condition_id = get_node_string_id(cond_node);
    } else {
        info.condition_id = kInvalidStringID;
    }

    current_func.while_loops.push_back(std::move(info));

    TSNode body_node = ts_node_child_by_field_name(while_node, "body", 4);
    if (!ts_node_is_null(body_node)) {
        analyze_block(body_node, current_func);
    }
}

void Analyzer::analyze_parameters(TSNode params_node, FunctionInfo& current_func) {
    uint32_t count = ts_node_named_child_count(params_node);

    for (uint32_t i = 0; i < count; ++i) {
        TSNode param_node = ts_node_named_child(params_node, i);

        if (ts_node_type(param_node) == std::string_view("parameter")) {
            ParameterInfo param_info;

            TSNode name_node = ts_node_child_by_field_name(param_node, "name", 4);
            param_info.name_id = get_node_string_id(name_node);

            TSNode type_node = ts_node_child_by_field_name(param_node, "type", 4);
            param_info.type_id = get_node_string_id(type_node);

            current_func.parameters.push_back(param_info);
        }
    }
}

} // namespace nexa