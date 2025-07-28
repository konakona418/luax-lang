#include "lib.hpp"
#include "ir.hpp"
#include <iostream>

namespace luaxc {

#define __LUAXC_EXTRACT_STRING_FROM_PRIM_VALUE(_prim_value) \
    (static_cast<StringObject*>(static_cast<GCObject*>(_prim_value.get_inner_value<GCObject*>()))->contained_string())

    Functions IO::load(IRRuntime& runtime) {
        Functions result;

        FunctionObject* println = FunctionObject::create_native_function(
                [](std::vector<PrimValue> args) -> PrimValue {
                    for (auto& arg: args) {
                        if (arg.is_string()) {
                            printf("%s ", __LUAXC_EXTRACT_STRING_FROM_PRIM_VALUE(arg).c_str());
                        } else {
                            printf("%s ", arg.to_string().c_str());
                        }
                    }
                    printf("\n");
                    return PrimValue::unit();
                });
        runtime.gc_regist_no_collect(println);
        auto* println_identifier = runtime.push_string_pool_if_not_exists("__builtin_io_println");
        result.emplace_back(println_identifier, PrimValue(ValueType::Function, println));

        FunctionObject* print = FunctionObject::create_native_function(
                [](std::vector<PrimValue> args) -> PrimValue {
                    for (auto& arg: args) {
                        if (arg.is_string()) {
                            printf("%s ", __LUAXC_EXTRACT_STRING_FROM_PRIM_VALUE(arg).c_str());
                        } else {
                            printf("%s ", arg.to_string().c_str());
                        }
                    }
                    return PrimValue::unit();
                });
        runtime.gc_regist_no_collect(print);
        auto* print_identifier = runtime.push_string_pool_if_not_exists("__builtin_io_print");
        result.emplace_back(print_identifier, PrimValue(ValueType::Function, print));

        FunctionObject* readline = FunctionObject::create_native_function(
                [](std::vector<PrimValue> args) -> PrimValue {
                    std::string buffer;
                    std::getline(std::cin, buffer);
                    return PrimValue::from_string(buffer);
                });
        runtime.gc_regist_no_collect(readline);
        auto* readline_identifier = runtime.push_string_pool_if_not_exists("__builtin_io_readline");
        result.emplace_back(readline_identifier, PrimValue(ValueType::Function, readline));

        return result;
    }

#define __LUAXC_MAKE_TYPEING_TYPE(name, fn_name)                                                                         \
    {                                                                                                                    \
        FunctionObject* _type = FunctionObject::create_native_function([&runtime](std::vector<PrimValue>) -> PrimValue { \
            return PrimValue(ValueType::Type, runtime.get_type_info(name));                                              \
        });                                                                                                              \
        runtime.gc_regist_no_collect(_type);                                                                             \
        auto* _identifier = runtime.push_string_pool_if_not_exists(fn_name);                                             \
        result.emplace_back(_identifier, PrimValue(ValueType::Function, _type));                                         \
    }

