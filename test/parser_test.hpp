#pragma once

#include "test_helper.hpp"
#include "parser.hpp"
#include "ir.hpp"

namespace parser_test {
    inline luaxc::IRInterpreter compile_run(const std::string& input) {
        auto lexer = luaxc::Lexer(input);
        auto parser = luaxc::Parser(lexer);
        auto program = parser.parse_program();
        auto ir_generator = luaxc::IRGenerator(std::move(program));
        auto byte_code = ir_generator.generate();
        std::cout << luaxc::dump_bytecode(byte_code) << std::endl;

        auto ir_interpreter = luaxc::IRInterpreter(std::move(byte_code));
        ir_interpreter.run();

        return ir_interpreter;
    }

    inline void test_declaration() {
        auto ir_interpreter = compile_run("let a = 1 + 2 * (3 + 4);");
        auto value = std::get<int32_t>(ir_interpreter.retrieve_value("a"));
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_declaration_no_initializer() {
        auto ir_interpreter = compile_run("let a;");
        assert(ir_interpreter.has_identifier("a"));
    }

    inline void test_assignment() {
        auto ir_interpreter = compile_run("let a = 1; a = 1 + 2 * (3 + 4);");
        auto value = std::get<int32_t>(ir_interpreter.retrieve_value("a"));
        assert(value == 1 + 2 * (3 + 4));
    }

    inline void test_arithmetic_with_identifier() {
        auto ir_interpreter = compile_run("let a = 1; let b = 2; let c = a + b;");
        auto value = std::get<int32_t>(ir_interpreter.retrieve_value("c"));
        assert(value == 1 + 2);
    }

    inline void test_if_statement() {
        auto ir_interpreter = compile_run("let a = 1; if (a > 0) { let b = 2; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("b")) == 2);
    }

    inline void test_if_statement_false() {
        auto ir_interpreter = compile_run("let a = 1; if (a < 0) { let b = 2; }");
        assert(ir_interpreter.has_identifier("b") == false);
    }

    inline void test_if_statement_with_else() {
        auto ir_interpreter = 
            compile_run("let a = 1; if (a < 0) { let b = 2; } else { let c = 3; }");
        assert(ir_interpreter.has_identifier("b") == false);
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("c")) == 3);
    }

    inline void test_if_statement_else_if() {
        auto ir_interpreter = 
            compile_run("let a = 1; if (a < 0) { let b = 2; } "
                "else if (a > 0) { let c = 3; } else { let d = 4; }");
        assert(ir_interpreter.has_identifier("b") == false);
        assert(ir_interpreter.has_identifier("d") == false);
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("c")) == 3);
    }

    inline void test_if_statement_const_condition() { 
        auto ir_interpreter = 
            compile_run("if (1) { let a = 1; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 1);
    }

    inline void test_if_statement_const_false_condition() { 
        auto ir_interpreter = 
            compile_run("if (0) { let a = 1; }");
        assert(ir_interpreter.has_identifier("a") == false);
    }

    inline void test_if_statement_const_expr_condition() { 
        auto ir_interpreter = 
            compile_run("if (1 + 2) { let a = 1; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 1);
    }

    inline void test_if_statement_const_expr_false_condition() { 
        auto ir_interpreter = 
            compile_run("if (1 - 1) { let a = 1; } else { let a = 0; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 0);
    }

    inline void test_while_statement() { 
        auto ir_interpreter = 
            compile_run("let a = 0; while (a < 10) { a = a + 1; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 10);
    }

    inline void test_while_statement_break() { 
        auto ir_interpreter = 
            compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { break; } }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 5);
    }

    inline void test_while_statement_continue() { 
        auto ir_interpreter = 
            compile_run("let a = 0; while (a < 10) { a = a + 1; if (a == 5) { continue; } }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 10);
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
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 7);
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("b")) == 12);
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
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("sum")) == 50);
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
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("sum")) == 30);
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
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("c")) == 0);
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("d")) == 1);
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("e")) == 1);
    }

    inline void run_parser_test() {
        begin_test("parser-basics") {
            test(test_declaration);
            test(test_declaration_no_initializer);
            test(test_assignment);
            test(test_arithmetic_with_identifier);
            test(test_logical_operators);
        } end_test()

        begin_test("parser-if stmt") {
            test(test_if_statement);
            test(test_if_statement_false);
            test(test_if_statement_with_else);
            test(test_if_statement_else_if);
            test(test_if_statement_const_condition);
            test(test_if_statement_const_false_condition);
            test(test_if_statement_const_expr_condition);
            test(test_if_statement_const_expr_false_condition);
        } end_test()

        begin_test("parser-while stmt") {
            test(test_while_statement);
            test(test_while_statement_break);
            test(test_while_statement_continue);
            test(test_while_statement_mixed);
            test(test_while_statement_nested);
            test(test_while_statement_nested_break);
        } end_test()
    }
}