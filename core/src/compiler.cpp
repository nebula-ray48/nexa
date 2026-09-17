#include "nexa/compiler.h"

namespace nexa {

void TreeSitterSymbols::Initialize(const TSLanguage* lang) {
    variable_declaration = ts_language_symbol_for_name(lang, "variable_declaration", 20, true);
    component_declaration = ts_language_symbol_for_name(lang, "component_declaration", 21, true);
    field_definition = ts_language_symbol_for_name(lang, "field_definition", 16, true);

    field_name = ts_language_field_id_for_name(lang, "name", 4);
    field_type = ts_language_field_id_for_name(lang, "type", 4);
    field_value = ts_language_field_id_for_name(lang, "value", 5);
}

void AnalyzeAST(TSNode root_node, std::string_view source_text, StringInterner& interner, VariableRegistry_DOD& registry, ComponentRegistry_DOD& comp_registry, const TreeSitterSymbols& symbols) {
    uint32_t child_count = ts_node_child_count(root_node);

    for (uint32_t i = 0; i < child_count; ++i) {
        TSNode child = ts_node_child(root_node, i);

        if (ts_node_symbol(child) == symbols.variable_declaration) {
            TSNode name_node = ts_node_child_by_field_id(child, symbols.field_name);
            TSNode type_node = ts_node_child_by_field_id(child, symbols.field_type);
            TSNode value_node = ts_node_child_by_field_id(child, symbols.field_value);

            uint32_t n_start = ts_node_start_byte(name_node);
            uint32_t n_end   = ts_node_end_byte(name_node);
            std::string_view name_str = source_text.substr(n_start, n_end - n_start);

            uint32_t t_start = ts_node_start_byte(type_node);
            uint32_t t_end   = ts_node_end_byte(type_node);
            std::string_view type_str = source_text.substr(t_start, t_end - t_start);

            uint32_t v_start = ts_node_start_byte(value_node);
            uint32_t v_end   = ts_node_end_byte(value_node);
            std::string_view value_str = source_text.substr(v_start, v_end - v_start);

            StringID name_id = interner.Intern(name_str);
            StringID type_id = interner.Intern(type_str);
            StringID value_id = interner.Intern(value_str);

            registry.names.push_back(name_id);
            registry.types.push_back(type_id);
            registry.initial_values.push_back(value_id);

        } else if (ts_node_symbol(child) == symbols.component_declaration) {
            TSNode name_node = ts_node_child_by_field_id(child, symbols.field_name);
            uint32_t n_start = ts_node_start_byte(name_node);
            uint32_t n_end   = ts_node_end_byte(name_node);
            std::string_view name_str = source_text.substr(n_start, n_end - n_start);
            StringID name_id = interner.Intern(name_str);

            uint32_t start_idx = static_cast<uint32_t>(comp_registry.field_names.size());
            uint32_t field_count = 0;

            uint32_t comp_child_count = ts_node_child_count(child);
            for (uint32_t j = 0; j < comp_child_count; ++j) {
                TSNode comp_child = ts_node_child(child, j);

                if (ts_node_symbol(comp_child) == symbols.field_definition) {
                    TSNode f_name_node = ts_node_child_by_field_id(comp_child, symbols.field_name);
                    TSNode f_type_node = ts_node_child_by_field_id(comp_child, symbols.field_type);

                    uint32_t fn_start = ts_node_start_byte(f_name_node);
                    uint32_t fn_end   = ts_node_end_byte(f_name_node);
                    std::string_view f_name_str = source_text.substr(fn_start, fn_end - fn_start);

                    uint32_t ft_start = ts_node_start_byte(f_type_node);
                    uint32_t ft_end   = ts_node_end_byte(f_type_node);
                    std::string_view f_type_str = source_text.substr(ft_start, ft_end - ft_start);

                    comp_registry.field_names.push_back(interner.Intern(f_name_str));
                    comp_registry.field_types.push_back(interner.Intern(f_type_str));
                    field_count++;
                }
            }

            comp_registry.names.push_back(name_id);
            comp_registry.field_starts.push_back(start_idx);
            comp_registry.field_counts.push_back(field_count);
        }
    }
}

void parse_test_code() {
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    std::string source_code = "val hp: i32 = 100; component Position { x: f32, y: f32 }";

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source_code.c_str(),
        static_cast<uint32_t>(source_code.length())
    );

    TSNode root_node = ts_tree_root_node(tree);

    StringInterner interner;
    TreeSitterSymbols symbols;
    symbols.Initialize(tree_sitter_nexa());

    VariableRegistry_DOD registry;
    ComponentRegistry_DOD comp_registry;

    AnalyzeAST(root_node, source_code, interner, registry, comp_registry, symbols);

    // テストコード移行に向け、std::coutで確認する処理は一旦削除してスッキリさせました

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

} // namespace nexa