    Functions Typing::load(IRRuntime& runtime) {
        Functions result;

        __LUAXC_MAKE_TYPEING_TYPE("Any", "__builtin_typings_any")
        __LUAXC_MAKE_TYPEING_TYPE("Int", "__builtin_typings_int")
        __LUAXC_MAKE_TYPEING_TYPE("Float", "__builtin_typings_float")
        __LUAXC_MAKE_TYPEING_TYPE("String", "__builtin_typings_string")
        __LUAXC_MAKE_TYPEING_TYPE("Bool", "__builtin_typings_bool")
        __LUAXC_MAKE_TYPEING_TYPE("Array", "__builtin_typings_array")
        __LUAXC_MAKE_TYPEING_TYPE("Function", "__builtin_typings_function")
        __LUAXC_MAKE_TYPEING_TYPE("Object", "__builtin_typings_object")
        __LUAXC_MAKE_TYPEING_TYPE("Unit", "__builtin_typings_unit_type")
        __LUAXC_MAKE_TYPEING_TYPE("Null", "__builtin_typings_none_type")
        __LUAXC_MAKE_TYPEING_TYPE("Type", "__builtin_typings_type_type")

#undef __LUAXC_MAKE_TYPEING_TYPE

#define __LUAXC_EXTRACT_TYPE_OBJECT(_value) (static_cast<TypeObject*>(_value.get_inner_value<GCObject*>()))
#define __LUAXC_DECLARE_TYPE_MEMBER_FUNCTION(op_name, op)                                                                          \
    {                                                                                                                              \
        FunctionObject* _op = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> IRPrimValue {      \
            if (args.size() != 2) {                                                                                                \
                throw IRInterpreterException("Invalid arg size, reqires at least 2 strings to perform string operation " op_name); \
            }                                                                                                                      \
                                                                                                                                   \
            auto lhs = args[0];                                                                                                    \
            auto rhs = args[1];                                                                                                    \
                                                                                                                                   \
            if (lhs.get_type() != ValueType::Type || rhs.get_type() != ValueType::Type) {                                          \
                throw IRInterpreterException("Invalid arg type, reqires strings");                                                 \
            }                                                                                                                      \
                                                                                                                                   \
            auto* lhs_type = __LUAXC_EXTRACT_TYPE_OBJECT(lhs);                                                                     \
            auto* rhs_type = __LUAXC_EXTRACT_TYPE_OBJECT(rhs);                                                                     \
                                                                                                                                   \
            return PrimValue::from_bool(lhs_type op rhs_type);                                                                     \
        });                                                                                                                        \
        runtime.gc_regist_no_collect(_op);                                                                                         \
        auto* _identifier = runtime.push_string_pool_if_not_exists(op_name);                                                       \
        type_type_info->add_field(_identifier, {TypeObject::function()});                                                          \
        type_type_info->add_method(_identifier, _op);                                                                              \
    }

        auto* type_type_info = runtime.get_type_info("Type");
        __LUAXC_DECLARE_TYPE_MEMBER_FUNCTION("opCompareEqual", ==);
        __LUAXC_DECLARE_TYPE_MEMBER_FUNCTION("opCompareNotEqual", !=);

        for (auto& [_, type_object]: TypeObject::get_all_static_type_info()) {
            runtime.init_type_info(type_object, "Type");
        }

#undef __LUAXC_DECLARE_TYPE_MEMBER_FUNCTION
#undef __LUAXC_EXTRACT_TYPE_OBJECT

