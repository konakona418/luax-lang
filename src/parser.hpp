#pragma once

#include <sstream>
#include "ast.hpp"

#include "lexer.hpp"

namespace luaxc {

    namespace error {
        class ParserError : public std::exception {
        public:
            std::string message;
            std::size_t line;
            std::size_t column;

            ParserError(std::string message, size_t line, size_t column) : line(line), column(column) {
                std::stringstream ss;
                ss << "ParserError: " << message << " at line " << line << ", column " << column;
                this->message = ss.str();
            }

            const char* what() const noexcept override {
                return message.c_str();
            }
        };
    }

    class Parser {
    public:
        Parser();
        explicit Parser(Lexer& lexer) : lexer(lexer) {
            current_token = lexer.next();
        };

        std::unique_ptr<AstNode> parse_program();

    private:
        Lexer& lexer;
        Token current_token;

        void consume(TokenType expected);

        void consume(TokenType expected, const std::string& err_message);

        void next_token();

        std::unique_ptr<AstNode> parse_statement();

        std::unique_ptr<AstNode> parse_assignment_statement();

        std::unique_ptr<AstNode> parse_declaration_statement();

        std::unique_ptr<AstNode> parse_expression();

        std::unique_ptr<AstNode> parse_additive_expression();

        std::unique_ptr<AstNode> parse_multiplicative_expression();

        std::unique_ptr<AstNode> parse_comparison_expression();

        std::unique_ptr<AstNode> parse_relational_expression();

        std::unique_ptr<AstNode> parse_factor();

        std::unique_ptr<AstNode> parse_block_statement();

        std::unique_ptr<AstNode> parse_if_statement();

        std::unique_ptr<AstNode> parse_while_statement();

        std::unique_ptr<AstNode> parse_break_statement();

        std::unique_ptr<AstNode> parse_continue_statement();
    };
}
