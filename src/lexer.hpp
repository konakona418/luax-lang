#pragma once

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

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
    }// namespace error

    enum class TokenType {
        INVALID,
        TERMINATOR,// eof

        NUMBER,

        PLUS,  // symbol: +
        MINUS, // symbol: -
        MUL,   // symbol: *
        DIV,   // symbol: /
        MOD,   // symbol: %
        ASSIGN,// symbol: =

        BITWISE_AND,// symbol: &
        BITWISE_OR, // symbol: |
        BITWISE_XOR,// symbol: ^
        BITWISE_NOT,// symbol: ~

        AT,// symbol: @

        EQUAL,              // symbol: ==
        NOT_EQUAL,          // symbol: !=
        LESS_THAN_EQUAL,    // symbol: <=
        GREATER_THAN_EQUAL, // symbol: >=
        INCREMENT,          // symbol: ++
        DECREMENT,          // symbol: --
        INCREMENT_BY,       // symbol: +=
        DECREMENT_BY,       // symbol: -=
        BITWISE_SHIFT_LEFT, // symbol: <<
        BITWISE_SHIFT_RIGHT,// symbol: >>
        LOGICAL_AND,        // symbol: &&
        LOGICAL_OR,         // symbol: ||
        LOGICAL_NOT,        // symbol: !

        MODULE_ACCESS,// symbol: ::

        KEYWORD_LET,
        KEYWORD_USE,
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
        KEYWORD_TYPE,
        KEYWORD_FIELD,
        KEYWORD_METHOD,
        KEYWORD_MOD,
        KEYWORD_NULL,
        KEYWORD_TRUE,
        KEYWORD_FALSE,
        KEYWORD_RULE,
        KEYWORD_CONSTRAINT,

        L_PARENTHESIS,// symbol: ()
        R_PARENTHESIS,

        L_SQUARE_BRACE,// symbol: []
        R_SQUARE_BRACE,

        L_CURLY_BRACKET,// symbol: {}
        R_CURLY_BRACKET,

        LESS_THAN,   // symbol: <
        GREATER_THAN,// symbol: >

        COMMA,// symbol: ,

        COLON,    // symbol: :
        SEMICOLON,// symbol: ;

        DOT,// symbol: .

        IDENTIFIER,

        STRING_LITERAL,
    };

    struct Token {
        TokenType type;
        std::string value;
    };

    class Lexer {
    public:
        Lexer() : input(""), pos(0), len(0) {}

        Lexer(std::string input, const std::string& filename) : input(input), pos(0), len(input.size()) {
            statistics.line = 1;
            statistics.column = 1;
            statistics.filename = filename;

            last_cached_statistics = statistics;
        }

        std::vector<Token> lex();

        Token next();

        void set_input(std::string input, const std::string& filename) {
            this->input = input;
            this->pos = 0;
            this->len = input.size();

            statistics.line = 1;
            statistics.column = 1;
            statistics.filename = filename;

            last_cached_statistics = statistics;
        }

        std::pair<size_t, size_t> get_line_and_column() const { return {statistics.line, statistics.column}; }

        std::string get_filename() const { return statistics.filename; }

        struct Statistics {
            size_t line;
            size_t column;
            std::string filename;
        };

        Statistics get_cached_statistics() const { return last_cached_statistics; }

    private:
        std::string input;
        size_t pos;
        size_t len;

        Statistics statistics;
        Statistics last_cached_statistics;

        void set_statistics_next_line();

        void advance();

        bool is_eof() { return pos >= len; }

        bool is_peek_eof() { return pos + 1 >= len; }

        char current_char() {
            assert(pos <= len);
            return input[pos];
        }

        char peek() {
            assert(pos + 1 <= len);
            return input[pos + 1];
        }

        void skip_whitespace();

        void skip_comment();

        Token get_digit();

        Token get_identifier_or_keyword();

        Token get_string_literal();

        Token get_next_token();

        bool is_keyword(const std::string& candidate);

        TokenType keyword_type(const std::string& candidate);
    };
}// namespace luaxc