#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <sstream>

namespace luaxc {
    namespace error {
        class LexerError : public std::exception {
        public:
            LexerError(std::string message, int line, int column) : line(line), column(column) {
                std::stringstream ss;
                ss << "LexerError: " << message << " at line " << line << ", column " << column;
                this->message = ss.str();
            }

            std::string message;
            int line;
            int column;

            const char* what() const noexcept override { 
                return message.c_str();
            }
        };
    }

    enum class TokenType {
        INVALID,
        TERMINATOR, // eof

        NUMBER,

        PLUS, // symbol: +
        MINUS, // symbol: -
        MUL, // symbol: *
        DIV, // symbol: /
        MOD, // symbol: %
        ASSIGN, // symbol: =

        BITWISE_AND, // symbol: &
        BITWISE_OR, // symbol: |
        BITWISE_XOR, // symbol: ^
        BITWISE_NOT, // symbol: ~

        EQUAL, // symbol: ==
        NOT_EQUAL, // symbol: !=
        LESS_THAN_EQUAL, // symbol: <=
        GREATER_THAN_EQUAL, // symbol: >=
        INCREMENT, // symbol: ++
        DECREMENT, // symbol: --
        INCREMENT_BY, // symbol: +=
        DECREMENT_BY, // symbol: -=
        BITWISE_SHIFT_LEFT, // symbol: <<
        BITWISE_SHIFT_RIGHT, // symbol: >>
        LOGICAL_AND, // symbol: &&
        LOGICAL_OR, // symbol: ||
        LOGICAL_NOT, // symbol: !

        KEYWORD_LET,
        KEYWORD_CONST,
        KEYWORD_IF,
        KEYWORD_ELSE,
        KEYWORD_ELIF,
        KEYWORD_FOR,
        KEYWORD_WHILE,
        KEYWORD_DO,
        KEYWORD_BREAK,
        KEYWORD_CONTINUE,
        KEYWORD_RETURN,
        KEYWORD_FUNC,

        L_PARENTHESIS, // symbol: ()
        R_PARENTHESIS,

        L_SQUARE_BRACE, // symbol: []
        R_SQUARE_BRACE,

        L_CURLY_BRACKET, // symbol: {}
        R_CURLY_BRACKET,

        LESS_THAN, // symbol: <
        GREATER_THAN, // symbol: >

        COMMA, // symbol: ,

        COLON, // symbol: :
        SEMICOLON, // symbol: ;

        DOT, // symbol: .

        IDENTIFIER,

        STRING_LITERAL,
    };

    struct Token {
        TokenType type;
        std::string value;
    };

    class Lexer {
    public:
        Lexer(std::string input) : input(input), pos(0), len(input.size()) {
            statistics.line = 1;
            statistics.column = 1;
        }

        std::vector<Token> lex();

        Token next();

        std::pair<size_t, size_t> get_line_and_column() const { return {statistics.line, statistics.column}; }
    private:
        std::string input;
        size_t pos;
        size_t len;

        struct {
            size_t line;
            size_t column;
        } statistics;

        void set_statistics_next_line();

        void advance();

        bool is_eof() { return pos >= len; }

        bool is_peek_eof() { return pos + 1 >= len; }

        char current_char() { assert(pos <= len); return input[pos]; }

        char peek() { assert(pos + 1 <= len); return input[pos + 1]; }

        void skip_whitespace();

        void skip_comment();

        Token get_digit();

        Token get_identifier_or_keyword();

        Token get_string_literal();

        Token get_next_token();

        bool is_keyword(const std::string& candidate);

        TokenType keyword_type(const std::string& candidate);
    };
}