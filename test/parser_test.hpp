#pragma once

#include "test_helper.hpp"
#include "parser.hpp"
#include "ir.hpp"

namespace parser_test {
    inline void test_basic_assign() {
        std::string input = "let a = 1 + 2 * (3 + 4);";
        auto lexer = luaxc::Lexer(input);
        luaxc::Parser parser(lexer);
        auto program = parser.parse_program();
        luaxc::IRGenerator ir_generator(std::move(program));
        auto byte_code = ir_generator.generate();

        auto ir_interpreter = luaxc::IRInterpreter(std::move(byte_code));
        ir_interpreter.run();
        auto value = std::get<int32_t>(ir_interpreter.retrieve_value("a"));
        assert(value == 1 + 2 * (3 + 4));
    }



    inline void run_parser_test() {
        begin_test("parser") {
            test(test_basic_assign);
        } end_test()
    }
}