        FunctionObject* type_of = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() != 1) {
                throw IRInterpreterException("Invalid arg size");
            }

            return PrimValue(ValueType::Type, args[0].get_type_info());
        });
        runtime.gc_regist_no_collect(type_of);
        auto* type_of_identifier = runtime.push_string_pool_if_not_exists("__builtin_typings_type_of");
        result.emplace_back(type_of_identifier, PrimValue(ValueType::Function, type_of));

        FunctionObject* array_type = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() == 0) {
                throw IRInterpreterException("Invalid arg size");
            }

            auto guard = runtime.gc_guard();

            ArrayObject* array;
            auto& first = args[0];
            if (first.get_type_info() == TypeObject::type()) {
                if (args.size() != 2) {
                    delete array;
                    throw IRInterpreterException("Invalid arg size");
                }

                auto size = args[1].get_inner_value<Int>();

                auto* element_type = static_cast<TypeObject*>(first.get_inner_value<GCObject*>());
                array = runtime.gc_allocate<ArrayObject>(size, element_type);

                for (size_t i = 0; i < size; i++) {
                    array->get_element_ref(i) =
                            default_value(element_type);
                }
            } else {
                auto* candidate_value_type = first.get_type_info();
                array = runtime.gc_allocate<ArrayObject>(args.size(), candidate_value_type);

                for (size_t i = 0; i < args.size(); i++) {
                    if (args[i].get_type_info() != candidate_value_type) {
                        delete array;
                        throw IRInterpreterException("Invalid arg type");
                    }

                    array->get_element_ref(i) = args[i];
                }
            }

            // regist the array prototype
            runtime.init_type_info(array, "Array");

            return PrimValue(ValueType::Array, (GCObject*){array});
        });
        runtime.gc_regist_no_collect(array_type);
        auto* array_identifier = runtime.push_string_pool_if_not_exists("__builtin_typings_array_of");
        result.emplace_back(array_identifier, PrimValue(ValueType::Function, array_type));

        FunctionObject* array_method_size = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() != 1) {
                throw IRInterpreterException("This method must be performed on an array object");
            }

            if (args[0].get_type() != ValueType::Array) {
                throw IRInterpreterException("The argument self is not an array object");
            }

            return PrimValue::from_i64(static_cast<ArrayObject*>(args[0].get_inner_value<GCObject*>())->get_size());
        });
        runtime.gc_regist_no_collect(array_method_size);
        auto* array_method_size_identifier = runtime.push_string_pool_if_not_exists("size");
        auto* array_type_info = runtime.get_type_info("Array");
        array_type_info->add_method(array_method_size_identifier, array_method_size);
        array_type_info->add_field(array_method_size_identifier, TypeObject::TypeField{TypeObject::function()});

        FunctionObject* array_method_op_index_at = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() != 2) {
                throw IRInterpreterException("Invalid args");
            }

            if (args[0].get_type() != ValueType::Array) {
                throw IRInterpreterException("The argument self is not an array object");
            }

            auto* array = static_cast<ArrayObject*>(args[0].get_inner_value<GCObject*>());

            if (args[1].get_type() != ValueType::Int) {
                throw IRInterpreterException("The argument index is not an int");
            }

            auto idx = args[1].get_inner_value<Int>();

            if (idx < 0 || idx >= array->get_size()) {
                LUAXC_GC_THROW_ERROR_EXPR("Index out of bounds");
            }

            return array->get_element(idx);
        });
        runtime.gc_regist_no_collect(array_method_op_index_at);
        auto* array_method_index_at_identifier = runtime.push_string_pool_if_not_exists("opIndexAt");
        array_type_info->add_method(array_method_index_at_identifier, array_method_op_index_at);
        array_type_info->add_field(array_method_index_at_identifier, TypeObject::TypeField{TypeObject::function()});

        FunctionObject* array_method_op_index_assign = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() != 3) {
                throw IRInterpreterException("Invalid args size");
            }

            if (args[0].get_type() != ValueType::Array) {
                throw IRInterpreterException("The argument self is not an array object");
            }

            auto* array = static_cast<ArrayObject*>(args[0].get_inner_value<GCObject*>());

            if (args[1].get_type() != ValueType::Int) {
                throw IRInterpreterException("The argument index is not an int");
            }

            if (args[2].get_type_info() != array->get_element_type()) {
                throw IRInterpreterException("The argument value is not the same type as the array element type");
            }

            auto idx = args[1].get_inner_value<Int>();

            if (idx < 0 || idx >= array->get_size()) {
                LUAXC_GC_THROW_ERROR_EXPR("Index out of bounds");
            }

            array->get_element_ref(idx) = args[2];

            return PrimValue::unit();
        });
        runtime.gc_regist_no_collect(array_method_op_index_assign);
        auto* array_method_index_assign_identifier = runtime.push_string_pool_if_not_exists("opIndexAssign");
        array_type_info->add_method(array_method_index_assign_identifier, array_method_op_index_assign);
        array_type_info->add_field(array_method_index_assign_identifier, TypeObject::TypeField{TypeObject::function()});

        return result;
    }

    Functions Runtime::load(IRRuntime& runtime) {
        Functions result;

        FunctionObject* gc_collect = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) {
            runtime.gc_collect();
            return PrimValue::unit();
        });
        runtime.gc_regist_no_collect(gc_collect);
        auto* gc_collect_identifier = runtime.push_string_pool_if_not_exists("__builtin_runtime_gc_collect");
        result.emplace_back(gc_collect_identifier, PrimValue(ValueType::Function, gc_collect));

        FunctionObject* runtime_abort = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) {
            std::stringstream ss;
            if (args.size() >= 1) {
                for (auto& arg: args) {
                    if (arg.is_string()) {
                        ss << __LUAXC_EXTRACT_STRING_FROM_PRIM_VALUE(arg);
                    } else {
                        ss << arg.to_string();
                    }
                    ss << " ";
                }
            } else {
                ss << "execution aborted by user";
            }
            runtime.abort(ss.str());

            return PrimValue::unit();
        });
        runtime.gc_regist_no_collect(runtime_abort);
        auto* runtime_abort_identifier = runtime.push_string_pool_if_not_exists("__builtin_runtime_abort");
        result.emplace_back(runtime_abort_identifier, PrimValue(ValueType::Function, runtime_abort));

        FunctionObject* runtime_invoke = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() < 1) {
                throw IRInterpreterException("Invalid arg size");
            }

            if (args[0].get_type() != ValueType::Function) {
                throw IRInterpreterException("Invalid arg type");
            }

            auto* fn = static_cast<FunctionObject*>(args[0].get_inner_value<GCObject*>());
            auto args_to_pass = args.size() > 1
                                        ? std::vector<PrimValue>(args.begin() + 1, args.end())
                                        : std::vector<PrimValue>{};

            // add an -1 offset, as the program counter is incremented after the call
            runtime.invoke_function(fn, args_to_pass,
                                    false, -1);

            // returns a never value
            // as this is a buitlin function,
            // and returning unit will not be consumed on time
            // causing unexpected behaviour
            return PrimValue::never();
        });
        runtime.gc_regist_no_collect(runtime_invoke);
        auto* runtime_invoke_identifier = runtime.push_string_pool_if_not_exists("__builtin_runtime_invoke");
        result.emplace_back(runtime_invoke_identifier, PrimValue(ValueType::Function, runtime_invoke));

        return result;
    }

    Functions Constraints::load(IRRuntime& runtime) {
        Functions result;

        FunctionObject* get_constraints = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> PrimValue {
            if (args.size() != 1) {
                throw IRInterpreterException("Invalid arg size");
            }

            auto& rule = args[0];

            if (rule.get_type() != ValueType::Rule) {
                throw IRInterpreterException("Invalid arg type");
            }

            auto constraints = static_cast<RuleObject*>(rule.get_inner_value<GCObject*>())->get_constraints();

            auto guard = runtime.gc_guard();
            auto* array = runtime.gc_allocate<ArrayObject>(constraints.size(), runtime.get_type_info("Function"));
            for (size_t i = 0; i < constraints.size(); i++) {
                array->get_element_ref(i) = PrimValue(ValueType::Function, constraints[i]);
            }

            // init type info
            runtime.init_type_info(array, "Array");

            return PrimValue(ValueType::Array, (GCObject*){array});
        });
        runtime.gc_regist_no_collect(get_constraints);
        auto* get_constraints_identifier = runtime.push_string_pool_if_not_exists("__builtin_constraints_get_constraints");
        result.emplace_back(get_constraints_identifier, PrimValue(ValueType::Function, get_constraints));

