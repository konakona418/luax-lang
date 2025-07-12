#include "test_helper.hpp"
#include "lexer.hpp"

inline void test_basic_expr() {
    luaxc::Lexer lexer("x = 5");
    
    std::vector<luaxc::Token> tokens = lexer.lex();
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER);
    assert(tokens[0].value == "x");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN);
    assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::NUMBER);
    assert(tokens[2].value == "5");
    assert(tokens.size() == 3);
}

inline void test_basic_assign() {
    luaxc::Lexer lexer("myVar = 123");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER);
    assert(tokens[0].value == "myVar");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN);
    assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::NUMBER);
    assert(tokens[2].value == "123");
}

inline void test_complex_assign() {
    luaxc::Lexer lexer("result = calc(a, b);");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 9);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "result");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN); assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::IDENTIFIER); assert(tokens[2].value == "calc");
    assert(tokens[3].type == luaxc::TokenType::L_PARENTHESIS); assert(tokens[3].value == "(");
    assert(tokens[4].type == luaxc::TokenType::IDENTIFIER); assert(tokens[4].value == "a");
    assert(tokens[5].type == luaxc::TokenType::COMMA); assert(tokens[5].value == ",");
    assert(tokens[6].type == luaxc::TokenType::IDENTIFIER); assert(tokens[6].value == "b");
    assert(tokens[7].type == luaxc::TokenType::R_PARENTHESIS); assert(tokens[7].value == ")");
}


inline void test_integer_numbers() {
    luaxc::Lexer lexer("123 0 98765");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::NUMBER); assert(tokens[0].value == "123");
    assert(tokens[1].type == luaxc::TokenType::NUMBER); assert(tokens[1].value == "0");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "98765");
}

inline void test_float_numbers() {
    luaxc::Lexer lexer("3.14 0.5 123.0");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::NUMBER); assert(tokens[0].value == "3.14");
    assert(tokens[1].type == luaxc::TokenType::NUMBER); assert(tokens[1].value == "0.5");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "123.0");
}

inline void test_scientific_notation() {
    luaxc::Lexer lexer("1e5 1.23e-4 6.02E+23");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::NUMBER); assert(tokens[0].value == "1e5");
    assert(tokens[1].type == luaxc::TokenType::NUMBER); assert(tokens[1].value == "1.23e-4");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "6.02E+23");
}

inline void test_number_suffixes() {
    luaxc::Lexer lexer("123u 42i 3.14f");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::NUMBER); assert(tokens[0].value == "123u");
    assert(tokens[1].type == luaxc::TokenType::NUMBER); assert(tokens[1].value == "42i");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "3.14f");
}

inline void test_simple_string() {
    luaxc::Lexer lexer("\"hello world\"");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 1);
    assert(tokens[0].type == luaxc::TokenType::STRING_LITERAL);
    assert(tokens[0].value == "\"hello world\"");
}

inline void test_string_with_escapes() {
    luaxc::Lexer lexer("\"hello\\nworld\\t\"");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 1);
    assert(tokens[0].type == luaxc::TokenType::STRING_LITERAL);
    assert(tokens[0].value == "\"hello\nworld\t\"");
}

inline void test_empty_string() {
    luaxc::Lexer lexer("\"\"");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 1);
    assert(tokens[0].type == luaxc::TokenType::STRING_LITERAL);
    assert(tokens[0].value == "\"\"");
}

inline void test_keywords() {
    luaxc::Lexer lexer("if else while func return elif let const break continue for");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 11);
    assert(tokens[0].type == luaxc::TokenType::KEYWORD_IF); assert(tokens[0].value == "if");
    assert(tokens[1].type == luaxc::TokenType::KEYWORD_ELSE); assert(tokens[1].value == "else");
    assert(tokens[2].type == luaxc::TokenType::KEYWORD_WHILE); assert(tokens[2].value == "while");
    assert(tokens[3].type == luaxc::TokenType::KEYWORD_FUNC); assert(tokens[3].value == "func");
    assert(tokens[4].type == luaxc::TokenType::KEYWORD_RETURN); assert(tokens[4].value == "return");
    assert(tokens[5].type == luaxc::TokenType::KEYWORD_ELIF); assert(tokens[5].value == "elif");
    assert(tokens[6].type == luaxc::TokenType::KEYWORD_LET); assert(tokens[6].value == "let");
    assert(tokens[7].type == luaxc::TokenType::KEYWORD_CONST); assert(tokens[7].value == "const");
    assert(tokens[8].type == luaxc::TokenType::KEYWORD_BREAK); assert(tokens[8].value == "break");
    assert(tokens[9].type == luaxc::TokenType::KEYWORD_CONTINUE); assert(tokens[9].value == "continue");
    assert(tokens[10].type == luaxc::TokenType::KEYWORD_FOR); assert(tokens[10].value == "for");

}

inline void test_identifiers() {
    luaxc::Lexer lexer("myVariable another_one _temp");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 3);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "myVariable");
    assert(tokens[1].type == luaxc::TokenType::IDENTIFIER); assert(tokens[1].value == "another_one");
    assert(tokens[2].type == luaxc::TokenType::IDENTIFIER); assert(tokens[2].value == "_temp");
}

