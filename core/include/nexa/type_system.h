// core/include/nexa/type_system.h
#pragma once

#include <array>
#include "compiler.h" // StringInterner と StringID が定義されているヘッダー

namespace nexa {

class TypeRegistry {
public:
    // コンストラクタで、コンパイラが扱うすべての組み込み型を登録する
    explicit TypeRegistry(StringInterner& interner) : interner_(interner) {
        // 整数型
        type_int8_    = interner_.Intern("int8");
        type_int16_   = interner_.Intern("int16");
        type_int32_   = interner_.Intern("int32");
        type_int64_   = interner_.Intern("int64");

        // 符号なし整数型
        type_uint8_   = interner_.Intern("uint8");
        type_uint16_  = interner_.Intern("uint16");
        type_uint32_  = interner_.Intern("uint32");
        type_uint64_  = interner_.Intern("uint64");

        // 浮動小数点型とその他
        type_float32_ = interner_.Intern("float32");
        type_float64_ = interner_.Intern("float64");
        type_bool_    = interner_.Intern("bool");
        type_string_  = interner_.Intern("string");

        builtin_type_ids_ = {
            type_int8_, type_int16_, type_int32_, type_int64_,
            type_uint8_, type_uint16_, type_uint32_, type_uint64_,
            type_float32_, type_float64_,
            type_bool_, type_string_,
        };
    }

    // 指定されたIDが組み込み型かどうかを判定する
    [[nodiscard]] bool is_builtin(StringID type_id) const noexcept {
        for (auto id : builtin_type_ids_) {
            if (id == type_id) return true;
        }
        return false;
    }

    // 型検査の時に「これはfloat32か？」と確認するためのゲッター
    [[nodiscard]] StringID get_float32() const noexcept { return type_float32_; }
    [[nodiscard]] StringID get_bool() const noexcept { return type_bool_; }
    [[nodiscard]] StringID get_int32() const noexcept { return type_int32_; }

private:
    StringInterner& interner_;

    // IDをキャッシュしておく変数
    StringID type_int8_;
    StringID type_int16_;
    StringID type_int32_;
    StringID type_int64_;

    StringID type_uint8_;
    StringID type_uint16_;
    StringID type_uint32_;
    StringID type_uint64_;

    StringID type_float32_;
    StringID type_float64_;

    StringID type_bool_;
    StringID type_string_;
    
    std::array<StringID, 12> builtin_type_ids_;
};

} // namespace nexa