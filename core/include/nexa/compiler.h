#pragma once

#include <deque>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tree_sitter/api.h>

extern "C" {
const TSLanguage* tree_sitter_nexa();
}

namespace nexa {

using StringID = uint32_t;

constexpr StringID kInvalidStringID = std::numeric_limits<StringID>::max(); // 0xFFFFFFFFU

// 判定用のインライン関数
constexpr bool is_valid(StringID id) noexcept {
    return id != kInvalidStringID;
}


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

    std::string_view GetString(StringID id) const {
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

struct ComponentRegistry_DOD {
    std::vector<StringID> names;
    std::vector<uint32_t> field_starts;
    std::vector<uint32_t> field_counts;
    std::vector<StringID> field_names;
    std::vector<StringID> field_types;
};

struct TreeSitterSymbols {
    TSSymbol variable_declaration;
    TSSymbol component_declaration;
    TSSymbol field_definition;

    TSFieldId field_name;
    TSFieldId field_type;
    TSFieldId field_value;

    void Initialize(const TSLanguage* lang);
};

void AnalyzeAST(TSNode root_node, std::string_view source_text, StringInterner& interner, VariableRegistry_DOD& registry, ComponentRegistry_DOD& comp_registry, const TreeSitterSymbols& symbols);

void parse_test_code();

} // namespace nexa