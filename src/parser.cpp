#include "parser.hpp"
#include "ast.hpp"

#define LUAXC_PARSER_THROW_ERROR(message) do {  \
        auto line_and_column = lexer.get_line_and_column(); \
        throw error::ParserError(message, line_and_column.first, line_and_column.second); \
    } while(0);

#define LUAXC_PARSER_THROW_NOT_IMPLEMENTED(feature) LUAXC_PARSER_THROW_ERROR("Not implemented feature: " #feature )

namespace luaxc {
    void Parser::consume(TokenType expected) {
        consume(expected, "Unexpected token");
    }

    void Parser::consume(TokenType expected, const std::string& err_message) {
        if (current_token.type != expected) {
            LUAXC_PARSER_THROW_ERROR(err_message);
        }
        next_token();
    }

    void Parser::next_token() {
        current_token = lexer.next();
    }

    std::unique_ptr<AstNode> Parser::parse_program() { 
        auto program = std::make_unique<ProgramNode>();
        while (current_token.type != TokenType::TERMINATOR) {
            program->add_statement(parse_statement());
        }
        return program;
    }

    std::unique_ptr<AstNode> Parser::parse_statement() { 
        if (current_token.type == TokenType::KEYWORD_LET) { 
            return parse_declaration_statement();
        } else if (current_token.type == TokenType::IDENTIFIER) {
            return parse_assignment_statement();
        }
        return parse_expression();
    }

    std::unique_ptr<AstNode> Parser::parse_declaration_statement() { 
        /*
        declaration_statement:
            let identifier = expression;
            let identifier;
        */
        consume(TokenType::KEYWORD_LET);

        auto identifier = std::make_unique<IdentifierNode>(current_token.value);
        consume(TokenType::IDENTIFIER, "Expected identifier after 'let' keyword");

        if (current_token.type == TokenType::SEMICOLON) {
            consume(TokenType::SEMICOLON, "Expected semicolon after declaration identifier");
            return std::make_unique<DeclarationStmtNode>(std::move(identifier), nullptr);
        }
        
        consume(TokenType::ASSIGN, "Expected '=' after identifier in declaration statement");;

        std::unique_ptr<AstNode> value;
        
        if (current_token.type == TokenType::STRING_LITERAL) {
            LUAXC_PARSER_THROW_NOT_IMPLEMENTED("Assigning strings");
            // todo
        } else {
            value = parse_expression();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after assignment");

        return std::make_unique<DeclarationStmtNode>(std::move(identifier), std::move(value));
    }

    std::unique_ptr<AstNode> Parser::parse_assignment_statement() { 
        /*
        assignment_statement:
            identifier = expression;
        */
        auto identifier = std::make_unique<IdentifierNode>(current_token.value);
        consume(TokenType::IDENTIFIER, "Expected identifier");
        consume(TokenType::ASSIGN, "Expected '=' after identifier in assignment statement");

        std::unique_ptr<AstNode> value;
        if (current_token.type == TokenType::STRING_LITERAL) {
            LUAXC_PARSER_THROW_NOT_IMPLEMENTED("Assigning strings")
        } else {
            value = parse_expression();
        }

        consume(TokenType::SEMICOLON, "Expected ';' after assignment statement");
        return std::make_unique<AssignmentStmtNode>(std::move(identifier), std::move(value));
    }

    std::unique_ptr<AstNode> Parser::parse_expression() { 
        auto node = parse_term();
        while (current_token.type == TokenType::PLUS || current_token.type == TokenType::MINUS) {
            BinaryExpressionNode::BinaryOperator op;
            if (current_token.type == TokenType::PLUS) {
                consume(TokenType::PLUS);
                op = BinaryExpressionNode::BinaryOperator::Add;
            } else {
                consume(TokenType::MINUS);
                op = BinaryExpressionNode::BinaryOperator::Subtract;
            }
            auto right = parse_term();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_term() { 
        auto node = parse_factor();
        
        while (current_token.type == TokenType::MUL || current_token.type == TokenType::DIV || 
            current_token.type == TokenType::MOD) {
            BinaryExpressionNode::BinaryOperator op;
            if (current_token.type == TokenType::MUL) {
                consume(TokenType::MUL);
                op = BinaryExpressionNode::BinaryOperator::Multiply;
            } else if (current_token.type == TokenType::DIV) {
                consume(TokenType::DIV);
                op = BinaryExpressionNode::BinaryOperator::Divide;
            } else {
                consume(TokenType::MOD);
                op = BinaryExpressionNode::BinaryOperator::Modulo;
            }
            auto right = parse_term();
            return std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_factor() { 
        std::unique_ptr<AstNode> node;
        // todo: this does not handle the case where the token is an identifier
        if (current_token.type == TokenType::NUMBER) {
            if (current_token.value.find('.') != std::string::npos) {
                node = std::make_unique<NumericLiteralNode>(
                    NumericLiteralNode::NumericLiteralType::Float, current_token.value);
            } else {
                node = std::make_unique<NumericLiteralNode>(
                    NumericLiteralNode::NumericLiteralType::Integer, current_token.value);
            }
            consume(TokenType::NUMBER);
        } else if (current_token.type == TokenType::L_PARENTHESIS) {
            consume(TokenType::L_PARENTHESIS);
            node = parse_expression();
            consume(TokenType::R_PARENTHESIS);
        }
        return node;
    }
}