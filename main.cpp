#include "lexer_test.hpp"
#include "parser_test.hpp"
#include <iostream>

int main(int, char**) {
    std::cout << "Hello, from luaxc!\n";
    lexer_test::run_lexer_test();
    parser_test::run_parser_test();
}
