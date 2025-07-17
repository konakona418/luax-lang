#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <variant>


namespace luaxc {
    namespace error {
        class GCError : public std::exception {
        public:
            std::string message;
            explicit GCError(std::string message) {
                std::stringstream ss;
                ss << "GC Error: " << message;
                this->message = ss.str();
            }

            const char* what() const noexcept override {
                return message.c_str();
            }
        };

#define LUAXC_GC_THROW_ERROR(message) throw error::GCError(message);
#define LUAXC_GC_THROW_ERROR_EXPR(message) throw error::GCError(message)
    }// namespace error

    enum class ValueType {
        Boolean,
        Int,
        Float,
        String,
        Function,
        Array,
        Object,
        Type,
        Null,
        Unit,
        Unknown
    };

    using Bool = bool;
    using Int = int64_t;
    using Float = double;

    class TypeObject;

    class GCObject {
    public:
        virtual std::string to_string() const { return "[gc object]"; };

    protected:
        TypeObject* object_type_info;
    };

    template<typename Encoding>
    class BasicStringObject : public GCObject {
    public:
        explicit BasicStringObject<Encoding>(const std::basic_string<Encoding>& str) {
            length = str.length();// no null terminator.
            this->data = static_cast<Encoding*>(std::malloc(sizeof(Encoding) * (length + 1)));
            std::memcpy(this->data, str.data(), length + 1);
        }

        BasicStringObject<Encoding> operator=(const BasicStringObject<Encoding>& other) {
            length = other.length;
            this->data = static_cast<Encoding*>(std::malloc(sizeof(Encoding) * (length + 1)));
            std::memcpy(this->data, other.data, length + 1);
            return *this;
        }

        ~BasicStringObject<Encoding>() {
            length = 0;
            std::free(this->data);
        }

        const Encoding* c_str() const { return static_cast<const char*>(data); }

        static BasicStringObject<Encoding>* from_string(const std::basic_string<Encoding>& str) {
            return new BasicStringObject<Encoding>(str);
        }

        std::string to_string() const override {
            return std::string(c_str());
        }

    private:
        Encoding* data;
        size_t length;
    };

    class StringObject : public BasicStringObject<char> {
    };

    class ArrayObject;
    class FunctionObject;

    class NullObject {
    public:
        NullObject() = default;

        operator bool() const { return false; }

        operator int() const { return 0; }

        operator float() const { return 0.0f; }

        bool operator==(const NullObject&) const { return true; }

        bool operator!=(const NullObject&) const { return false; }
    };

    class UnitObject {
    public:
        UnitObject() = default;

        bool operator==(const UnitObject&) const { return true; }

        bool operator!=(const UnitObject&) const { return false; }
    };

#define LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(fn_name, type_name) \
    static TypeObject* fn_name() {                                  \
        static TypeObject int_type = TypeObject(type_name);         \
        return &int_type;                                           \
    }

    class TypeObject : public GCObject {
    public:
        struct TypeField {
            TypeObject* type_ptr;
        };

        TypeObject() : type_name("<anonymous>") {};

        explicit TypeObject(const std::string& type_name) : type_name(type_name) {}

        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(any, "Any")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(int_, "Int")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(float_, "Float")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(bool_, "Bool")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(function, "Function")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(gc_string, "String")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(gc_object, "Object")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(unit, "Unit")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(null, "Null")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(type, "Type")

        static std::vector<std::pair<std::string, TypeObject*>> get_all_static_type_info() {
            return {
                    {"Any", any()},
                    {"Int", int_()},
                    {"Float", float_()},
                    {"Bool", bool_()},
                    {"Function", function()},
                    {"String", gc_string()},
                    {"Object", gc_object()},
                    {"Unit", unit()},
                    {"Null", null()},
                    {"Type", type()}};
        }

        void add_field(const std::string& name, TypeField field) { fields.emplace(name, field); }

        TypeField get_field(const std::string& name) { return fields.at(name); }

        bool has_field(const std::string& name) { return fields.find(name) != fields.end(); }

        static TypeObject* create(const std::string& type_name) { return new TypeObject(type_name); }

        static TypeObject* create() { return new TypeObject(); }

    private:
        std::string type_name;
        std::unordered_map<std::string, TypeField> fields;
        std::unordered_map<std::string, FunctionObject*> member_funcs;
        std::unordered_map<std::string, FunctionObject*> static_funcs;
    };

    class PrimValue {
    public:
        using Value = std::variant<
                std::monostate,
                Bool,
                Int,
                Float,
                GCObject*,
                NullObject,
                UnitObject>;

