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

    StackFrame& StackFrameRef::get_frame() {
        if (pending_return) {
            return *inner.frame_ref;
        }
        return inner.frame;
    }

    const StackFrame& StackFrameRef::get_frame() const {
        if (pending_return) {
            return *inner.frame_ref;
        }
        return inner.frame;
    }

    void StackFrameRef::notify_return(StackFrame frame) {
        pending_return = false;

        this->inner.frame.variables = frame.variables;
    }

    std::shared_ptr<StackFrameRef> StackFrame::make_ref() {
        auto ref = std::make_shared<StackFrameRef>(this);
        pending_refs.push_back(ref);
        return ref;
    }

    void StackFrame::notify_return() {
        for (auto& ref: pending_refs) {
            ref->notify_return(*this);
        }
    }

    size_t FrozenContextObject::get_object_size() const {
        size_t base_size = sizeof(FrozenContextObject);
        for (auto& frame: stack_frames) {
            base_size += frame->get_frame().variables.size() * sizeof(PrimValue);
        }
        return base_size;
    }

    std::vector<GCObject*> FrozenContextObject::get_referenced_objects() const {
        std::vector<GCObject*> referenced_objects;
        auto base = GCObject::get_referenced_objects();
        referenced_objects.insert(referenced_objects.end(), base.begin(), base.end());

        for (auto& frame: stack_frames) {
            for (auto& [identifier, value]: frame->get_frame().variables) {
                if (value.is_gc_object()) {
                    referenced_objects.push_back(value.get_inner_value<GCObject*>());
                }
            }
        }
        return referenced_objects;
    }

    std::optional<PrimValue> FrozenContextObject::query(StringObject* identifier) const {
        for (auto frame_ref = stack_frames.rbegin(); frame_ref != stack_frames.rend(); ++frame_ref) {
            auto& frame = (*frame_ref)->get_frame();
            auto it = frame.variables.find(identifier);
            if (it != frame.variables.end()) {
                return it->second;
            }
        }
        return std::nullopt;
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

    std::vector<GCObject*> GCObject::get_referenced_objects() const {
        std::vector<GCObject*> referenced_objects;
        for (auto& [_, object]: storage.fields) {
            if (object.is_gc_object()) {
                referenced_objects.push_back(object.get_inner_value<GCObject*>());
            }
        }
        return referenced_objects;
    }

    size_t GCObject::get_object_size() const {
        size_t size = sizeof(GCObject);
        for (auto& [_, object]: storage.fields) {
            if (object.is_gc_object()) {
                size += object.get_inner_value<GCObject*>()->get_object_size();
            } else {
                size += sizeof(PrimValue);
            }
        }
        return size;
    }

    std::vector<GCObject*> ArrayObject::get_referenced_objects() const {
        std::vector<GCObject*> referenced_objects;
        auto base = GCObject::get_referenced_objects();
        referenced_objects.insert(referenced_objects.end(), base.begin(), base.end());

        for (size_t i = 0; i < size; i++) {
            if (data[i].is_gc_object()) {
                referenced_objects.push_back(data[i].get_inner_value<GCObject*>());
            }
        }
        return referenced_objects;
    }

    size_t ArrayObject::get_object_size() const {
        size_t size = sizeof(ArrayObject);
        for (size_t i = 0; i < this->size; i++) {
            if (data[i].is_gc_object()) {
                size += data[i].get_inner_value<GCObject*>()->get_object_size();
            } else {
                size += sizeof(PrimValue);
            }
        }
        return size;
    }

    std::vector<GCObject*> FunctionObject::get_referenced_objects() const {
        std::vector<GCObject*> referenced_objects;
        auto base = GCObject::get_referenced_objects();
        referenced_objects.insert(referenced_objects.end(), base.begin(), base.end());

        if (this->ctx != nullptr) {
            referenced_objects.push_back(this->ctx);
            // gc will look into the ctx
            // referenced_objects = this->ctx->get_referenced_objects();
        }
        return referenced_objects;
    }

    std::vector<GCObject*> TypeObject::get_referenced_objects() const {
        std::vector<GCObject*> referenced_objects;
        auto base = GCObject::get_referenced_objects();
        referenced_objects.insert(referenced_objects.end(), base.begin(), base.end());

        for (auto& [name, type_info]: fields) {
            if (type_info.type_ptr) {
                referenced_objects.push_back(type_info.type_ptr);
            }
        }

        for (auto& [name, method]: member_funcs) {
            referenced_objects.push_back(method);
        }

        for (auto& [name, method]: static_funcs) {
            referenced_objects.push_back(method);
        }

        return referenced_objects;
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