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

    inline void test_if_statement_const_expr_condition() { 
        auto ir_interpreter = 
            compile_run("if (1 + 2) { let a = 1; }");
        assert(std::get<int32_t>(ir_interpreter.retrieve_value("a")) == 1);
    }

    inline void run_parser_test() {
        begin_test("parser-basics") {
            test(test_declaration);
            test(test_declaration_no_initializer)
            test(test_assignment);
            test(test_arithmetic_with_identifier);
        } end_test()

        begin_test("parser-if stmt") {
            test(test_if_statement);
            test(test_if_statement_false);
            test(test_if_statement_with_else);
            test(test_if_statement_else_if);
            test(test_if_statement_const_condition);
            test(test_if_statement_const_expr_condition);
        } end_test()
    }
}