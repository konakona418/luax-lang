#pragma once

#include <sstream>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"

namespace luaxc {

    namespace error {
        class ParserError : public std::exception {
        public:
            std::string message;
            std::size_t line;
            std::size_t column;
            std::string filename;

            ParserError(std::string message, size_t line, size_t column, std::string filename)
                : line(line), column(column), filename(filename) {
                std::stringstream ss;
                ss << "ParserError: " << message << " at line " << line << ", column " << column << " of file " << filename;
                this->message = ss.str();
            }

            const char* what() const noexcept override {
                return message.c_str();
            }
        };
    }// namespace error

    class Parser {
    public:
        enum class ParserState {
            Start,
            InScope,
            InTypeDeclarationScope,
            InModuleDeclarationScope,
            InInitializerListScope,
            InFunctionOrMethodScope,
            InRuleDeclarationScope,
            // todo: add when needed
        };

        Parser();
        explicit Parser(Lexer& lexer) : lexer(lexer) {
            current_token = lexer.next();
        };

        std::unique_ptr<AstNode> parse_program(ParserState init_state = ParserState::Start);

        void reset() {
            scopes.clear();
            current_token = lexer.next();
        }

    private:
        struct ParserStackFrame {
            ParserState state;
            std::unordered_set<std::string> identifiers;

            explicit ParserStackFrame(ParserState state) : state(state) {}
        };

        Lexer& lexer;
        Token current_token;

        std::vector<ParserStackFrame> scopes;

        void consume(TokenType expected);

        void consume(TokenType expected, const std::string& err_message);

        void next_token();

        void enter_scope(ParserState state = ParserState::InScope);

        void exit_scope();

        void declare_identifier(const std::string& identifier);

        bool is_identifier_declared(const std::string& identifier) const;

        bool is_in_scope(ParserState state) const;

        bool is_in_scope_no_propagation(ParserState state) const;

        enum class TypeAnnotationType {
            Args,
            Return,
            Declare,
        };

        void consume_type_annotation(TypeAnnotationType type);

        std::unique_ptr<AstNode> parse_statement();

        std::unique_ptr<AstNode> parse_declaration_statement(bool consume_semicolon = true);

        std::unique_ptr<AstNode> parse_field_declaration_statement();

        std::unique_ptr<AstNode> parse_method_declaration_statement();

        std::unique_ptr<AstNode> parse_function_declaration_statement();

        std::unique_ptr<AstNode> parse_return_statement();

        std::unique_ptr<AstNode> parse_identifier();

        std::unique_ptr<AstNode> parse_expression(bool consume_semicolon = true);

        std::unique_ptr<AstNode> parse_type_declaration_expression();

        std::unique_ptr<AstNode> parse_module_declaration_expression();

        std::unique_ptr<AstNode> parse_module_import_expression();

        std::unique_ptr<AstNode> parse_assignment_expression(bool consume_semicolon = true);

        std::unique_ptr<AstNode> parse_basic_assignment_expression(std::unique_ptr<AstNode> identifier, bool consume_semicolon = true);

        std::unique_ptr<AstNode> parse_combinative_assignment_expression(std::unique_ptr<AstNode> identifier, bool consume_semicolon = true);

        std::unique_ptr<AstNode> parse_simple_expression();

        std::unique_ptr<AstNode> parse_additive_expression();

        std::unique_ptr<AstNode> parse_multiplicative_expression();

        std::unique_ptr<AstNode> parse_comparison_expression();

        std::unique_ptr<AstNode> parse_relational_expression();

        std::unique_ptr<AstNode> parse_logical_and_expression();

        std::unique_ptr<AstNode> parse_logical_or_expression();

        std::unique_ptr<AstNode> parse_bitwise_and_expression();

        std::unique_ptr<AstNode> parse_bitwise_or_expression();

        std::unique_ptr<AstNode> parse_bitwise_shift_expression();

        std::unique_ptr<AstNode> parse_unary_expression();

        std::unique_ptr<AstNode> parse_array_like_member_access_expression(std::unique_ptr<AstNode> initial_expr);

        std::unique_ptr<AstNode> parse_member_access_or_method_invoke_expression(std::unique_ptr<AstNode> initial_expr);

        std::unique_ptr<AstNode> parse_method_invocation_expression(std::unique_ptr<AstNode> initial_expr, std::unique_ptr<AstNode> method_identifier);

        std::unique_ptr<AstNode> parse_module_access_expression(std::unique_ptr<AstNode> initial_expr);

        std::unique_ptr<AstNode> parse_initializer_list_expression(std::unique_ptr<AstNode> type_expr);

        std::unique_ptr<AstNode> parse_trailing_closure_expression();

        std::unique_ptr<AstNode> parse_closure_expression();

        std::unique_ptr<AstNode> parse_simple_closure_expression(std::unique_ptr<AstNode> receiver_alias);

        std::unique_ptr<AstNode> parse_rule_expression();

        std::unique_ptr<AstNode> parse_constraint_expression();

        std::unique_ptr<AstNode> parse_primary();

        std::unique_ptr<AstNode> parse_function_invocation_statement(std::unique_ptr<AstNode> function_identifier);

        std::unique_ptr<AstNode> parse_block_statement();

        std::unique_ptr<AstNode> parse_if_statement();

        std::unique_ptr<AstNode> parse_for_statement();

        std::unique_ptr<AstNode> parse_while_statement();

        std::unique_ptr<AstNode> parse_break_statement();

        std::unique_ptr<AstNode> parse_continue_statement();
    };
}// namespace luaxc
