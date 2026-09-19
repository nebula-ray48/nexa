#pragma once

#include <vector>

#include "compiler.h"

namespace nexa {

struct Symbol {
    StringID name_id;
    StringID type_id;
    bool is_mutable;
};

class SymbolTable {
public:
    SymbolTable() = default;

    void enter_scope();
    void exit_scope() noexcept;
    [[nodiscard]] bool declare(StringID name_id, StringID type_id, bool is_mutable);
    [[nodiscard]] StringID lookup(StringID name_id) const noexcept;

private:
    std::vector<Symbol> symbols_;
    std::vector<size_t> scope_markers_;

};

}  // namespace nexa