#define __LUAXC_LIB_DEFINE_HAS_WHAT(what)                                                                                     \
    {                                                                                                                         \
        FunctionObject* _fn = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> IRPrimValue { \
            if (args.size() != 2) {                                                                                           \
                throw IRInterpreterException("Invalid arg size");                                                             \
            }                                                                                                                 \
            if (args[0].get_type() != ValueType::Type || !args[1].is_string()) {                                              \
                throw IRInterpreterException("Invalid arg type");                                                             \
            }                                                                                                                 \
            auto* type_object = static_cast<TypeObject*>(args[0].get_inner_value<GCObject*>());                               \
            auto* method_name = static_cast<StringObject*>(args[1].get_inner_value<GCObject*>());                             \
            return PrimValue::from_bool(type_object->has_##what(method_name));                                                \
        });                                                                                                                   \
        runtime.gc_regist_no_collect(_fn);                                                                                    \
        auto* _identifier = runtime.push_string_pool_if_not_exists("__builtin_constraints_has_" #what);                       \
        result.emplace_back(_identifier, PrimValue(ValueType::Function, _fn));                                                \
    }

        __LUAXC_LIB_DEFINE_HAS_WHAT(method);
        __LUAXC_LIB_DEFINE_HAS_WHAT(static_method);
        __LUAXC_LIB_DEFINE_HAS_WHAT(field);

#undef __LUAXC_LIB_DEFINE_HAS_WHAT

        return result;
    }

    Functions Strings::load(IRRuntime& runtime) {
        Functions result;

        auto* string_type_info = runtime.get_type_info("String");

#define __LUAXC_EXTRACT_STRING_OBJECT(value) (static_cast<StringObject*>(value.get_inner_value<GCObject*>()))
#define __LUAXC_DECLARE_STRING_MEMBER_FUNCTION(op_name, op)                                                                        \
    {                                                                                                                              \
        FunctionObject* _op = FunctionObject::create_native_function([&runtime](std::vector<PrimValue> args) -> IRPrimValue {      \
            if (args.size() != 2) {                                                                                                \
                throw IRInterpreterException("Invalid arg size, reqires at least 2 strings to perform string operation " op_name); \
            }                                                                                                                      \
                                                                                                                                   \
            auto lhs = args[0];                                                                                                    \
            auto rhs = args[1];                                                                                                    \
                                                                                                                                   \
            if (!lhs.is_string() || !rhs.is_string()) {                                                                            \
                throw IRInterpreterException("Invalid arg type, reqires strings");                                                 \
            }                                                                                                                      \
                                                                                                                                   \
            auto* lhs_str = __LUAXC_EXTRACT_STRING_OBJECT(lhs);                                                                    \
            auto* rhs_str = __LUAXC_EXTRACT_STRING_OBJECT(rhs);                                                                    \
                                                                                                                                   \
            return PrimValue::from_bool(*lhs_str op * rhs_str);                                                                    \
        });                                                                                                                        \
        runtime.gc_regist_no_collect(_op);                                                                                         \
        auto* _identifier = runtime.push_string_pool_if_not_exists(op_name);                                                       \
        string_type_info->add_field(_identifier, {TypeObject::function()});                                                        \
        string_type_info->add_method(_identifier, _op);                                                                            \
    }

        __LUAXC_DECLARE_STRING_MEMBER_FUNCTION("opCompareEqual", ==);
        __LUAXC_DECLARE_STRING_MEMBER_FUNCTION("opCompareNotEqual", !=);

#undef __LUAXC_DECLARE_STRING_MEMBER_FUNCTION

        FunctionObject* string_op_add = FunctionObject ::create_native_function([&runtime](std ::vector<PrimValue> args) -> IRPrimValue {
            auto guard = runtime.gc_guard();

            if (args.size() != 2) {
                throw IRInterpreterException(
                        "Invalid arg size, reqires at least 2 strings to perform string operation opAdd");
            }

            auto lhs = args[0];
            auto rhs = args[1];

            if (!lhs.is_string() || !rhs.is_string()) {
                throw IRInterpreterException("Invalid arg type, reqires strings");
            }

            auto* lhs_str = (static_cast<StringObject*>(lhs.get_inner_value<GCObject*>()));
            auto* rhs_str = (static_cast<StringObject*>(rhs.get_inner_value<GCObject*>()));

            auto* result = *lhs_str + *rhs_str;
            runtime.gc_regist(result);

            runtime.init_type_info(result, "String");

            return PrimValue(ValueType::String, (GCObject*){result});
        });
        runtime.gc_regist_no_collect(string_op_add);
        auto* string_op_add_identifier = runtime.push_string_pool_if_not_exists("opAdd");
        string_type_info->add_field(string_op_add_identifier, {TypeObject ::function()});
        string_type_info->add_method(string_op_add_identifier, string_op_add);

        FunctionObject* string_op_index_at = FunctionObject ::create_native_function([&runtime](std ::vector<PrimValue> args) -> IRPrimValue {
            if (args.size() != 2) {
                throw IRInterpreterException("Invalid args");
            }

            if (args[0].get_type() != ValueType::String) {
                throw IRInterpreterException("The argument self is not an array object");
            }

            auto* string = __LUAXC_EXTRACT_STRING_OBJECT(args[0]);

            if (args[1].get_type() != ValueType::Int) {
                throw IRInterpreterException("The argument index is not an int");
            }

            auto idx = args[1].get_inner_value<Int>();

            if (idx < 0 || idx >= string->get_length()) {
                LUAXC_GC_THROW_ERROR_EXPR("Index out of bounds");
            }

            return PrimValue::from_string(std::string(1, string->c_str()[idx]));
        });
        runtime.gc_regist_no_collect(string_op_index_at);
        auto* string_op_index_at_identifier = runtime.push_string_pool_if_not_exists("opIndexAt");
        string_type_info->add_field(string_op_index_at_identifier, {TypeObject::function()});
        string_type_info->add_method(string_op_index_at_identifier, string_op_index_at);

        FunctionObject* string_op_index_assign = FunctionObject ::create_native_function([&runtime](std ::vector<PrimValue> args) -> IRPrimValue {
            if (args.size() != 3) {
                throw IRInterpreterException("Invalid args");
            }

            if (args[0].get_type() != ValueType::String) {
                throw IRInterpreterException("The argument self is not an array object");
            }

            auto* string = __LUAXC_EXTRACT_STRING_OBJECT(args[0]);

            if (args[1].get_type() != ValueType::Int) {
                throw IRInterpreterException("The argument index is not an int");
            }

            auto idx = args[1].get_inner_value<Int>();

            if (idx < 0 || idx >= string->get_length()) {
                LUAXC_GC_THROW_ERROR_EXPR("Index out of bounds");
            }

            if (!args[2].is_string()) {
                LUAXC_GC_THROW_ERROR_EXPR("The argument replacement is not a string");
            }

            auto* replacement = __LUAXC_EXTRACT_STRING_OBJECT(args[2]);

            if (replacement->get_length() > 1) {
                LUAXC_GC_THROW_ERROR_EXPR("The argument replacement is not a single character");
            }

            string->get_data()[idx] = replacement->c_str()[0];

            return PrimValue::unit();
        });
        runtime.gc_regist_no_collect(string_op_index_assign);
        auto* string_op_index_assign_identifier = runtime.push_string_pool_if_not_exists("opIndexAssign");
        string_type_info->add_field(string_op_index_assign_identifier, {TypeObject::function()});
        string_type_info->add_method(string_op_index_assign_identifier, string_op_index_assign);

        FunctionObject* string_op_size = FunctionObject ::create_native_function([&runtime](std ::vector<PrimValue> args) -> IRPrimValue {
            auto guard = runtime.gc_guard();

            if (args.size() != 1) {
                throw IRInterpreterException("Invalid arguments count");
            }

            if (!args[0].is_string()) {
                throw IRInterpreterException("Invalid arguments type");
            }

            return PrimValue::from_i64(__LUAXC_EXTRACT_STRING_OBJECT(args[0])->get_length());
        });
        runtime.gc_regist_no_collect(string_op_size);
        auto* string_op_size_identifier = runtime.push_string_pool_if_not_exists("size");
        string_type_info->add_field(string_op_size_identifier, {TypeObject::function()});
        string_type_info->add_method(string_op_size_identifier, string_op_size);

        return result;
    }
}// namespace luaxc