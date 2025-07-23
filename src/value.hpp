#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
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
        Module,
        Type,
        Null,
        Unit,
        Unknown
    };

    using Bool = bool;
    using Int = int64_t;
    using Float = double;

    class GCObject;
    class FunctionObject;
    class TypeObject;
    class PrimValue;
    class StringObject;
    class ModuleObject;

    class GCObject {
    public:
        bool marked = false;

        bool no_collect = false;

        virtual std::string to_string() const { return "[gc object]"; };

        virtual ~GCObject() = default;

        virtual std::vector<GCObject*> get_referenced_objects() const;

        virtual size_t get_object_size() const;

        struct {
            std::unordered_map<StringObject*, PrimValue> fields;
        } storage;
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

        std::string contained_string() const { return std::basic_string<Encoding>(data, length); }

        static BasicStringObject<Encoding>* from_string(const std::basic_string<Encoding>& str) {
            return new BasicStringObject<Encoding>(str);
        }

        std::string to_string() const override {
            return std::string(c_str());
        }

        size_t get_object_size() const override { return sizeof(BasicStringObject<Encoding>) + length * sizeof(Encoding); }

    private:
        Encoding* data;
        size_t length;
    };

    class StringObject : public BasicStringObject<char> {
    };

    struct StackFrameRef;

    struct StackFrame {
        std::unordered_map<StringObject*, PrimValue> variables;
        size_t return_addr;
        bool allow_upward_propagation = false;
        bool force_pop_return_value = false;

        std::vector<std::shared_ptr<StackFrameRef>> pending_refs;

        std::shared_ptr<StackFrameRef> make_ref();

        void notify_return();

        explicit StackFrame(size_t return_addr) : return_addr(return_addr) {};
        StackFrame(size_t return_addr, bool allow_propagation, bool force_pop_return_value = false)
            : return_addr(return_addr),
              allow_upward_propagation(allow_propagation),
              force_pop_return_value(force_pop_return_value) {};
    };

    struct StackFrameRef {
        bool pending_return = true;
        struct {
            StackFrame* frame_ref = nullptr;
            StackFrame frame{0};
        } inner;

        StackFrameRef(StackFrame* frame) {
            inner.frame_ref = frame;
        }

        ~StackFrameRef() {
        }

        StackFrame& get_frame();

        const StackFrame& get_frame() const;

        void notify_return(StackFrame frame);
    };

    using SharedStackFrameRef = std::shared_ptr<StackFrameRef>;

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
        static TypeObject fn_name = TypeObject(type_name);          \
        return &fn_name;                                            \
    }

    class TypeObject : public GCObject {
    public:
        struct TypeField {
            TypeObject* type_ptr;
        };

        TypeObject() : type_name("<anonymous>") {};

        explicit TypeObject(const std::string& type_name) : type_name(type_name) {}

        std::string to_string() const override {
            return "[type object]";
        }

        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(any, "Any")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(int_, "Int")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(float_, "Float")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(bool_, "Bool")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(function, "Function")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(gc_string, "String")
        LUAXC_GC_VALUE_DECLARE_STATIC_TYPE_INFO(gc_array, "Array")
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
                    {"Array", gc_array()},
                    {"Object", gc_object()},
                    {"Unit", unit()},
                    {"Null", null()},
                    {"Type", type()}};
        }

        void add_field(StringObject* name, TypeField field) { fields.emplace(name, field); }

        TypeField get_field(StringObject* name) { return fields.at(name); }

        bool has_field(StringObject* name) { return fields.find(name) != fields.end(); }

        void add_method(StringObject* name, FunctionObject* fn) { member_funcs.emplace(name, fn); }

        FunctionObject* get_method(StringObject* name) { return member_funcs.at(name); }

        void add_static_method(StringObject* name, FunctionObject* fn) { static_funcs.emplace(name, fn); }

        FunctionObject* get_static_method(StringObject* name) { return static_funcs.at(name); }

        const std::unordered_map<StringObject*, TypeField>& get_fields() const { return fields; }

        const std::unordered_map<StringObject*, FunctionObject*>& get_methods() const { return member_funcs; }

        bool has_method(StringObject* name) { return member_funcs.find(name) != member_funcs.end(); }

        bool has_static_method(StringObject* name) { return static_funcs.find(name) != static_funcs.end(); }

        static TypeObject* create(const std::string& type_name) { return new TypeObject(type_name); }

        static TypeObject* create() { return new TypeObject(); }

        std::vector<GCObject*> get_referenced_objects() const override;

    private:
        std::string type_name;
        std::unordered_map<StringObject*, TypeField> fields;
        std::unordered_map<StringObject*, FunctionObject*> member_funcs;
        std::unordered_map<StringObject*, FunctionObject*> static_funcs;
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

        bool is_gc_object() const {
            static std::unordered_set<ValueType> types = {
                    ValueType::Type, ValueType::String, ValueType::Function,
                    ValueType::Array, ValueType::Object, ValueType::Module};

            return types.find(type) != types.end();
        }

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

    inline PrimValue default_value(TypeObject* type_info) {
        if (type_info == TypeObject::bool_()) {
            return PrimValue::from_bool(false);
        } else if (type_info == TypeObject::int_()) {
            return PrimValue::from_i32(0);
        } else if (type_info == TypeObject::float_()) {
            return PrimValue::from_f64(0.0);
        } else if (type_info == TypeObject::gc_string()) {
            return PrimValue::from_string("");
        } else {
            return PrimValue::null();
        }
    }

    class FrozenContextObject : public GCObject {
    public:
        FrozenContextObject(const std::vector<SharedStackFrameRef>& ctx) : stack_frames(ctx) {}

        FrozenContextObject() = default;

        size_t get_object_size() const override;

        std::vector<GCObject*> get_referenced_objects() const override;

        std::optional<PrimValue> query(StringObject* identifier) const;

        void set_stack_frame(const std::vector<SharedStackFrameRef>& ctx) { stack_frames = ctx; }

        void set_next(FrozenContextObject* next) { this->next = next; }

        FrozenContextObject* get_next() const { return next; }

    private:
        std::vector<SharedStackFrameRef> stack_frames;

        FrozenContextObject* next = nullptr;
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
            func->is_method = false;
            func->begin_offset = 0;
            func->arity = 1;
            return func;
        }

        static FunctionObject* create_function(size_t begin_offset, size_t module_id, size_t arity) {
            auto* func = new FunctionObject();
            func->is_native = false;
            func->is_method = false;
            func->native_function = nullptr;
            func->arity = arity;

            func->begin_offset = begin_offset;
            func->module_id = module_id;
            return func;
        }

        static FunctionObject* create_method(size_t begin_offset, size_t module_id, size_t arity) {
            auto* func = new FunctionObject();
            func->is_native = false;
            func->is_method = true;
            func->native_function = nullptr;
            func->arity = arity;

            func->begin_offset = begin_offset;
            func->module_id = module_id;
            return func;
        }

        bool is_native_function() const { return is_native; }

        bool is_method_function() const { return is_method; }

        PrimValue call_native(std::vector<PrimValue> args) { return native_function(std::move(args)); };

        size_t get_begin_offset() const { return begin_offset; }

        size_t get_module_id() const { return module_id; }

        size_t get_arity() const { return arity; }

        void set_context(FrozenContextObject* ctx) { this->ctx = ctx; }

        FrozenContextObject* get_context() const { return ctx; }

        size_t get_object_size() const override {
            // we don't care about the size of the context
            return sizeof(FunctionObject);
        }

        std::vector<GCObject*> get_referenced_objects() const override;

    private:
        bool is_native;
        bool is_method;
        std::function<PrimValue(std::vector<PrimValue>)> native_function;
        size_t arity;

        size_t begin_offset;
        size_t module_id;

        FrozenContextObject* ctx;
    };

    class ArrayObject : public GCObject {
    public:
        std::vector<GCObject*> get_referenced_objects() const override;

        ArrayObject(size_t size, TypeObject* element_type)
            : data(new PrimValue[size]), size(size),
              element_type_info(element_type) {
            for (size_t i = 0; i < size; i++) {
                data[i] = PrimValue::null();
            }
        }

        ArrayObject(size_t size, PrimValue* data, TypeObject* element_type) : ArrayObject(size, element_type) {
            std::copy(data, data + size, this->data);
        }

        ~ArrayObject() { delete[] data; }

        std::string to_string() const override {
            std::stringstream ss;
            ss << "[";
            for (size_t i = 0; i < size; i++) {
                ss << data[i].to_string();
            }
            ss << "]";
            return ss.str();
        }

        size_t get_size() const { return size; }

        PrimValue* get_data() const { return data; }

        PrimValue get_element(size_t index) const { return data[index]; }

        PrimValue& get_element_ref(size_t index) const { return data[index]; }

        TypeObject* get_element_type() const { return element_type_info; }

        size_t get_object_size() const override;

    private:
        PrimValue* data;
        size_t size;

        TypeObject* element_type_info;
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