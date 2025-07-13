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
        switch (current_token.type) {
            case TokenType::KEYWORD_LET:
                return parse_declaration_statement();
            case TokenType::IDENTIFIER:
                return parse_assignment_statement();
            case TokenType::L_CURLY_BRACKET: // '{': block statement start
                return parse_block_statement();
            case TokenType::KEYWORD_IF:
                return parse_if_statement();
            default:
                return parse_expression();
        }
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
        return parse_comparison_expression();
    }

    std::unique_ptr<AstNode> Parser::parse_additive_expression() {
        auto node = parse_multiplicative_expression();

        while (current_token.type == TokenType::PLUS || current_token.type == TokenType::MINUS) {
            BinaryExpressionNode::BinaryOperator op;
            if (current_token.type == TokenType::PLUS) {
                consume(TokenType::PLUS);
                op = BinaryExpressionNode::BinaryOperator::Add;
            } else {
                consume(TokenType::MINUS);
                op = BinaryExpressionNode::BinaryOperator::Subtract;
            }
            auto right = parse_multiplicative_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }

        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_multiplicative_expression() {
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
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }
        
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_comparison_expression() {
        auto node = parse_additive_expression();

        while (current_token.type == TokenType::EQUAL || current_token.type == TokenType::NOT_EQUAL || 
            current_token.type == TokenType::GREATER_THAN || current_token.type == TokenType::GREATER_THAN_EQUAL || 
            current_token.type == TokenType::LESS_THAN || current_token.type == TokenType::LESS_THAN_EQUAL) {
            BinaryExpressionNode::BinaryOperator op;

            switch (current_token.type) {
                case TokenType::EQUAL:
                    op = BinaryExpressionNode::BinaryOperator::Equal;
                    break;
                case TokenType::NOT_EQUAL:
                    op = BinaryExpressionNode::BinaryOperator::NotEqual;
                    break;
                case TokenType::GREATER_THAN:
                    op = BinaryExpressionNode::BinaryOperator::GreaterThan;
                    break;
                case TokenType::GREATER_THAN_EQUAL:
                    op = BinaryExpressionNode::BinaryOperator::GreaterThanEqual;
                    break;
                case TokenType::LESS_THAN:
                    op = BinaryExpressionNode::BinaryOperator::LessThan;
                    break;
                case TokenType::LESS_THAN_EQUAL: 
                    op = BinaryExpressionNode::BinaryOperator::LessThanEqual;
                    break;
                default:
                    break;
            }
            next_token();
            auto right = parse_additive_expression();
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
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_factor() { 
        std::unique_ptr<AstNode> node;
        if (current_token.type == TokenType::NUMBER) {
            if (current_token.value.find('.') != std::string::npos) {
                node = std::make_unique<NumericLiteralNode>(
                    NumericLiteralNode::NumericLiteralType::Float, current_token.value);
            } else {
                node = std::make_unique<NumericLiteralNode>(
                    NumericLiteralNode::NumericLiteralType::Integer, current_token.value);
            }
            consume(TokenType::NUMBER);
        } else if (current_token.type == TokenType::IDENTIFIER) {
            node = std::make_unique<IdentifierNode>(current_token.value);
            consume(TokenType::IDENTIFIER);
        } else if (current_token.type == TokenType::L_PARENTHESIS) {
            consume(TokenType::L_PARENTHESIS);
            node = parse_expression();
            consume(TokenType::R_PARENTHESIS);
        } else if (current_token.type == TokenType::STRING_LITERAL) {
            // todo: string literal
            LUAXC_PARSER_THROW_NOT_IMPLEMENTED("String literals are not implemented yet.")
        }
        return node;
    }
    
    std::unique_ptr<AstNode> Parser::parse_block_statement() {
        std::vector<std::unique_ptr<AstNode>> statements;

        consume(TokenType::L_CURLY_BRACKET, "Expected '{'");
        while (current_token.type != TokenType::R_CURLY_BRACKET) {
            statements.push_back(parse_statement());

            if (current_token.type == TokenType::TERMINATOR) {
                consume(TokenType::R_CURLY_BRACKET, "Expected '}' but meet unexpected EOF");
            }
        }
        consume(TokenType::R_CURLY_BRACKET, "Block statement not closed with '}'");
        return std::make_unique<BlockNode>(std::move(statements));
    }

    std::unique_ptr<AstNode> Parser::parse_if_statement() {
        consume(TokenType::KEYWORD_IF, "Expected 'if' keyword");
        consume(TokenType::L_PARENTHESIS, "Expected '(' after 'if' keyword");

        auto condition = parse_expression();

        consume(TokenType::R_PARENTHESIS, "Expected ')' after condition");

        std::unique_ptr<AstNode> body = parse_statement();
        std::unique_ptr<AstNode> else_body = nullptr;

        if (current_token.type == TokenType::KEYWORD_ELSE) {
            consume(TokenType::KEYWORD_ELSE, "Expected 'else' keyword");
            else_body = parse_statement();
        }
        return std::make_unique<IfNode>(
            std::move(condition), std::move(body), std::move(else_body));
    }
}