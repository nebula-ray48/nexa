#include "nexa/symbol_table.h"

#include <__ranges/reverse_view.h>

namespace nexa {

// --- スコープに入る時の処理 ---
void SymbolTable::enter_scope() {
    scope_markers_.push_back(symbols_.size());
}

// --- スコープから出る時の処理 ---
void SymbolTable::exit_scope() noexcept {

    if (scope_markers_.empty()) {
        return;
    }

    size_t previous_size = scope_markers_.back();
    symbols_.resize(previous_size);
    scope_markers_.pop_back();

}

bool SymbolTable::declare(StringID name_id, StringID type_id, bool is_mutable) {

    const size_t current_scope_start = scope_markers_.empty() ? 0 : scope_markers_.back();

    for (size_t i = current_scope_start; i < symbols_.size(); ++i) {
        if (symbols_[i].name_id == name_id) {
            return false;
        }
    }

    symbols_.push_back(Symbol{.name_id=name_id, .type_id=type_id, .is_mutable=is_mutable});
    return true;
}

StringID SymbolTable::lookup(StringID name_id) const noexcept {
    for (const auto& symbol : symbols_ | std::ranges::views::reverse) {
        if (symbol.name_id == name_id) {
            return symbol.type_id;
        }
    }

    return kInvalidStringID;
}

} // namespace nexa