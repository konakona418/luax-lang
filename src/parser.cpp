#include "parser.hpp"
#include "ast.hpp"

#define LUAXC_PARSER_THROW_ERROR(message)                                                                 \
    do {                                                                                                  \
        auto cached_stats = lexer.get_cached_statistics();                                                \
        throw error::ParserError(message, cached_stats.line, cached_stats.column, cached_stats.filename); \
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

    bool Parser::is_in_scope_no_propagation(ParserState state) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->state == state) {
                return true;
            }

            // allow fall through block statements
            if (it->state != ParserState::InScope) {
                break;
            }
        }
        return false;
    }

    void Parser::consume_type_annotation(TypeAnnotationType type) {
        // todo: handling type annotations like this is not ideal.
        // it would be better to parse them as simple expressions.
        // for annotations like :optional::Optional(typing::Int),
        // which contains generic types, this function will crash.
        // so i made it a feature.

        if (current_token.type != TokenType::COLON) {
            return;
        }

#ifdef LUAXC_PARSER_FEATURE_TYPE_ANNOTATION
        next_token();

        switch (type) {
            case TypeAnnotationType::Args: {
                while (current_token.type != TokenType::COMMA &&        // arg1, arg2, arg3
                       current_token.type != TokenType::R_PARENTHESIS) {// arg1, arg2)
                    next_token();
                }
                break;
            }
            case TypeAnnotationType::Return: {
                while (current_token.type != TokenType::L_CURLY_BRACKET &&// func (): type {}
                       current_token.type != TokenType::SEMICOLON) {      // func (): type;
                    next_token();
                }
                break;
            }
            case TypeAnnotationType::Declare: {
                while (current_token.type != TokenType::EQUAL &&    // let a: type = 1;
                       current_token.type != TokenType::COMMA &&    // let a: type, b: type;
                       current_token.type != TokenType::SEMICOLON) {// let a: type;
                    next_token();
                }
                break;
            }
        }
#else
        LUAXC_PARSER_THROW_ERROR("Feature 'type annotation' disabled")
#endif
    }

    std::unique_ptr<AstNode> Parser::parse_program(ParserState init_state) {
        enter_scope(init_state);

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
                parsed = parse_module_import_expression();
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
            case TokenType::KEYWORD_CONSTRAINT:
                parsed = parse_constraint_expression();
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
        consume_type_annotation(TypeAnnotationType::Declare);

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

            consume_type_annotation(TypeAnnotationType::Declare);
        }

        if (is_multi_declaration) {
            consume(TokenType::SEMICOLON, "Expected a semicolon after multi declaration");
            return std::make_unique<DeclarationStmtNode>(std::move(identifiers), nullptr);
        }

        consume(TokenType::ASSIGN, "Expected '=' after identifier in declaration statement");

        std::unique_ptr<AstNode> value;

        value = parse_simple_expression();

        if (consume_semicolon) {
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
        }

        return std::make_unique<DeclarationStmtNode>(std::move(identifiers), std::move(value));
    }

    std::unique_ptr<AstNode> Parser::parse_field_declaration_statement() {
        // 'field' identifier ('=' expr)? ';'
        if (!is_in_scope_no_propagation(ParserState::InTypeDeclarationScope)) {
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
        if (!is_in_scope_no_propagation(ParserState::InTypeDeclarationScope)) {
            LUAXC_PARSER_THROW_ERROR("Method declaration must be in a scope of a type declaration");
        }

        consume(TokenType::KEYWORD_METHOD, "Expected 'method'");
        std::unique_ptr<AstNode> identifier = parse_identifier();
        declare_identifier(static_cast<IdentifierNode*>(identifier.get())->get_name());

        consume(TokenType::L_PARENTHESIS, "Expected '(' after method name");
        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope(ParserState::InFunctionOrMethodScope);

        while (current_token.type != TokenType::R_PARENTHESIS) {
            auto param = parse_identifier();
            declare_identifier(static_cast<IdentifierNode*>(param.get())->get_name());

            parameters.push_back(std::move(param));

            consume_type_annotation(TypeAnnotationType::Args);

            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ','");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' enclosing method parameters");

        consume_type_annotation(TypeAnnotationType::Return);

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

        return std::make_unique<MethodDeclarationNode>(
                std::move(identifier),
                std::move(parameters),
                std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_module_import_expression() {
        consume(TokenType::KEYWORD_USE, "Expected 'use'");

        auto import_name_expr = parse_primary();

        return std::make_unique<ModuleImportExpresionNode>(std::move(import_name_expr));
    }

    std::unique_ptr<AstNode> Parser::parse_function_declaration_statement() {
        consume(TokenType::KEYWORD_FUNC, "Expected 'func'");
        std::unique_ptr<AstNode> identifier = parse_identifier();
        declare_identifier(static_cast<IdentifierNode*>(identifier.get())->get_name());

        consume(TokenType::L_PARENTHESIS, "Expected '(' after function name");
        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope(ParserState::InFunctionOrMethodScope);

        while (current_token.type != TokenType::R_PARENTHESIS) {
            auto param = parse_identifier();
            declare_identifier(static_cast<IdentifierNode*>(param.get())->get_name());

            parameters.push_back(std::move(param));

            consume_type_annotation(TypeAnnotationType::Args);

            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ','");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' enclosing function parameters");

        consume_type_annotation(TypeAnnotationType::Return);

        if (current_token.type != TokenType::L_CURLY_BRACKET) {
            // forward function declaration
            consume(TokenType::SEMICOLON, "Expected ';' after forward function declaraton");
            exit_scope();

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

    std::unique_ptr<AstNode> Parser::parse_module_declaration_expression() {
        consume(TokenType::KEYWORD_MOD, "Expected keyword 'module'");

        enter_scope(ParserState::InModuleDeclarationScope);
        auto module_decl = parse_block_statement();
        exit_scope();

        return std::make_unique<ModuleDeclarationExpressionNode>(std::move(module_decl));
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

    std::unique_ptr<AstNode> Parser::parse_array_like_member_access_expression(std::unique_ptr<AstNode> initial_expr) {
        consume(TokenType::L_SQUARE_BRACE, "Expected '['");

        auto expr = parse_simple_expression();

        consume(TokenType::R_SQUARE_BRACE, "Expected ']'");

        return std::make_unique<MemberAccessExpressionNode>(
                MemberAccessExpressionNode::MemberAccessType::ArrayStyleMemberAccess,
                std::move(initial_expr), std::move(expr));
    }

    std::unique_ptr<AstNode> Parser::parse_member_access_or_method_invoke_expression(std::unique_ptr<AstNode> initial_expr) {
        consume(TokenType::DOT, "Expected dot");

        auto identifier = parse_identifier();

        if (current_token.type == TokenType::L_PARENTHESIS ||
            current_token.type == TokenType::AT) {// trailing closure expression
            return parse_method_invocation_expression(std::move(initial_expr), std::move(identifier));
        }

        return std::make_unique<MemberAccessExpressionNode>(
                MemberAccessExpressionNode::MemberAccessType::DotMemberAccess,
                std::move(initial_expr), std::move(identifier));
    }

    std::unique_ptr<AstNode> Parser::parse_method_invocation_expression(std::unique_ptr<AstNode> initial_expr, std::unique_ptr<AstNode> method_identifier) {
        std::vector<std::unique_ptr<AstNode>> arguments;

        if (current_token.type != TokenType::AT) {
            consume(TokenType::L_PARENTHESIS, "Expected '(' after method identifier");

            while (current_token.type != TokenType::R_PARENTHESIS) {
                arguments.push_back(parse_simple_expression());
                if (current_token.type == TokenType::COMMA) {
                    consume(TokenType::COMMA, "Expected ',' after argument");
                }
            }

            consume(TokenType::R_PARENTHESIS, "Expected ')' ending method invocation");
        }

        if (current_token.type == TokenType::AT) {
            auto trailing_closure = parse_trailing_closure_expression();

            arguments.push_back(std::move(trailing_closure));
        }

        return std::make_unique<MethodInvocationExpressionNode>(
                std::move(initial_expr),
                std::move(method_identifier),
                std::move(arguments));
    }

    std::unique_ptr<AstNode> Parser::parse_initializer_list_expression(std::unique_ptr<AstNode> type_expr) {
        std::vector<std::unique_ptr<AstNode>> statements;

        consume(TokenType::L_CURLY_BRACKET, "Expected '{'");

        enter_scope(ParserState::InInitializerListScope);

        while (current_token.type != TokenType::R_CURLY_BRACKET) {
            statements.push_back(parse_assignment_expression(false));

            if (current_token.type != TokenType::COMMA) {
                break;
            }
            consume(TokenType::COMMA, "Expected ','");

            if (current_token.type == TokenType::TERMINATOR) {
                consume(TokenType::R_CURLY_BRACKET, "Expected '}' but meet unexpected EOF");
            }
        }

        exit_scope();

        consume(TokenType::R_CURLY_BRACKET, "Block statement not closed with '}'");

        auto block = std::make_unique<BlockNode>(std::move(statements));

        return std::make_unique<InitializerListExpressionNode>(std::move(type_expr), std::move(block));
    }

    std::unique_ptr<AstNode> Parser::parse_trailing_closure_expression() {
        consume(TokenType::AT, "Expected '@' before trailing closure");
        std::unique_ptr<AstNode> trailing_closure;

        std::unique_ptr<AstNode> implicit_receiver_alias = nullptr;
        if (current_token.type == TokenType::IDENTIFIER) {
            implicit_receiver_alias = parse_identifier();
        }

        if (current_token.type == TokenType::KEYWORD_FUNC) {
            if (implicit_receiver_alias) {
                LUAXC_PARSER_THROW_ERROR(
                        "Implicit receiver alias cannot be used with closures "
                        "whose parameters has been explicitly declared")
            }
            trailing_closure = parse_closure_expression();
        } else if (current_token.type == TokenType::L_CURLY_BRACKET) {
            trailing_closure = parse_simple_closure_expression(std::move(implicit_receiver_alias));
        } else {
            LUAXC_PARSER_THROW_ERROR("Not a valid trailing closure")
        }

        return trailing_closure;
    }

    std::unique_ptr<AstNode> Parser::parse_closure_expression() {
        consume(TokenType::KEYWORD_FUNC, "Expected 'func'");
        consume(TokenType::L_PARENTHESIS, "Expected '(' in closure expression");

        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope(ParserState::InFunctionOrMethodScope);

        while (current_token.type != TokenType::R_PARENTHESIS) {
            auto param = parse_identifier();
            declare_identifier(static_cast<IdentifierNode*>(param.get())->get_name());

            parameters.push_back(std::move(param));

            consume_type_annotation(TypeAnnotationType::Args);

            if (current_token.type == TokenType::COMMA) {
                consume(TokenType::COMMA, "Expected ','");
            }
        }

        consume(TokenType::R_PARENTHESIS, "Expected ')' enclosing parameters");

        consume_type_annotation(TypeAnnotationType::Return);

        auto body = parse_block_statement();

        exit_scope();

        return std::make_unique<ClosureExpressionNode>(std::move(parameters), std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_simple_closure_expression(std::unique_ptr<AstNode> receiver_alias) {
        std::vector<std::unique_ptr<AstNode>> parameters;

        enter_scope(ParserState::InFunctionOrMethodScope);

        if (receiver_alias != nullptr) {
            declare_identifier(static_cast<IdentifierNode*>(receiver_alias.get())->get_name());
            parameters.push_back(std::move(receiver_alias));
        } else {
            parameters.push_back(std::make_unique<IdentifierNode>("self"));
            declare_identifier("self");
        }

        consume_type_annotation(TypeAnnotationType::Return);

        auto body = parse_block_statement();

        exit_scope();

        return std::make_unique<ClosureExpressionNode>(std::move(parameters), std::move(body));
    }

    std::unique_ptr<AstNode> Parser::parse_module_access_expression(std::unique_ptr<AstNode> initial_expr) {
        consume(TokenType::MODULE_ACCESS, "Expected '::'");

        auto identifier = parse_identifier();

        if (current_token.type == TokenType::L_PARENTHESIS) {
            auto fn =
                    parse_function_invocation_statement(std::move(identifier));
            return std::make_unique<ModuleAccessExpressionNode>(std::move(initial_expr), std::move(fn));
        }

        return std::make_unique<ModuleAccessExpressionNode>(std::move(initial_expr), std::move(identifier));
    }

    std::unique_ptr<AstNode> Parser::parse_rule_expression() {
        consume(TokenType::KEYWORD_RULE, "Expected 'rule'");

        enter_scope(ParserState::InRuleDeclarationScope);
        auto block = parse_block_statement();
        exit_scope();

        return std::make_unique<RuleExpressionNode>(std::move(block));
    }

    std::unique_ptr<AstNode> Parser::parse_constraint_expression() {
        if (!is_in_scope(ParserState::InRuleDeclarationScope)) {
            LUAXC_PARSER_THROW_ERROR("Constraint expression can only be used inside rule declaration");
        }
        consume(TokenType::KEYWORD_CONSTRAINT, "Expected 'constraint'");
        auto identifier = parse_identifier();

        consume(TokenType::ASSIGN, "Expected '='");
        auto expr = parse_simple_expression();
        consume(TokenType::SEMICOLON, "Expected ';'");

        return std::make_unique<ConstraintStatementNode>(
                std::move(identifier), std::move(expr));
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
                if (!is_identifier_declared(name) &&
                    !is_in_scope(ParserState::InInitializerListScope) &&
                    config.enable_undefined_identifier_check) {
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
            case (TokenType::L_CURLY_BRACKET): {
                if (is_in_scope(ParserState::InInitializerListScope)) {
                    // anonymous initializer list
                    // only allowed in an existing initializer list
                    node = parse_initializer_list_expression(nullptr);
                }
                break;
            }
            case (TokenType::DOT): {
                if (is_in_scope_no_propagation(ParserState::InFunctionOrMethodScope)) {
                    // implicit receiver syntax
                    node = parse_member_access_or_method_invoke_expression(
                            std::make_unique<ImplicitReceiverExpressionNode>());
                }
                break;
            }
            case (TokenType::STRING_LITERAL): {
                auto& str = current_token.value;
                // remove the prefix and postfix quotation marks.
                node = std::make_unique<StringLiteralNode>(str.substr(1, str.length() - 2));
                consume(TokenType::STRING_LITERAL);

                break;
            }
            case (TokenType::KEYWORD_TRUE): {
                node = std::make_unique<BoolLiteralNode>(true);
                consume(TokenType::KEYWORD_TRUE);
                break;
            }
            case (TokenType::KEYWORD_FALSE): {
                node = std::make_unique<BoolLiteralNode>(false);
                consume(TokenType::KEYWORD_FALSE);
                break;
            }
            case (TokenType::KEYWORD_NULL): {
                node = std::make_unique<NullLiteralNode>();
                consume(TokenType::KEYWORD_NULL);
                break;
            }
            case (TokenType::KEYWORD_TYPE): {
                node = parse_type_declaration_expression();
                break;
            }
            case (TokenType::KEYWORD_MOD): {
                node = parse_module_declaration_expression();
                break;
            }
            case (TokenType::KEYWORD_USE): {
                node = parse_module_import_expression();
                break;
            }
            case (TokenType::KEYWORD_FUNC): {
                node = parse_closure_expression();
                break;
            }
            case (TokenType::KEYWORD_RULE): {
                node = parse_rule_expression();
                break;
            }
            default:
                LUAXC_PARSER_THROW_NOT_IMPLEMENTED("unknown primary expr")
        }

        while (true) {
            if (current_token.type == TokenType::L_PARENTHESIS ||
                current_token.type == TokenType::AT) {// trailing closure expression
                // potential function invocation
                node = parse_function_invocation_statement(std::move(node));
            } else if (current_token.type == TokenType::DOT) {
                // potential member access
                node = parse_member_access_or_method_invoke_expression(std::move(node));
            } else if (current_token.type == TokenType::L_SQUARE_BRACE) {
                // array-like member access
                node = parse_array_like_member_access_expression(std::move(node));
            } else if (current_token.type == TokenType::MODULE_ACCESS) {
                // module access
                node = parse_module_access_expression(std::move(node));
            } else {
                break;
            }
        }

        // initializer list - named
        if (current_token.type == TokenType::L_CURLY_BRACKET) {
            node = parse_initializer_list_expression(std::move(node));
        }

        return node;
    }

    std::unique_ptr<AstNode> Parser::parse_function_invocation_statement(std::unique_ptr<AstNode> function_identifier) {
        std::vector<std::unique_ptr<AstNode>> arguments;

        if (current_token.type != TokenType::AT) {
            consume(TokenType::L_PARENTHESIS, "Expected '(' after function identifier");

            while (current_token.type != TokenType::R_PARENTHESIS) {
                arguments.push_back(parse_simple_expression());
                if (current_token.type == TokenType::COMMA) {
                    consume(TokenType::COMMA, "Expected ',' after argument");
                }
            }

            consume(TokenType::R_PARENTHESIS, "Expected ')' ending function invocation");
        }

        if (current_token.type == TokenType::AT) {
            // trailing closure
            auto trailing_closure = parse_trailing_closure_expression();

            arguments.push_back(std::move(trailing_closure));
        }

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