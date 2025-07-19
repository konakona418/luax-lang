#pragma once

#include "ir.hpp"
#include "test_helper.hpp"

namespace parser_test {
    inline luaxc::IRRuntime compile_run(const std::string& input) {
        auto runtime = luaxc::IRRuntime();

        runtime.compile(input);

        auto byte_code = runtime.get_byte_code();
        std::cout << luaxc::dump_bytecode(byte_code) << std::endl;

        runtime.run();

        return runtime;
    }

    inline void test_declaration() {
        auto runtime = compile_run("let a = 1 + 2 * (3 + 4);");
        auto value = runtime.retrieve_value<luaxc::Int>("a");
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_declaration_no_initializer() {
        auto runtime = compile_run("let a;");
        assert(runtime.has_identifier("a"));
    }

    inline void test_multiple_declarations() {
        auto runtime = compile_run("let a, b, c;");
        assert(runtime.has_identifier("a"));
        assert(runtime.has_identifier("b"));
        assert(runtime.has_identifier("c"));
    }

    inline void test_assignment() {
        auto runtime = compile_run("let a = 1; a = 1 + 2 * (3 + 4);");
        auto value = runtime.retrieve_value<luaxc::Int>("a");
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_arithmetic_with_identifier() {
        auto runtime = compile_run("let a = 1; let b = 2; let c = a + b;");
        auto value = runtime.retrieve_value<luaxc::Int>("c");
        assert(value == 1 + 2);
    }

    inline void test_unary_operator_logical_not() {
        auto runtime = compile_run("let a = 1; if (!(a == 1)) { let b = 2; } else { let b = 3; }");
        assert(runtime.retrieve_value<luaxc::Int>("b") == 3);
    }

    inline void test_unary_operator_minus() {
        auto runtime = compile_run("let a = -1;");
        assert(runtime.retrieve_value<luaxc::Int>("a") == -1);
    }

    inline void test_combinative_assignment() {
        auto runtime = compile_run("let a = 1; a += 1;");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 2);
    }

    inline void test_if_statement() {
        auto runtime = compile_run("let a = 1; if (a > 0) { let b = 2; }");
        assert(runtime.retrieve_value<luaxc::Int>("b") == 2);
    }

    inline void test_if_statement_false() {
        auto runtime = compile_run("let a = 1; if (a < 0) { let b = 2; }");
        assert(runtime.has_identifier("b") == false);
    }

    inline void test_if_statement_with_else() {
        auto runtime =
                compile_run("let a = 1; if (a < 0) { let b = 2; } else { let c = 3; }");
        assert(runtime.has_identifier("b") == false);
        assert(runtime.retrieve_value<luaxc::Int>("c") == 3);
    }

    inline void test_if_statement_else_if() {
        auto runtime =
                compile_run("let a = 1; if (a < 0) { let b = 2; } "
                            "else if (a > 0) { let c = 3; } else { let d = 4; }");
        assert(runtime.has_identifier("b") == false);
        assert(runtime.has_identifier("d") == false);
        assert(runtime.retrieve_value<luaxc::Int>("c") == 3);
    }

    inline void test_if_statement_const_condition() {
        auto runtime =
                compile_run("if (1) { let a = 1; }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 1);
    }

    inline void test_if_statement_const_false_condition() {
        auto runtime =
                compile_run("if (0) { let a = 1; }");
        assert(runtime.has_identifier("a") == false);
    }

    inline void test_if_statement_const_expr_condition() {
        auto runtime =
                compile_run("if (1 + 2) { let a = 1; }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 1);
    }

    inline void test_if_statement_const_expr_false_condition() {
        auto runtime =
                compile_run("if (1 - 1) { let a = 1; } else { let a = 0; }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 0);
    }

    inline void test_while_statement() {
        auto runtime =
                compile_run("let a = 0; while (a < 10) { a = a + 1; }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 10);
    }

    inline void test_while_statement_break() {
        auto runtime =
                compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { break; } }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 5);
    }

    inline void test_while_statement_continue() {
        auto runtime =
                compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { continue; } }");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 10);
    }

    inline void test_while_statement_mixed() {
        auto runtime =
                compile_run(
                        "let a = 0; let b = 0; "
                        "while (a < 10) { "
                        "if (a == 5) { a = a + 1; continue; } "
                        "a = a + 1; b = b + 2; "
                        "if (a == 7) { break;} "
                        "}");
        assert(runtime.retrieve_value<luaxc::Int>("a") == 7);
        assert(runtime.retrieve_value<luaxc::Int>("b") == 12);
    }

    inline void test_while_statement_nested() {
        std::string input = R"(
        let a = 0;
        let b = 0;
        let sum = 0;
        while (a < 10) {
            b = 0;
            while (b < 5) {
                sum = sum + 1;
                b = b + 1;
            }
            a = a + 1;
        })";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("sum") == 50);
    }

    inline void test_while_statement_nested_break() {
        std::string input = R"(
        let a = 0;
        let b = 0;
        let sum = 0;
        while (a < 10) {
            b = 0;
            while (b < 5) {
                if (b == 3) {
                    break;
                }
                sum = sum + 1;
                b = b + 1;
            }
            a = a + 1;
        })";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("sum") == 30);
    }