        PrimValue() : type(ValueType::Unknown) {
            type_info = TypeObject::any();
        }

        explicit PrimValue(ValueType type) : type(type), value(std::monostate()) {
            type_info = select_value_type_info(type);
        }

        PrimValue(ValueType type, Value value) : type(type), value(std::move(value)) {
            type_info = select_value_type_info(type);
        }

        static PrimValue from_string(const std::string& str) { return PrimValue(ValueType::String, StringObject::from_string(str)); };

        static PrimValue from_i32(int32_t i) { return PrimValue(ValueType::Int, i); }

        static PrimValue from_i64(int64_t i) { return PrimValue(ValueType::Int, i); }

        static PrimValue from_f32(float f) { return PrimValue(ValueType::Float, f); }

        static PrimValue from_f64(double f) { return PrimValue(ValueType::Float, f); }

        static PrimValue from_bool(bool b) { return PrimValue(ValueType::Boolean, b); }

        static PrimValue null() { return PrimValue(ValueType::Null, NullObject()); }

        static PrimValue unit() { return PrimValue(ValueType::Unit, UnitObject()); }

        bool is_null() const { return type == ValueType::Null; }

        bool is_unit() const { return type == ValueType::Unit; }

        bool is_int() const { return type == ValueType::Int; }

        bool is_float() const { return type == ValueType::Float; }

        bool is_number() const { return is_int() || is_float(); }

        bool is_string() const { return type == ValueType::String; }

        bool is_boolean() const { return type == ValueType::Boolean; }

        std::string to_string() const;

        bool to_bool() const;

        ValueType get_type() const { return type; }

        Value get_value() const { return value; }

        const Value& get_value_ref() const { return value; }

        template<typename T>
        T get_inner_value() const { return std::get<T>(value); }

        template<typename T>
        bool holds_alternative() const { return std::holds_alternative<T>(value); }

        bool operator==(const PrimValue& other) const;

        void set_type_info(TypeObject* info) { type_info = info; }

        TypeObject* get_type_info() const { return type_info; }

    private:
        ValueType type;
        Value value;

        TypeObject* type_info;

        static TypeObject* select_value_type_info(ValueType type);
    };

    class FunctionObject : public GCObject {
    public:
        FunctionObject() = default;

        std::string to_string() const override {
            return "[function object]";
        }

        template<typename Fn,
                 typename = std::enable_if_t<std::is_invocable_r_v<PrimValue, Fn, std::vector<PrimValue>>>>
        static FunctionObject* create_native_function(Fn function) {
            auto* func = new FunctionObject();
            func->native_function = std::move(function);
            func->is_native = true;
            func->begin_offset = 0;
            func->arity = 1;
            return func;
        }

        static FunctionObject* create_function(size_t begin_offset, size_t arity) {
            auto* func = new FunctionObject();
            func->is_native = false;
            func->native_function = nullptr;
            func->begin_offset = begin_offset;
            func->arity = arity;
            return func;
        }

        bool is_native_function() const { return is_native; }

        PrimValue call_native(std::vector<PrimValue> args) { return native_function(std::move(args)); };

        size_t get_begin_offset() const { return begin_offset; }

        size_t get_arity() const { return arity; }

    private:
        bool is_native;
        std::function<PrimValue(std::vector<PrimValue>)> native_function;
        size_t begin_offset;
        size_t arity;
    };

    namespace detail {
        template<class... Ts>
        struct overloaded : Ts... {
            using Ts::operator()...;
        };
        template<class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;
#define LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(op_fn_name) \
    PrimValue op_fn_name(const PrimValue& lhs, const PrimValue& rhs);
#define LUAXC_GC_VALUE_DECLARE_UNARY_OPERATOR(op_fn_name) \
    PrimValue op_fn_name(const PrimValue& val);

        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_eq)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_neq)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_lt)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_gt)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_lte)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_gte)

        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_add)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_sub)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_mul)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_div)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_mod)

        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_shl)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_shr)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_band)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_bor)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_bxor)

        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_land)
        LUAXC_GC_VALUE_DECLARE_BINARY_OPERATOR(prim_value_lor)

        LUAXC_GC_VALUE_DECLARE_UNARY_OPERATOR(prim_value_lnot)
        LUAXC_GC_VALUE_DECLARE_UNARY_OPERATOR(prim_value_bnot)
        LUAXC_GC_VALUE_DECLARE_UNARY_OPERATOR(prim_value_neg)
        LUAXC_GC_VALUE_DECLARE_UNARY_OPERATOR(prim_value_pos)
    }// namespace detail

}// namespace luaxc