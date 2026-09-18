#pragma once

#include <tree_sitter/api.h>

#include "registry.h"

namespace nexa {

class Analyzer {
public:
    Analyzer(std::string_view source_code, StringInterner& interner);

    void analyze_root(TSNode root_node);
    [[nodiscard]] const std::vector<FunctionInfo>& get_functions() const { return functions_; }

private:
    std::string_view source_;
    StringInterner& interner_;
    std::vector<FunctionInfo> functions_;

    StringID get_node_string_id(TSNode node);
    void analyze_function(TSNode func_node);
    void analyze_block(TSNode block_node, FunctionInfo& current_func);
    void analyze_parameters(TSNode params_node, FunctionInfo& current_func);
    void analyze_if(TSNode if_node, FunctionInfo& current_func);
    void analyze_while(TSNode while_node, FunctionInfo& current_func);
    void analyze_variable(TSNode var_node, FunctionInfo& current_func);
};

}  // namespace nexa