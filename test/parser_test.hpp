#pragma once

#include "ir.hpp"
#include "parser.hpp"
#include "test_helper.hpp"

namespace parser_test {
    inline luaxc::IRInterpreter compile_run(const std::string& input) {
        auto lexer = luaxc::Lexer(input);
        auto parser = luaxc::Parser(lexer);
        auto program = parser.parse_program();
        auto runtime = luaxc::IRRuntime();

        runtime.compile(std::move(program));

        auto byte_code = runtime.get_byte_code();
        std::cout << luaxc::dump_bytecode(byte_code) << std::endl;

        runtime.run();

        auto ir_interpreter = runtime.get_interpreter();

        return ir_interpreter;
    }

    inline void test_declaration() {
        auto ir_interpreter = compile_run("let a = 1 + 2 * (3 + 4);");
        auto value = ir_interpreter.retrieve_value<luaxc::Int>("a");
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_declaration_no_initializer() {
        auto ir_interpreter = compile_run("let a;");
        assert(ir_interpreter.has_identifier("a"));
    }

    inline void test_multiple_declarations() {
        auto ir_interpreter = compile_run("let a, b, c;");
        assert(ir_interpreter.has_identifier("a"));
        assert(ir_interpreter.has_identifier("b"));
        assert(ir_interpreter.has_identifier("c"));
    }

    inline void test_assignment() {
        auto ir_interpreter = compile_run("let a = 1; a = 1 + 2 * (3 + 4);");
        auto value = ir_interpreter.retrieve_value<luaxc::Int>("a");
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_arithmetic_with_identifier() {
        auto ir_interpreter = compile_run("let a = 1; let b = 2; let c = a + b;");
        auto value = ir_interpreter.retrieve_value<luaxc::Int>("c");
        assert(value == 1 + 2);
    }

    inline void test_unary_operator_logical_not() {
        auto ir_interpreter = compile_run("let a = 1; if (!(a == 1)) { let b = 2; } else { let b = 3; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("b") == 3);
    }

    inline void test_unary_operator_minus() {
        auto ir_interpreter = compile_run("let a = -1;");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == -1);
    }

    inline void test_combinative_assignment() {
        auto ir_interpreter = compile_run("let a = 1; a += 1;");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 2);
    }

    inline void test_if_statement() {
        auto ir_interpreter = compile_run("let a = 1; if (a > 0) { let b = 2; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("b") == 2);
    }

    inline void test_if_statement_false() {
        auto ir_interpreter = compile_run("let a = 1; if (a < 0) { let b = 2; }");
        assert(ir_interpreter.has_identifier("b") == false);
    }

    inline void test_if_statement_with_else() {
        auto ir_interpreter =
                compile_run("let a = 1; if (a < 0) { let b = 2; } else { let c = 3; }");
        assert(ir_interpreter.has_identifier("b") == false);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("c") == 3);
    }

    inline void test_if_statement_else_if() {
        auto ir_interpreter =
                compile_run("let a = 1; if (a < 0) { let b = 2; } "
                            "else if (a > 0) { let c = 3; } else { let d = 4; }");
        assert(ir_interpreter.has_identifier("b") == false);
        assert(ir_interpreter.has_identifier("d") == false);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("c") == 3);
    }

    inline void test_if_statement_const_condition() {
        auto ir_interpreter =
                compile_run("if (1) { let a = 1; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 1);
    }

    inline void test_if_statement_const_false_condition() {
        auto ir_interpreter =
                compile_run("if (0) { let a = 1; }");
        assert(ir_interpreter.has_identifier("a") == false);
    }

    inline void test_if_statement_const_expr_condition() {
        auto ir_interpreter =
                compile_run("if (1 + 2) { let a = 1; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 1);
    }

    inline void test_if_statement_const_expr_false_condition() {
        auto ir_interpreter =
                compile_run("if (1 - 1) { let a = 1; } else { let a = 0; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 0);
    }

    inline void test_while_statement() {
        auto ir_interpreter =
                compile_run("let a = 0; while (a < 10) { a = a + 1; }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 10);
    }

    inline void test_while_statement_break() {
        auto ir_interpreter =
                compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { break; } }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 5);
    }

    inline void test_while_statement_continue() {
        auto ir_interpreter =
                compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { continue; } }");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 10);
    }

    inline void test_while_statement_mixed() {
        auto ir_interpreter =
                compile_run(
                        "let a = 0; let b = 0; "
                        "while (a < 10) { "
                        "if (a == 5) { a = a + 1; continue; } "
                        "a = a + 1; b = b + 2; "
                        "if (a == 7) { break;} "
                        "}");
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 7);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("b") == 12);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("sum") == 50);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("sum") == 30);
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
        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("c") == 0);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("d") == 1);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("e") == 1);
    }

    inline void test_for_loop() {
        std::string input = R"(
        let a = 0;
        for (let i = 0; i < 10; i += 1) {
            a = a + i;
        }
        )";
        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 45);
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
        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 10);
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
        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("a") == 40);
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

        auto ir_interpreter = compile_run(input);
    }

    inline void test_function_invocation_multiple_args() {
        std::string input = R"(
        use println;
        let a = 114514;
        let b = 1919;
        let c = 2;
        let unit = println(a, b, 800 + c * (1 + 4));
        )";

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::UnitObject>("unit") == luaxc::UnitObject{});
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("result") == 3);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("result") == 3);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("result") == 12);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("result") == 3);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("result") == 3);
    }

    inline void test_string_literal() {
        std::string input = R"(
        use println;
        let str = "hello world!";
        println(str);
        )";

        auto ir_interpreter = compile_run(input);
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

        auto ir_interpreter = compile_run(input);
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

        auto ir_interpreter = compile_run(input);
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

        auto ir_interpreter = compile_run(input);
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

        auto ir_interpreter = compile_run(input);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("x") == 1);
        assert(ir_interpreter.retrieve_value<luaxc::Int>("y") == 3);
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
        }
        end_test()
    }
}// namespace parser_test