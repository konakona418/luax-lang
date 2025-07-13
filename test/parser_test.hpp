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

    inline void run_parser_test() {
        begin_test("parser") {
            test(test_declaration);
            test(test_declaration_no_initializer)
            test(test_assignment);
            test(test_arithmetic_with_identifier);
        } end_test()
    }
}