    inline void test_logical_operators() {
        std::string input = R"(
        let a = 1;
        let b = 0;
        if (a && b) {
            let c = 1;
        } else {
            let c = 0;
        }
        if (a == 1 && b == 0) {
            let d = 1;
        } else {
            let d = 0;
        }
        if (a == 1 || b) { 
            let e = 1;
        } else { 
            let e = 0;
        }
        )";
        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("c") == 0);
        assert(runtime.retrieve_value<luaxc::Int>("d") == 1);
        assert(runtime.retrieve_value<luaxc::Int>("e") == 1);
    }

    inline void test_for_loop() {
        std::string input = R"(
        let a = 0;
        for (let i = 0; i < 10; i += 1) {
            a = a + i;
        }
        )";
        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("a") == 45);
    }

    inline void test_for_loop_break() {
        std::string input = R"(
        let a = 0;
        for (let i = 0; i < 10; i += 1) {
            if (i == 5) {
                break;
            }
            a = a + i;
        }
        )";
        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("a") == 10);
    }

    inline void test_for_loop_continue() {
        std::string input = R"(
        let a = 0;
        for (let i = 0; i < 10; i += 1) {
            if (i == 5) {
                continue;
            }
            a = a + i;
        }
        )";
        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("a") == 40);
    }

    inline void test_scopes_error_check() {
        std::string input = R"(
        {
            let a = 0;;
        }
        a = 1;
        )";
        bool caught_error = false;
        try {
            compile_run(input);
        } catch (std::exception& e) {
            caught_error = true;
            std::cout << e.what() << std::endl;
        }
        assert(caught_error);
    }


    inline void test_function_invocation() {
        std::string input = R"(
        use println;
        let a = 114514;
        println(a);
        )";

        auto runtime = compile_run(input);
    }

    inline void test_function_invocation_multiple_args() {
        std::string input = R"(
        use println;
        let a = 114514;
        let b = 1919;
        let c = 2;
        let unit = println(a, b, 800 + c * (1 + 4));
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::UnitObject>("unit") == luaxc::UnitObject{});
    }

    inline void test_function_declaration() {
        std::string input = R"(
        use println;
        func add(a, b) {
            return a + b;
        }
        let result = add(1, 2);
        println(result);
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("result") == 3);
    }

    inline void test_function_chain_invoke() {
        std::string input = R"(
        use println;
        func _anonymous(a, b) {
            return a + b;
        }
        func add() {
            return _anonymous;
        }
        let result = add()(1, 2);
        println(result);
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("result") == 3);
    }

    inline void test_multiple_function_declarations() {
        std::string input = R"(
        use println;
        func add(a, b) {
            return a + b;
        }
        func mul(a, b) {
            return a * b;
        }
        let result = mul(add(1, 2), 4);
        println(result);
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("result") == 12);
    }

    inline void test_deferred_function_declarations() {
        std::string input = R"(
        use println;
        func add(a, b);

        let result;

        func main() {
            result = add(1, 2);
            println(result);
        }

        func add(a, b) {
            return a + b;
        }

        main();
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("result") == 3);
    }

    inline void test_nested_function_declaration() {
        std::string input = R"(
        use println;

        let result;
        func main() {
            func add(a, b) {
                return a + b;
            }

            result = add(1, 2);
            println(result);
        }

        main();
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("result") == 3);
    }

    inline void test_string_literal() {
        std::string input = R"(
        use println;
        let str = "hello world!";
        println(str);
        )";

        auto runtime = compile_run(input);
    }

    inline void test_type_decl() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = Int();
        };
        )";

        auto runtime = compile_run(input);
    }

    inline void test_generic_type_factory() {
        std::string input = R"(
        use println;
        use Int;

        func Vector2(TData) {
            return type {
                field x = TData;
                field y = TData;
            };
        }

        let Vector2Int = Vector2(Int());
        )";

        auto runtime = compile_run(input);
    }

    inline void test_object_construction() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = Int();
        };

        let myObject = MyType { x = 1, y = 2, };
        )";

        auto runtime = compile_run(input);
    }

    inline void test_object_member_access_basic() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = Int();
        };

        let myObject = MyType { x = 1, y = 2, };

        let x = myObject.x;
        println(x);

        let y = myObject.y;
        println(myObject.y);

        myObject.y = 3;
        println(myObject.y);
        y = myObject.y;
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("x") == 1);
        assert(runtime.retrieve_value<luaxc::Int>("y") == 3);
    }

    inline void test_object_member_access_nested() {
        std::string input = R"(
        use println;
        use Int;

        let Nested = type { field z = Int(); };

        let MyType = type {
            field x = Int();
            field y = Nested;
        };

        let myObject = MyType { x = 1, y = Nested { z = 2 } };
        println(myObject.y.z);

        let z = myObject.y.z;
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("z") == 2);
    }

    inline void test_object_member_access_anonymous() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = type { field z = Int(); };
        };

        let myObject = MyType { x = 1, y = { z = 2 } };
        println(myObject.y.z);
        
        let z = myObject.y.z;
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("z") == 2);
    }

    inline void test_object_return_from_func() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = Int();
        };

        func makeObject() {
            return MyType { x = 1, y = 2 };
        }

        let myObject = makeObject();
        println(myObject.y);
        
        let y = myObject.y;
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("y") == 2);
    }

    inline void test_generic_type_object() {
        std::string input = R"(
        use println;
        use Int;

        func Vector2(TData) {
            return type {
                field x = TData;
                field y = TData;
            };
        }

        let Vector2Int = Vector2(Int());

        let vec = Vector2Int { x = 1, y = 2 };
        println(vec.x, vec.y);

        let x = vec.x;
        let y = vec.y;
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("x") == 1);
        assert(runtime.retrieve_value<luaxc::Int>("y") == 2);
    }

    inline void test_method() {
        std::string input = R"(
        use println;
        use Int;

        let MyType = type {
            field x = Int();
            field y = Int();

            method foo(self, bar) {
                return self.x + self.y + bar;
            }

            method bar(self, x, y) {
                return self.x * x + self.y * y;
            }

            method me(self) {
                return self;
            }

            method new(self, x, y) {
                self.x = x;
                self.y = y;
            }
        };

        let myObject =  MyType { x = 1, y = 2 };
        println(myObject.me().foo(3));
        
        let bar = myObject.bar(3, 4);

        let newObject = MyType {};
        newObject.new(5, 6);
        println(newObject.x, newObject.y);
        )";

        auto runtime = compile_run(input);
        assert(runtime.retrieve_value<luaxc::Int>("bar") == 11);
    }

    inline void run_parser_test() {
        begin_test("parser-basics") {
            test(test_declaration);
            test(test_declaration_no_initializer);
            test(test_multiple_declarations);
            test(test_assignment);
            test(test_arithmetic_with_identifier);
            test(test_logical_operators);
            test(test_unary_operator_logical_not);
            test(test_unary_operator_minus);
            test(test_combinative_assignment);
            test(test_string_literal)
        }
        end_test();

        begin_test("parser-if stmt") {
            test(test_if_statement);
            test(test_if_statement_false);
            test(test_if_statement_with_else);
            test(test_if_statement_else_if);
            test(test_if_statement_const_condition);
            test(test_if_statement_const_false_condition);
            test(test_if_statement_const_expr_condition);
            test(test_if_statement_const_expr_false_condition);
        }
        end_test();

        begin_test("parser-while stmt") {
            test(test_while_statement);
            test(test_while_statement_break);
            test(test_while_statement_continue);
            test(test_while_statement_mixed);
            test(test_while_statement_nested);
            test(test_while_statement_nested_break);
        }
        end_test();

        begin_test("parser-for stmt") {
            test(test_for_loop);
            test(test_for_loop_break);
            test(test_for_loop_continue);
        }
        end_test();

        begin_test("parser-scopes") {
            test(test_scopes_error_check);
        }
        end_test();

        begin_test("parser-fn invoke") {
            test(test_function_invocation);
            test(test_function_invocation_multiple_args);
            test(test_function_declaration);
            test(test_function_chain_invoke);
            test(test_multiple_function_declarations);
            test(test_deferred_function_declarations);
            test(test_nested_function_declaration);
        }
        end_test();

        begin_test("parser-type") {
            test(test_type_decl);
            test(test_generic_type_factory);
            test(test_object_construction);
            test(test_object_member_access_basic);
            test(test_object_member_access_nested);
            test(test_object_member_access_anonymous);
            test(test_object_return_from_func);
            test(test_generic_type_object);
            test(test_method);
        }
        end_test()
    }
}// namespace parser_test