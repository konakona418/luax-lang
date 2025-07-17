#include "parser.hpp"
#include "ast.hpp"

#define LUAXC_PARSER_THROW_ERROR(message)                                                 \
    do {                                                                                  \
        auto line_and_column = lexer.get_line_and_column();                               \
        throw error::ParserError(message, line_and_column.first, line_and_column.second); \
    } while (0);

#define LUAXC_PARSER_THROW_NOT_IMPLEMENTED(feature) LUAXC_PARSER_THROW_ERROR("Not implemented feature: " #feature)


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

    void Parser::enter_scope(Parser::ParserState state) {
        scopes.emplace_back(state);
    }

    void Parser::exit_scope() {
        scopes.pop_back();
    }

    void Parser::declare_identifier(const std::string& identifier) {
        scopes.back().identifiers.insert(identifier);
    }

    bool Parser::is_identifier_declared(const std::string& identifier) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->identifiers.find(identifier) != it->identifiers.end()) {
                return true;
            }
        }
        return false;
    }

    bool Parser::is_in_scope(ParserState state) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->state == state) {
                return true;
            }
        }
        return false;
    }

    std::unique_ptr<AstNode> Parser::parse_program() {
        enter_scope(ParserState::Start);

        auto program = std::make_unique<ProgramNode>();
        while (current_token.type != TokenType::TERMINATOR) {
            program->add_statement(parse_statement());
        }

        exit_scope();
        return program;
    }

    std::unique_ptr<AstNode> Parser::parse_statement() {
        std::unique_ptr<AstNode> parsed;

        switch (current_token.type) {
            case TokenType::KEYWORD_LET:
                parsed = parse_declaration_statement();
                break;
            case TokenType::KEYWORD_FIELD:
                parsed = parse_field_declaration_statement();
                break;
            case TokenType::KEYWORD_METHOD:
                parsed = parse_method_declaration_statement();
                break;
            case TokenType::KEYWORD_USE:
                parsed = parse_forward_declaration_statement();
                break;
            case TokenType::L_CURLY_BRACKET:// '{': block statement start
                parsed = parse_block_statement();
                break;
            case TokenType::KEYWORD_IF:
                parsed = parse_if_statement();
                break;
            case TokenType::KEYWORD_WHILE:
                parsed = parse_while_statement();
                break;
            case TokenType::KEYWORD_FOR:
                parsed = parse_for_statement();
                break;
            case TokenType::KEYWORD_BREAK:
                parsed = parse_break_statement();
                break;
            case TokenType::KEYWORD_CONTINUE:
                parsed = parse_continue_statement();
                break;
            case TokenType::KEYWORD_FUNC:
                parsed = parse_function_declaration_statement();
                break;
            case TokenType::KEYWORD_RETURN:
                parsed = parse_return_statement();
                break;
            default:
                parsed = parse_expression();
                break;
        }

        while (current_token.type == TokenType::SEMICOLON) {
            next_token();
        }
        return parsed;
    }

    std::unique_ptr<AstNode> Parser::parse_declaration_statement(bool consume_semicolon) {
        /*
        declaration_statement:
            let identifier = expression;
            let identifier;
        */
        consume(TokenType::KEYWORD_LET);

        auto identifiers = std::vector<std::unique_ptr<AstNode>>();

        identifiers.push_back(std::make_unique<IdentifierNode>(current_token.value));
        declare_identifier(current_token.value);

        consume(TokenType::IDENTIFIER, "Expected identifier after 'let' keyword");


        // 'let' identifier ';'
        if (current_token.type == TokenType::SEMICOLON) {
            consume(TokenType::SEMICOLON, "Expected semicolon after declaration identifier");
            return std::make_unique<DeclarationStmtNode>(std::move(identifiers), nullptr);
        }

        // 'let' identifier1 ',' identifier2 ';'
        bool is_multi_declaration = current_token.type == TokenType::COMMA;
        while (current_token.type == TokenType::COMMA) {
            consume(TokenType::COMMA, "Expected comma after declaration identifier");

            identifiers.push_back(std::make_unique<IdentifierNode>(current_token.value));
            declare_identifier(current_token.value);

            consume(TokenType::IDENTIFIER, "Expected identifier after comma in declaration statement");
        }

        if (is_multi_declaration) {
            consume(TokenType::SEMICOLON, "Expected a semicolon after multi declaration");
            return std::make_unique<DeclarationStmtNode>(std::move(identifiers), nullptr);
        }

        consume(TokenType::ASSIGN, "Expected '=' after identifier in declaration statement");
        ;

        std::unique_ptr<AstNode> value;

        value = parse_simple_expression();

        if (consume_semicolon) {
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
        }

        return std::make_unique<DeclarationStmtNode>(std::move(identifiers), std::move(value));
    }

    std::unique_ptr<AstNode> Parser::parse_field_declaration_statement() {
        // 'field' identifier ('=' expr)? ';'
        if (!is_in_scope(ParserState::InTypeDeclarationScope)) {
            LUAXC_PARSER_THROW_ERROR("Field declaration outside of a type declaration")
        }

        consume(TokenType::KEYWORD_FIELD, "Expected keyword 'field'");

        auto identifier = std::make_unique<IdentifierNode>(current_token.value);
        declare_identifier(current_token.value);

        consume(TokenType::IDENTIFIER, "Expected identifier in field declaration");

        std::unique_ptr<AstNode> type_decl_expr = nullptr;
        if (current_token.type == TokenType::ASSIGN) {
            consume(TokenType::ASSIGN);

            type_decl_expr = parse_simple_expression();
        }

        consume(TokenType::SEMICOLON, "Expected semicolon after field declaration");

        return std::make_unique<FieldDeclarationStatementNode>(
                std::move(identifier), std::move(type_decl_expr));
    }

    std::unique_ptr<AstNode> Parser::parse_method_declaration_statement() {
        if (!is_in_scope(ParserState::InTypeDeclarationScope)) {
            LUAXC_PARSER_THROW_ERROR("Method declaration must be in a scope of a type declaration");
        }

        consume(TokenType::KEYWORD_METHOD, "Expected 'method'");
        std::unique_ptr<AstNode> identifier = parse_identifier();
        declare_identifier(static_cast<IdentifierNode*>(identifier.get())->get_name());

        consume(TokenType::L_PARENTHESIS, "Expected '(' after method name");
        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope();

        while (current_token.type != TokenType::R_PARENTHESIS) {
            auto param = parse_identifier();
            declare_identifier(static_cast<IdentifierNode*>(param.get())->get_name());

            parameters.push_back(std::move(param));

            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ','");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' enclosing method parameters");

        if (current_token.type != TokenType::L_CURLY_BRACKET) {
            // forward function declaration
            consume(TokenType::SEMICOLON, "Expected ';' after forward method declaraton");
            return std::make_unique<MethodDeclarationNode>(
                    std::move(identifier),
                    std::move(parameters),
                    nullptr);
        }

        auto body = parse_block_statement();

        exit_scope();

        return std::make_unique<FunctionDeclarationNode>(
                std::move(identifier),
                std::move(parameters),
                std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_forward_declaration_statement() {
        consume(TokenType::KEYWORD_USE, "Expected 'use'");

        declare_identifier(current_token.value);
        std::unique_ptr<AstNode> identifier = parse_identifier();
        consume(TokenType::SEMICOLON, "Expected ';' after forward declaration");

        return std::make_unique<ForwardDeclarationStmtNode>(std::move(identifier));
    }

    std::unique_ptr<AstNode> Parser::parse_function_declaration_statement() {
        consume(TokenType::KEYWORD_FUNC, "Expected 'func'");
        std::unique_ptr<AstNode> identifier = parse_identifier();
        declare_identifier(static_cast<IdentifierNode*>(identifier.get())->get_name());

        consume(TokenType::L_PARENTHESIS, "Expected '(' after function name");
        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope();

        while (current_token.type != TokenType::R_PARENTHESIS) {
            auto param = parse_identifier();
            declare_identifier(static_cast<IdentifierNode*>(param.get())->get_name());

            parameters.push_back(std::move(param));

            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ','");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' enclosing function parameters");

        if (current_token.type != TokenType::L_CURLY_BRACKET) {
            // forward function declaration
            consume(TokenType::SEMICOLON, "Expected ';' after forward function declaraton");
            return std::make_unique<FunctionDeclarationNode>(
                    std::move(identifier),
                    std::move(parameters),
                    nullptr);
        }

        auto body = parse_block_statement();

        exit_scope();

        return std::make_unique<FunctionDeclarationNode>(
                std::move(identifier),
                std::move(parameters),
                std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_return_statement() {
        consume(TokenType::KEYWORD_RETURN, "Expected 'return'");

        if (current_token.type == TokenType::SEMICOLON) {
            consume(TokenType::SEMICOLON, "Expected ';'");
            return std::make_unique<ReturnNode>(nullptr);
        }

        auto expression = parse_simple_expression();
        consume(TokenType::SEMICOLON, "Expected ';' after return statement");

        return std::make_unique<ReturnNode>(std::move(expression));
    }

    std::unique_ptr<AstNode> Parser::parse_identifier() {
        auto identifier = std::make_unique<IdentifierNode>(current_token.value);
        consume(TokenType::IDENTIFIER, "Expected identifier");
        return identifier;
    }

    std::unique_ptr<AstNode> Parser::parse_basic_assignment_expression(std::unique_ptr<AstNode> identifier, bool consume_semicolon) {
        /*
        assignment_statement:
            identifier = expression;
        */
        //auto identifier = std::make_unique<IdentifierNode>(current_token.value);
        //consume(TokenType::IDENTIFIER, "Expected identifier");
        consume(TokenType::ASSIGN, "Expected '=' after identifier in assignment statement");

        std::unique_ptr<AstNode> value;

        value = parse_simple_expression();

        if (consume_semicolon) {
            consume(TokenType::SEMICOLON, "Expected ';' after assignment statement");
        }

        return std::make_unique<AssignmentExpressionNode>(std::move(identifier), std::move(value));
    }

    std::unique_ptr<AstNode> Parser::parse_expression(bool consume_semicolon) {
        auto node = parse_assignment_expression(false);
        if (consume_semicolon) {
            consume(TokenType::SEMICOLON, "Expected ';' after expression");
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_type_declaration_expression() {
        consume(TokenType::KEYWORD_TYPE, "Expected keyword 'type'");

        enter_scope(ParserState::InTypeDeclarationScope);
        auto type_decl = parse_block_statement();
        exit_scope();

        return std::make_unique<TypeDeclarationExpressionNode>(std::move(type_decl));
    }

    std::unique_ptr<AstNode> Parser::parse_assignment_expression(bool consume_semicolon) {
        auto left = parse_simple_expression();
        bool is_result_discarded = false;

        if (current_token.type == TokenType::ASSIGN) {
            return parse_basic_assignment_expression(std::move(left), consume_semicolon);
        } else if (current_token.type == TokenType::INCREMENT_BY || current_token.type == TokenType::DECREMENT_BY) {
            return parse_combinative_assignment_expression(std::move(left), consume_semicolon);
        } else {
            // if we are not assigning the result to anything, we can discard it
            is_result_discarded = true;
        }

        static_cast<ExpressionNode*>(left.get())->set_result_discarded(is_result_discarded);

        return left;
    }

    std::unique_ptr<AstNode> Parser::parse_simple_expression() {
        return parse_logical_and_expression();
    }

    std::unique_ptr<AstNode> Parser::parse_combinative_assignment_expression(std::unique_ptr<AstNode> identifier, bool consume_semicolon) {
        // only one combinative assignment expression is allowed
        if (current_token.type == TokenType::INCREMENT_BY || current_token.type == TokenType::DECREMENT_BY) {
            BinaryExpressionNode::BinaryOperator op;
            if (current_token.type == TokenType::INCREMENT_BY) {
                op = BinaryExpressionNode::BinaryOperator::IncrementBy;
            } else {
                op = BinaryExpressionNode::BinaryOperator::DecrementBy;
            }

            next_token();
            auto right = parse_simple_expression();

            if (consume_semicolon) {
                consume(TokenType::SEMICOLON, "Expected semicolon after assignment statement");
            } else {
                /*if (current_token.type == TokenType::SEMICOLON) {
                    LUAXC_PARSER_THROW_ERROR("Unexpected semicolon after assignment statement")
                }*/
            }

            return std::make_unique<BinaryExpressionNode>(std::move(identifier), std::move(right), op);
        }

        LUAXC_PARSER_THROW_ERROR("Not a valid combinative assignment expression");
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
        auto node = parse_unary_expression();

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
            auto right = parse_unary_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }

        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_comparison_expression() {
        auto node = parse_bitwise_and_expression();

        while (current_token.type == TokenType::EQUAL || current_token.type == TokenType::NOT_EQUAL) {
            BinaryExpressionNode::BinaryOperator op;

            switch (current_token.type) {
                case TokenType::EQUAL:
                    op = BinaryExpressionNode::BinaryOperator::Equal;
                    break;
                case TokenType::NOT_EQUAL:
                    op = BinaryExpressionNode::BinaryOperator::NotEqual;
                    break;
                default:
                    break;
            }
            next_token();
            auto right = parse_bitwise_and_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }

        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_relational_expression() {
        std::unique_ptr<AstNode> node = parse_additive_expression();

        while (current_token.type == TokenType::GREATER_THAN || current_token.type == TokenType::GREATER_THAN_EQUAL ||
               current_token.type == TokenType::LESS_THAN || current_token.type == TokenType::LESS_THAN_EQUAL) {
            BinaryExpressionNode::BinaryOperator op;

            switch (current_token.type) {
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

    std::unique_ptr<AstNode> Parser::parse_logical_and_expression() {
        auto node = parse_logical_or_expression();

        while (current_token.type == TokenType::LOGICAL_AND) {
            next_token();
            auto right = parse_logical_or_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right),
                                                          BinaryExpressionNode::BinaryOperator::LogicalAnd);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_logical_or_expression() {
        auto node = parse_comparison_expression();
        while (current_token.type == TokenType::LOGICAL_OR) {
            next_token();
            auto right = parse_comparison_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right),
                                                          BinaryExpressionNode::BinaryOperator::LogicalOr);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_bitwise_and_expression() {
        // todo: testing
        auto node = parse_bitwise_or_expression();
        while (current_token.type == TokenType::BITWISE_AND) {
            next_token();
            auto right = parse_bitwise_or_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right),
                                                          BinaryExpressionNode::BinaryOperator::BitwiseAnd);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_bitwise_or_expression() {
        // todo: testing
        auto node = parse_bitwise_shift_expression();
        while (current_token.type == TokenType::BITWISE_OR) {
            next_token();
            auto right = parse_bitwise_shift_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right),
                                                          BinaryExpressionNode::BinaryOperator::BitwiseOr);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_bitwise_shift_expression() {
        // todo: testing
        auto node = parse_relational_expression();
        while (current_token.type == TokenType::BITWISE_SHIFT_LEFT || current_token.type == TokenType::BITWISE_SHIFT_RIGHT) {
            BinaryExpressionNode::BinaryOperator op;

            if (current_token.type == TokenType::BITWISE_SHIFT_LEFT) {
                op = BinaryExpressionNode::BinaryOperator::BitwiseShiftLeft;
            } else {
                op = BinaryExpressionNode::BinaryOperator::BitwiseShiftRight;
            }

            next_token();
            auto right = parse_relational_expression();
            node = std::make_unique<BinaryExpressionNode>(std::move(node), std::move(right), op);
        }
        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_unary_expression() {
        // todo: testing
        // todo: other unary operators
        if (current_token.type == TokenType::MINUS || current_token.type == TokenType::LOGICAL_NOT ||
            current_token.type == TokenType::BITWISE_NOT || current_token.type == TokenType::PLUS) {
            UnaryExpressionNode::UnaryOperator op;
            switch (current_token.type) {
                case TokenType::MINUS:
                    op = UnaryExpressionNode::UnaryOperator::Minus;
                    break;
                case TokenType::BITWISE_NOT:
                    op = UnaryExpressionNode::UnaryOperator::BitwiseNot;
                    break;
                case TokenType::PLUS:
                    op = UnaryExpressionNode::UnaryOperator::Plus;
                    break;
                case TokenType::LOGICAL_NOT:
                    op = UnaryExpressionNode::UnaryOperator::LogicalNot;
                    break;
                default:
                    LUAXC_PARSER_THROW_NOT_IMPLEMENTED("unary operator");
                    break;
            }
            next_token();
            auto operand = parse_unary_expression();
            return std::make_unique<UnaryExpressionNode>(std::move(operand), op);
        }
        return parse_primary();
    }

    std::unique_ptr<AstNode> Parser::parse_member_access_expression(std::unique_ptr<AstNode> initial_expr) {
        consume(TokenType::DOT, "Expected dot");

        auto identifier = parse_identifier();
        consume(TokenType::IDENTIFIER, "Expected identifier after member access '.'");

        if (current_token.type == TokenType::L_PARENTHESIS) {
            identifier = parse_function_invocation_statement(std::move(identifier));
        }

        auto member_access = std::make_unique<MemberAccessExpressionNode>(
                std::move(initial_expr), std::move(identifier));

        /*if (current_token.type == TokenType::DOT) {
            return parse_member_access_expression(std::move(member_access));
        }*/

        return member_access;
    }

    std::unique_ptr<AstNode> Parser::parse_primary() {
        std::unique_ptr<AstNode> node;
        switch (current_token.type) {
            case (TokenType::NUMBER): {
                if (current_token.value.find('.') != std::string::npos) {
                    node = std::make_unique<NumericLiteralNode>(
                            NumericLiteralNode::NumericLiteralType::Float, current_token.value);
                } else {
                    node = std::make_unique<NumericLiteralNode>(
                            NumericLiteralNode::NumericLiteralType::Integer, current_token.value);
                }
                consume(TokenType::NUMBER);

                break;
            }
            case (TokenType::IDENTIFIER): {
                node = std::make_unique<IdentifierNode>(current_token.value);

                auto name = static_cast<IdentifierNode*>(node.get())->get_name();
                if (!is_identifier_declared(name)) {
                    LUAXC_PARSER_THROW_ERROR("Identifier not declared: '" + name + "'")
                }

                consume(TokenType::IDENTIFIER);
                break;
            }
            case (TokenType::L_PARENTHESIS): {
                consume(TokenType::L_PARENTHESIS);
                node = parse_simple_expression();
                consume(TokenType::R_PARENTHESIS);

                break;
            }
            case (TokenType::STRING_LITERAL): {
                auto& str = current_token.value;
                // remove the prefix and postfix quotation marks.
                node = std::make_unique<StringLiteralNode>(str.substr(1, str.length() - 2));
                consume(TokenType::STRING_LITERAL);

                break;
            }
            case (TokenType::KEYWORD_TYPE): {
                node = parse_type_declaration_expression();
                break;
            }
            default:
                LUAXC_PARSER_THROW_NOT_IMPLEMENTED("unknown primary expr")
        }

        while (true) {
            if (current_token.type == TokenType::L_PARENTHESIS) {
                // potential function invocation
                node = parse_function_invocation_statement(std::move(node));
            } else if (current_token.type == TokenType::DOT) {
                // potential member access
                node = parse_member_access_expression(std::move(node));
            } else {
                break;
            }
        }

        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_function_invocation_statement(std::unique_ptr<AstNode> function_identifier) {
        consume(TokenType::L_PARENTHESIS, "Expected '(' after function identifier");
        std::vector<std::unique_ptr<AstNode>> arguments;

        while (current_token.type != TokenType::R_PARENTHESIS) {
            arguments.push_back(parse_simple_expression());
            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ',' after argument");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' ending function invocation");

        return std::make_unique<FunctionInvocationExpressionNode>(
                std::move(function_identifier), std::move(arguments));
    }

    std::unique_ptr<AstNode> Parser::parse_block_statement() {
        // '{' stmt* '}' (';')?
        std::vector<std::unique_ptr<AstNode>> statements;

        consume(TokenType::L_CURLY_BRACKET, "Expected '{'");

        enter_scope(ParserState::InScope);

        while (current_token.type != TokenType::R_CURLY_BRACKET) {
            statements.push_back(parse_statement());

            if (current_token.type == TokenType::TERMINATOR) {
                consume(TokenType::R_CURLY_BRACKET, "Expected '}' but meet unexpected EOF");
            }
        }

        exit_scope();

        consume(TokenType::R_CURLY_BRACKET, "Block statement not closed with '}'");

        return std::make_unique<BlockNode>(std::move(statements));
    }

    std::unique_ptr<AstNode> Parser::parse_if_statement() {
        consume(TokenType::KEYWORD_IF, "Expected 'if' keyword");
        consume(TokenType::L_PARENTHESIS, "Expected '(' after 'if' keyword");

        auto condition = parse_simple_expression();

        consume(TokenType::R_PARENTHESIS, "Expected ')' after condition");

        enter_scope();
        std::unique_ptr<AstNode> body = parse_statement();
        exit_scope();

        std::unique_ptr<AstNode> else_body = nullptr;

        if (current_token.type == TokenType::KEYWORD_ELSE) {
            consume(TokenType::KEYWORD_ELSE, "Expected 'else' keyword");

            enter_scope();
            else_body = parse_statement();
            exit_scope();
        }
        return std::make_unique<IfNode>(
                std::move(condition), std::move(body), std::move(else_body));
    }

    std::unique_ptr<AstNode> Parser::parse_for_statement() {
        consume(TokenType::KEYWORD_FOR, "Expected 'for' keyword");
        consume(TokenType::L_PARENTHESIS, "Expected '(' after 'for' keyword");

        // parse initializer stmt.
        // this can have two possible forms:
        // a declaration statement or an assignment statement.

        // the scope is entered before parsing the initializer statement,
        // for that the iterator itself, if declared here, should be only visible to the loop body
        enter_scope();

        std::unique_ptr<AstNode> initializer;
        if (current_token.type == TokenType::KEYWORD_LET) {
            initializer = parse_declaration_statement(false);
        } else {
            initializer = parse_basic_assignment_expression(parse_identifier(), false);
        }
        consume(TokenType::SEMICOLON, "Expected ';' after iterator initializer");// consume semicolon.

        std::unique_ptr<AstNode> condition = parse_simple_expression();
        consume(TokenType::SEMICOLON, "Expected ';' after iterator condition");

        std::unique_ptr<AstNode> update = parse_expression(false);

        consume(TokenType::R_PARENTHESIS, "Expected an ')' enclosing the for-loop statements");

        std::unique_ptr<AstNode> body = parse_statement();

        exit_scope();

        return std::make_unique<ForNode>(std::move(initializer),
                                         std::move(condition), std::move(update), std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_while_statement() {
        consume(TokenType::KEYWORD_WHILE, "Expected 'while' keyword");
        consume(TokenType::L_PARENTHESIS, "Expected '(' after 'while' keyword");

        // currently I don't want while condition to support complex assignment or declarations
        auto condition = parse_simple_expression();

        consume(TokenType::R_PARENTHESIS, "Expected ')' after while condition");

        enter_scope();
        auto body = parse_statement();
        exit_scope();

        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_break_statement() {
        consume(TokenType::KEYWORD_BREAK, "Expected 'break' keyword");
        consume(TokenType::SEMICOLON, "Expected ';' after 'break'");
        return std::make_unique<BreakNode>();
    }

    std::unique_ptr<AstNode> Parser::parse_continue_statement() {
        consume(TokenType::KEYWORD_CONTINUE, "Expected 'continue' keyword");
        consume(TokenType::SEMICOLON, "Expected ';' after 'continue'");
        return std::make_unique<ContinueNode>();
    }
}// namespace luaxc