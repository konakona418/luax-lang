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

            return PrimValue(ValueType::Array, (GCObject*){array});
        });
        runtime.gc_regist_no_collect(array_type);
        auto* array_identifier = runtime.push_string_pool_if_not_exists("__builtin_typings_array_of");
        result.emplace_back(array_identifier, PrimValue(ValueType::Function, array_type));

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
            runtime.gc_collect();
            return PrimValue::unit();
        });
        runtime.gc_regist_no_collect(runtime_abort);
        auto* runtime_abort_identifier = runtime.push_string_pool_if_not_exists("__builtin_runtime_abort");
        result.emplace_back(runtime_abort_identifier, PrimValue(ValueType::Function, runtime_abort));

        return result;
    }
}// namespace luaxc