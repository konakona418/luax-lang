#include "value.hpp"

namespace luaxc {
#define LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(NAME, OP)                                                                      \
    PrimValue NAME(const PrimValue& lhs, const PrimValue& rhs) {                                                                 \
        return std::visit(detail::overloaded{                                                                                    \
                                  [](Int l, Int r) -> PrimValue { return PrimValue::from_bool(l OP r); },                        \
                                  [](Float l, Float r) -> PrimValue { return PrimValue::from_bool(l OP r); },                    \
                                  [](Bool l, Bool r) -> PrimValue { return PrimValue::from_bool(l OP r); },                      \
                                                                                                                                 \
                                  [](Bool l, Int r) -> PrimValue { return PrimValue::from_bool(static_cast<Int>(l) OP r); },     \
                                  [](Int l, Bool r) -> PrimValue { return PrimValue::from_bool(l OP static_cast<Int>(r)); },     \
                                                                                                                                 \
                                  [](Int l, Float r) -> PrimValue { return PrimValue::from_bool(static_cast<Float>(l) OP r); },  \
                                  [](Float l, Int r) -> PrimValue { return PrimValue::from_bool(l OP static_cast<Float>(r)); },  \
                                                                                                                                 \
                                  [](Bool l, Float r) -> PrimValue { return PrimValue::from_bool(static_cast<Float>(l) OP r); }, \
                                  [](Float l, Bool r) -> PrimValue { return PrimValue::from_bool(l OP static_cast<Float>(r)); }, \
                                                                                                                                 \
                                  [](NullObject, NullObject) -> PrimValue { return PrimValue::from_bool(true); },                \
                                                                                                                                 \
                                  [](auto&&, auto&&) -> PrimValue {                                                              \
                                      LUAXC_GC_THROW_ERROR("No available overloaded function for " #NAME);                       \
                                  }},                                                                                            \
                          lhs.get_value(), rhs.get_value());                                                                     \
    }

#define LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH(NAME, OP)                                                                       \
    PrimValue NAME(const PrimValue& lhs, const PrimValue& rhs) {                                                                \
        return std::visit(detail::overloaded{                                                                                   \
                                  [](Int l, Int r) -> PrimValue { return PrimValue::from_i64(l OP r); },                        \
                                  [](Float l, Float r) -> PrimValue { return PrimValue::from_f64(l OP r); },                    \
                                                                                                                                \
                                  [](Int l, Float r) -> PrimValue { return PrimValue::from_f64(static_cast<Float>(l) OP r); },  \
                                  [](Float l, Int r) -> PrimValue { return PrimValue::from_f64(l OP static_cast<Float>(r)); },  \
                                                                                                                                \
                                  [](Bool l, Int r) -> PrimValue { return PrimValue::from_i64(static_cast<Int>(l OP r)); },     \
                                  [](Int l, Bool r) -> PrimValue { return PrimValue::from_i64(l OP static_cast<Int>(r)); },     \
                                                                                                                                \
                                  [](Bool l, Float r) -> PrimValue { return PrimValue::from_f64(static_cast<Float>(l OP r)); }, \
                                  [](Float l, Bool r) -> PrimValue { return PrimValue::from_f64(l OP static_cast<Float>(r)); }, \
                                  [](auto&&, auto&&) -> PrimValue {                                                             \
                                      LUAXC_GC_THROW_ERROR("No available overloaded function for " #NAME);                      \
                                  }},                                                                                           \
                          lhs.get_value(), rhs.get_value());                                                                    \
    }

#define LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(NAME, OP)                                                          \
    PrimValue NAME(const PrimValue& lhs, const PrimValue& rhs) {                                                            \
        return std::visit(detail::overloaded{                                                                               \
                                  [](Int l, Int r) -> PrimValue { return PrimValue::from_i64(l OP r); },                    \
                                  [](Bool l, Int r) -> PrimValue { return PrimValue::from_i64(static_cast<Int>(l OP r)); }, \
                                  [](Int l, Bool r) -> PrimValue { return PrimValue::from_i64(l OP static_cast<Int>(r)); }, \
                                  [](auto&&, auto&&) -> PrimValue {                                                         \
                                      LUAXC_GC_THROW_ERROR("No available overloaded function for " #NAME);                  \
                                  }},                                                                                       \
                          lhs.get_value(), rhs.get_value());                                                                \
    }

#define LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_LOGICAL_OP(NAME, OP)                               \
    PrimValue NAME(const PrimValue& lhs, const PrimValue& rhs) {                             \
        auto to_bool = [](auto v) -> bool {                                                  \
            using T = std::decay_t<decltype(v)>;                                             \
            if constexpr (std::is_same_v<T, Bool>) return v;                                 \
            else if constexpr (std::is_same_v<T, Int>)                                       \
                return v != 0;                                                               \
            else if constexpr (std::is_same_v<T, Float>)                                     \
                return v != 0.0;                                                             \
            else if constexpr (std::is_same_v<T, NullObject>)                                \
                return false;                                                                \
            else                                                                             \
                LUAXC_GC_THROW_ERROR("Invalid operand type for boolean coercion");           \
        };                                                                                   \
        return std::visit(detail::overloaded{                                                \
                                  [&](auto&& l, auto&& r) -> PrimValue {                     \
                                      return PrimValue::from_bool(to_bool(l) OP to_bool(r)); \
                                  }},                                                        \
                          lhs.get_value(), rhs.get_value());                                 \
    }

#define LUAXC_GC_VALUE_DEFINE_PRIM_UNARY_BOOL_OP(NAME, EXPR)                             \
    PrimValue NAME(const PrimValue& val) {                                               \
        return std::visit([](auto&& v) -> PrimValue {                                    \
            using T = std::decay_t<decltype(v)>;                                         \
            if constexpr (std::is_same_v<T, Bool>) return PrimValue::from_bool(EXPR(v)); \
            else if constexpr (std::is_same_v<T, Int>)                                   \
                return PrimValue::from_bool(EXPR(v != 0));                               \
            else if constexpr (std::is_same_v<T, Float>)                                 \
                return PrimValue::from_bool(EXPR(v != 0.0));                             \
            else                                                                         \
                LUAXC_GC_THROW_ERROR("Invalid type for unary bool op")                   \
        },                                                                               \
                          val.get_value());                                              \
    }

#define LUAXC_GC_VALUE_DEFINE_PRIM_UNARY_ARITH_OP(NAME, INT_EXPR, FLOAT_EXPR)              \
    PrimValue NAME(const PrimValue& val) {                                                 \
        return std::visit([](auto&& v) -> PrimValue {                                      \
            using T = std::decay_t<decltype(v)>;                                           \
            if constexpr (std::is_same_v<T, Int>) return PrimValue::from_i64(INT_EXPR(v)); \
            else if constexpr (std::is_same_v<T, Float>)                                   \
                return PrimValue::from_f64(FLOAT_EXPR(v));                                 \
            else                                                                           \
                LUAXC_GC_THROW_ERROR("Invalid type for primitive value")                   \
        },                                                                                 \
                          val.get_value());                                                \
    }


    bool PrimValue::operator==(const PrimValue& other) const {
        return detail::prim_value_eq(*this, other).get_inner_value<Bool>();
    }

    std::string PrimValue::to_string() const {
        auto to_string = [](auto v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, Int>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, Float>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, GCObject*>) {
                if (v == nullptr) {
                    return "[gc object null]";
                }

                std::stringstream ss;
                ss << std::hex << v;
                return v->to_string() + " (" + ss.str() + ")";
            } else if constexpr (std::is_same_v<T, NullObject>) {
                return "[null]";
            } else if constexpr (std::is_same_v<T, UnitObject>) {
                return "[unit object]";
            } else {
                return "[unknown object]";
            }
        };
        return std::visit(to_string, value);
    }

    bool PrimValue::to_bool() const {
        auto to_bool_impl = [](auto v) -> bool {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Bool>) return v;
            else if constexpr (std::is_same_v<T, Int>)
                return v != 0;
            else if constexpr (std::is_same_v<T, Float>)
                return v != 0.0;
            else if constexpr (std::is_same_v<T, NullObject>)
                return false;
            else
                LUAXC_GC_THROW_ERROR("Invalid operand type for boolean coercion");
        };

        return std::visit(to_bool_impl, value);
    }

    TypeObject* PrimValue::select_value_type_info(ValueType type) {
        switch (type) {
            case ValueType::Int:
                return TypeObject::int_();
            case ValueType::Float:
                return TypeObject::float_();
            case ValueType::Boolean:
                return TypeObject::bool_();
            case ValueType::Unit:
                return TypeObject::unit();
            case ValueType::Null:
                return TypeObject::null();
            case ValueType::String:
                return TypeObject::gc_string();
            case ValueType::Array:
                return TypeObject::gc_array();
            case ValueType::Function:
                return TypeObject::function();
            case ValueType::Type:
                return TypeObject::type();
            case ValueType::Object:
                return TypeObject::gc_object();
            default:
                return TypeObject::any();
        }
    }

    namespace detail {
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_eq, ==)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_neq, !=)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_lt, <)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_gt, >)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_lte, <=)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_COMPARE(prim_value_gte, >=)

        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH(prim_value_add, +)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH(prim_value_sub, -)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH(prim_value_mul, *)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH(prim_value_div, /)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_mod, %)

        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_shl, <<)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_shr, >>)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_band, &)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_bor, |)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_ARITH_INT_ONLY(prim_value_bxor, ^)

        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_LOGICAL_OP(prim_value_land, &&)
        LUAXC_GC_VALUE_DEFINE_PRIM_BINARY_LOGICAL_OP(prim_value_lor, ||)

        LUAXC_GC_VALUE_DEFINE_PRIM_UNARY_BOOL_OP(prim_value_lnot, !)

        LUAXC_GC_VALUE_DEFINE_PRIM_UNARY_ARITH_OP(prim_value_pos, +, +)
        LUAXC_GC_VALUE_DEFINE_PRIM_UNARY_ARITH_OP(prim_value_neg, -, -)
        PrimValue prim_value_bnot(const PrimValue& val) {
            return std ::visit(
                    [](auto&& v) -> PrimValue {
                        using T = std ::decay_t<decltype(v)>;
                        if constexpr (std ::is_same_v<T, Int>)
                            return PrimValue ::from_i64(~(v));
                        else if constexpr (std ::is_same_v<T, Float>)
                            LUAXC_GC_THROW_ERROR("Floating point is not available for bitwise not operation")
                        else
                            LUAXC_GC_THROW_ERROR("Invalid type for unary operation")
                    },
                    val.get_value());
        }
    }// namespace detail

}// namespace luaxc