/*inline void test_utf8_identifiers() {
    luaxc::Lexer lexer("你好变量 myFunc_函数名😊");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 2);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "你好变量");
    assert(tokens[1].type == luaxc::TokenType::IDENTIFIER); assert(tokens[1].value == "myFunc_函数名😊");
}*/

inline void test_single_line_comment() {
    luaxc::Lexer lexer("// This is a comment\nx = 10;");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 4);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "x");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN); assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "10");
    assert(tokens[3].type == luaxc::TokenType::SEMICOLON); assert(tokens[3].value == ";");
}

inline void test_multi_line_comment() {
    luaxc::Lexer lexer("/* This is a\nmulti-line\ncomment */y = 20;");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 4);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "y");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN); assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "20");
    assert(tokens[3].type == luaxc::TokenType::SEMICOLON); assert(tokens[3].value == ";");
}

inline void test_unterminated_multi_line_comment() {
    luaxc::Lexer lexer("/* This comment is not closed");
    bool caught_exception = false;
    try {
        std::vector<luaxc::Token> tokens = lexer.lex();
    } catch (const luaxc::error::LexerError& e) {
        caught_exception = true;
    }
    assert(caught_exception);
}

inline void test_mixed_whitespace() {
    luaxc::Lexer lexer(" a =   1 + 2  \n  ;");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 6);
    assert(tokens[0].type == luaxc::TokenType::IDENTIFIER); assert(tokens[0].value == "a");
    assert(tokens[1].type == luaxc::TokenType::ASSIGN); assert(tokens[1].value == "=");
    assert(tokens[2].type == luaxc::TokenType::NUMBER); assert(tokens[2].value == "1");
    assert(tokens[3].type == luaxc::TokenType::PLUS);
    assert(tokens[4].type == luaxc::TokenType::NUMBER); assert(tokens[4].value == "2");
    assert(tokens[5].type == luaxc::TokenType::SEMICOLON); assert(tokens[5].value == ";");
}

inline void test_all_symbols() {
    luaxc::Lexer lexer(":=,.;()[]{}<>!+-*/%&|^~");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == std::string(":=,.;()[]{}<>!+-*/%&|^~").length());
}

inline void test_operators() {
    luaxc::Lexer lexer("== != <= >= ++ -- += -= << >> && || !");
    std::vector<luaxc::Token> tokens = lexer.lex();

    assert(tokens.size() == 13);

    assert(tokens[0].type == luaxc::TokenType::EQUAL);
    assert(tokens[0].value == "==");

    assert(tokens[1].type == luaxc::TokenType::NOT_EQUAL);
    assert(tokens[1].value == "!=");

    assert(tokens[2].type == luaxc::TokenType::LESS_THAN_EQUAL);
    assert(tokens[2].value == "<=");

    assert(tokens[3].type == luaxc::TokenType::GREATER_THAN_EQUAL);
    assert(tokens[3].value == ">=");

    assert(tokens[4].type == luaxc::TokenType::INCREMENT);
    assert(tokens[4].value == "++");

    assert(tokens[5].type == luaxc::TokenType::DECREMENT);
    assert(tokens[5].value == "--");

    assert(tokens[6].type == luaxc::TokenType::INCREMENT_BY);
    assert(tokens[6].value == "+=");

    assert(tokens[7].type == luaxc::TokenType::DECREMENT_BY);
    assert(tokens[7].value == "-=");

    assert(tokens[8].type == luaxc::TokenType::BITWISE_SHIFT_LEFT);
    assert(tokens[8].value == "<<");

    assert(tokens[9].type == luaxc::TokenType::BITWISE_SHIFT_RIGHT);
    assert(tokens[9].value == ">>");

    assert(tokens[10].type == luaxc::TokenType::LOGICAL_AND);
    assert(tokens[10].value == "&&");

    assert(tokens[11].type == luaxc::TokenType::LOGICAL_OR);
    assert(tokens[11].value == "||");

    assert(tokens[12].type == luaxc::TokenType::LOGICAL_NOT);
    assert(tokens[12].value == "!");
}

inline void test_invalid_character() {
    luaxc::Lexer lexer("@invalid");
    bool caught_exception = false;
    try {
        std::vector<luaxc::Token> tokens = lexer.lex();
    } catch (const luaxc::error::LexerError& e) {
        caught_exception = true;
    }
    assert(caught_exception);
}

inline void run_lexer_test() {
    begin_test("lexer") {
        test(test_basic_expr)
        test(test_basic_assign)
        test(test_complex_assign)
        test(test_integer_numbers)
        test(test_float_numbers)        
        test(test_single_line_comment)
        test(test_multi_line_comment)
        test(test_unterminated_multi_line_comment)

        test(test_mixed_whitespace)
        test(test_all_symbols)
        test(test_operators)

        test(test_invalid_character)

        test(test_identifiers)
        test(test_keywords);
        //test(test_utf8_identifiers)

        test(test_simple_string)
        test(test_string_with_escapes)
        test(test_empty_string)
    } end_test();
}