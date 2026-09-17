#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <tree_sitter/api.h>
#include <deque>

extern "C" {
    const TSLanguage* tree_sitter_nexa();
}

namespace nexa {

using StringID = uint32_t;

class StringInterner {
private:
    std::deque<std::string> id_to_string;
    std::unordered_map<std::string_view, StringID> string_to_id;

public:
    StringID Intern(std::string_view str) {
        auto it = string_to_id.find(str);
        if (it != string_to_id.end()) return it->second;

        id_to_string.push_back(std::string(str));
        StringID new_id = static_cast<StringID>(id_to_string.size() - 1);
        string_to_id[id_to_string.back()] = new_id;
        return new_id;
    }

    std::string_view GetString(StringID id) {
        if (id < id_to_string.size()) {
            return id_to_string[id];
        }
        return "<unknown>";
    }
};

struct VariableRegistry_DOD {
    std::vector<StringID> names;
    std::vector<StringID> types;
    std::vector<StringID> initial_values;
};

struct TreeSitterSymbols {
    TSSymbol variable_declaration;
    TSFieldId field_name;
    TSFieldId field_type;
    TSFieldId field_value;

    void Initialize(const TSLanguage* lang) {
        variable_declaration = ts_language_symbol_for_name(lang, "variable_declaration", 20, true);
        field_name = ts_language_field_id_for_name(lang, "name", 4);
        field_type = ts_language_field_id_for_name(lang, "type", 4);
        field_value = ts_language_field_id_for_name(lang, "value", 5);
    }
};

void AnalyzeAST(TSNode root_node, std::string_view source_text, StringInterner& interner, VariableRegistry_DOD& registry, const TreeSitterSymbols& symbols) {
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

            std::cout << "[Nexa Analyzer] Registered Variable ID: " << name_id
                      << " (Type ID: " << type_id
                      << ", Value ID: " << value_id << ")\n";
        }
    }
}

void parse_test_code() {

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_nexa());

    std::string source_code = "val hp: i32 = 100;";

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source_code.c_str(),
        source_code.length()
    );

    TSNode root_node = ts_tree_root_node(tree);

    std::cout << "\n--- Analysis Start ---\n";

    StringInterner interner;
    TreeSitterSymbols symbols;
    symbols.Initialize(tree_sitter_nexa());

    VariableRegistry_DOD registry;

    AnalyzeAST(root_node, source_code, interner, registry, symbols);

    std::cout << "\n--- Memory Verification ---\n";

    if (!registry.names.empty()) {
        std::cout << "Restored Name  (ID " << registry.names[0] << "): "
                  << interner.GetString(registry.names[0]) << "\n";

        std::cout << "Restored Type  (ID " << registry.types[0] << "): "
                  << interner.GetString(registry.types[0]) << "\n";

        std::cout << "Restored Value (ID " << registry.initial_values[0] << "): "
                  << interner.GetString(registry.initial_values[0]) << "\n";
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

} // namespace nexa
